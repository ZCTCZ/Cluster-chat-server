#include "../../include/server/model/groupmodel.h"
#include "../../include/server/db/database.h"
#include <string>
#include <memory>

// 创建群组
bool GroupModel::createGroup(Group& group)
{
    std::string sql = "INSERT INTO `Groups` (groupname, groupdesc) VALUES ('" + group.getGroupName() + "', '" + group.getGroupDescription() + "')";

    MySQL mysql;
    auto conn = mysql.Connect();
    if (conn)
    {
        if (mysql.Insert(sql))
        {
            group.setGroupId(mysql_insert_id(conn));
            return true;    
        }
    }

    return false;
}

// 加入群组
bool GroupModel::joinGroup(unsigned int userId, unsigned int groupId, const std::string &role)
{
    std::string sql = "INSERT INTO GroupUser (userid, groupid, grouprole) VALUES ('" + std::to_string(userId) + "', '" + std::to_string(groupId) + "', '" + role + "')";

    MySQL mysql;
    if (mysql.Connect())
    {
        if (mysql.Insert(sql))
        {
            return true;
        }
    }

    return false;
}

// 查询用户所在群组的消息
std::vector<Group> GroupModel::queryGroups(unsigned int userId)
{
    // 查询用户所在群组的群组信息
    std::string sql1 = "SELECT Groups.id, Groups.groupname, Groups.groupdesc FROM `Groups` INNER JOIN GroupUser ON Groups.id = GroupUser.groupid WHERE GroupUser.userid = " + std::to_string(userId);
    std::vector<Group> vec;
    MySQL mysql;
    if (mysql.Connect())
    {
        auto res = std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)>(mysql.Select(sql1), mysql_free_result);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res.get())))
            {
                Group group;
                group.setGroupId(std::stoi(row[0]));
                group.setGroupName(row[1]);
                group.setGroupDescription(row[2]);
                vec.push_back(group);
            }
        }
    }

    // 查询用户所在组的组成员信息
    for (auto& e : vec)
    {
        std::string sql2 = "SELECT User.id, User.name, User.state, GroupUser.grouprole FROM User INNER JOIN GroupUser ON User.id = GroupUser.userid WHERE GroupUser.groupid = " + std::to_string(e.getGroupId());
        auto res = std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)>(mysql.Select(sql2), mysql_free_result);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res.get())))
            {
                GroupUser groupUser;
                groupUser.setId(std::stoi(row[0]));
                groupUser.setName(row[1]);
                groupUser.setState(row[2]);
                groupUser.setRole(row[3]);

                e.getGroupMembers().push_back(groupUser);
            }
        }
    }

    return vec;
}

// 查询群组成员的ID，除去自己
std::vector<unsigned int> GroupModel::queryGroupsUserId(unsigned int userId, unsigned int groupId)
{
    std::string sql = "SELECT userid FROM GroupUser WHERE groupid = " + std::to_string(groupId) + " AND userid <> " + std::to_string(userId);
    std::vector<unsigned int> vec;
    MySQL mysql;
    if (mysql.Connect())
    {
        auto res = std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)>(mysql.Select(sql), mysql_free_result);
        if (res)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res.get())))
            {
                vec.push_back(std::stoi(row[0]));
            }
        }
    }

    return vec;
}