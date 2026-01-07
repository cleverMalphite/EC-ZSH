
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


struct st_arg{

    char inifilepath[101];
    int dtu;
    char localip[51];
    char remoteip[51];
    int localport;
    int remoteport;

    //std::string filename;
    //std::string servicename;
    char filename[301];
    char servicename[301];

} starg;


bool keep_running = true;

uint32_t cnt_ = 0;


dd::Lock fileblockmsg_queue_lock;
//create list queue(two direction link list) rather than dequeue(random rw array) queue
std::queue<hh::FileBlockMsg, std::list<hh::FileBlockMsg>> fileblockmsg_queue;


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

// write file thread function
void task_thread();

//recv mrudp callback function
void mrudp_fileblockrecvfunc(
              DWORD dTermId, 
              const std::shared_ptr<BYTE> &recv_data, 
              DWORD dRecvDataByteLength);


int main( int argc, char *argv[])
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

    //这个可以搞成线程池，只要有一个fileblock收到，就发送一个任务
    std::thread filewritethread(task_thread);

    //回调里，收到fileblock，存到列表，并发送一个对应的任务。(fileblock+func+argc 任务队列)
    if(register_mrudp_dataserver_callback(starg.servicename,  mrudp_fileblockrecvfunc )==false) 
    {
        logfile.Write("接收方的mrudp数据服务回调-注册失败！\n"); 
        return -1;
    }


    while(keep_running){
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    filewritethread.join();

    return 0;
}


void task_thread()
{
    uint32_t cnt = 0;
    std::unique_ptr<WriteFileBlock> p_write_file_block;
    while(keep_running) {
        usleep(1);
        while(fileblockmsg_queue.size()>0 && keep_running) {
            hh::FileBlockMsg tmp_fileblockmsg = fileblockmsg_queue.front();
            {
              dd::AutoLock autolock(fileblockmsg_queue_lock);
              fileblockmsg_queue.pop();
            } 

            if(p_write_file_block==nullptr) {
                p_write_file_block = std::move(
                              std::unique_ptr<WriteFileBlock>( new WriteFileBlock(
                                                starg.filename,
                                                tmp_fileblockmsg.filesize_total_(), 1,
                                                tmp_fileblockmsg.fileblocksize_())));
            }

            p_write_file_block->writeNextBlock(tmp_fileblockmsg);

            if(cnt%5000==0)
            printf("write a file block [%d][total block: %ld].\n", cnt, tmp_fileblockmsg.fileblocknumber_total_());
            cnt++;

            if(cnt==tmp_fileblockmsg.fileblocknumber_total_())
            {
                p_write_file_block->flushWriteFileBlock();
                p_write_file_block->finishWriteFileBlock();

                logfile.Write("finish write a file");
                
                return ;
            }

        }
    }
}

//recv mrudp callback function
void mrudp_fileblockrecvfunc(
              DWORD dTermId, 
              const std::shared_ptr<BYTE> &recv_data, 
              DWORD dRecvDataByteLength)
{
    //这个东西可以通过对象池技术优化！
    hh::FileBlockMsg fileblockmsg;

    if(fileblockmsg.ParseFromArray(recv_data.get(), dRecvDataByteLength)==false) {
      printf("[mrudp_dataserver]::fileblockmsg.ParseFromArray failed\n");
    }else{
      printf("[mrudp_dataserver]::fileblockmsg.ParseFromArray succeed!\n");
    }
    
    {
        dd::AutoLock autolock(fileblockmsg_queue_lock);
        fileblockmsg_queue.push(std::move(fileblockmsg));
    }

    cnt_++;
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
		"./DataTransfer/main_rcv ../log/main_rcv.log \""\
			"<inifilepath>../SysConfig102.ini</inifilepath>"\
			"<localip>127.0.0.1</localip>"\
			"<localport>9000</localport>"\
            "<servicename>FILE</servicename>"\
            "<filename>../../FileRecv/file.txt</filename>"\
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
  GetXMLBuffer(strxmlbuffer,"filename",starg.filename);
  if (strlen(starg.filename)==0) { logfile.Write("localip is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer,"servicename",starg.servicename);
  if (strlen(starg.servicename)==0) { logfile.Write("localip is null.\n"); return false; }

  return true;
}
