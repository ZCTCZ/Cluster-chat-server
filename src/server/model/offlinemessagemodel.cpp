#include "../../include/server/model/offlinemessagemodel.h"
// #include "../../include/server/db/database.h"
#include "../../include/server/db/MySQLConnectionPool.h"
#include <memory>

// 存储用户的离线消息
void OfflineMessageModel::storeOfflineMessage(const OfflineMessage& message)
{
    auto pMcp = MySQLConnectionPool::getInstance();
    auto pConn = pMcp->getConnection();
    if (pConn)
    {
        pConn->execute("INSERT INTO OfflineMessage (userid, message) VALUES(?, ?)", 
            {Param::UInt32(message.getUserId()), Param::String(message.getMessage())});
    }
}

// void OfflineMessageModel::storeOfflineMessage(const OfflineMessage& message)
// {
//     std::string sql =  "INSERT INTO OfflineMessage (userid, message) VALUES (" + std::to_string(message.getUserId()) + ", '" + message.getMessage() + "')";

//     MySQL mysql;
//     if (mysql.Connect())
//     {
//         mysql.Insert(sql);
//     }
// }

// 删除用户的离线消息
void OfflineMessageModel::deleteOfflineMessage(unsigned int id)
{
    auto pMcp = MySQLConnectionPool::getInstance();
    auto pConn = pMcp->getConnection();
    if (pConn)
    {
        pConn->execute("DELETE FROM OfflineMessage WHERE userid = (?)", {Param::UInt32(id)});
    }
}


// void OfflineMessageModel::deleteOfflineMessage(unsigned int id)
// {
//     std::string sql = "DELETE FROM OfflineMessage WHERE userid = " + std::to_string(id);

//     MySQL mysql;
//     if (mysql.Connect())
//     {
//         mysql.Delete(sql);
//     }
// }

// 获取用户的离线消息
std::vector<std::string> OfflineMessageModel::getOfflineMessage(unsigned int id)
{
    std::vector<std::string> vec;
    auto pMcp = MySQLConnectionPool::getInstance();
    auto pConn = pMcp->getConnection();
    if (pConn)
    {
        auto res = pConn->query("SELECT message FROM OfflineMessage WHERE userid = (?)", {Param::UInt32(id)});
        if (res)
        {
            while (res->next())
            {
                vec.emplace_back(res->getString("message"));
            }
        }
    }

    return vec;
}


// std::vector<std::string> OfflineMessageModel::getOfflineMessage(unsigned int id)
// {
//     std::string sql = "SELECT message FROM OfflineMessage WHERE userid = '" + std::to_string(id) + "'";
//     std::vector<std::string> vec;
    
//     MySQL mysql;
//     auto conn = mysql.Connect(); // 连接数据库
//     if (conn != nullptr)
//     {
//         auto res = std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)>(mysql.Select(sql), mysql_free_result); //使用智能指针管理MYSQL_RES资源
//         if (res != nullptr)
//         {
//             MYSQL_ROW row;
//             while ((row = mysql_fetch_row(res.get())) != nullptr)
//             {
//                 vec.push_back(row[0]);
//             }
//         }
//     }
//     return vec;
// }
