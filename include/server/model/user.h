/**
 * @file user.h
 * @author Loopy (2932742577@qq.com)
 * @brief ORM类，映射User表
 * @version 1.0
 * @date 2025-09-19
 * 
 * 
 */

#ifndef USER_H
#define USER_H

#include <string>

class User
{
public:
    User() = default;
    
    User(const std::string &name, const std::string &password, const std::string &state = "offline")
        :_name(name), _password(password), _state(state)
    {}

    // Getters
    unsigned int getId() const { return _id; }
    unsigned int getId() { return _id; }
    
    const std::string& getName() const { return _name; }
    std::string& getName() { return _name; }

    const std::string& getPassword() const { return _password; }
    std::string& getPassword() { return _password; }

    const std::string& getState() const { return _state; }
    std::string& getState() { return _state; }

    // Setters
    void setId(unsigned int id) { _id = id; }
    void setName(const std::string &name) { _name = name; }
    void setPassword(const std::string &password) { _password = password; }
    void setState(const std::string &state) { _state = state; }    

    virtual ~User() = default;

protected:
    unsigned int _id = 0;
    std::string _name;
private:
    std::string _password;
protected:
    std::string _state;
};

#endif // USER_H