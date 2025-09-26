/**
 * @file group.h
 * @author Loopy (2932742577@qq.com)
 * @brief ORM类，映射Group表 
 * @version 1.0
 * @date 2025-09-21
 * 
 * 
 */

#ifndef GROUP_H
#define GROUP_H

#include <string>
#include <vector>
#include "groupuser.h"

class Group
{
public:
    Group(std::string groupName = "", std::string groupDescription = "", std::vector<GroupUser> groupMembers = std::vector<GroupUser>())
    : _groupName(groupName), _groupDescription(groupDescription), _groupMembers(groupMembers)
    {}

    unsigned int getGroupId() const { return _groupId; }
    unsigned int getGroupId() { return _groupId; }

    const std::string& getGroupName() const { return _groupName; }
    std::string& getGroupName() { return _groupName; }

    const std::string& getGroupDescription() const { return _groupDescription; }
    std::string& getGroupDescription() { return _groupDescription; }
    
    const std::vector<GroupUser>& getGroupMembers() const { return _groupMembers; }
    std::vector<GroupUser>& getGroupMembers() { return _groupMembers; }

    void setGroupId(unsigned int groupId) { _groupId = groupId; }
    void setGroupName(const std::string& groupName) { _groupName = groupName; }
    void setGroupDescription(const std::string& groupDescription) { _groupDescription = groupDescription; }
    void setGroupMembers(const std::vector<GroupUser>& groupMembers) { _groupMembers = groupMembers; }

private:
    unsigned int _groupId = 0;  // 组ID
    std::string _groupName; // 组名
    std::string _groupDescription; // 组描述
    std::vector<GroupUser> _groupMembers; // 组成员
};

#endif // GROUP_H