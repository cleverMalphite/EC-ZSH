#include <iostream>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <queue>
//get time string
#include "../ec2/public/_public.h"

using namespace std;

typedef void(*callback_function)(string recv_data, int length);

//----------------------底层模块---------------------
//内置回调机制，上层模块向其注册回调，收到数据时调用回调上传
class ModuleLower
{
public:
  
  bool register_callback( callback_function callback ) {
    if( nullptr == callback ) return false;
    _callback_queue.push_back( callback );
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
    char strLocalTime[21];
    memset(strLocalTime,0,sizeof(strLocalTime));
    LocalTime(strLocalTime,"yyyymmddhh24miss");

    char tmpdata[301];
    memset(tmpdata,0,sizeof(tmpdata));
    snprintf(tmpdata,300,"id:%d-conten:%s",generate_seed,strLocalTime);

    //printf( "generate_data: data: %s\n", tmpdata);
    string strdata(tmpdata);
    _string_buffer.push(strdata);

    generate_seed++;
    if(generate_seed>=99999) generate_seed = 0;

  }

  //注意：因为所有回调遍历完后，接收缓冲会pop
  //      因此为了保证所有回调收到全部数据（不缺不漏）
  //      需要一下前提：
  //_callback_queue不能为空，至少要有一个回调函数
  //_callback_queue中的回调函数，必须初始化时都全部注册
  void spin(){
    string strdata ;
    strdata = _string_buffer.front();

    printf( "spin: data: %s\n", strdata.c_str());

    for( callback_function cb : _callback_queue ){
      cb( strdata, strdata.size() );
    }

    _string_buffer.pop();
  }
  
  int generate_seed = 0;

  vector<callback_function> _callback_queue;
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
    module_lower.register_callback(OnRecvData);
  }

private:
  static void OnRecvData(string recv_data, int length){
    printf("MU_A recv data: %s \n", recv_data.c_str());
  }

};

//============上层模块A==============
class ModuleUpperB
{
public:
  void init(){
    module_lower.register_callback(OnRecvData);
  }

private:
  static void OnRecvData(string recv_data, int length){
    printf("MU_B recv data: %s \n", recv_data.c_str());
  }

};



int main(void)
{
  ModuleUpperA mu_a;
  ModuleUpperB mu_b;

  mu_a.init();
  mu_b.init();

  for(;;){
    module_lower.run();
  }

  return 0;
}
