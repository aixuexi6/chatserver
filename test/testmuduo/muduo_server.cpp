#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include<functional>
#include<string>
using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;

class chatServer
{
public:
    chatServer(EventLoop *loop,
               const InetAddress &listenAddr,
               const string &nameArg)
        : _server(loop, listenAddr, nameArg), _loop(loop)
    {
        _server.setConnectionCallback(std::bind(&chatServer::onConnection, this, _1));
        _server.setMessageCallback(std::bind(&chatServer::onMessage,this,_1,_2,_3));
        _server.setThreadNum(4);
    }

    void start(){
        _server.start();
    }

private:
    void onConnection(const TcpConnectionPtr &conn)
    {
        if(conn->connected()){
            cout<<conn->peerAddress().toIpPort()<<"->"<<
                conn->localAddress().toIpPort()<<"state:online"<<endl;
        }else{
            cout<<conn->peerAddress().toIpPort()<<"->"<<
                conn->localAddress().toIpPort()<<"state:offline"<<endl;
            conn->shutdown();
        }
    }

    void onMessage(const TcpConnectionPtr &conn,
                   Buffer *buffer,
                   Timestamp time)
    {
        string buf=buffer->retrieveAllAsString();
        cout<<"recv data:"<<buf<<"time:"<<time.toString()<<endl;
        conn->send(buf);
    }
    TcpServer _server;
    EventLoop *_loop;
};

int main(){
    EventLoop loop;
    InetAddress addr("127.0.0.1",6000);
    chatServer server(&loop,addr,"chatServer");
    
    server.start();
    loop.loop();
    return 0;
}
