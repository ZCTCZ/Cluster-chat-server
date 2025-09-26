#include "../../../include/server/db/database.h"
#include <muduo/base/Logging.h>

// 数据库配置信息
static std::string server = "127.0.0.1";// 数据库服务器地址
static std::string user = "root";// 数据库用户名
static std::string password = "zct010601";// 数据库密码
static std::string dbname = "ClusterChatServer";// 数据库名称

// 初始化数据库连接
MySQL::MySQL()
{
    _conn = mysql_init(nullptr);
}

// 释放数据库连接资源
MySQL::~MySQL()
{
    if (_conn != nullptr)
    {
        mysql_close(_conn);
        _conn = nullptr;
    }
}

// 连接数据库
MYSQL* MySQL::Connect()
{
    MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str(),
                                  password.c_str(), dbname.c_str(), 3306, nullptr, 0);
    if (p != nullptr)
    {
        LOG_INFO << "数据库连接成功！";
        mysql_query(_conn, "SET NAMES utf8mb4");
    }
    else
    {
        LOG_INFO << "数据库连接失败！";
    }

    return p;
}

// 插入数据
bool MySQL::Insert(const std::string& sql)
{
    LOG_INFO << sql;
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << sql << "插入失败!";
        return false;
    }

    return true;
}

// 删除数据
bool MySQL::Delete(const std::string& sql)
{
    LOG_INFO << sql;
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << sql << "删除失败!";
        return false;
    }

    return true;
}

// 更新数据
bool MySQL::Update(const std::string& sql)
{
    LOG_INFO << sql;

    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << sql << "更新失败!";
        return false;
    }

    return true;
}

// 查询数据
MYSQL_RES* MySQL::Select(const std::string& sql)
{
    LOG_INFO << sql;

    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                 << sql << "查询失败!";
        return nullptr;
    }

    return mysql_use_result(_conn);
}