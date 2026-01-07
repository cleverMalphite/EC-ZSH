#include <iostream>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <queue>
//get time string
#include "../ec2/public/_public.h"
#include "./msg_cpp/msg.pb.h"

using namespace std;

typedef void(*callback_function)(string recv_data, int length);

//----------------------底层模块---------------------
//内置回调机制，上层模块向其注册回调，收到数据时调用回调上传
class ModuleLower
{
public:
  
  //需要注册服务类型的名称+回调函数,
  //  以便接收数据时根据名称判断数据类型后,
  //  分发到的响应的回调函数中
  bool register_server_callback( string server_name,  callback_function callback ) {
    //空参数判别
    if( "" == server_name || nullptr == callback ) return false;
    //重复服务名判别
    auto it = _server_callback_map.find(server_name);
    if( it != _server_callback_map.end() ) return false;
    //将键值插入服务回调表
    _server_callback_map[server_name] = callback;
    return true;
  }

  void run(){
    //模拟生成接收的数据，放到接收队列中
    generate_data();
    //每次spin都会遍历一遍回调函数表,
    //将缓冲队列里的数据上传给上层模块
    spin();
  }

private:
  void generate_data(){
    /*生成序列号*/
    generate_seed++;
    if(generate_seed>=99999) generate_seed = 0;

    /*生成时间戳*/
    char strLocalTime[21];
    memset(strLocalTime,0,sizeof(strLocalTime));
    LocalTime(strLocalTime,"yyyymmddhh24miss");

    //char tmpdata[301];
    //memset(tmpdata,0,sizeof(tmpdata));
    //snprintf(tmpdata,300,"id:%d-conten:%s",generate_seed,strLocalTime);

    /*生成消息*/
    if( generate_seed % 2 == 0 )
    {
      //生成消息a
      msg::msg_a msg ;;
      msg.set_server_type("msg_a");
      msg.set_time(strLocalTime);
      msg.set_id(generate_seed);

      //序列化消息到二进制字符串
      string binary_string;
      msg.SerializeToString(&binary_string);

      /*接收缓存消息(uint8_t*)*/
      //printf( "generate_data: data: %s\n\n", binary_string.c_str());
      _string_buffer.push(binary_string);
      
    }
    else if( generate_seed % 2 == 1 )
    {
      //生成消息b
      msg::msg_b msg ;
      msg.set_server_type("msg_b");
      msg.set_time(strLocalTime);
      msg.set_id(generate_seed);
      //序列化消息到二进制字符串
      string binary_string;
      msg.SerializeToString(&binary_string);

      /*接收缓存消息(uint8_t*)*/
      //printf( "generate_data: data: %s\n\n", binary_string.c_str());
      _string_buffer.push(binary_string);
    }

  }

  //注意：因为所有回调遍历完后，接收缓冲会pop
  //      因此为了保证所有回调收到全部数据（不缺不漏）
  //      需要一下前提：
  //_callback_queue不能为空，至少要有一个回调函数
  //_callback_queue中的回调函数，必须初始化时都全部注册
  void spin(){
    if(_string_buffer.size()==0)return ;

    //从缓冲区获取数据
    string strdata ;
    strdata = _string_buffer.front();
    //解析数据头,获取消息类型
    msg::msg_head mh;
    mh.ParseFromString(strdata);
    string server_type = mh.server_type();

    //根据消息类型，分别解析
    //if(server_type=="msg_a"){
    //  msg::msg_a ma; ma.ParseFromString(strdata);
    //  printf( "spin: get A data:=%s:%s:%d\n=",ma.server_type().c_str(),  ma.time().c_str(), ma.id());
    //}
    //else if(server_type=="msg_b"){
    //  msg::msg_b mb; mb.ParseFromString(strdata);
    //  printf( "spin: get B data:=%s:%s:%d\n=",mb.server_type().c_str(), mb.time().c_str(), mb.id());
    //}
    //printf( "spin: data: %s\n", strdata.c_str());

    //for( callback_function cb : _callback_queue ){
    //  cb( strdata, strdata.size() );
    //}
    
    //查询服务回调表，调用对应服务的回调函数
    _server_callback_map[server_type](strdata, strdata.size());

    _string_buffer.pop();
  }
  
  int generate_seed = 0;

  map<string, callback_function> _server_callback_map;//服务回调表
  queue<string> _string_buffer;
};

ModuleLower module_lower;

//----------------------上层模块---------------------
//向底层模块注册OnRecvData，从底层模块获取接收数据

//============上层模块A==============
class ModuleUpperA
{
public:
  void init(){
  }

  static void OnRecvData(string recv_data, int length){
    msg::msg_a ma; ma.ParseFromString(recv_data);
    printf( "MU_A recv data: =%s:%s:%d=\n\n",ma.server_type().c_str(),  ma.time().c_str(), ma.id());
  }

};

//============上层模块A==============
class ModuleUpperB
{
public:
  void init(){
  }

  static void OnRecvData(string recv_data, int length){
    msg::msg_a ma; ma.ParseFromString(recv_data);
    printf( "MU_B recv data: =%s:%s:%d=\n\n",ma.server_type().c_str(),  ma.time().c_str(), ma.id());
  }

};



int main(void)
{
  ModuleUpperA mu_a; mu_a.init(); 
  ModuleUpperB mu_b; mu_b.init();

  module_lower.register_server_callback("msg_a", mu_a.OnRecvData);
  module_lower.register_server_callback("msg_b", mu_b.OnRecvData);

  for(;;){
    module_lower.run();
  }

  return 0;
}
