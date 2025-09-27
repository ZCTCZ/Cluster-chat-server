#include "../../include/server/server.h"
#include "../../include/server/service.h"
#include <signal.h>
#include <muduo/base/Logging.h>
#include <iostream>

muduo::net::EventLoop loop;
std::unique_ptr<ClusterChatServer> pServer;

void signalHandler(int sig)
{
    if (sig == SIGINT || sig == SIGTERM || sig == SIGQUIT || sig == SIGSTOP)
    {
        loop.quit();
    }
}

// 注册信号捕捉
void registerSignal()
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa)); // 同时清空信号掩码和信号处理标志
    sa.sa_handler = signalHandler;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGSTOP, &sa, NULL);
}

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " ip port" << std::endl;
    }
    registerSignal();
    muduo::net::InetAddress addr(argv[1], static_cast<uint16_t>(atoi(argv[2])));
    pServer = std::make_unique<ClusterChatServer>(&loop, addr, "ClusterCharServer");
    pServer->start();
    loop.loop();

    pServer->gracefulShutdown();
    LOG_INFO << "ClusterChatServer exit!";
    return 0;
}