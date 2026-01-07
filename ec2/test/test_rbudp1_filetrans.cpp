#include <iostream>
#include "GlobalMessage.h"
#include "epollComm/EpollCommApi.h"
#include "RBUDP/RBUDPApi.h"
#include "MRUDP/MRUDPApi.h"
#include "Util/LockGuard.h"
#include "NetCombTransferApi.h"
#include "BigDataTransfer/BigDataTransferApi.h"
#include "CNetTerminalManager.h"
#include "SpeedControl/SpeedControlManager.h"
#include "time.h"

#include "public/_public.h"

struct st_arg{
  char inifilepath[101];
	int role;
  char localip[51];
  char remoteip[51];
  int localport;
  int remoteport;
  int runtime;

  int remotetid;

  char sendfilename[301];
  char sendfilepath[301];
} starg;

CLogFile logfile;

bool isrbudp;

//退出函数
void EXIT(int sig);

//显示程序帮助
void _help(char *argv[]);

//把xml解析到参数starg结构中
bool _xmltoarg(char *strxmlbuffer);

//系统启动
void System_start(const char *config_path,  bool IsRBUDP = false) ;

//系统关闭
void System_shutdown() ;


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

    System_start(starg.inifilepath, true);
    logfile.Write("System_start 运行成功!\n");

    //发送方
    if(starg.role==1){
      sleep(5);//等待接收方启动后，再创建发送端
      DWORD TaskID;
      bool res;
      res = CreateTcpClientChannel(starg.localport, 
                                   starg.remoteport, 
                                   starg.localip, 
                                   starg.remoteip, 
                                   3200);\
      if(res==true) 
        logfile.Write("发送方通道创建成功！!\n");
      else 
      { 
        logfile.Write("发送方通道创建失败！!\n"); 
        return -1;
      }

      printf("准备创建文件传输任务...");
      for(int i = 0; i < 5; i++)
      {
        sleep(1);
        printf("... ");
      }
      printf("... \n");
      printf("开始创建文件传输任务...\n");


      /*****************发送文件*************************/
      //向目标终端发送文件
      //res = Create_BigDataTransferTask(starg.remotetid,
      //                                 starg.sendfilename,
      //                                 starg.sendfilepath,
      //                                 TaskID);
      res = Create_BigDataTransferTask(starg.remotetid,
                                       starg.sendfilename,
                                       starg.sendfilepath,
                                       TaskID,
                                       true);//is_RBUDP
      if(res==true) 
      {
        logfile.Write("发送任务创建成功！(sendfilename:%s;"\
          " sendfilepath:%s)!\n", starg.sendfilename, starg.sendfilepath);
        printf("开始创建文件传输任务...\n");
      }
      else
      { 
        logfile.Write("发送任务创建失败！(sendfilename:%s;"\
          " sendfilepath:%s)!\n", starg.sendfilename, starg.sendfilepath); 
        printf("创建文件传输任务失败!\n");
        return -1;
      }

    }
    //接收方
    else if(starg.role==2){
      //创建非DTU的Tcp服务端通道 或 创建DTU的Tcp服务端通道
      bool res = CreateTcpServerChannel(starg.localport,
                                        starg.localip, 
                                        3020,
                                        100,
                                        true);\
      if(res==true) 
      {
        logfile.Write("接收方通道创建成功！!\n");
      }
      else 
      {
        logfile.Write("接收方通道创建失败！!\n");
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
void System_start(const char *config_path,  bool IsRBUDP) {
    InitIniHandler(config_path);
    DWORD TermId = GetIntegerKeyIni("Main", "DeviceID", 1);
    bool Is_RBUDP = isrbudp = IsRBUDP;
    // ... ...
    Init_NetCombTransfer(TermId);
    InitSpeedControl(TermId);
    sleep(1);

    //NOTE MRUDP first
    InitMRUDP("");
    InitRBUDP("");

    InitBigDataTransfer();
    //SystemControlInit();
    //printf("[SystemTest]::Terminal %d had inited\n", TermId);
}

//系统关闭
void System_shutdown() {
    DWORD TermId = GetIntegerKeyIni("Main", "DeviceID", 1);
    bool Is_RBUDP = isrbudp;
    UninitBigDataTransfer();
    UnInitRBUDP();
    UnInitMRUDP();
    UninitSpeedControl();
    UnInit_NetCombTransfer();
    // ... ...
    UninitIniHandle();
    //printf("[SystemTest]::Terminal %d had shut down\n", TermId);
}

void _help(char *argv[])
{
  printf("\n");
  printf("Using:./test_rbudp1_filetrans logfilename xmlbuffer\n\n");

  printf("Sample0.1 [发送方]:\n"\
		"./test_rbudp1_filetrans ../log/testrbudp1_01.log \""\
			"<inifilepath>../SysConfig101.ini</inifilepath>"\
			"<role>1</role>"\
			"<remotetid>102</remotetid>"\
			"<localip>127.0.0.1</localip>"\
			"<remoteip>127.0.0.1</remoteip>"\
			"<localport>8000</localport>"\
			"<remoteport>9000</remoteport>"\
			"<runtime>10000</runtime>"\
			"<sendfilename>file.txt</sendfilename>"\
			"<sendfilepath>../FileSend/file.txt</sendfilepath>"\
      "\"\n\n");
  printf("Sample0.2 [接收方]:\n"\
		"./test_rbudp1_filetrans ../log/testrbudp1_02.log \""\
			"<inifilepath>../SysConfig102.ini</inifilepath>"\
			"<role>2</role>"\
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

    GetXMLBuffer(strxmlbuffer,"sendfilename",starg.sendfilename);
    if (strlen(starg.sendfilename)==0) { logfile.Write("sendfilename is null.\n"); return false; }

    GetXMLBuffer(strxmlbuffer,"sendfilepath",starg.sendfilepath);
    if (strlen(starg.sendfilepath)==0) { logfile.Write("sendfilepath is null.\n"); return false; }
  }
  
  return true;

}
