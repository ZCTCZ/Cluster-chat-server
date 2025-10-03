// MySQL官方原生的API，在这个项目中不再使用！

// /**
//  * @file database.h
//  * @author Loopy (2932742577@qq.com)
//  * @brief 封装了数据库操作的类
//  * @version 1.0
//  * @date 2025-09-18
//  *
//  *
//  */
// #ifndef DATABASE_H
// #define DATABASE_H

// #include <mysql/mysql.h>
// #include <string>

// // 数据库操作类
// class MySQL
// {
// public:
//     // 初始化数据库连接
//     MySQL();

//     // 释放数据库连接资源
//     ~MySQL();
    
//     // 连接数据库 
//     MYSQL* Connect();
    
//     // 插入操作
//     bool Insert(const std::string& sql);

//     // 删除操作
//     bool Delete(const std::string& sql);

//     // 更新操作
//     bool Update(const std::string& sql);
    
//     // 查询操作 
//     MYSQL_RES* Select(const std::string& sql);

// private:
//     MYSQL *_conn; // 数据库连接句柄
// };

// #endif