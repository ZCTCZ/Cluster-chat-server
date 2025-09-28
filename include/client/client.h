#include "../../thirdtools/json.hpp"
#include "../server/model/user.h"
#include "../server/model/group.h"
#include "../../include/public.h"

#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <unordered_map>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

extern std::mutex mtx;
extern std::condition_variable cv;
extern std::atomic<bool> loginSuccess;
extern bool isRunning;

using std::cout;
using std::endl;
using std::cin;
using std::cerr;
using std::unordered_map;
using std::function;

// 登录页面
int loginMenu();

// 显示当前登录的用户的信息
void showCurrentUserData(unsigned int socketfd = 0, std::string str = "");

// 接收数据的线程的回调函数
void receiveTaskHandler(unsigned int socketfd);

// 获取系统时间
std::string getCurrentTime();

// 注册
void registerOpt(unsigned int socketfd);

// 处理服务器返回的注册响应消息
void handleRegisterReply(const nlohmann::json &recvJson);

// 登录
void loginOpt(unsigned int socketfd);

// 处理服务器返回的登录响应消息
void handleLoginReply(const nlohmann::json& jsn);

// 主菜单
void mainMenu(unsigned int socketfd);

// 帮助手册
void help(unsigned int socketfd = 0, std::string str = "");

// 一对一聊天
void chat(unsigned int socketfd, std::string str);

// 添加好友
void addfriend(unsigned int socketfd, std::string str);

// 创建群组
void creategroup(unsigned int socketfd, std::string str);

// 加入群组
void addgroup(unsigned int socketfd, std::string str);

// 群聊
void groupchat(unsigned int socketfd, std::string str);

// 注销
void logout(unsigned int socketfd = 0, std::string str = "");
