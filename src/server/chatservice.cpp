#include "chatservice.hpp"
#include "public.hpp"
#include "muduo/base/Logging.h"
#include <string>
#include <vector>
#include <iostream>

using namespace muduo;
using namespace std;

ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}

// 注册消息以及对应的Handle回调操作
ChatService::ChatService()
{
    _MsgHandleMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _MsgHandleMap.insert({LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3)});
    _MsgHandleMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    _MsgHandleMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    _MsgHandleMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});
    _MsgHandleMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _MsgHandleMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _MsgHandleMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});

    if (_redis.connect())
    {
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
}

void ChatService::reset()
{
    // 将所有online用户的信息设置为offline
    _userModel.resetState();
}

MsgHandler ChatService::getHandler(int msgid)
{
    // 记录错误日志，misgid没有对应的事件处理回调
    auto it = _MsgHandleMap.find(msgid);
    if (it == _MsgHandleMap.end())
    {
        // 返回一个默认的处理器，空操作
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp)
        {
            LOG_ERROR << "msgid " << msgid << "can not find handle!";
        };
    }
    else
    {
        return _MsgHandleMap[msgid];
    }
}

// 登录业务
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["user_id"].get<int>();
    // int id = js["user_id"];
    string pwd = js["password"];

    User user = _userModel.query(id);
    if (user.getId() == id && user.getPwd() == pwd)
    {
        if (user.getState() == "online")
        {
            // 该用户已登录，不允许重复登录，登录失败
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "this account is using,input another!";
            conn->send(response.dump());
        }
        else
        {
            // 登录成功，更新用户状态信息
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn});
            }

            // id登陆成功后，向redis订阅channel(id)
            _redis.subscribe(id);

            user.setState("online");
            _userModel.updateState(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["user_id"] = user.getId();
            response["username"] = user.getName();

            // 查询该用户是否有离线消息
            vector<string> vec = _offlineMsgModel.query(id);
            if (!vec.empty())
            {
                response["offlinemsg"] = vec;
                _offlineMsgModel.remove(id);
            }

            // 查询该用户的好友信息并返回
            vector<User> uservec = _friendModel.query(id);
            if (!uservec.empty())
            {
                vector<string> vec2;
                for (User &user : uservec)
                {
                    json js;
                    js["user_id"] = user.getId();
                    js["username"] = user.getName();
                    js["status"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }

            // 查询该用户的群组信息和组员信息并返回
            vector<Group> groupvec = _groupModel.queryGroups(id);
            if (!groupvec.empty())
            {
                vector<string> vec3;
                for (Group &group : groupvec)
                {
                    json grpjs;
                    grpjs["group_id"] = group.getId();
                    grpjs["group_name"] = group.getName();
                    grpjs["groupdesc"] = group.getDesc();

                    vector<string> uservec;
                    for (GroupUser &user : group.getUsers())
                    {
                        json userjs;
                        userjs["user_id"] = user.getId();
                        userjs["username"] = user.getName();
                        userjs["status"] = user.getState();
                        userjs["role"] = user.getRole();
                        uservec.push_back(userjs.dump());
                    }
                    grpjs["users"] = uservec;
                    vec3.push_back(grpjs.dump());
                }
                response["groups"] = vec3;
            }
            conn->send(response.dump());
        }
    }
    else
    {
        // 该用户不存在，或者用户存在但是密码错误，登录失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 2;
        response["errmsg"] = "id or password is invaild!";
        conn->send(response.dump());
    }
}

// 注册业务
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["username"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state = _userModel.insert(user);

    if (state)
    {
        // 注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["user_id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = js["to"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if (it != _userConnMap.end())
        {
            // toid在线，转发消息
            it->second->send(js.dump());
            return;
        }
    }

    // 查询toid是否在线
    User user = _userModel.query(toid);
    if (user.getState() == "online")
    {
        _redis.publish(toid, js.dump());
        return;
    }

    // toid不在线，存储离线消息
    _offlineMsgModel.insert(toid, js.dump());
}

void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["user_id"].get<int>();
    int friendid = js["friend_id"].get<int>();
    // 添加好友信息
    _friendModel.insert(userid, friendid);
}

// 创建群组
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["user_id"].get<int>();
    string name = js["group_name"];
    string desc = js["groupdesc"];

    // 存储新创建的群组信息
    Group group(-1, name, desc);
    // LOG_INFO << "创建群组：" << name << " | 描述：" << desc;
    if (_groupModel.createGroup(group))
    {
        // 存储群组创建人信息
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}

// 加入群组
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["user_id"].get<int>();
    int groupid = js["group_id"].get<int>();
    // LOG_INFO << "用户：" << userid << " | 群组：" << groupid;
    _groupModel.addGroup(groupid, userid, "normal");
}

// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["user_id"].get<int>();
    int groupid = js["group_id"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupsUsers(userid, groupid);

    lock_guard<mutex> lock(_connMutex);
    for (int id : useridVec)
    {
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            it->second->send(js.dump());
        }
        else
        {
            User user = _userModel.query(id);
            if (user.getState() == "online")
            {
                _redis.publish(id, js.dump());
            }
            else
            {
                _offlineMsgModel.insert(id, js.dump());
            }
        }
    }
}

void ChatService::handleRedisSubscribeMessage(int userid,string msg)
{
    lock_guard<mutex> lock(_connMutex);
    auto it=_userConnMap.find(userid);
    if(it!=_userConnMap.end())
    {
        it->second->send(msg);
        return;
    }

    _offlineMsgModel.insert(userid,msg);
}

// 处理注销业务
void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid=js["user_id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }

    // 用户下线，在redis中取消订阅通道
    _redis.unsubscribe(userid);

    // 更新用户信息
    User user(userid, "", "", "offline");
    _userModel.updateState(user);
}

void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                // 从map表删除conn连接
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }

    // 用户下线，在redis中取消订阅通道
    _redis.unsubscribe(user.getId());

    // 更新用户信息
    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}
