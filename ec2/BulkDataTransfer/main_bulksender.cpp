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
#include "Term2TermQos/TermToTermQosApi.h"
#include "time.h"

#include "public/_public.h"

#include "./BulkDataTransferApi.h"
#include "../DataTransfer/FileReceiver.h"

struct st_arg{
  //初始化参数
	int role;
  int remotetid;
  char inifilepath[101];
  char localip[51];
  char remoteip[51];
  int localport;
  int remoteport;
  int runtime;

  //批量文件传输参数
  char sendpath[301];
  char matchname[301];
  char okfilename[301];
  char recvpath[301];
  int timetvl;
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
void System_start(const char *config_path, bool QosOpen = true, bool isRBUDP=false) ;

//系统关闭
void System_shutdown(bool QosOpen = true) ;

int main(int argc, char *argv[]) {
		
		if(argc!=3) { _help(argv); return -1; }

    //关闭全部的信号和输入输出
   // CloseIOAndSignal();

    //处理程序退出的信号
    signal(SIGINT,EXIT); signal(SIGTERM,EXIT);

    if(logfile.Open(argv[1], "a+")==false)
    { 
        printf("打开日志文件失败(%s). \n", argv[1]); return -1;
    }

    if(_xmltoarg(argv[2])==false) return -1;

    System_start(starg.inifilepath, false);
    logfile.Write("System_start 运行成功!\n");
    printf("System_start 运行成功!\n");

    //------------------初始化程序------------------
    if(starg.role==1) //发送方
    { 
      sleep(5);//等待接收方启动后，再创建发送端
      bool res = CreateTcpClientChannel(starg.localport, 
                                        starg.remoteport, 
                                        starg.localip, 
                                        starg.remoteip, 3200);
      if(res==true)
        logfile.Write("发送方通道创建成功!(localport:%d))\n", 
                                            starg.localport);
      else 
        logfile.Write("发送方通道创建失败!\n");
    }
    else if(starg.role==2) //接收方
    { 
      bool res = CreateTcpServerChannel(starg.localport, 
                                        starg.localip, 
                                        3020, 100, true);
      if(res==true)logfile.Write("接收方通道创建成功!(localport:%d)\n", 
                                 starg.localport);
      else logfile.Write("接收方通道创建失败!\n");
    }

    //------------------批量传输------------------
    //发送方进行批量传输
    if(starg.role==1)
    {
      BulkDataTransfer bulkDataTransfer;
      DWORD TermId = GetIntegerKeyIni("Main", "DeviceID", 1);
      bulkDataTransfer.Init(  TermId, 
                starg.remotetid, 
                "FILE",// service name 
                4,  //nthreads
                false, //is1by1trans
                starg.sendpath,
    		    starg.matchname, 
    		    starg.okfilename, 
    		    starg.timetvl ,
                5000);//blocksize 

      logfile.Write("发送方开始批量传输!\n");
      printf("发送方开始批量传输!\n");
      bulkDataTransfer.AsynRun(1) ;
      //bulkDataTransfer.Run(1) ;

      //待机进入后运行
      logfile.Write("待机进入后台运行...\n");
      printf("待机进入后台运行...\n");
      sleep(starg.runtime);
    }

    //------------------接收数据------------------
    //发送方进行批量传输
    if(starg.role==2)
    {
        DWORD localTermId = GetIntegerKeyIni("Main", "DeviceID", 1);
        //ec2::FileReceiver fileReceiver(localTermId, 4);
        ec2::FileReceiver fileReceiver(localTermId, 80);
        //fileReceiver.start("../../FileRecv", "FILE", 80);
        fileReceiver.start(
                           //"../../FileRecv",//recvpath, 
                           starg.recvpath,//recvpath, 
                           "FILE" //"starg.servicename
                            );

        logfile.Write("接收方开始接收数据!\n");

        //待机进入后运行
        logfile.Write("待机进入后台运行...\n");
        sleep(starg.runtime);
    }


    return 0;
}

void EXIT(int sig)
{
  System_shutdown();

  logfile.Write("程序退出, sig=%d\n\n", sig);

  exit(0);
}

//系统启动
void System_start(const char *config_path, bool QosOpen, bool isRBUDP) {

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
  printf("Using:./main_bulksender logfilename xmlbuffer\n\n");

  printf("Sample0.1 [非DTU，发送方]:\n"\
		"./BulkDataTransfer/main_bulksender ../log/ECBulkTransfer_RSdata01.log \""\
			"<inifilepath>../SysConfig101.ini</inifilepath>"/*初始化参数*/\
			"<role>1</role>"\
			"<remotetid>102</remotetid>"\
			"<localip>127.0.0.1</localip>"\
			"<remoteip>127.0.0.1</remoteip>"\
			"<localport>7000</localport>"\
			"<remoteport>8000</remoteport>"\
			"<runtime>10000</runtime>"\
			"<sendpath>../../../FileSend</sendpath>"/*批量传输参数*/\
			"<matchname>*.PNG,*.JPEG,*.TXT,*.DAT</matchname>"\
			"<okfilename>../BulkDataTransfer/ecbulktrans_rsdata.xml</okfilename>"\
			"<timetvl>30</timetvl>"\
      "\"\n\n");
  printf("Sample0.2 [非DTU，接收方]:\n"\
		"./BulkDataTransfer/main_bulksender ../log/ECBulkTransfer_RSdata02.log \""\
			"<inifilepath>../SysConfig102.ini</inifilepath>"/*初始化参数*/\
			"<role>2</role>"\
			"<remotetid>101</remotetid>"\
			"<localip>127.0.0.1</localip>"\
			"<localport>8000</localport>"\
            "<recvpath>../../../FileRecv/</recvpath>"\
			"<runtime>10000</runtime>"\
      "\"\n\n\n");


  printf("本程序是应急系统的传输主程序，用于进行远程批量数据传输。\n");
  printf("logfilename是本程序运行的日志文件。\n");
  printf("xmlbuffer为文件传输的参数，如下：\n");
  /*批量传输参数*/
  printf("  <inifilepath>../SysConfig.ini</inifilepath> 传输系统的配置文件。\n");
  printf("  <role>1</role>    传输角色，1-发送方，2-接收方。\n");
	printf("  <remotetid>101</remotetid>            远端终端号\n");  
  printf("  <localip>192.168.1.100</localip>    本地服务IP地址。\n");
  printf("  <remoteip>192.168.1.101</remoteip>  远程服务IP地址。\n");
  printf("  <localport>8000</remoteport>        本地服务端口号。\n");
	printf("  <remoteport>8000</remoteport>       远程服务端口号。\n");  
	printf("  <runtime>10000</runtime>            最长待机时间\n");  
  /*批量传输参数*/
	printf("  <sendpath>../FileSend</sendpath>  本地文件存放的目录。\n");
	printf("  <matchname>SURF_*.TXT,*.DAT,*.GIF</matchname> 待发送文件匹配的文件名，采用大写匹配，"\
         "不匹配的文件不会被发送，本字段尽可能设置精确，不允许用*匹配全部的文件。\n");
	printf("  <okfilename>../ecbulktrans_rsdata.xml</okfilename> 已经发送成功的文件名清单，此参数只有当ptype=1时才有效。\n");
	printf("  <timetvl>30</timetvl> 发送时间间隔，单位：秒；当所发文件无文件锁、或临时文件缓存机制时，建议大于10。\n\n");
	printf("\n\n\n");  
}

// 把xml解析到参数starg结构中
bool _xmltoarg(char *strxmlbuffer)                                                                                       
{   
	memset(&starg,0,sizeof(struct st_arg));

  /*批量传输参数*/
  GetXMLBuffer(strxmlbuffer,"inifilepath",starg.inifilepath);  
	if (strlen(starg.inifilepath)==0) { logfile.Write("host is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer,"role",&starg.role);
  if ( (starg.role!=1) && (starg.role!=2) ) starg.role=1;
  
  GetXMLBuffer(strxmlbuffer,"remotetid",&starg.remotetid);
  if ( (starg.remotetid<=0) || (starg.remotetid>2000) ){ logfile.Write("remotetid is error.\n"); return false; }

  GetXMLBuffer(strxmlbuffer,"localip",starg.localip);
  if (strlen(starg.localip)==0) { logfile.Write("localip is null.\n"); return false; }
  
  GetXMLBuffer(strxmlbuffer,"localport",&starg.localport);
  if (starg.localport<=0) { logfile.Write("localport is null.\n"); return false; }


  GetXMLBuffer(strxmlbuffer,"runtime",&starg.runtime);
  if ( (starg.runtime<=0) || (starg.runtime>60*60*24) ){ logfile.Write("runtime is error.\n"); return false; }

  if( starg.role==1 ) // 发送方
  {
    GetXMLBuffer(strxmlbuffer,"remoteip",starg.remoteip);
    if (strlen(starg.remoteip)==0) { logfile.Write("remoteip is null.\n"); return false; }

    GetXMLBuffer(strxmlbuffer,"remoteport",&starg.remoteport);
    if (starg.remoteport<=0) { logfile.Write("remoteport is null.\n"); return false; }
  
    /*批量传输参数*/

    GetXMLBuffer(strxmlbuffer,"sendpath",starg.sendpath);
    if ( strlen(starg.sendpath)==0) { logfile.Write("sendpath is null.\n"); return false; }


    GetXMLBuffer(strxmlbuffer,"matchname",starg.matchname);
    if ( (starg.role==1) && strlen(starg.matchname)==0) { logfile.Write("matchname is null.\n");return false;}



    GetXMLBuffer(strxmlbuffer,"okfilename",starg.okfilename);
    if ( (strlen(starg.okfilename)==0)) {logfile.Write("okfilename is null.\n");return false;}

    GetXMLBuffer(strxmlbuffer,"timetvl",&starg.timetvl);
    if (starg.timetvl==0) { logfile.Write("timetvl is null.\n"); return false; }
  }

  if( starg.role==2 ) // 接收方
  {

    GetXMLBuffer(strxmlbuffer,"recvpath",starg.recvpath);
    if ( strlen(starg.recvpath)==0) { logfile.Write("recvpath is null.\n"); return false; }
  }

  
  return true;

}
