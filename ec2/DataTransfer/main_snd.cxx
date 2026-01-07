#include <iostream>
#include <thread>
#include "GlobalMessage.h"
#include "epollComm/EpollCommApi.h"
#include "NetCombTransferApi.h"
#include "SpeedControl/SpeedControlManager.h"
#include "MRUDP/MRUDPApi.h"

#include "public/_public.h"

#include "fileblock/FileBlock.h"
#include "fileblock/ReadFileBlock.h"
#include "fileblock/WriteFileBlock.h"

#include "fileblock/msg_cpp/FileBlockMsg.pb.h"


struct st_arg{
    char inifilepath[101];
    int dtu;
    char localip[51];
    char remoteip[51];
    int localport;
    int remoteport;

    int remotetid;

    //std::string filename;
    //std::string servicename;
    char filename[301];
    char servicename[301];
    int blocksize;
} starg;

bool keep_running = true;

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

void TransferFile( int tid, std::string servicename, std::string filename, int blocksize);


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


    TransferFile(starg.remotetid, starg.servicename, starg.filename, starg.blocksize);

    logfile.Write("finish file read and all is published.");
    while(keep_running){
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    };

    return 0;
}

void TransferFile( int tid, std::string servicename, std::string filename, int blocksize)
{

  hh::FileBlockMsg fileblockmsg;
  fileblockmsg.set_server_type(servicename);

  ReadFileBlock readfileblock(filename, blocksize);

  while(1){
    //std::this_thread::sleep_for(std::chrono::milliseconds(20));
    usleep(1);

    if(readfileblock.readNextBlock( fileblockmsg))
    {
        //序列化消息到二进制字符串
        int msg_size = fileblockmsg.ByteSizeLong(); 
        //std::shared_ptr<BYTE> pBuffer(new BYTE[msg_size], releaseArrays<BYTE>);
        std::shared_ptr<BYTE> pBuffer(new BYTE[msg_size]);
        if(fileblockmsg.SerializeToArray(pBuffer.get(), msg_size)==false)
        {
          logfile.Write("ma序列化失败!\n");
        }

        //没发送成功，可能是双端还未连接好
        while( SendDataBytesToTermByMRUDP(102, pBuffer, msg_size) == false )
        {sleep(1);};
        logfile.Write("发送ma类型的数据成功！\n");

    }else
    {
        break;
    }
  }

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
  printf("Using:./main_mrudp1_1server logfilename xmlbuffer\n\n");

  printf("Sample0.1 [非DTU，发送方]:\n"\
		"./DataTransfer/main_snd ../log/main_snd.log \""\
			"<inifilepath>../SysConfig101.ini</inifilepath>"\
			"<remotetid>102</remotetid>"\
			"<localip>127.0.0.1</localip>"\
			"<remoteip>127.0.0.1</remoteip>"\
			"<localport>8000</localport>"\
			"<remoteport>9000</remoteport>"\
            "<servicename>FILE</servicename>"\
            "<filename>../../FileSend/file.txt</filename>"\
            "<blocksize>5024</blocksize>"\
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
  GetXMLBuffer(strxmlbuffer,"remotetid",&starg.remotetid);
  if (starg.remotetid<=0) { logfile.Write("remotetid is null.\n"); return false; }
  
  GetXMLBuffer(strxmlbuffer,"remoteip",starg.remoteip);
  if (strlen(starg.remoteip)==0) { logfile.Write("remoteip is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer,"remoteport",&starg.remoteport);
  if (starg.remoteport<=0) { logfile.Write("remoteport is null.\n"); return false; }
  
  // file transfer
  GetXMLBuffer(strxmlbuffer,"filename",starg.filename);
  if (strlen(starg.filename)==0) { logfile.Write("localip is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer,"servicename",starg.servicename);
  if (strlen(starg.servicename)==0) { logfile.Write("localip is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer,"blocksize",&starg.blocksize);
  if (starg.blocksize<=0) { logfile.Write("localip is null.\n"); return false; }

  return true;

}
