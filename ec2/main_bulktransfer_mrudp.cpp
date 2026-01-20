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

#include "msg_cpp/msg.pb.h"


struct st_arg{
  //初始化参数
	int role;
	int dtu; // 数据传输单元ID
  int remotetid;
  char inifilepath[101]; // 配置文件路径
  char localip[51]; // 本地IP地址
  char remoteip[51]; // 远程IP地址
  int localport; // 本地端口号
  int remoteport; // 远程端口号
  int runtime; // 运行时间

  //批量文件传输参数
  char localpath[301]; // 本地文件目录路径
  char matchname[301]; // 匹配文件名表达式
  int ptype; // 处理类型
  char localpathbak[301]; // 本地备份目录路径
  char okfilename[301]; // 成功文件名
  int timetvl; // 时间间隔
} starg;

CLogFile logfile;

bool isrbudp;

bool startAutoSend = false;

//本程序的业务流程主函数
bool _ectransfiles();

vector<struct st_fileinfo> vlistfile, vlistfile1; // 本地文件列表容器
vector<struct st_fileinfo> vokfilename, vokfilename1; // 成功文件名列表容器

//把localpath目录下的文件加载到vlistfile容器中
bool LoadListFile();

//把okfilename文件内容加载到vokfilename容器中
bool LoadOKFileName();

//把vlistfile容器中的文件与vokfilename容器中的文件对比，得到两个容器
//一、在vlistfile中存在，并已经发送成功的文件vokfilename1
//二、在vlistfile中存在，新文件或需要重新发送的文件vlistfile1
bool CompVector();

//把vokfilename1容器中的内容写入okfilename文件中，覆盖之前的旧okfilename文件
bool WriteToOKFileName();

//如果ptype==1,把发送成功的文件记录追加到okfilename文件中
bool AppendToOKFileName(struct st_fileinfo *stfileinfo);

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

//MRUDP数据服务a回调函数
void mrudp_dataserver_a_callback(
                DWORD dTermId,  // 终端ID
                const std::shared_ptr<BYTE> &recv_data, 
                DWORD dRecvDataByteLength); // 接收数据字节长度

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

    System_start(starg.inifilepath, false);
    logfile.Write("System_start 运行成功!\n");

    //------------------初始化程序------------------
    if(starg.role==1) //发送方
    { 
      //注册服务数据回调函数——数据服务名称是"msg_a"
      if(register_mrudp_dataserver_callback("msg_a", mrudp_dataserver_a_callback)==false)
      {
        logfile.Write("接收方的mrudp数据服务a的回调-注册失败！\n");
        return -1;
      }

      sleep(5);//等待接收方启动后，再创建发送端
    
      //创建非DTU的Tcp客户端通道 或 创建DTU的Tcp客户端通道
      bool res = starg.dtu==0? \
                 CreateTcpClientChannel(starg.localport, starg.remoteport, starg.localip, starg.remoteip, 3200):\
                 CreateDtuTcpClientChannel(starg.localport, starg.remoteport, starg.localip, starg.remoteip, 3200);
      if(res==true)logfile.Write("发送方通道创建成功!(dtu:%d)\n", starg.dtu);
      else logfile.Write("发送方通道创建失败!(dtu:%d)\n", starg.dtu);


      logfile.Write("Wait for startAutoSendMessage ...\n");
      while(res==false || startAutoSend==false) { usleep(1000); }

      //------------------批量传输------------------
      //发送方进行批量传输
      logfile.Write("收到StartAutoSend信号，发送方开始批量传输!\n");
      printf("收到StartAutoSend信号，发送方开始批量传输！\n");
      while(true)
      {
        _ectransfiles();

        sleep(starg.timetvl);
      }

    }
    else if(starg.role==2) //接收方
    { 
      //创建非DTU的Tcp服务端通道 或 创建DTU的Tcp服务端通道
      bool res = starg.dtu==0?\
                CreateTcpServerChannel(starg.localport, starg.localip, 3020, 100, true):\
                CreateDtuTcpServerChannel(starg.localport, starg.localip, 3020, 100);
      if(res==true)logfile.Write("接收方通道创建成功!(dtu:%d)\n", starg.dtu);
      else logfile.Write("接收方通道创建失败!(dtu:%d)\n", starg.dtu);

      if(res == true) 
      {
        //创建protobuf消息类型
        /*生成序列号*/
        int generate_seed = 0;
        
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
        while( SendDataBytesToTermByMRUDP(starg.remotetid, pBuffer, msg_size) == false )
        {sleep(1);};
        logfile.Write("发送ma类型的数据(这里作为StartAutoSend信号)成功！\n");
      }

    }


    //待机进入后运行
    logfile.Write("待机进入后台运行...\n");
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
void System_start(const char *config_path, bool QosOpen, bool isRBUDP) {
    InitIniHandler(config_path);
    DWORD TermId = GetIntegerKeyIni("Main", "DeviceID", 1);
    bool Is_RBUDP = isrbudp = isRBUDP;
    // ... ...
    Init_NetCombTransfer(TermId);
    InitSpeedControl(TermId);
    sleep(1);
    if (QosOpen) {
        InitTermToTermQos(TermId);
    }
    if (Is_RBUDP) {
        InitRBUDP("");
        std::cout << "此时可靠传输模块为：RBUDP" << endl;
    } else {
        InitMRUDP("");
        std::cout << "此时可靠传输模块为：MRUDP" << endl;
    }
    InitBigDataTransfer();
}

//系统关闭
void System_shutdown(bool QosOpen ) {
    DWORD TermId = GetIntegerKeyIni("Main", "DeviceID", 1);
    bool Is_RBUDP = isrbudp;
    UninitBigDataTransfer();
    if (Is_RBUDP) {
        UnInitRBUDP();
    } else {
        UnInitMRUDP();
    }
    if (QosOpen) {
        UninitTermToTermQos();
    }
    UninitSpeedControl();
    UnInit_NetCombTransfer();
    UninitIniHandle();
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
    printf("[mrudp_dataserver a]::recv data: =%s:%s:%d=\n", ma.server_type().c_str(), ma.time().c_str(), ma.id());
    printf("[mrudp_dataserver a]::recv StartAutoSend Message!!!\n\n");
    startAutoSend = true;
  }
}


// 本程序的业务流程主函数(发送方)
bool _ectransfiles()
{
  //须确保对端存在指定的接收目录
  
  if(LoadListFile()==false)
  {
    logfile.Write("LoadListFile() failed.\n"); return false;
  }

  if(starg.ptype==1)
  {
    //加载okfilename文件中的内容到vokfilename中
    LoadOKFileName();

    //把vlistfile容器中的文件与vokfilename容器中文件对比，得到两个容器
    //一、在vlistfile中存在，并且已经发送成功的文件vokfilename1
    //二、在vlistfile中存在，新文件或需要重新发送的文件vlistfile1
    CompVector();

    //把vokfilename1容器中的内容先写入okfilename文件中，覆盖之前的旧okfilename文件
    WriteToOKFileName();

    //把vlistfile1容器中的内容复制到vlistfile容器中
    vlistfile.clear(); vlistfile.swap(vlistfile1);
  }

  //把客户端的新文件或已经改动后的文件发送给服务端
  for(int ii=0; ii<vlistfile.size(); ii++)
  {
      char strremotefilename[301],strlocalfilename[301];
      SNPRINTF(strlocalfilename,300,"%s/%s",starg.localpath,vlistfile[ii].filename);

      logfile.Write("transfer %s ...",strlocalfilename);

      // 发送文件
      DWORD TaskID;
      if(Create_BigDataTransferTask(starg.remotetid, vlistfile[ii].filename, strlocalfilename, TaskID)==false)
      {
        logfile.WriteEx("failed.\n"); break;
      }

      logfile.WriteEx("ok.\n");

      // 删除文件
      if (starg.ptype==2) REMOVE(strlocalfilename);

      // 转存到备份目录
      if (starg.ptype==3)
      {
        char strfilenamebak[301];
        memset(strfilenamebak,0,sizeof(strfilenamebak));
        sprintf(strfilenamebak,"%s/%s",starg.localpathbak,vlistfile[ii].filename);
        if (RENAME(strlocalfilename,strfilenamebak)==false)
        {
          logfile.Write("RENAME %s to %s failed.\n",strlocalfilename,strfilenamebak); return false;
        }
      }

      // 如果ptype==1，把发送成功的文件记录追加到okfilename文件中
      if (starg.ptype==1) AppendToOKFileName(&vlistfile[ii]);

  }
}

// 把localpath目录下的文件加载到vlistfile容器中
bool LoadListFile()
{
  vlistfile.clear();

  CDir Dir;

  // 不包括子目录
  // 注意，如果目录下的总文件数超过50000，增量发送文件功能将有问题
  if (Dir.OpenDir(starg.localpath,starg.matchname,50000,false,false)==false)
  {
    logfile.Write("Dir.OpenDir(%s) 失败。\n",starg.localpath); return false;
  }

  struct st_fileinfo stfileinfo;

  while (true)
  {
    memset(&stfileinfo,0,sizeof(struct st_fileinfo));

    if (Dir.ReadDir()==false) break;

    strcpy(stfileinfo.filename,Dir.m_FileName);  // 文件名，不包括目录名
    strcpy(stfileinfo.mtime,Dir.m_ModifyTime);
    stfileinfo.filesize=Dir.m_FileSize;

    vlistfile.push_back(stfileinfo);

    // logfile.Write("vlistfile filename=%s,mtime=%s\n",stfileinfo.filename,stfileinfo.mtime);
  }

  return true;
}

// 把okfilename文件内容加载到vokfilename容器中
bool LoadOKFileName()
{
  vokfilename.clear();

  CFile File;

  // 注意：如果程序是第一次发送，okfilename是不存在的，并不是错误，所以也返回true。
  if (File.Open(starg.okfilename,"r") == false) return true;

  struct st_fileinfo stfileinfo;

  char strbuffer[301];

  while (true)
  {
    memset(&stfileinfo,0,sizeof(struct st_fileinfo));

    if (File.Fgets(strbuffer,300,true)==false) break;

    GetXMLBuffer(strbuffer,"filename",stfileinfo.filename,300);
    GetXMLBuffer(strbuffer,"mtime",stfileinfo.mtime,20);

    vokfilename.push_back(stfileinfo);

    // logfile.Write("vokfilename filename=%s,mtime=%s\n",stfileinfo.filename,stfileinfo.mtime);
  }

  return true;
}


// 把vlistfile容器中的文件与vokfilename容器中文件对比，得到两个容器
// 一、在vlistfile中存在，并已经发送成功的文件vokfilename1
// 二、在vlistfile中存在，新文件或需要重新发送的文件vlistfile1
bool CompVector()
{                                                                                                                                                        
  vokfilename1.clear();  vlistfile1.clear();

  for (int ii=0;ii<vlistfile.size();ii++)
  {
    int jj=0;
    for (jj=0;jj<vokfilename.size();jj++)
    {
      if ( (strcmp(vlistfile[ii].filename,vokfilename[jj].filename)==0) &&
           (strcmp(vlistfile[ii].mtime,vokfilename[jj].mtime)==0) )
      {
        vokfilename1.push_back(vlistfile[ii]); break;
      }    
    } 
        
    if (jj==vokfilename.size())
    { 
      vlistfile1.push_back(vlistfile[ii]);
    }
  } 
      
  /*
  for (int ii=0;ii<vokfilename1.size();ii++)
  {   
    logfile.Write("vokfilename1 filename=%s,mtime=%s\n",vokfilename1[ii].filename,vokfilename1[ii].mtime);
  }

  for (int ii=0;ii<vlistfile1.size();ii++)
  {
    logfile.Write("vlistfile1 filename=%s,mtime=%s\n",vlistfile1[ii].filename,vlistfile1[ii].mtime);
  }
  */

  return true;
}
// 把vokfilename1容器中的内容先写入okfilename文件中，覆盖之前的旧okfilename文件
bool WriteToOKFileName()
{
  CFile File;

  // 注意，打开文件不要采用缓冲机制
  if (File.Open(starg.okfilename,"w",false) == false)
  {
    logfile.Write("File.Open(%s) failed.\n",starg.okfilename); return false;
  }
  
  for (int ii=0;ii<vokfilename1.size();ii++)
  {
    File.Fprintf("<filename>%s</filename><mtime>%s</mtime>\n",vokfilename1[ii].filename,vokfilename1[ii].mtime);
  }
  
  return true;
} 
// 如果ptype==1，把发送成功的文件记录追加到okfilename文件中
bool AppendToOKFileName(struct st_fileinfo *stfileinfo)
{ 
  CFile File;

  // 注意，打开文件不要采用缓冲机制
  if (File.Open(starg.okfilename,"a",false) == false)
  {
    logfile.Write("File.Open(%s) failed.\n",starg.okfilename); return false;
  }
  
  File.Fprintf("<filename>%s</filename><mtime>%s</mtime>\n",stfileinfo->filename,stfileinfo->mtime);
  
  return true;
} 


void _help(char *argv[])
{
  printf("\n");
  printf("Using:./ec_bulktransfer_mrudp logfilename xmlbuffer\n\n");

  printf("Sample0.1 [非DTU，发送方]:\n"\
		"./ec_bulktransfer_mrudp ../log/ECBulkTransfer_mrudp_RSdata01.log \""\
			"<inifilepath>../SysConfig101.ini</inifilepath>"/*初始化参数*/\
			"<dtu>0</dtu>"\
			"<role>1</role>"\
			"<remotetid>102</remotetid>"\
			"<localip>127.0.0.1</localip>"\
			"<remoteip>127.0.0.1</remoteip>"\
			"<localport>8000</localport>"\
			"<remoteport>8000</remoteport>"\
			"<runtime>10000</runtime>"\
			"<localpath>../../FileSend</localpath>"/*批量传输参数*/\
			"<matchname>SURF_*.TXT,*.DAT</matchname>"\
			"<ptype>1</ptype>"\
			"<localpathbak></localpathbak>"\
			"<okfilename>../BulkDataTransfer/ecbulktrans_rsdata.xml</okfilename>"\
			"<timetvl>30</timetvl>"\
      "\"\n\n");
  printf("Sample0.2 [非DTU，接收方]:\n"\
		"./ec_bulktransfer_mrudp ../log/ECBulkTransfer_mrudp_RSdata02.log \""\
			"<inifilepath>../SysConfig102.ini</inifilepath>"/*初始化参数*/\
			"<dtu>0</dtu>"\
			"<role>2</role>"\
			"<remotetid>101</remotetid>"\
			"<localip>127.0.0.1</localip>"\
			"<localport>8000</localport>"\
			"<runtime>10000</runtime>"\
      "\"\n\n\n");

  printf("Sample1.1 [DTU，发送方]:\n"\
		"./ec_bulktransfer ../log/ECBulkTransfer_RSdata11.log \""\
			"<inifilepath>../config_dtu.ini</inifilepath>"/*初始化参数*/\
			"<dtu>1</dtu>"\
			"<role>1</role>"\
			"<remotetid>102</remotetid>"\
			"<localip>192.168.1.101</localip>"\
			"<remoteip>10.10.207.1</remoteip>"\
			"<localport>7000</localport>"\
			"<remoteport>8000</remoteport>"\
			"<runtime>10000</runtime>"\
			"<localpath>../../FileSend</localpath>"/*批量传输参数*/\
			"<matchname>SURF_*.TXT,*.DAT</matchname>"\
			"<ptype>1</ptype>"\
			"<localpathbak></localpathbak>"\
			"<okfilename>../BulkDataTransfer/ecbulktrans_rsdata.xml</okfilename>"\
			"<timetvl>30</timetvl>"\
      "\"\n\n");
  printf("Sample1.2 [DTU，接收方]:\n"\
		"./ec_bulktransfer ../log/ECBulkTransfer_RSdata12.log \""\
			"<inifilepath>../config_dtu.ini</inifilepath>"/*初始化参数*/\
			"<dtu>1</dtu>"\
			"<role>2</role>"\
			"<remotetid>101</remotetid>"\
			"<localip>192.168.1.102</localip>"\
			"<localport>7000</localport>"\
			"<runtime>10000</runtime>"\
      "\"\n\n\n");


  printf("本程序是应急系统的传输主程序，用于进行远程批量数据传输。\n");
  printf("logfilename是本程序运行的日志文件。\n");
  printf("xmlbuffer为文件传输的参数，如下：\n");
  /*批量传输参数*/
  printf("  <inifilepath>../SysConfig.ini</inifilepath> 传输系统的配置文件。\n");
  printf("  <role>1</role>    传输角色，1-发送方，2-接收方。\n");
  printf("  <dtu>0</dtu>      DTU启用，0-不启用，1-启用。\n");
	printf("  <remotetid>101</remotetid>            远端终端号\n");  
  printf("  <localip>192.168.1.100</localip>    本地服务IP地址。\n");
  printf("  <remoteip>192.168.1.101</remoteip>  远程服务IP地址。\n");
  printf("  <localport>8000</remoteport>        本地服务端口号。\n");
	printf("  <remoteport>8000</remoteport>       远程服务端口号。\n");  
	printf("  <runtime>10000</runtime>            最长待机时间\n");  
  /*批量传输参数*/
	printf("  <localpath>../FileSend</localpath>  本地文件存放的目录。\n");
	printf("  <matchname>SURF_*.TXT,*.DAT,*.GIF</matchname> 待发送文件匹配的文件名，采用大写匹配，"\
         "不匹配的文件不会被发送，本字段尽可能设置精确，不允许用*匹配全部的文件。\n");
	printf("  <ptype>1</ptype>  文件发送成功后，本地文件的处理方式：1-什么也不做；2-删除；3-备份，如果为3，还要指定备份的目录。\n");
	printf("  <localpathbak></localpathbak> 文件发送成功后，本地文件的备份目录，此参数只有当ptype=3时才有效。\n");
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
  
  GetXMLBuffer(strxmlbuffer,"dtu",&starg.dtu);
  if ( (starg.dtu!=0) && (starg.dtu!=1) ) starg.dtu=0;

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

    GetXMLBuffer(strxmlbuffer,"localpath",starg.localpath);
    if ( strlen(starg.localpath)==0) { logfile.Write("localpath is null.\n"); return false; }

    GetXMLBuffer(strxmlbuffer,"matchname",starg.matchname);
    if ( (starg.role==1) && strlen(starg.matchname)==0) { logfile.Write("matchname is null.\n");return false;}

    GetXMLBuffer(strxmlbuffer,"ptype",&starg.ptype);
    if ( (starg.ptype!=1) && (starg.ptype!=2) && (starg.ptype!=3) ){logfile.Write("ptype is error.\n");return false;}

    GetXMLBuffer(strxmlbuffer,"localpathbak",starg.localpathbak);
    if ((starg.ptype==3) && (strlen(starg.localpathbak)==0)) {logfile.Write("localpathbak is null.\n");return false;}

    GetXMLBuffer(strxmlbuffer,"okfilename",starg.okfilename);
    if ((starg.ptype==1) && (strlen(starg.okfilename)==0)) {logfile.Write("okfilename is null.\n");return false;}

    GetXMLBuffer(strxmlbuffer,"timetvl",&starg.timetvl);
    if (starg.timetvl==0) { logfile.Write("timetvl is null.\n"); return false; }
  }

  
  return true;

}
