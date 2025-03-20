#include "groupmodel.hpp"
#include "db.h"
#include <iostream>

bool GroupModel::createGroup(Group &group)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into groups(group_name,groupdesc) values('%s','%s')",
            group.getName().c_str(), group.getDesc().c_str());

    //cout << "插入群组：" << group.getName().c_str() << " | 描述：" << group.getDesc().c_str() << endl;
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}

// 加入群组
void GroupModel::addGroup(int groupid, int userid, string role)
{
    char sql[1024] = {0};
    //cout << "用户：" << userid << " | 群组：" << groupid << " | 角色：" << role << endl;
    sprintf(sql, "insert into group_user values(%d,%d,'%s')", groupid, userid, role.c_str());
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

// 查询用户所在群组信息
vector<Group> GroupModel::queryGroups(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "select a.group_id,a.group_name,a.groupdesc from groups a inner join group_user b on b.group_id=a.group_id where b.user_id=%d", userid);

    vector<Group> groupvec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                // 查userid的所有群组信息
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                groupvec.push_back(group);
            }
            mysql_free_result(res);
        }
    }

    // 查询群组的用户信息
    for (Group &group : groupvec)
    {
        sprintf(sql, "select a.user_id,a.username,a.status,b.grouprole from user a inner join group_user b on a.user_id=b.user_id where b.group_id=%d", group.getId());

        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                // 查userid的所有群组信息
                GroupUser user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setRole(row[3]);
                group.getUsers().push_back(user);
            }
            mysql_free_result(res);
        }
    }
    return groupvec;
}

// 根据指定的groupid查询群组用户id列表，除userid自己，主要用于群聊业务给其他其他成员群发消息
vector<int> GroupModel::queryGroupsUsers(int userid, int groupid)
{
    char sql[1024] = {0};
    sprintf(sql, "select user_id from group_user where group_id=%d and user_id!=%d", groupid, userid);

    vector<int> idVec;
    MySQL mysql;
    if (mysql.connect())
    {

        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            // 把userid的多条离线消息放入到vec中返回
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                idVec.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
            return idVec;
        }
    }
    return idVec;
}