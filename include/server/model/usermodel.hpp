#ifndef USERMODEL_H
#define USERMODEL_H

#include "user.hpp"

class UserModel
{
public:
    //插入用户记录
    bool insert(User &user);
    //在数据库查询用户记录并返回记录
    User query(int id);
    //更新用户状态信息
    bool updateState(User user);
    //重置用户的状态信息
    void resetState();
};

#endif