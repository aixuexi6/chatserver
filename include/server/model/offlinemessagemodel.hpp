#ifndef OFFLINEMESSAGEMODEL_H
#define OFFLINEMESSAGEMODEL_H

#include <string>
#include <vector>
using namespace std;

class OfflineMsgModel
{
public:
    //用户插入到数据库中
    void insert(int userid,string msg);
    //从数据库中移除消息
    void remove(int userid);
    // 查询某一个用户的所有离线消息并存储到容器中 
    vector<string> query(int userid);
private:

};

#endif