/**
 * @file offlinemessagemodel.h
 * @author Loopy (2932742577@qq.com)
 * @brief OfflineMessageModel表的数据操作类
 * @version 1.0
 * @date 2025-09-21
 * 
 * 
 */

#ifndef OFFLINEMESSAGEMODEL_H
#define OFFLINEMESSAGEMODEL_H

#include "offlinemessage.h"
#include <vector>

class OfflineMessageModel
{
public:
    // 存储用户的离线消息
    void storeOfflineMessage(const OfflineMessage& message);

    // 删除用户的离线消息
    void deleteOfflineMessage(unsigned int id);

    // 获取用户的离线消息
    std::vector<std::string> getOfflineMessage(unsigned int id);

};

#endif // OFFLINEMESSAGEMODEL_H