#include "../../thirdtools/json.hpp"
#include "../server/model/user.h"
#include "../server/model/group.h"
#include "../../include/public.h"

#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <unordered_map>
#include <functional>
using std::cout;
using std::endl;
using std::cin;
using std::cerr;
using std::unordered_map;
using std::function;


extern User g_currentUser; // 全局变量声明，记录当前登录的用户的信息
extern std::vector<User> g_friendList; // 全局变量声明，记录当前用户的好友列表
extern std::vector<Group> g_groupList; // 全局变量声明，记录当前用户的群组列表
extern bool mainMenuRunning; // 全局变量声明，记录主菜单是否正在运行

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

// 登录
void loginOpt(unsigned int socketfd);

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
