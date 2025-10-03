#include "../../include/server/model/friendmodel.h"
// #include "../../include/server/db/database.h"
#include "../../include/server/db/MySQLConnectionPool.h"
#include <string>
#include <memory>

// 添加好友
bool FriendModel::addFriend(const Friend& friend_)
{
    auto pMcp = MySQLConnectionPool::getInstance();
    auto pConn = pMcp->getConnection();
    if (pConn)
    {
        if (pConn->execute("INSERT INTO Friend (userid, friendid) VALUES (?, ?)", 
            {Param::UInt32(friend_.getUserId()), Param::UInt32(friend_.getFriendId())}) &&
            pConn->execute("INSERT INTO Friend (userid, friendid) VALUES (?, ?)", 
            {Param::UInt32(friend_.getFriendId()), Param::UInt32(friend_.getUserId())}))
        {
            return true;
        }
    }

    return false;
}

// bool FriendModel::addFriend(const Friend& friend_)
// {
//     // a 和 b 互为好友,所以好友关系表中应该要插入两条数据
//     std::string sql1 = "INSERT INTO Friend (userid, friendid) VALUES (" + std::to_string(friend_.getUserId()) + ", " + std::to_string(friend_.getFriendId()) + ")";
//     std::string sql2 = "INSERT INTO Friend (userid, friendid) VALUES (" + std::to_string(friend_.getFriendId()) + ", " + std::to_string(friend_.getUserId()) + ")";
//     MySQL mysql;
//     if (mysql.Connect())
//     {
//         mysql.Insert(sql1);
//         mysql.Insert(sql2);
//         return true;
//     }

//     return false;
// }

// 查询用户的好友
std::vector<User> FriendModel::queryFriend(unsigned int id)
{
    auto pMcp = MySQLConnectionPool::getInstance();
    std::vector<User> friends;
    auto pConn = pMcp->getConnection();
    if (pConn)
    {
        auto res = pConn->query("SELECT u.id, u.name, u.state FROM User AS u INNER JOIN Friend AS f ON u.id = f.friendid WHERE f.userid = (?)", 
            {Param::UInt32(id)});
        if (res)
        {
            while (res->next())
            {
                User user;
                user.setId(res->getUInt("id"));
                user.setName(res->getString("name"));
                user.setState(res->getString("state"));
                friends.emplace_back(user);
            }
        }
    }

    return friends;
}


// std::vector<User> FriendModel::queryFriend(unsigned int id)
// {
//     std::string sql = "SELECT u.id, u.name, u.state FROM User AS u INNER JOIN Friend AS f ON u.id = f.friendid WHERE f.userid = " + std::to_string(id);

//     std::vector<User> friends;
//     MySQL mysql;
//     if (mysql.Connect())
//     {
//         auto res = std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)>(mysql.Select(sql), mysql_free_result);
//         if (res != nullptr)
//         {
//             MYSQL_ROW row;
//             while ((row = mysql_fetch_row(res.get())))
//             {
//                 User user;
//                 user.setId(std::stoi(row[0]));
//                 user.setName(row[1]);
//                 user.setState(row[2]);
//                 friends.push_back(user);  
//             }
//         }
//     }

//     return friends;
// }