/**
 * @file friendmodel.h
 * @author Loopy (2932742577@qq.com)
 * @brief Friend表的数据操作类
 * @version 1.0
 * @date 2025-09-21
 *
 *
 */

#include "friend.h"
#include "user.h"
#include <vector>

class FriendModel
{
public:
    bool addFriend(const Friend& friend_); // 添加好友

    std::vector<User> queryFriend(unsigned int id); // 查询用户的好友
};
