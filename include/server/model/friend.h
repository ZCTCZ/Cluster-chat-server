/**
 * @file friend.h
 * @author Loopy (2932742577@qq.com)
 * @brief ORM类，映射Friend表
 * @version 1.0
 * @date 2025-09-21
 * 
 * 
 */

#ifndef FRIEND_H
#define FRIEND_H

class Friend
{
public:
    Friend(unsigned int userId = 0, unsigned int friendId = 0) : _userId(userId), _friendId(friendId) {}

    unsigned int getUserId() const { return _userId; }
    unsigned int getUserId() { return _userId; }
    
    unsigned int getFriendId() const { return _friendId; }
    unsigned int getFriendId() { return _friendId; }
    
    void setUserId(unsigned int userId) { _userId = userId; }
    void setFriendId(unsigned int friendId) { _friendId = friendId; }

private:
    unsigned int _userId;
    unsigned int _friendId;
};

#endif // FRIEND_H