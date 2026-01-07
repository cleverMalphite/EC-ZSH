#include <iostream>
#include "GlobalMessage.h"
#include "epollComm/EpollCommApi.h"
#include "NetCombTransferApi.h"
#include "SpeedControl/SpeedControlManager.h"
#include "MRUDP/MRUDPApi.h"
//#include "BigDataTransfer/BigDataTransferApi.h"

#include "time.h"

#include "public/_public.h"
#include "msg_cpp/msg.pb.h"

struct st_arg{
  char inifilepath[101];
  int role;
  int dtu;
  char localip[51];
  char remoteip[51];
  int localport;
  int remoteport;
  int runtime;

  int remotetid;

} starg;

CLogFile logfile;

//退出函数
void EXIT(int sig);

//显示程序帮助
void _help(char *argv[]);

//把xml解析到参数starg结构中
bool _xmltoarg(char *strxmlbuffer);

//系统启动
void System_start(const char *config_path) ;

//系统关闭
void System_shutdown(bool QosOpen = true) ;

//MRUDP数据服务a回调函数
void mrudp_dataserver_a_callback(
              DWORD dTermId, 
              const std::shared_ptr<BYTE> &recv_data, 
              DWORD dRecvDataByteLength);


int main(int argc, char *argv[]) {
		
		if(argc!=3) { _help(argv); return -1; }

    //关闭全部的信号和输入输出
    //CloseIOAndSignal();

    //处理程序退出的信号
    signal(SIGINT,EXIT); signal(SIGTERM,EXIT);

		if(logfile.Open(argv[1], "a+")==false)
		{ 
			printf("打开日志文件失败(%s). \n", argv[1]); return -1;
		}

    if(_xmltoarg(argv[2])==false) return -1;

    System_start(starg.inifilepath);
    logfile.Write("System_start 运行成功!\n");

    //发送方
    if(starg.role==1){
      sleep(5);//等待接收方启动后，再创建发送端
      DWORD TaskID;
      //创建非DTU的Tcp客户端通道 或 创建DTU的Tcp客户端通道
      bool res;
      res = CreateTcpClientChannel(starg.localport, starg.remoteport, starg.localip, starg.remoteip, 3200);
      if(res==true) 
        logfile.Write("发送方通道创建成功！\n");
      else 
      { 
        logfile.Write("发送方通道创建失败！\n"); 
        return -1;
      }

      int generate_seed = 0;
      //创建protobuf消息类型
      /*生成序列号*/
      generate_seed++;
      if(generate_seed>=99999) generate_seed = 0;

      /*生成时间戳*/
      char strLocalTime[21];
      memset(strLocalTime,0,sizeof(strLocalTime));
      LocalTime(strLocalTime,"yyyymmddhh24miss");

      /**************send a*********************/
      /*mu_a 用ml发送数*/
      //生成消息a
      msg::msg_a ma ;
      ma.set_server_type("msg_a");
      ma.set_time(strLocalTime);
      ma.set_id(generate_seed);
      //序列化消息到二进制字符串
      int msg_size = ma.ByteSizeLong(); 
      //std::shared_ptr<BYTE> pBuffer(new BYTE[msg_size], releaseArrays<BYTE>);
      std::shared_ptr<BYTE> pBuffer(new BYTE[msg_size]);
      if(ma.SerializeToArray(pBuffer.get(), msg_size)==false)
      {
        logfile.Write("ma序列化失败!\n");
      }

      //没发送成功，可能是双端还未连接好
      while( SendDataBytesToTermByMRUDP(102, pBuffer, msg_size) == false )
      {sleep(1);};
      logfile.Write("发送ma类型的数据成功！\n");

      
      /**************send ok*********************/

      //res = SendDataBytesToTermByMRUDPWithoutReliable(102, pBuffer, msg_size);
      
    }
    //接收方
    else if(starg.role==2){
      //创建非DTU的Tcp服务端通道 或 创建DTU的Tcp服务端通道
      bool res = CreateTcpServerChannel(starg.localport, starg.localip, 3020, 100, true);
      if(res==true) 
      {
        logfile.Write("接收方通道创建成功！\n");
      }
      else 
      {
        logfile.Write("接收方通道创建失败！\n");
        return -1;
      }
      
      //注册服务数据回调函数——数据服务名称是"msg_a"
      if(register_mrudp_dataserver_callback("msg_a", mrudp_dataserver_a_callback)==false)
      {
        logfile.Write("接收方的mrudp数据服务a的回调-注册失败！\n");
        return -1;
      }

    }

    //待机进入后运行
    sleep(starg.runtime);

    return 0;
}

void EXIT(int sig)
{
  System_shutdown();
  logfile.Write("程序退出, sig=%d\n\n", sig);

  exit(0);
}

//系统启动
void System_start(const char *config_path) {
    InitIniHandler(config_path);
    DWORD TermId = GetIntegerKeyIni("Main", "DeviceID", 1);
    // ... ...
    Init_NetCombTransfer(TermId);
    InitSpeedControl(TermId);

    sleep(1);

    InitMRUDP("");
    //InitBigDataTransfer();
    std::cout << "此时可靠传输模块为：MRUDP" << endl;
}

//系统关闭
void System_shutdown(bool QosOpen ) {
    DWORD TermId = GetIntegerKeyIni("Main", "DeviceID", 1);
    //UninitBigDataTransfer();
    UnInitMRUDP();
    UninitSpeedControl();
    UnInit_NetCombTransfer();
    // ... ...
    UninitIniHandle();
    printf("[SystemTest]::Terminal %d had shut down\n", TermId);
}

//MRUDP数据服务a回调函数
void mrudp_dataserver_a_callback(
              DWORD dTermId, 
              const std::shared_ptr<BYTE> &recv_data, 
              DWORD dRecvDataByteLength)
{
  msg::msg_a ma;
  logfile.Write("mrudp_dataserver_a_callback get data\n");
  if(ma.ParseFromArray(recv_data.get(), dRecvDataByteLength)==false) {
    printf("[mrudp_dataserver a]::ma.ParseFromArray failed\n");
  }else{
    printf("[mrudp_dataserver a]::ma.ParseFromArray succeed!\n");
    printf("[mrudp_dataserver a]::recv data: =%s:%s:%d=\n\n", ma.server_type().c_str(), ma.time().c_str(), ma.id());
  }
}

void _help(char *argv[])
{
  printf("\n");
  printf("Using:./test_mrudp1_1server logfilename xmlbuffer\n\n");

  printf("Sample0.1 [非DTU，发送方]:\n"\
		"./test_mrudp1_1server ../log/testmrudp_madata01.log \""\
			"<inifilepath>../SysConfig101.ini</inifilepath>"\
			"<role>1</role>"\
			"<remotetid>102</remotetid>"\
			"<localip>127.0.0.1</localip>"\
			"<remoteip>127.0.0.1</remoteip>"\
			"<localport>8000</localport>"\
			"<remoteport>9000</remoteport>"\
			"<runtime>10000</runtime>"\
      "\"\n\n");
  printf("Sample0.2 [非DTU，接收方]:\n"\
		"./test_mrudp1_1server ../log/testmrudp_madata02.log \""\
			"<inifilepath>../SysConfig102.ini</inifilepath>"\
			"<role>2</role>"\
			"<remotetid>101</remotetid>"\
			"<localip>127.0.0.1</localip>"\
			"<localport>9000</localport>"\
			"<runtime>10000</runtime>"\
      "\"\n\n\n");

  printf("本程序是应急系统的传输主程序，用于进行远程单文件数据传输。\n");
  printf("logfilename是本程序运行的日志文件。\n");
  printf("xmlbuffer为文件传输的参数，如下：\n");
  printf("  <inifilepath>../SysConfig.ini</inifilepath> 传输系统的配置文件。\n");
  printf("  <role>1</role>    传输角色，1-发送方，2-接收方。\n");
  printf("  <localip>192.168.1.100</localip>    本地服务IP地址。\n");
  printf("  <remoteip>192.168.1.101</remoteip>  远程服务IP地址。\n");
  printf("  <localport>8000</remoteport>        本地服务端口号。\n");       
  printf("  <remoteport>8000</remoteport>       远程服务端口号。\n");  
	printf("  <runtime>10000</runtime>            最长待机时间\n");  
	printf("\n\n\n");  
}

// 把xml解析到参数starg结构中
bool _xmltoarg(char *strxmlbuffer)                                                                                       
{   
	memset(&starg,0,sizeof(struct st_arg));
  GetXMLBuffer(strxmlbuffer,"inifilepath",starg.inifilepath);  
	if (strlen(starg.inifilepath)==0) { logfile.Write("host is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer,"role",&starg.role);
  if ( (starg.role!=1) && (starg.role!=2) ) starg.role=1;
  
  GetXMLBuffer(strxmlbuffer,"localip",starg.localip);
  if (strlen(starg.localip)==0) { logfile.Write("localip is null.\n"); return false; }
  
  GetXMLBuffer(strxmlbuffer,"localport",&starg.localport);
  if (starg.localport<=0) { logfile.Write("localport is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer,"runtime",&starg.runtime);
  if ( (starg.runtime<=0) || (starg.runtime>60*60*24) ){ logfile.Write("runtime is error.\n"); return false; }

  /*发送方参数*/
  if(starg.role==1)
  {
    GetXMLBuffer(strxmlbuffer,"remotetid",&starg.remotetid);
    if (starg.remotetid<=0) { logfile.Write("remotetid is null.\n"); return false; }
    
    GetXMLBuffer(strxmlbuffer,"remoteip",starg.remoteip);
    if (strlen(starg.remoteip)==0) { logfile.Write("remoteip is null.\n"); return false; }

    GetXMLBuffer(strxmlbuffer,"remoteport",&starg.remoteport);
    if (starg.remoteport<=0) { logfile.Write("remoteport is null.\n"); return false; }
  }
  
  return true;

}
