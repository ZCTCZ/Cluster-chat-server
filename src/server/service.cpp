#include "../../include/server/service.h"
#include "../../include/public.h"
#include <muduo/base/Logging.h>

ClusterChatService::ClusterChatService()
{
    // 登录事件 的回调函数
    _msgHandleMap.insert({LOGIN_MSG, [this](const muduo::net::TcpConnectionPtr &pConn, nlohmann::json jsn, muduo::Timestamp timetmp)
                          {
                              this->handleLogin(pConn, jsn, timetmp);
                          }});

    // 注册事件 的回调函数
    _msgHandleMap.insert({REGISTER_MSG, [this](const muduo::net::TcpConnectionPtr &pConn, nlohmann::json jsn, muduo::Timestamp timetmp)
                          {
                              this->handleRegister(pConn, jsn, timetmp);
                          }});

    // 私聊事件 的回调函数
    _msgHandleMap.insert({PRIVATE_CHAT_MSG, [this](const muduo::net::TcpConnectionPtr &pConn, nlohmann::json jsn, muduo::Timestamp timetmp)
                          {
                              this->handlePrivateChat(pConn, jsn, timetmp);
                          }});

    // 添加好友事件 的回调函数
    _msgHandleMap.insert({ADD_FRIEND_MSG, [this](const muduo::net::TcpConnectionPtr &pConn, nlohmann::json jsn, muduo::Timestamp timetmp)
                          {
                              this->handleAddFriend(pConn, jsn, timetmp);
                          }});

    // 创建群组事件 的回调函数
    _msgHandleMap.insert({CREATE_GROUP_MSG, [this](const muduo::net::TcpConnectionPtr &pConn, nlohmann::json jsn, muduo::Timestamp timetmp)
                          {
                              this->handleCreateGroup(pConn, jsn, timetmp);
                          }});

    // 加入群组事件 的回调函数
    _msgHandleMap.insert({JOIN_GROUP_MSG, [this](const muduo::net::TcpConnectionPtr &pConn, nlohmann::json jsn, muduo::Timestamp timetmp)
                          {
                              this->handleJoinGroup(pConn, jsn, timetmp);
                          }});

    // 群聊事件 的回调函数
    _msgHandleMap.insert({GROUP_CHAT_MSG, [this](const muduo::net::TcpConnectionPtr &pConn, nlohmann::json jsn, muduo::Timestamp timetmp)
                          {
                              this->handleGroupChat(pConn, jsn, timetmp);
                          }});

    // 用户登出事件 的回调函数
    _msgHandleMap.insert({LOGOUT_MSG, [this](const muduo::net::TcpConnectionPtr &pConn, nlohmann::json jsn, muduo::Timestamp timetmp)
                          {
                              this->handleClientLogout(pConn, jsn, timetmp);
                          }});

    // 聊天服务器连接Redis服务器
    if (_redis.connect())
    {
        _redis.init_notify_handler([this](int channel, std::string message)
                                    { 
                                        this->handleRedisSubscribeMessage(channel, message); 
                                    });
    }

    // 启动数据库连接池
    auto pMcp = MySQLConnectionPool::getInstance();
    static std::once_flag flag;
	std::call_once(flag, [&] {
		pMcp->startProduceThread(); // 启动生产数据库连接的生产者线程
		pMcp->startMonitorThread(); // 启动监视数据库空闲连接存活时间的监视者线程 
		});
}

// 获取业务类的实例（懒汉模式）
ClusterChatService *ClusterChatService::getInstance()
{
    static ClusterChatService service;
    return &service;
}

// 获取事件的回调函数
ClusterChatService::MsgHandle ClusterChatService::getHandle(int msgId)
{
    if (_msgHandleMap.find(msgId) == _msgHandleMap.end()) // msgId对应的事件，不存在处理函数
    {
        return [msgId](const muduo::net::TcpConnectionPtr &pConn, nlohmann::json jsn, muduo::Timestamp timetmp)
        {
            LOG_ERROR << "msgId:" << msgId << " can't find handler";
        };
    }
    else
    {
        return _msgHandleMap[msgId];
    }
}

// 获取用户的TCP连接映射
std::unordered_map<int, muduo::net::TcpConnectionPtr> &ClusterChatService::getUserConnMap()
{
    std::lock_guard<std::mutex> lock(_mtx); // 操作_userConnMap这种共享资源需要加锁
    return _userConnMap;
}

/**
 * @brief 处理客户端登录业务
 * {
 *         "msgId": LOGIN_MSG,
 *         "name": "username",
 *         "password": "password"
 * }
 * @param pConn
 * @param jsn
 * @param timetmp
 */
void ClusterChatService::handleLogin(const muduo::net::TcpConnectionPtr &pConn, const nlohmann::json &jsn, muduo::Timestamp timetmp)
{
    // 反序列化，解析json数据
    std::string username = jsn["name"];
    std::string password = jsn["password"];

    User user = _userModel.QueryByName(username); // 根据用户名在数据库中查询用户信息

    nlohmann::json response;
    response["msgId"] = LOGIN_MSG_REPLY;

    if (user.getId() != 0 && user.getPassword() == password) // 用户ID存在，用户名和密码正确
    {
        std::lock_guard<std::mutex> lock(_mtx);            // 加锁保护整个登录流程
        User freshUser = _userModel.QueryByName(username); // 再查一次最新状态
        if (freshUser.getState() != "online")              // 用户状态为离线，可以登录
        {
            _userConnMap.insert({freshUser.getId(), pConn}); // 保存连接，处于加锁期间操作哈希map，不会出现竞态条件

            freshUser.setState("online");
            if (_userModel.UpdateState(freshUser)) // 更新用户状态成功
            {
                _redis.subscribe(freshUser.getId()); // 订阅Redis频道channel = userId，接收该用户的所有消息

                response["code"] = 0;
                response["msg"] = "login success";
                response["id"] = freshUser.getId();
                response["name"] = freshUser.getName();

                // 获取离线消息
                std::vector<std::string> vecMsg = _offlineMessageModel.getOfflineMessage(freshUser.getId());
                if (!vecMsg.empty())
                {
                    response["offlineMessage"] = vecMsg;
                    // 将离线消息从OfflineMessage表中删除
                    _offlineMessageModel.deleteOfflineMessage(freshUser.getId());
                }

                // 获取好友列表
                std::vector<User> vecUser = _friendModel.queryFriend(freshUser.getId());
                if (!vecUser.empty())
                {
                    std::vector<std::string> friendList;
                    for (const auto &user : vecUser)
                    {
                        nlohmann::json friendInfo;
                        friendInfo["id"] = user.getId();
                        friendInfo["name"] = user.getName();
                        friendInfo["state"] = user.getState();
                
                        friendList.push_back(friendInfo.dump()); // 将Json序列化为字符串后再存入
                    }
                    response["friends"] = friendList;
                }

                // 获取群组列表
                std::vector<Group> groupVec = _groupModel.queryGroups(freshUser.getId());
                if (!groupVec.empty())
                {
                    std::vector<std::string> groupList;
                    for (const auto &group : groupVec)
                    {
                        nlohmann::json groupInfo;
                        groupInfo["groupId"] = group.getGroupId();
                        groupInfo["groupName"] = group.getGroupName();
                        groupInfo["groupDesc"] = group.getGroupDescription();

                        std::vector<std::string> groupMembers;
                        for (const auto &user : group.getGroupMembers())
                        {
                            nlohmann::json userInfo;
                            userInfo["id"] = user.getId();
                            userInfo["name"] = user.getName();
                            userInfo["state"] = user.getState();
                            userInfo["role"] = user.getRole();
                            groupMembers.push_back(userInfo.dump());
                        }
                        groupInfo["groupMembers"] = groupMembers;
                        groupList.push_back(groupInfo.dump());
                    }
                    response["groups"] = groupList;
                }
            }
            else // 更新用户状态失败
            {
                response["code"] = -3;
                response["msg"] = "update state error";
            }
        }
        else // 用户状态为已在线，无法重复登录
        {
            response["code"] = -1;
            response["msg"] = "user is online";
        }
    }
    else // 用户名或密码错误
    {
        response["code"] = -2;
        response["msg"] = "username or password error";
    }

    std::string sendData = response.dump();
    uint32_t len = static_cast<uint32_t>(sendData.length()); //获取JSON字符串长度,不统计最后的\0
    uint32_t netLen = muduo::net::sockets::hostToNetwork32(len); //转换为网络字节序

    muduo::net::Buffer buf;
    buf.append(&netLen, sizeof(netLen)); // 添加4字节头
    buf.append(sendData);                // 添加JSON字符串,但最后的\0不包括在内

    pConn->send(&buf);
}

/**
 * @brief 处理客户端注册业务
 * {
 *         "msgId": REGISTER_MSG,
 *         "name": "username",
 *         "password": "password"
 * }
 * @param pConn
 * @param jsn
 * @param timetmp
 */
void ClusterChatService::handleRegister(const muduo::net::TcpConnectionPtr &pConn, const nlohmann::json &jsn, muduo::Timestamp timetmp)
{
    // 反序列化，解析json数据
    std::string username = jsn["name"];
    std::string password = jsn["password"];

    User user(username, password);

    nlohmann::json response;
    response["msgId"] = REGISTER_MSG_REPLY;

    if (_userModel.Insert(user))
    {

        response["code"] = 0;
        response["msg"] = "register success";
        response["id"] = user.getId();
        response["name"] = user.getName();
    }
    else
    {
        response["code"] = -1;
        response["msg"] = "register failed";
    }

    std::string sendData = response.dump();
    uint32_t len = static_cast<uint32_t>(sendData.length()); //获取JSON字符串长度,不统计最后的\0
    uint32_t netLen = muduo::net::sockets::hostToNetwork32(len); //转换为网络字节序

    muduo::net::Buffer buf;
    buf.append(&netLen, sizeof(netLen)); // 添加4字节头
    buf.append(sendData);                // 添加JSON字符串,但最后的\0不包括在内

    pConn->send(&buf);
}

// 处理客户端异常退出（TCP连接断开）
void ClusterChatService::handleClientExit(const muduo::net::TcpConnectionPtr &pConn)
{
    auto it = _userConnMap.begin();

    User user;
    {
        std::lock_guard<std::mutex> lock(_mtx); // 操作_userConnMap这种共享资源需要加锁
        while (it != _userConnMap.end()) // 用户中注销，然后退出系统，则用户的ID在_userConnMap中找不到
        {
            if (it->second == pConn)
            {

                user.setId(it->first);
                user.setState("offline");
                _userConnMap.erase(it); // 处于加锁期间操作哈希map，不会出现竞态条件

                _redis.unsubscribe(user.getId()); // 取消Redis频道订阅

                break;
            }

            it++;
        }
    }

    if (it != _userConnMap.end())
    {
        _userModel.UpdateState(user);
    }
}

/**
 * @brief 处理客户端正常退出的业务，不关闭通信套接字
 * {
 *         "msgId": LOGOUT_MSG,
 *         "userId": "user_id"
 * }
 * @param pConn
 * @param jsn
 * @param timetmp
 */
void ClusterChatService::handleClientLogout(const muduo::net::TcpConnectionPtr &pConn, const nlohmann::json &jsn, muduo::Timestamp timetmp)
{
    unsigned int userId = jsn["userId"].get<unsigned int>();
    User user;

    {
        std::lock_guard<std::mutex> lock(_mtx); // 操作_userConnMap这种共享资源需要加锁
        _userConnMap.erase(userId);             // 将当前用户的ID从{ID, 连接}映射表中移除
    }

    user.setId(userId);
    user.setState("offline");
    _userModel.UpdateState(user); // 在数据库里面更新用户状态为离线

    _redis.unsubscribe(userId); // 取消Redis频道订阅

    nlohmann::json response;
    response["msgId"] = LOGOUT_MSG_REPLY;
    response["code"] = 0;
    response["msg"] = "logout success";

    std::string sendData = response.dump();
    uint32_t len = static_cast<uint32_t>(sendData.length()); //获取JSON字符串长度,不统计最后的\0
    uint32_t netLen = muduo::net::sockets::hostToNetwork32(len); //转换为网络字节序

    muduo::net::Buffer buf;
    buf.append(&netLen, sizeof(netLen)); // 添加4字节头
    buf.append(sendData);                // 添加JSON字符串,但最后的\0不包括在内

    pConn->send(&buf);
}

/**
 * @brief 处理客户端的私聊业务
 * {
 *     "msgId": PRIVATE_CHAT_MSG,
 *     "from": "from_user_id",
 *     "user": "user_name"
 *     "to": "to_user_id",
 *     "message": "hello world"
 *     "time": "%Y-%m-%d %H:%M:%S"
 * }
 *
 * @param pConn
 * @param jsn
 * @param timetmp
 */
void ClusterChatService::handlePrivateChat(const muduo::net::TcpConnectionPtr &pConn, const nlohmann::json &jsn, muduo::Timestamp timetmp)
{
    int toUserId = jsn["to"].get<int>();
    {
        std::lock_guard<std::mutex> lock(_mtx); // 操作_userConnMap这种共享资源需要加锁
        auto it = _userConnMap.find(toUserId);
        if (it != _userConnMap.end()) // 找到接收者的连接，说明该接受者在线
        {
            std::string sendData = jsn.dump();
            uint32_t len = static_cast<uint32_t>(sendData.length()); //获取JSON字符串长度,不统计最后的\0
            uint32_t netLen = muduo::net::sockets::hostToNetwork32(len); //转换为网络字节序

            muduo::net::Buffer buf;
            buf.append(&netLen, sizeof(netLen)); // 添加4字节头
            buf.append(sendData);                // 添加JSON字符串,但最后的\0不包括在内

            (it->second)->send(&buf); // 发送消息到接收者
        }
    }

    // 在服务器的 用户-连接映射表 中没有找到该用户，说明该用户没有在本台服务器上登录，或者没有在线
    auto fromUser = _userModel.QueryById(toUserId);
    if (fromUser.getState() == "online") // 接收者在线，只不过在其他的服务器上面登录了
    {
        // 将消息发送给redis频道，由订阅该频道的服务器接收
        _redis.publish(toUserId, jsn.dump());
    }
    else // 接受者不在线，将消息存入OfflineMessage表，待接收者上线后再发送，此时就不需要再存入前缀长度了
    {
        _offlineMessageModel.storeOfflineMessage(OfflineMessage(toUserId, jsn.dump())); // 将json序列化之后再存入数据库
    }

    nlohmann::json response;
    response["msgId"] = PRIVATE_CHAT_MSG_REPLY;
    response["code"] = 0;
    response["msg"] = "send success";

    std::string sendData = response.dump();
    uint32_t len = static_cast<uint32_t>(sendData.length()); //获取JSON字符串长度,不统计最后的\0
    uint32_t netLen = muduo::net::sockets::hostToNetwork32(len); //转换为网络字节序

    muduo::net::Buffer buf;
    buf.append(&netLen, sizeof(netLen)); // 添加4字节头
    buf.append(sendData);                // 添加JSON字符串,但最后的\0不包括在内

    pConn->send(&buf); // 发送消息到发送者, 告知消息发送成功
}

/**
 * @brief 处理客户端的添加好友业务
 * {
 *      "msgId": ADD_FRIEND_MSG,
 *      "id" : "user_id",
 *      "friendId" : "friend_id"
 * }
 * @param pConn
 * @param jsn
 * @param timetmp
 */
void ClusterChatService::handleAddFriend(const muduo::net::TcpConnectionPtr &pConn, const nlohmann::json &jsn, muduo::Timestamp timetmp)
{
    unsigned int userId = jsn["id"].get<unsigned int>();
    unsigned int friendId = jsn["friendId"].get<unsigned int>();

    nlohmann::json response;
    response["msgId"] = ADD_FRIEND_MSG_REPLY;

    if (_userModel.QueryById(friendId).getId() != 0) // 好友ID存在，可以添加
    {
        _friendModel.addFriend(Friend(userId, friendId));
        response["code"] = 0;
        response["msg"] = "add friend success";
    }
    else // 好友ID不存在，无法添加
    {
        response["code"] = -1;
        response["msg"] = "user not exist";
    }

    std::string sendData = response.dump();
    uint32_t len = static_cast<uint32_t>(sendData.length()); //获取JSON字符串长度,不统计最后的\0
    uint32_t netLen = muduo::net::sockets::hostToNetwork32(len); //转换为网络字节序

    muduo::net::Buffer buf;
    buf.append(&netLen, sizeof(netLen)); // 添加4字节头
    buf.append(sendData);                // 添加JSON字符串,但最后的\0不包括在内

    pConn->send(&buf); // 发送消息到发送者, 告知消息发送成功
}

/**
 * @brief 处理客户端的创建群组业务
 * {
 *      "msgId": CREATE_GROUP_MSG,
 *      "userId": "user_id", // 群主
 *      "groupName": "group_name",
 *      "groupDesc": "group_description"
 * }
 * @param pConn
 * @param jsn
 * @param timetmp
 */
// 处理创建群组业务
void ClusterChatService::handleCreateGroup(const muduo::net::TcpConnectionPtr &pConn, const nlohmann::json &jsn, muduo::Timestamp timetmp)
{
    unsigned int userId = jsn["userId"].get<unsigned int>();
    std::string groupName = jsn["groupName"].get<std::string>();
    std::string groupDescription = jsn["groupDesc"].get<std::string>();
    Group group(groupName, groupDescription);

    nlohmann::json response;
    response["msgId"] = CREATE_GROUP_MSG_REPLY;
    if (_groupModel.createGroup(group))
    {
        response["code"] = 0;
        response["msg"] = "create group success";
    }
    else
    {
        response["code"] = -1;
        response["msg"] = "create group failed";
    }

    std::string sendData = response.dump();
    uint32_t len = static_cast<uint32_t>(sendData.length()); //获取JSON字符串长度,不统计最后的\0
    uint32_t netLen = muduo::net::sockets::hostToNetwork32(len); //转换为网络字节序

    muduo::net::Buffer buf;
    buf.append(&netLen, sizeof(netLen)); // 添加4字节头
    buf.append(sendData);                // 添加JSON字符串,但最后的\0不包括在内

    pConn->send(&buf); // 发送消息到发送者, 告知消息发送成功

    if (response["code"] == 0)
    {
        // 将群组的创建人加入到GroupUser表中
        _groupModel.joinGroup(userId, group.getGroupId(), "Creator");
    }
}

/**
 * @brief 处理客户端的加入群组业务
 * {
 *      "msgId": JOIN_GROUP_MSG,
 *      "userId": "user_id",
 *      "groupId": "group_id",
 * }
 * @param pConn
 * @param jsn
 * @param timetmp
 */
void ClusterChatService::handleJoinGroup(const muduo::net::TcpConnectionPtr &pConn, const nlohmann::json &jsn, muduo::Timestamp timetmp)
{
    unsigned int userId = jsn["userId"].get<unsigned int>();
    unsigned int groupId = jsn["groupId"].get<unsigned int>();

    nlohmann::json response;
    response["msgId"] = JOIN_GROUP_MSG_REPLY;

    if (_groupModel.joinGroup(userId, groupId, "Normal"))
    {
        response["code"] = 0;
        response["msg"] = "join group success";
    }
    else
    {
        response["code"] = -1;
        response["msg"] = "join group failed";
    }

    std::string sendData = response.dump();
    uint32_t len = static_cast<uint32_t>(sendData.length()); //获取JSON字符串长度,不统计最后的\0
    uint32_t netLen = muduo::net::sockets::hostToNetwork32(len); //转换为网络字节序

    muduo::net::Buffer buf;
    buf.append(&netLen, sizeof(netLen)); // 添加4字节头
    buf.append(sendData);                // 添加JSON字符串,但最后的\0不包括在内

    pConn->send(&buf); // 发送消息到发送者, 告知消息发送成功
}

/**
 * @brief 处理客户端的群组聊天业务
 * {
 *      "msgId": GROUP_CHAT_MSG,
 *      "userId": "user_id",
 *      "user": "user_name",
 *      "groupId": "group_id",
 *      "message": "hello world"
 *      "time": "%Y-%m-%d %H:%M:%S"
 * }
 * @param pConn
 * @param jsn
 * @param timetmp
 */
void ClusterChatService::handleGroupChat(const muduo::net::TcpConnectionPtr &pConn, const nlohmann::json &jsn, muduo::Timestamp timetmp)
{
    unsigned int userId = jsn["userId"].get<unsigned int>();   // 发送者ID
    unsigned int groupId = jsn["groupId"].get<unsigned int>(); // 群组ID

    // 向群组内所有成员发送群组消息
    std::vector<unsigned int> toUserIds = _groupModel.queryGroupsUserId(userId, groupId); // 查询群组内成员
    std::lock_guard<std::mutex> lock(_mtx);                                               // _userConnMap是共享资源，访问时需要加锁

    for (auto toUserId : toUserIds) // 遍历所有接受者Id
    {
        auto it = _userConnMap.find(toUserId);
        if (it != _userConnMap.end()) // 用户在线，直接转发
        {
            std::string sendData = jsn.dump();
            uint32_t len = static_cast<uint32_t>(sendData.length()); //获取JSON字符串长度,不统计最后的\0
            uint32_t netLen = muduo::net::sockets::hostToNetwork32(len); //转换为网络字节序

            muduo::net::Buffer buf;
            buf.append(&netLen, sizeof(netLen)); // 添加4字节头
            buf.append(sendData);                // 添加JSON字符串,但最后的\0不包括在内

            it->second->send(&buf); // 发送消息到发送者, 告知消息发送成功
        }
        else // 用户不在线，或者不在该台服务器上登录
        {
            auto fromUser = _userModel.QueryById(toUserId);
            if (fromUser.getState() == "online") // 用户在线，只不过在其他的服务器上面登录了
            {
                _redis.publish(toUserId, jsn.dump()); // 将消息发送给redis频道，由订阅该频道的服务器接收、redis客户端会按照RESP协议自动封装数据
            }
            else // 用户离线，将消息存入OfflineMessage表
            {
                _offlineMessageModel.storeOfflineMessage(OfflineMessage(toUserId, jsn.dump()));
            }
        }
    }

    nlohmann::json response;
    response["msgId"] = GROUP_CHAT_MSG_REPLY;
    response["code"] = 0;
    response["msg"] = "send success";
    
    std::string sendData = response.dump();
    uint32_t len = static_cast<uint32_t>(sendData.length()); //获取JSON字符串长度,不统计最后的\0
    uint32_t netLen = muduo::net::sockets::hostToNetwork32(len); //转换为网络字节序

    muduo::net::Buffer buf;
    buf.append(&netLen, sizeof(netLen)); // 添加4字节头
    buf.append(sendData);                // 添加JSON字符串,但最后的\0不包括在内

    pConn->send(&buf); // 发送消息到发送者, 告知消息发送成功
}

// 处理从Redis发来的订阅消息
void ClusterChatService::handleRedisSubscribeMessage(unsigned int channel, const std::string &message)
{
    {
        std::lock_guard<std::mutex> lock(_mtx); // 操作_userConnMap这种共享资源需要加锁
        auto it = _userConnMap.find(channel);
        if (it != _userConnMap.end())
        {
            uint32_t len = static_cast<uint32_t>(message.length());
            uint32_t netLen = muduo::net::sockets::hostToNetwork32(len);
            
            muduo::net::Buffer buf;
            buf.append(&netLen, sizeof(netLen));
            buf.append(message);
            it->second->send(&buf); // 将消息转发给用户
            return;
        }
    }

    // 用户不在线，将数据存到OfflineMessage表
    _offlineMessageModel.storeOfflineMessage(OfflineMessage(channel, message));
}