#include "json.hpp"
using json=nlohmann::json;

#include <iostream>
#include <vector>
#include <map>
using namespace std;

void func1(){
    json js;
    js["msg_type"]=2;
    js["from"]="zhang san";
    js["to"]="li si";
    js["msg"]="hello,how are you?";
    cout<<js<<endl;
}

int main(){
    func1();

    return 0;
}