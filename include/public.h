#ifndef PUBLIC_H
#define PUBLIC_H

enum MSGTYPE
{
    LOGIN_MSG = 1,                  // 登录消息
    LOGIN_MSG_REPLY = 2,            // 登录回复消息

    REGISTER_MSG = 3,               // 注册消息
    REGISTER_MSG_REPLY = 4,         // 注册回复消息

    PRIVATE_CHAT_MSG = 5,           // 私聊消息
    PRIVATE_CHAT_MSG_REPLY = 6,     // 私聊回复消息
    
    GROUP_CHAT_MSG = 7,             // 群聊消息
    GROUP_CHAT_MSG_REPLY = 8,       // 群聊回复消息
    
    ADD_FRIEND_MSG = 9,             // 添加好友消息
    ADD_FRIEND_MSG_REPLY = 10,      // 添加好友回复消息
    
    CREATE_GROUP_MSG = 11,          // 创建群组消息
    CREATE_GROUP_MSG_REPLY = 12,    // 创建群组回复消息

    JOIN_GROUP_MSG = 13,             // 加入群组消息
    JOIN_GROUP_MSG_REPLY = 14,       // 加入群组回复消息

    LOGOUT_MSG = 15,                // 登出消息
    LOGOUT_MSG_REPLY = 16,          // 登出回复消息
};

#endif