/**
 * @file groupmodel.h
 * @author Loopy (2932742577@qq.com)
 * @brief Group表的数据操作类
 * @version 1.0
 * @date 2025-09-22
 * 
 * 
 */

#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include "group.h"
#include <vector>

class GroupModel
{
public:
    // 创建群组
    bool createGroup(Group& group);

    // 加入群组
    bool joinGroup(unsigned int userId, unsigned int groupId, const std::string& role);

    // 查询用户所在群组的消息
    std::vector<Group> queryGroups(unsigned int userId);

    // 查询群组成员的ID，除去自己
    std::vector<unsigned int> queryGroupsUserId(unsigned int userId, unsigned int groupId);
};

#endif