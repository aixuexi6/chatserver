#include "offlinemessagemodel.hpp"
#include "db.h"

// 用户插入到数据库中
void OfflineMsgModel::insert(int userid, string msg)
{
    char sql[1024] = {0};
    // sprintf(sql, "insert into offline_message(user_id,message) values('%d','%s')",userid,msg);
    sprintf(sql, "insert into offline_message values(%d,'%s')", userid, msg.c_str());
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

// 从数据库中移除消息
void OfflineMsgModel::remove(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "delete from offline_message where user_id=%d", userid);

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

// 查询某一个用户的所有离线消息并存储到容器中
vector<string> OfflineMsgModel::query(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "select message from offline_message where user_id=%d", userid);

    vector<string> vec;
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
                vec.push_back(row[0]);
            }
            mysql_free_result(res);
            return vec;
        }
    }
    return vec;
}