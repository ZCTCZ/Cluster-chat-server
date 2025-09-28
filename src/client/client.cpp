#include "../../include/client/client.h"
#include <iomanip>
#include <thread>

User g_currentUser;                     // 全局变量，记录当前登录的用户的信息
std::vector<User> g_friendList;         // 全局变量，记录当前用户的好友列表
std::vector<Group> g_groupList;         // 全局变量，记录当前用户的群组列表
std::vector<std::string> g_messageList; // 全局变量，记录当前用户的离线消息
bool mainMenuRunning = false;           // 全局变量，记录主菜单是否正在运行

std::mutex mtx;
std::condition_variable cv;
std::atomic<bool> loginSuccess = {false}; // 登录成功标志

// 系统支持的客户端命令列表
unordered_map<std::string, std::string> commandMap = {
    {"help", "显示所有支持的命令，格式 help"},
    {"chat", "一对一聊天，格式 chat:friendid:message"},
    {"addfriend", "添加好友，格式 addfriend:friendid"},
    {"creategroup", "创建群组，格式 creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组，格式 addgroup:groupid"},
    {"groupchat", "群聊，格式 groupchat:groupid:message"},
    {"logout", "注销，格式 logout"},
    {"info", "显示当前登录的用户信息，格式 info"}};

// 注册系统支持的客户端命令处理
unordered_map<std::string, function<void(unsigned int, std::string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"logout", logout},
    {"info", showCurrentUserData}};

// 获取当前时间
std::string getCurrentTime()
{
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm tm_time{};
    localtime_r(&now_time, &tm_time); // Ubuntu 线程安全
    std::ostringstream oss;
    oss << std::put_time(&tm_time, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// 显示当前登录的用户的信息
void showCurrentUserData(unsigned int socketfd, std::string str)
{
    cout << "------------------------------ login user info ------------------------------" << endl;
    cout << "ID: " << g_currentUser.getId() << "   " << "name: " << g_currentUser.getName() << endl;

    if (!g_messageList.empty())
    {
        cout << endl;
        cout << "------------------------------  offline message  ------------------------------" << endl;
        for (const auto &msg : g_messageList)
        {
            cout << msg << endl;
        }

        memset((void *)&g_messageList, 0, sizeof(g_messageList));
    }

    if (!g_friendList.empty())
    {
        cout << endl;
        cout << "------------------------------  friend   list  ------------------------------" << endl;
        for (const auto &e : g_friendList)
        {
            cout << "ID: " << std::setw(3) << std::left << e.getId()
                 << "name: " << std::setw(15) << std::left << e.getName()
                 << "state: " << std::setw(10) << std::left << e.getState()
                 << endl;
        }
    }

    if (!g_groupList.empty())
    {
        cout << endl;
        cout << "------------------------------  group    list  ------------------------------" << endl;
        for (const auto &e : g_groupList)
        {
            cout << "group  ID: " << std::setw(3) << std::left << e.getGroupId()
                 << "group name: " << std::setw(15) << std::left << e.getGroupName()
                 << "desc: " << e.getGroupDescription() << std::left << endl;
            for (const auto &user : e.getGroupMembers())
            {
                cout << "member ID: " << std::setw(3) << std::left << user.getId()
                     << "name: " << std::setw(15) << std::left << user.getName()
                     << "state: " << std::setw(10) << std::left << user.getState()
                     << "role: " << std::setw(10) << std::left << user.getRole() << endl;
            }

            cout << endl;
        }
    }

    cout << "------------------------------       end       ------------------------------" << endl;
}

// 登录页面
int loginMenu()
{
    cout << "=============== login menu ===============" << endl;
    cout << "1. login" << endl;
    cout << "2. register" << endl;
    cout << "3. exit" << endl;
    cout << "==========================================" << endl;
    cout << "please input your choice: ";
    int choice;
    cin >> choice;
    cin.get(); // 吃掉回车
    return choice;
}

/**
 * @brief 注册操作
 * {
 *      "msgId": REGISTER_MSG,
 *      ""name": "username",
 *      "password": "password"
 * }
 * @param socketfd
 */

void registerOpt(unsigned int socketfd)
{
    std::string name;
    std::string password;

    cout << "username:";
    getline(cin, name);
    cout << "password:";
    getline(cin, password);

    nlohmann::json jsn;
    jsn["msgId"] = REGISTER_MSG;
    jsn["name"] = name;
    jsn["password"] = password;
    std::string sendData = jsn.dump(); // 需要先序列化，再计算长度
    uint32_t len = static_cast<uint32_t>(sendData.size());
    uint32_t netLen = htonl(len);
    sendData.insert(0, (char *)&netLen, sizeof(netLen));

    if (send(socketfd, sendData.data(), sendData.size(), 0) == -1) // 向服务器发送注册请求
    {
        perror("send error");
        cerr << "register failed" << endl;
    }
    else
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock); // 等待被处理注册响应消息的线程唤醒
    }
}

// 处理服务器返回的注册响应消息
void handleRegisterReply(const nlohmann::json &recvJson)
{
    if (recvJson["code"] == 0) // 注册成功
    {
        cout << "id:" << recvJson["id"] << "    " << "name:" << recvJson["name"] << endl;
    }
    else
    {
        cout << "The name is already registered" << endl;
    }

    cout << "[" << recvJson["msg"].get<std::string>() << "]" << endl;
    cv.notify_one(); // 唤醒等主线程
}

/**
 * @brief 登录操作
 * {
 *      "msgId": LOGIN_MSG,
 *      "name": "username",
 *      "password": "password"
 * }
 * @param socketfd
 */
void loginOpt(unsigned int socketfd)
{
    std::string name;
    std::string password;
    cout << "username:";
    getline(cin, name);
    cout << "password:";
    getline(cin, password);

    nlohmann::json jsn;
    jsn["msgId"] = LOGIN_MSG;
    jsn["name"] = name;
    jsn["password"] = password;
    std::string sendData = jsn.dump();
    uint32_t len = static_cast<uint32_t>(sendData.size());
    uint32_t netLen = htonl(len);
    sendData.insert(0, (char *)&netLen, sizeof(netLen));

    if (send(socketfd, sendData.data(), sendData.size(), 0) == -1) // 向服务器发送登录请求
    {
        perror("send error");
        cerr << "login failed" << endl;
    }
    else
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock);           // 等待被处理登录响应消息的线程唤醒
        if (loginSuccess.load()) // 登录成功
        {
            // 显示登录成功的用户信息
            showCurrentUserData();

            // 进入聊天主菜单
            mainMenu(socketfd);
        }
    }
}

// 处理服务器返回的登录响应消息
void handleLoginReply(const nlohmann::json &recvJson)
{
    if (recvJson["code"] == 0) // 登录成功
    {
        mainMenuRunning = true; // 设置主菜单正在运行

        memset((void *)&g_currentUser, 0, sizeof(User));
        memset((void *)&g_friendList, 0, sizeof(g_friendList));
        memset((void *)&g_groupList, 0, sizeof(g_groupList));
        memset((void *)&g_messageList, 0, sizeof(g_messageList));

        g_currentUser.setId(recvJson["id"].get<unsigned int>());
        g_currentUser.setName(recvJson["name"].get<std::string>());

        if (recvJson.contains("offlineMessage")) // 存在离线消息,将离线消息加入到g_messageList中
        {
            std::vector<std::string> msgVec = recvJson["offlineMessage"].get<std::vector<std::string>>();
            for (const auto &msg : msgVec)
            {
                nlohmann::json msgJson = nlohmann::json::parse(msg);
                std::string str;
                if (msgJson["msgId"].get<int>() == PRIVATE_CHAT_MSG)
                {
                    str = "[" + msgJson["time"].get<std::string>() + "]" +
                          "[userId:" + std::to_string(msgJson["from"].get<unsigned int>()) + "]" +
                          "[name:" + msgJson["user"].get<std::string>() + "]" +
                          "said: " + msgJson["message"].get<std::string>();
                }
                else if (msgJson["msgId"].get<int>() == GROUP_CHAT_MSG)
                {
                    str = "[" + msgJson["time"].get<std::string>() + "]" +
                          "[userId:" + std::to_string(msgJson["userId"].get<unsigned int>()) + "]" +
                          "[name:" + msgJson["user"].get<std::string>() + "]" +
                          "at group[" + std::to_string(msgJson["groupId"].get<unsigned int>()) + "]" +
                          "said: " + msgJson["message"].get<std::string>();
                }
                g_messageList.push_back(str);
            }
        }

        if (recvJson.contains("friends")) // 存在好友列表,将好友列表加入到g_friendList中
        {
            std::vector<std::string> friendVec = recvJson["friends"].get<std::vector<std::string>>();
            for (const auto &friendStr : friendVec)
            {
                nlohmann::json msgJson = nlohmann::json::parse(friendStr);
                User user;
                user.setId(msgJson["id"].get<unsigned int>());
                user.setName(msgJson["name"].get<std::string>());
                user.setState(msgJson["state"].get<std::string>());
                g_friendList.push_back(user);
            }
        }

        if (recvJson.contains("groups")) // 存在群组列表，将群组列表加入到g_groupList中
        {
            std::vector<std::string> groupVec = recvJson["groups"].get<std::vector<std::string>>();
            for (const auto &groupStr : groupVec)
            {
                nlohmann::json msgJson = nlohmann::json::parse(groupStr);
                Group group;
                group.setGroupId(msgJson["groupId"].get<unsigned int>());
                group.setGroupName(msgJson["groupName"].get<std::string>());
                group.setGroupDescription(msgJson["groupDesc"].get<std::string>());

                std::vector<std::string> membersInfo = msgJson["groupMembers"].get<std::vector<std::string>>();
                for (const auto &memberStr : membersInfo)
                {
                    nlohmann::json memberJson = nlohmann::json::parse(memberStr);
                    GroupUser user;
                    user.setId(memberJson["id"].get<unsigned int>());
                    user.setName(memberJson["name"].get<std::string>());
                    user.setState(memberJson["state"].get<std::string>());
                    user.setRole(memberJson["role"].get<std::string>());
                    group.getGroupMembers().push_back(user);
                }
                g_groupList.push_back(group);
            }
        }
        cout << "[" << recvJson["msg"].get<std::string>() << "]" << endl;
        loginSuccess.store(true); // 设置登录成功标志
    }
    else if (recvJson["code"] == -1) // 用户状态为已在线，无法重复登录
    {
        cout << "[" << recvJson["msg"].get<std::string>() << "]" << endl;
    }
    else if (recvJson["code"] == -2) // 用户名或者密码错误
    {
        cout << "[" << recvJson["msg"].get<std::string>() << "]" << endl;
    }

    cv.notify_one(); // 唤醒主线程
}

// 接收数据的线程的回调函数
void receiveTaskHandler(unsigned int socketfd)
{
    while (true)
    {
        uint32_t netLen = 0; // 用来存放数据包头部的长度信息

        // Step 1: 接收数据包头部的长度信息
        ssize_t head_done = 0;
        while (head_done < 4)
        {
            ssize_t recvLen = recv(socketfd, (char *)&netLen + head_done, 4 - head_done, 0);
            if (recvLen == -1)
            {
                perror("recv error");
                mainMenuRunning = false;
                isRunning = false;
                return;
            }
            else if (recvLen == 0)
            {
                cout << "server closed" << endl;
                mainMenuRunning = false;
                isRunning = false;
                return;
            }
            else
            {
                head_done += recvLen;
            }
        }

        uint32_t hostLen = ntohl(netLen);
        if (hostLen > 64 * 1024)
        {
            mainMenuRunning = false;
            isRunning = false;
            return;
        }

        // Step 2: 接收数据包的body信息
        char buffer[1024 * 64] = {0};
        ssize_t body_done = 0;
        while (body_done < hostLen)
        {
            ssize_t recvLen = recv(socketfd, buffer + body_done, hostLen - body_done, 0);
            if (recvLen == -1)
            {
                perror("recv error");
                mainMenuRunning = false;
                isRunning = false;
                return;
            }
            else if (recvLen == 0)
            {
                cout << "server closed" << endl;
                mainMenuRunning = false;
                isRunning = false;
                return;
            }
            else
            {
                body_done += recvLen;
            }
        }

        std::string jsonStr(buffer, hostLen);
        nlohmann::json recvJson = nlohmann::json::parse(jsonStr); // 反序列化服务器发送过来的数据
        if (recvJson["msgId"].get<int>() == PRIVATE_CHAT_MSG)     // 收到私聊消息
        {
            cout << "[" << recvJson["time"].get<std::string>() << "]"
                 << "[userId:" << recvJson["from"].get<unsigned int>() << "]"
                 << "[name:" << recvJson["user"].get<std::string>() << "]"
                 << "said: " << recvJson["message"].get<std::string>() << endl;
        }
        else if (recvJson["msgId"].get<int>() == PRIVATE_CHAT_MSG_REPLY)
        {
            cout << "[" << recvJson["msg"].get<std::string>() << "]" << endl;
        }
        else if (recvJson["msgId"].get<int>() == ADD_FRIEND_MSG_REPLY)
        {
            cout << "[" << recvJson["msg"].get<std::string>() << "]" << endl;
        }
        else if (recvJson["msgId"].get<int>() == CREATE_GROUP_MSG_REPLY)
        {
            cout << "[" << recvJson["msg"].get<std::string>() << "]" << endl;
        }
        else if (recvJson["msgId"] == JOIN_GROUP_MSG_REPLY)
        {
            cout << "[" << recvJson["msg"].get<std::string>() << "]" << endl;
        }
        else if (recvJson["msgId"].get<int>() == GROUP_CHAT_MSG)
        {
            cout << "[" << recvJson["time"].get<std::string>() << "]"
                 << "[userId:" << recvJson["userId"].get<unsigned int>() << "]"
                 << "[name:" << recvJson["user"].get<std::string>() << "]"
                 << "at group[" << recvJson["groupId"].get<unsigned int>() << "]"
                 << "said: " << recvJson["message"].get<std::string>() << endl;
        }
        else if (recvJson["msgId"] == GROUP_CHAT_MSG_REPLY)
        {
            cout << "[" << recvJson["msg"].get<std::string>() << "]" << endl;
        }
        else if (recvJson["msgId"] == LOGIN_MSG_REPLY)
        {
            handleLoginReply(recvJson);
        }
        else if (recvJson["msgId"] == REGISTER_MSG_REPLY)
        {
            handleRegisterReply(recvJson);
        }
        else if (recvJson["msgId"] == LOGOUT_MSG_REPLY)
        {
            cout << "[" << recvJson["msg"].get<std::string>() << "]" << endl;
        }
    }
}
// 主菜单
void mainMenu(unsigned int socketfd)
{
    help(); // 显示帮助手册

    std::string buffer;
    // 获取用户输入的命令
    while (mainMenuRunning)
    {
        getline(cin, buffer);
        std::string command; // 命令

        auto index = buffer.find_first_of(':');
        if (index == std::string::npos) // 没有冒号，说明没有参数，只有命令
        {
            command = buffer;
        }
        else
        {
            command = buffer.substr(0, index);
        }

        auto it = commandHandlerMap.find(command);
        if (it == commandHandlerMap.end())
        {
            cerr << "invalid command" << endl;
            continue;
        }
        else
        {
            (it->second)(socketfd, buffer.substr(index + 1)); // 执行相应的命令
        }
    }
}

// 帮助手册
void help(unsigned int socketfd, std::string str)
{
    cout << "=============== help menu ===============" << endl;
    for (const auto &cmd : commandMap)
    {
        cout << cmd.first << " : " << cmd.second << endl;
    }
    cout << "===============    end    ===============" << endl;
    cout << endl;
}

// 一对一聊天
//{"chat",        "一对一聊天，格式 chat:friendid:message"}
/* {
 *     "msgId": PRIVATE_CHAT_MSG,
 *     "from": "from_user_id",
 *     "user": "user_name"
 *     "to": "to_user_id",
 *     "message": "hello world"
 *     "time": "%Y-%m-%d %H:%M:%S"
 * }
 */
void chat(unsigned int socketfd, std::string str)
{
    auto index = str.find_first_of(':');
    if (index == std::string::npos)
    {
        cerr << "invalid command" << endl;
        return;
    }
    unsigned int friendId = std::stoi(str.substr(0, index));
    std::string message = str.substr(index + 1);

    nlohmann::json sendJson;
    sendJson["msgId"] = PRIVATE_CHAT_MSG;
    sendJson["from"] = g_currentUser.getId();
    sendJson["user"] = g_currentUser.getName();
    sendJson["to"] = friendId;
    sendJson["message"] = message;
    sendJson["time"] = getCurrentTime();

    std::string sendData = sendJson.dump();
    uint32_t len = static_cast<uint32_t>(sendData.size());
    uint32_t netLen = htonl(len);
    
    sendData.insert(0, (char *)&netLen, sizeof(netLen));
    if (send(socketfd, sendData.data(), sendData.size(), 0) == -1)
    {
        perror("send error");
        cerr << "chat failed" << endl;
    }
}

// 添加好友
//{"addfriend",   "添加好友，格式 addfriend:friendid"}
/*{
 *      "msgId": ADD_FRIEND_MSG,
 *      "id" : "user_id",
 *      "friendId" : "friend_id"
 * }
 */
void addfriend(unsigned int socketfd, std::string str)
{
    nlohmann::json sendJson;
    sendJson["msgId"] = ADD_FRIEND_MSG;
    sendJson["id"] = g_currentUser.getId();
    sendJson["friendId"] = std::stoi(str);

    std::string sendData = sendJson.dump();
    uint32_t len = static_cast<uint32_t>(sendData.size());
    uint32_t netLen = htonl(len);
    sendData.insert(0, (char *)&netLen, sizeof(netLen));

    if (send(socketfd, sendData.data(), sendData.size(), 0) == -1)
    {
        perror("send error");
        cerr << "add friend failed" << endl;
    }
}

// 创建群组
// {"creategroup", "创建群组，格式 creategroup:groupname:groupdesc"}
/*
 * {
 *      "msgId": CREATE_GROUP_MSG,
 *      "userId": "user_id", // 群主
 *      "groupName": "group_name",
 *      "groupDesc": "group_description"
 * }
 */
void creategroup(unsigned int socketfd, std::string str)
{
    auto index = str.find_first_of(':');
    if (index == std::string::npos)
    {
        cerr << "invalid command" << endl;
        return;
    }
    std::string groupName = str.substr(0, index);
    std::string groupDesc = str.substr(index + 1);

    nlohmann::json sendJson;
    sendJson["msgId"] = CREATE_GROUP_MSG;
    sendJson["userId"] = g_currentUser.getId();
    sendJson["groupName"] = groupName;
    sendJson["groupDesc"] = groupDesc;

    std::string sendData = sendJson.dump();
    uint32_t len = static_cast<uint32_t>(sendData.size());
    uint32_t netLen = htonl(len);
    sendData.insert(0, (char *)&netLen, sizeof(netLen));

    if (send(socketfd, sendData.data(), sendData.size(), 0) == -1)
    {
        perror("send error");
        cerr << "create group: " << groupName << " failed" << endl;
    }
}
// 加入群组
// {"addgroup",    "加入群组，格式 addgroup:groupid"}
/*
 * {
 *      "msgId": JOIN_GROUP_MSG,
 *      "userId": "user_id",
 *      "groupId": "group_id",
 * }
 */
void addgroup(unsigned int socketfd, std::string str)
{
    nlohmann::json sendJson;
    sendJson["msgId"] = JOIN_GROUP_MSG;
    sendJson["userId"] = g_currentUser.getId();
    sendJson["groupId"] = std::stoi(str);

    std::string sendData = sendJson.dump();
    uint32_t len = static_cast<uint32_t>(sendData.size());
    uint32_t netLen = htonl(len);
    sendData.insert(0, (char *)&netLen, sizeof(netLen));

    if (send(socketfd, sendData.data(), sendData.size(), 0) == -1)
    {
        perror("send error");
        cerr << "join group failed" << endl;
    }
}

// 群聊
// {"groupchat",   "群聊，格式 groupchat:groupid:message"}
/*
 * {
 *      "msgId": GROUP_CHAT_MSG,
 *      "userId": "user_id",
 *      "user": "user_name",
 *      "groupId": "group_id",
 *      "message": "hello world"
 *      "time": "%Y-%m-%d %H:%M:%S"
 * }
 */
void groupchat(unsigned int socketfd, std::string str)
{
    auto index = str.find_first_of(":");
    if (index == std::string::npos)
    {
        cerr << "invalid command" << endl;
        return;
    }
    unsigned int groupId = std::stoi(str.substr(0, index));
    std::string message = str.substr(index + 1);

    nlohmann::json sendJson;
    sendJson["msgId"] = GROUP_CHAT_MSG;
    sendJson["userId"] = g_currentUser.getId();
    sendJson["user"] = g_currentUser.getName();
    sendJson["groupId"] = groupId;
    sendJson["message"] = message;
    sendJson["time"] = getCurrentTime();

    std::string sendData = sendJson.dump();
    uint32_t len = static_cast<uint32_t>(sendData.size());
    uint32_t netLen = htonl(len);
    sendData.insert(0, (char *)&netLen, sizeof(netLen));

    if (send(socketfd, sendData.data(), sendData.size(), 0) == -1)
    {
        perror("send error");
        cerr << "group chat failed" << endl;
    }
}

// 注销
// {"logout",    "注销，格式 logout"}
/* {
 *         "msgId": LOGOUT_MSG,
 *         "userId": "user_id"
 * }
 */
void logout(unsigned int socketfd, std::string str)
{
    nlohmann::json sendJson;
    sendJson["msgId"] = LOGOUT_MSG;
    sendJson["userId"] = g_currentUser.getId();

    std::string sendData = sendJson.dump();
    uint32_t len = static_cast<uint32_t>(sendData.size());
    uint32_t netLen = htonl(len);
    sendData.insert(0, (char *)&netLen, sizeof(netLen));

    if (send(socketfd, sendData.data(), sendData.size(), 0) == -1)
    {
        perror("send error");
        cerr << "logout failed" << endl;
    }

    mainMenuRunning = false;
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 主线程睡眠100毫秒，等待接收数据的线程接收完服务器的数据
}