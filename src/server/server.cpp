#include "../../include/server/service.h"
#include "../../include/server/server.h"
#include <functional>
#include "../../thirdtools/json.hpp"
#include <string>
#include <chrono>

ClusterChatServer::ClusterChatServer(muduo::net::EventLoop *pLoop,
                                     const muduo::net::InetAddress &listenAddr,
                                     const muduo::string &nameArg)
    : _server(pLoop, listenAddr, nameArg), _pLoop(pLoop), _shuttingDown(false)
{
    // 注册客户端连接和断开的回调函数
    _server.setConnectionCallback(std::bind(&ClusterChatServer::onConnected, this, std::placeholders::_1));
    // 注册消息回调函数
    _server.setMessageCallback(std::bind(&ClusterChatServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    // 设置线程数量
    _server.setThreadNum(2);
}

ClusterChatServer::~ClusterChatServer()
{
    if (!_shuttingDown.load())
    {
        gracefulShutdown();
    }
}

void ClusterChatServer::start()
{
    _server.start();
}

// 客户端发生 连接或者断开 的回调
void ClusterChatServer::onConnected(const muduo::net::TcpConnectionPtr &pConn)
{
    if (!pConn->connected()) // 有连接断开（无论是客户端主动发起的断开，还是服务器关闭导致的客户端连接断开，都会触发onConnected回调）
    {
        ClusterChatService::getInstance()->handleClientExit(pConn); // 处理客户端断开后的状态清理，将断开连接的用户的状态更新为离线

        auto& userConnMap = (ClusterChatService::getInstance())->getUserConnMap();
        std::lock_guard<std::mutex> lock((ClusterChatService::getInstance())->_mtx);

        if (_shuttingDown.load() && userConnMap.empty()) // 如果服务器正在关闭，并且所有连接都断开了，通知主线程可以退出了
        {
            _shutdownCondition.notify_all(); // 所有的客户端都断开了，唤醒 waitForAllConnectionsClosed() 函数
        }
    }
}

// 停止主事件循环（负责监听新连接的EventLoop）
void ClusterChatServer::stopEventLoop()
{
    muduo::net::EventLoop *mainLoop = _server.getLoop();
    mainLoop->queueInLoop([mainLoop]
                          { mainLoop->quit(); });
}

// 优雅地关闭所有连接，服务器发出的
void ClusterChatServer::closeAllConnections()
{
    // 获取业务层的 ｛用户ID, 连接｝ 映射表
    auto& userConnMap = (ClusterChatService::getInstance())->getUserConnMap();
    std::lock_guard<std::mutex> lock((ClusterChatService::getInstance())->_mtx);

    for (const auto &pair : userConnMap) // 关闭所有现有的连接
    {
        if (pair.second->connected())
        {
            pair.second->shutdown();
        }
    }
}

// 强制关闭所有连接,一般发生在超时的时候，服务器发出的
void ClusterChatServer::forceCloseAllConnections()
{
    // 获取业务层的 ｛用户ID, 连接｝ 映射表
    auto& userConnMap = (ClusterChatService::getInstance())->getUserConnMap();
    std::lock_guard<std::mutex> lock((ClusterChatService::getInstance())->_mtx);

    for (const auto &pair : userConnMap) // 关闭所有现有的连接
    {
        if (pair.second->connected())
        {
            pair.second->forceClose();
        }
    }
}

// 等待所有连接关闭
bool ClusterChatServer::waitForAllConnectionsClosed(int timeoutSeconds)
{
    auto& userConnMap = (ClusterChatService::getInstance())->getUserConnMap();
    std::unique_lock<std::mutex> lock((ClusterChatService::getInstance())->_mtx);

    if (userConnMap.empty()) // 此时主EventLoop已经停止监听新连接，不会有新的连接产生
    {
        return true;
    }

    return _shutdownCondition.wait_for(lock, std::chrono::seconds(timeoutSeconds), [&userConnMap]()
                                       { return userConnMap.empty(); });
}

// 优雅关闭服务器
void ClusterChatServer::gracefulShutdown()
{
    // 1. 设置关闭标志
    _shuttingDown.store(true);

    // 2. 停止主事件循环（停止监听新连接）
    stopEventLoop();

    // 3. 关闭所有现有连接
    closeAllConnections();

    // 4. 等待所有连接关闭（带超时）
    if (!waitForAllConnectionsClosed(10))
    {
        forceCloseAllConnections();
    }
}

// 客户端 发送消息（数据） 的回调
// 函数将根据不同的消息类型，做出相应的处理
void ClusterChatServer::onMessage(const muduo::net::TcpConnectionPtr &pConn, muduo::net::Buffer *buffer, muduo::Timestamp timetmp)
{
    // 从buffer获取客户端发送过来的数据
    std::string buf = buffer->retrieveAllAsString();

    // 数据反序列化
    nlohmann::json jsn = nlohmann::json::parse(buf);

    // 获取处理消息的函数
    auto msgHandle = (ClusterChatService::getInstance())->getHandle(jsn["msgId"].get<int>());

    // 处理消息
    msgHandle(pConn, jsn, timetmp);
}