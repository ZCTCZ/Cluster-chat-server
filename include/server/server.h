/**
 * @file server.h
 * @author Loopy (2932742577@qq.com)
 * @brief 网络模块
 * @version 1.0
 * @date 2025-09-17
 * 
 * 
 */

#ifndef SERVER_SERVER_H
#define SERVER_SERVER_H

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <atomic>
#include <condition_variable>

// 服务器类
class ClusterChatServer
{
public:
	// 初始化服务器
	ClusterChatServer(muduo::net::EventLoop *pLoop,
					  const muduo::net::InetAddress &listenAddr,
					  const muduo::string &nameArg);
					  
	~ClusterChatServer();
	// 启动服务
	void start();

	// 优雅关闭服务器
	void gracefulShutdown(); 

private:
	void onConnected(const muduo::net::TcpConnectionPtr &pConn); // 处理连接的 请求/断开
	void onMessage(const muduo::net::TcpConnectionPtr &pConn, muduo::net::Buffer *buffer, muduo::Timestamp timetmp); // 处理I/O
	void closeAllConnections(); // 优雅地关闭所有连接
	void forceCloseAllConnections(); // 强制关闭所有连接,一般发生在超时的时候
	bool waitForAllConnectionsClosed(int timeoutSeconds); // 等待所有连接关闭,超时时间为timeoutSeconds秒
	void stopEventLoop(); // 停止主事件循环

	muduo::net::TcpServer _server;
	muduo::net::EventLoop *_pLoop;

	std::atomic<bool> _shuttingDown; // 服务器是否关闭
    std::condition_variable _shutdownCondition;
};

#endif