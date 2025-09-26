/**
 * @file groupuser.h
 * @author Loopy (2932742577@qq.com)
 * @brief 群成员类，继承自User类
 * @version 1.0
 * @date 2025-09-21
 * 
 * 
 */

#ifndef GROUPUSER_H
#define GROUPUSER_H

#include "user.h"

class GroupUser : public User
{
public:
    GroupUser(const std::string& name = "", const std::string& password = "", const std::string& role = "normal")
        : User(name, password), role(role) {}

    const std::string& getRole() const { return role; }
    std::string& getRole() { return role; }
    
    void setRole(const std::string& role) { this->role = role; }

private:
    std::string role;
};

#endif // GROUPUSER_H