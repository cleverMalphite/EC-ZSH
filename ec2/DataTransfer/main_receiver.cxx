
#include <iostream>
#include <thread>
#include "GlobalMessage.h"
#include "epollComm/EpollCommApi.h"
#include "NetCombTransferApi.h"
#include "SpeedControl/SpeedControlManager.h"
#include "MRUDP/MRUDPApi.h"

#include "public/utility.h"
#include "public/_public.h"

#include "fileblock/FileBlock.h"
#include "fileblock/ReadFileBlock.h"
#include "fileblock/WriteFileBlock.h"

#include "fileblock/msg_cpp/FileBlockMsg.pb.h"

#include "./FileReceiver.h"

#include "infoHub/infoHub.h"
#include "infoHub/infoHubRpcServer.h"

using namespace ec2;


std::shared_ptr<ec2::infoHub> infohub_instance =\
		      ec2::infoHub::getInstance();

infoHubRpcServer receiver_infohub_rpcserver;


struct st_arg{

    char inifilepath[101];
    int dtu;
    char localip[51];
    char remoteip[51];
    int localport;
    int remoteport;

    //std::string recvpath;
    //std::string servicename;
    char recvpath[301];
    char servicename[301];

} starg;


CLogFile logfile;

//显示程序帮助
void _help(char *argv[]);

//把xml解析到参数starg结构中
bool _xmltoarg(char *strxmlbuffer);

//系统启动
void System_start(const char *config_path) ;

//系统关闭
void System_shutdown(bool QosOpen = true) ;

//退出函数
void EXIT(int sig);

int main( int argc, char** argv)
{
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

    //创建非DTU的Tcp客户端通道 或 创建DTU的Tcp客户端通道
    bool res = CreateTcpServerChannel(starg.localport, starg.localip, 3020, 100, true);
    if(res==true) 
        logfile.Write("接收方通道创建成功！\n");
    else 
    { 
        logfile.Write("接收方通道创建失败！\n"); 
        return -1;
    }

   	receiver_infohub_rpcserver.init(infohub_instance, 8001);
   	receiver_infohub_rpcserver.async_run(1);


    DWORD TermId = GetIntegerKeyIni("Main", "DeviceID", 1);
    ec2::FileReceiver fileReceiver(TermId, 100);
    //fileReceiver.start("../../FileRecv", "FILE", 80);
    fileReceiver.start(starg.recvpath, starg.servicename);


    while(true){
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    fileReceiver.end();

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

void _help(char *argv[])
{
  printf("\n");
  printf("Using:./main_receiver logfilename xmlbuffer\n\n");

  printf("Sample0.1 [非DTU，发送方]:\n"\
		"./DataTransfer/main_receiver ../log/main_receiver.log \""\
			"<inifilepath>../SysConfig102.ini</inifilepath>"\
			"<localip>127.0.0.1</localip>"\
			"<localport>9000</localport>"\
            "<servicename>FILE</servicename>"\
            "<recvpath>../../FileRecv/</recvpath>"\
      "\"\n\n");
}

// 把xml解析到参数starg结构中
bool _xmltoarg(char *strxmlbuffer)                                                                                       
{   
  memset(&starg,0,sizeof(struct st_arg));

  GetXMLBuffer(strxmlbuffer,"inifilepath",starg.inifilepath);  
  if (strlen(starg.inifilepath)==0) { logfile.Write("host is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer,"localip",starg.localip);
  if (strlen(starg.localip)==0) { logfile.Write("localip is null.\n"); return false; }
  
  GetXMLBuffer(strxmlbuffer,"localport",&starg.localport);
  if (starg.localport<=0) { logfile.Write("localport is null.\n"); return false; }

  // send args
  // file transfer
  GetXMLBuffer(strxmlbuffer,"recvpath",starg.recvpath);
  if (strlen(starg.recvpath)==0) { logfile.Write("localip is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer,"servicename",starg.servicename);
  if (strlen(starg.servicename)==0) { logfile.Write("localip is null.\n"); return false; }

  return true;

}
