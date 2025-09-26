/**
 * @file offlinemessage.h
 * @author Loopy (2932742577@qq.com)
 * @brief ORM类，映射OfflineMessage表
 * @version 1.0
 * @date 2025-09-21
 * 
 * 
 */

#ifndef OFFLINEMESSAGE_H
#define OFFLINEMESSAGE_H

#include <string>

class OfflineMessage
{
public:
    OfflineMessage(unsigned int userId = 0, const std::string& message = "") : _userId(userId), _message(message) {}

    unsigned int getUserId() const { return _userId; }
    unsigned int getUserId() { return _userId; }

    const std::string& getMessage() const { return _message; }
    std::string& getMessage() { return _message; }

    void setUserId(unsigned int userId) { _userId = userId; }
    void setMessage(const std::string& message) { _message = message; }

private:
    unsigned int _userId; // 目的用户ID
    std::string _message; // 离线消息内容
};

#endif // OFFLINEMESSAGE_H