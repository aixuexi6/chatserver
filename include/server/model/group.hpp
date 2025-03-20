#ifndef GROUP_H
#define GROUP_H

#include "groupuser.hpp"
#include <vector>
#include <string>
using namespace std;

class Group
{
public:
    Group(int id=-1,string name="",string desc =""){
        this->group_id=id;
        this->group_name=name;
        this->groupdesc=desc;
    }

    void setId(int id){this->group_id=id;}
    void setName(string name){this->group_name=name;}
    void setDesc(string desc){this->groupdesc=desc;}

    int getId(){return this->group_id;}
    string getName(){return this->group_name;}
    string getDesc(){return this->groupdesc;}
    vector<GroupUser> &getUsers(){return this->users;}
private:
    int group_id;
    string group_name;
    string groupdesc;
    vector<GroupUser> users;
};

#endif