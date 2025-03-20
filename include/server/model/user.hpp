#ifndef USER_H
#define USER_H

#include <string>
using namespace std;

//ORM类
class User
{
public:
    User(int id = -1, string name = "", string pwd = "", string state = "offline")
    {
        this->user_id = id;
        this->username = name;
        this->password = pwd;
        this->status = state;
    }

    void setId(int id){this->user_id=id;}
    void setName(string name){this->username=name;}
    void setPwd(string pwd){this->password=pwd;}
    void setState(string state){this->status=state;}

    int getId(){return this->user_id;}
    string getName(){return this->username;}
    string getPwd(){return this->password;}
    string getState(){return this->status;}
private:
    int user_id;
    string username;
    string password;
    string status;
};

#endif