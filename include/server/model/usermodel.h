/**
 * @file usermodel.h
 * @author Loopy (2932742577@qq.com)
 * @brief User表的数据操作类
 * @version 1.0
 * @date 2025-09-19
 * 
 * 
 */

#ifndef USERMODEL_H
#define USERMODEL_H

#include "user.h"

class UserModel
{
public:
    bool Insert(User& user); // 向数据库插入用户数据

    User QueryByName(const std::string& name); // 根据用户名查询用户数据
    
    User QueryById(const int& id); // 根据用户id查询用户数据

    bool Update(const User& user); // 更新用户信息

    bool UpdateState(const User& user); // 更新用户状态
};


#endif // USERMODEL_H