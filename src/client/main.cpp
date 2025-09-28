#include "../../include/client/client.h"
#include <string>
#include <memory>
bool isRunning = true; 

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        cerr << argv[0] << " ip port" << endl;
        exit(-1);
    }

    std::string IP = argv[1];
    uint16_t PORT = atoi(argv[2]);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == fd)
    {
        cerr << "create socket error" << endl;
        exit(-1);
    }

    std::unique_ptr<int, void(*)(int*)> psockfd(new int(fd), [](int *p){
        if (p && *p > 0)
        {
            close(*p);
            delete p;
        }
    });

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, IP.c_str(), &serverAddr.sin_addr.s_addr);

    if (-1 == connect(fd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)))
    {
        cerr << "connect error" << endl;
        exit(-1);
    }

    // 连接上了服务器，开启子线程，负责接收服务器发送过来的数据
    std::thread receiveTask(receiveTaskHandler, fd);
    receiveTask.detach();

    while (isRunning)
    {
        int choice = loginMenu(); //登录页面
        switch (choice)
        {
            case 1:
                loginOpt(fd);    //登录
                break;
            case 2:
                registerOpt(fd); //注册 
                break;
            case 3:
                isRunning = false;
                break;
            default:
                cerr << "invalid choice" << endl;
                break;
        }
    }
    
    return 0;
}