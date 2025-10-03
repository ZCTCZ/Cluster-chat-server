/**
 * @file service.h
 * @author Loopy (2932742577@qq.com)
 * @brief 业务模块
 * @version 1.0
 * @date 2025-09-17
 * 
 * 
 */

#ifndef SERVICE_H
#define SERVICE_H

#include <unordered_map>
#include <functional>
#include <muduo/net/TcpConnection.h>
#include <mutex>

#include "../../thirdtools/json.hpp"
#include "model/usermodel.h"
#include "model/offlinemessagemodel.h"
#include "model/friendmodel.h"
#include "model/groupmodel.h"
#include "redis/redis.h"
#include "db/MySQLConnectionPool.h"

// 处理业务的类
class ClusterChatService
{
public:
    ClusterChatService(const ClusterChatService&) = delete;
    ClusterChatService& operator=(ClusterChatService&) = delete;

    std::mutex _mtx; // 互斥变量
    
    using MsgHandle = std::function<void(const muduo::net::TcpConnectionPtr& pConn, nlohmann::json jsn, muduo::Timestamp timetmp)>;
    
    // 使用单例模式，获取业务类的实例
    static ClusterChatService* getInstance();

    // 处理登录业务
    void handleLogin(const muduo::net::TcpConnectionPtr& pConn, const nlohmann::json& jsn, muduo::Timestamp timetmp);
    
    // 处理注册业务
    void handleRegister(const muduo::net::TcpConnectionPtr& pConn, const nlohmann::json& jsn, muduo::Timestamp timetmp);

    // 处理私聊业务
    void handlePrivateChat(const muduo::net::TcpConnectionPtr& pConn, const nlohmann::json& jsn, muduo::Timestamp timetmp);
    
    // 处理添加好友业务
    void handleAddFriend(const muduo::net::TcpConnectionPtr& pConn, const nlohmann::json& jsn, muduo::Timestamp timetmp);

    // 处理创建群组业务
    void handleCreateGroup(const muduo::net::TcpConnectionPtr& pConn, const nlohmann::json& jsn, muduo::Timestamp timetmp);

    // 处理加入群组业务
    void handleJoinGroup(const muduo::net::TcpConnectionPtr& pConn, const nlohmann::json& jsn, muduo::Timestamp timetmp);

    // 处理群聊业务
    void handleGroupChat(const muduo::net::TcpConnectionPtr& pConn, const nlohmann::json& jsn, muduo::Timestamp timetmp);
    
    // 获取事件的回调函数
    MsgHandle getHandle(int msgId);

    // 获取用户的TCP连接映射
    std::unordered_map<int, muduo::net::TcpConnectionPtr>& getUserConnMap();

    // 处理客户端异常退出（将用户从用户连接映射中移除，更新用户登录状态）
    void handleClientExit(const muduo::net::TcpConnectionPtr& pConn);

    // 处理客户端正常退出，不关闭通信套接字
    void handleClientLogout(const muduo::net::TcpConnectionPtr &pConn, const nlohmann::json &jsn, muduo::Timestamp timetmp);

    // 处理从Redis发来的订阅消息
    void handleRedisSubscribeMessage(unsigned int channel, const std::string& message); 

private:
    std::unordered_map<int, MsgHandle> _msgHandleMap; // {具体业务, 业务处理函数}，其中业务用枚举类型表示

    std::unordered_map<int, muduo::net::TcpConnectionPtr> _userConnMap; // {userID, TCP连接}，记录用户的TCP连接(类似于通信套接字)

    UserModel _userModel; // 处理数据库中用户表的类

    OfflineMessageModel _offlineMessageModel; // 处理离线消息的类

    FriendModel _friendModel; // 处理好友关系表的类

    GroupModel _groupModel; // 处理群组相关操作的类

    Redis _redis;          // 处理Redis发布订阅操作的类

    // 将构造函数私有化
    ClusterChatService();
};

#endif