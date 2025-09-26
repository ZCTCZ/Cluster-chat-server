#include "../../include/server/model/usermodel.h"
#include "../../include/server/db/database.h"
#include <memory>

// 将user对象插入到数据库的User表里面
bool UserModel::Insert(User &user)
{
    // 组装sql语句
    std::string sql = "INSERT INTO User (name, password) VALUES ('" + user.getName() + "', '" + user.getPassword() + "')";

    MySQL mysql;
    auto conn = mysql.Connect(); // 连接数据库
    if (conn != nullptr)
    {
        if (mysql.Insert(sql))
        {
            user.setId(mysql_insert_id(conn)); // mysql_insert_id()函数获取最后插入操作生成的自增 ID。
            return true;
        }
    }

    return false;
}

// 根据用户名查询用户信息
User UserModel::QueryByName(const std::string &name)
{
    // 组装sql语句
    std::string sql = "SELECT * FROM User WHERE name = '" + name + "'";
    MySQL mysql;
    auto conn = mysql.Connect(); // 连接数据库
    if (conn != nullptr)
    {
        auto res = std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)>(mysql.Select(sql), mysql_free_result);
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res.get());
            if (row != nullptr)
            {
                User user;
                user.setId(std::stoi(row[0]));
                user.setName(row[1]);
                user.setPassword(row[2]);
                user.setState(row[3]);
                return user;
            }
        }
    }

    return User();
}

// 根据用户id查询用户数据
User UserModel::QueryById(const int &id)
{
    // 组装sql语句
    std::string sql = "SELECT * FROM User WHERE id = " + std::to_string(id);
    MySQL mysql;
    User user;

    auto conn = mysql.Connect(); // 连接数据库
    if (conn != nullptr)
    {
        auto res = std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)>(mysql.Select(sql), mysql_free_result);
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res.get());
            if (row != nullptr)
            {
                user.setId(std::stoi(row[0]));
                user.setName(row[1]);
                user.setState(row[3]);
                return user;
            }
        }
    }

    return user;
}

// 更新用户信息
bool UserModel::Update(const User &user)
{
    // 组装sql
    std::string sql = " UPDATE User SET id = " + std::to_string(user.getId()) + ", name = '" + user.getName() + "', password = '" + user.getPassword() + "', state = '" + user.getState() + "' where id = " + std::to_string(user.getId());

    MySQL mysql;
    if (mysql.Connect())
    {
        mysql.Update(sql);
        return true;
    }

    return false;
}

// 更新用户状态
bool UserModel::UpdateState(const User &user)
{
    // 组装sql
    std::string sql = " UPDATE User SET state = '" + user.getState() + "' WHERE id = " + std::to_string(user.getId());

    MySQL mysql;
    if (mysql.Connect())
    {
        mysql.Update(sql);
        return true;
    }

    return false;
}