#ifndef _FILE_SENDER_H__
#define _FILE_SENDER_H__

#include <iostream>
#include <thread>
#include "GlobalMessage.h"
#include "epollComm/EpollCommApi.h"
#include "NetCombTransferApi.h"
#include "SpeedControl/SpeedControlManager.h"
#include "MRUDP/MRUDPApi.h"

#include "public/_public.h"
#include "public/utility.h"

#include "fileblock/FileBlock.h"
#include "fileblock/ReadFileBlock.h"
#include "fileblock/WriteFileBlock.h"

#include "fileblock/msg_cpp/FileBlockMsg.pb.h"

#include <hv/hv.h>
#include <hv/EventLoopThreadPool.h>

namespace ec2{

#define MAX_BUFFER_NUM 100000

#define MAX_ONLINE_TASK 5


CLogFile fslogger;

//每个文件对应一个索引号
dd::Lock _snd_file_index_lock;;
int snd_file_index=0;



struct FileSendTask{
    int tid; 
    std::string servicename; 
    std::string filename; 
    int blocksize;
    int onceReadNum=1;
};

typedef std::queue<FileSendTask, std::list<FileSendTask>> FileTaskQueue;


void FileRcvFnMrudpCallback(
              DWORD dTermId, 
              const std::shared_ptr<BYTE> &recv_data, 
              DWORD dRecvDataByteLength,
              void *handle);

//bool ConnectServer(int &localport, const std::string &localip);
//bool CreateServer(int &localport, int remoteport, const std::string localip, const std::string remoteip);
class FileSender
{
public:
    FileSender(int stid, int nthreads = 4){
        _src_tid = stid;

        eloop_thrdpool.setThreadNum(nthreads);
        eloop_thrdpool.start(true);

        if(fslogger.Open("../log/FileSender.log", "a+")==false)
        {
            printf("<FileSender>打开日志文件出错！\n");
            exit(0);
        }

        //回调里，收到fileblock，存到列表，并创建一个对应的文件任务Handle。
        if(register_mrudp_dataserver_callback("FILE_RECV_FINISH_MSG", 
                                              FileRcvFnMrudpCallback, 
                                              this)==false) 
        {
            printf("<FileReceiver>接收方的mrudp数据服务回调-注册失败！\n"); 
        }

    };


    FileSender(){}

    void init(int stid, int nthreads = 4){
        _src_tid = stid;

        eloop_thrdpool.setThreadNum(nthreads);
        eloop_thrdpool.start(true);

        if(fslogger.Open("../log/FileSender.log", "a+")==false)
        {
            printf("<FileSender>打开日志文件出错！\n");
            exit(0);
        }

        //回调里，收到fileblock，存到列表，并创建一个对应的文件任务Handle。
        if(register_mrudp_dataserver_callback("FILE_RECV_FINISH_MSG", 
                                              FileRcvFnMrudpCallback, 
                                              this)==false) 
        {
            printf("<FileReceiver>接收方的mrudp数据服务回调-注册失败！\n"); 
        }

    };

    ~FileSender(){
    };
    
    bool CreateTransferFileTask( int tid, 
                                 std::string servicename, 
                                 std::string filename, 
                                 int blocksize,
                                 int onceReadNum=1
                                 );

    void TransferFile( int tid, 
                       std::string servicename, 
                       std::string filename, 
                       int blocksize,
                       int onceReadNum=1
                       );

    void enableTrans1By1(bool enable=true){
        _enable_1by1trans = enable;
    }

    bool canTransferNextFile(){
        dd::AutoLock autoLock(_canTransLock);
        return _canTransferNextFile ;
    }

public:
    hv::EventLoopThreadPool eloop_thrdpool;
    int _src_tid;

    /* 文件任务和文件名的相互映射
     * task_name -> file_name map
     * file_name -> task_name map
    */
    dd::Lock _file_task_map_lock;;
    std::map<std::string, std::string> _file_names;
    std::map<std::string, std::string> _task_names;

    //int snd_file_index=0;
    dd::Lock _idx_file_map_lock;;
    std::map<int, std::string> _idx2file;
    std::map<std::string, int> _file2idx;

    
    bool _enable_1by1trans = true;

    dd::Lock _canTransLock;
    bool _canTransferNextFile = true;

    dd::Lock _onlineTaskLock;;
    int online_task_num = 0;

    dd::Lock _startLock;;
    bool _start = true;

    dd::Lock      _task_queue_lock;
    FileTaskQueue _task_queue;
};


void FileRcvFnMrudpCallback(
              DWORD dTermId, 
              const std::shared_ptr<BYTE> &recv_data, 
              DWORD dRecvDataByteLength,
              void *handle){

    //获取对应的FileReceiver调用对象
    FileSender* psender = static_cast<FileSender*>(handle);

    //TODO:这个东西可以通过对象池技术优化！
    //hh::FileBlockMsg filemsg;
    hh::RecvFinishMsg fnmsg;
    if(fnmsg.ParseFromArray(recv_data.get(), dRecvDataByteLength)==false) {
      printf("FileReceiver>FileReceiverMrudpCallback::filemsg.ParseFromArray failed\n");
    }else{
      printf("FileReceiver>FileReceiverMrudpCallback::filemsg.ParseFromArray succeed!\n");
    }

    //fnmsg.set_strmsg("FILE_RECV_RATE");
    
    int stid = fnmsg.stid();//文件从这个tid发来
    int dtid = fnmsg.dtid();//文件发到这个tid
    std::string filename    = fnmsg.filename();//文件发到这个tid
    std::string paramtype   = fnmsg.strmsg();

    printf("__________________________________\n");
    fslogger.Write("__________________________________\n");

    printf("[FileRecvFinishMessge]filename:%s\n", filename.c_str());
    fslogger.Write("[FileRecvFinishMessge]filename:%s\n", filename.c_str());

    if(paramtype == "FILE_RECV_RATE")
    {
        double rcvRate          = fnmsg.dblmsg();
        printf("[FileRecvFinishMessge]rcvRate:%lf\n", rcvRate);
        fslogger.Write("[FileRecvFinishMessge]rcvRate:%lf\n", rcvRate);
        {
        dd::AutoLock autolock(psender->_onlineTaskLock);
        psender->online_task_num--;
        }

        {
        dd::AutoLock autolock(psender->_canTransLock);
        psender->_canTransferNextFile = true;
        }
    }
    {
    dd::AutoLock autolock(psender->_startLock);
    if(psender->_start==true && paramtype == "FILE_STOP_TRANS")
    {
        psender->_start = false;
        printf("[FILE_STOP_MSG]\n");
        fslogger.Write("[FILE_STOP_MSG]\n");
    }
    if(psender->_start==false && paramtype == "FILE_START_TRANS")
    {
        psender->_start = true;
        printf("[FILE_START_MSG]\n");
        fslogger.Write("[FILE_START_MSG]\n");
    }
    }//end _startLock

}

bool
FileSender::CreateTransferFileTask( int tid, 
                                    std::string servicename, 
                                    std::string filename, 
                                    int blocksize,
                                    int onceReadNum
                                    )
{
    std::string fileName_withoutPath = GetFileNameFromPathStr(filename);
    std::string task_name = filename;

    {
    dd::AutoLock autolock(_file_task_map_lock);
    _file_names[task_name] = fileName_withoutPath;
    _task_names[fileName_withoutPath] = task_name;
    }

    {
    dd::AutoLock autolock(_snd_file_index_lock);
    snd_file_index++;

    {
    dd::AutoLock autolock(_idx_file_map_lock);
    _idx2file[snd_file_index] = fileName_withoutPath;
    _file2idx[fileName_withoutPath] = snd_file_index;
    }

    }

    bool canTrans;
    int onlineTaskNum;
    bool isstart;

    if(_enable_1by1trans == false)
    {
        canTrans = true;
    }else if(_enable_1by1trans == true)
    {
    dd::AutoLock autolock(_canTransLock);
    canTrans =  _canTransferNextFile;
    }

    {
    dd::AutoLock autolock(this->_onlineTaskLock);
    onlineTaskNum = this->online_task_num;
    }

    {
    dd::AutoLock autolock(this->_startLock);
    isstart = _start ;
    }

    if(isstart==false || canTrans== false || onlineTaskNum == MAX_ONLINE_TASK)
    {
        //usleep(5*1000);
        //this->eloop_thrdpool.nextLoop()->queueInLoop(
        //[ this, tid, servicename, filename, blocksize, onceReadNum]()
        //{
        //    this->CreateTransferFileTask(tid, servicename, filename, 
        //                                 blocksize, onceReadNum);
        //});
        return false;
                          
    }else if(isstart==true && canTrans==true && onlineTaskNum < MAX_ONLINE_TASK){
        {
        dd::AutoLock autolock(this->_onlineTaskLock);
        this->online_task_num++;
        }
        this->_canTransferNextFile = false;

        //添加对应文件的写文件任务
        hv::EventLoopPtr loop = eloop_thrdpool.nextLoop();
        loop->queueInLoop([this, tid, servicename, filename, blocksize, onceReadNum]()
        {
                this->TransferFile(tid, servicename, filename, blocksize, onceReadNum) ;
                usleep(5*1000);
        });
       return true;
    }
        
}

void 
FileSender::TransferFile( int tid, 
                          std::string servicename, 
                          std::string filename, 
                          int blocksize,
                          int onceReadNum
                          )
{

  hh::FileBlockMsg fileblockmsg;
  fileblockmsg.set_server_type(servicename);

  ReadFileBlock readfileblock(filename, onceReadNum, blocksize);

  unsigned long beginTime = GetTimeStampNow_us();
  unsigned long rdsn_size = 0;//byte num
                              
  fslogger.Write("TransferFile: %s\n", filename.c_str());

  while(readfileblock.readNextBlock( fileblockmsg))
  {
      fileblockmsg.set_stid(_src_tid);
      fileblockmsg.set_dtid(tid);

      //序列化消息到二进制字符串
      int msg_size = fileblockmsg.ByteSizeLong(); 
      std::shared_ptr<BYTE> pBuffer(new BYTE[msg_size]);
      if(fileblockmsg.SerializeToArray(pBuffer.get(), msg_size)==false)
      {
        printf("ma序列化失败!\n");
      }



      while(_start == false){
          usleep(100*1000);
      }


      //没发送成功，可能是双端还未连接好
      while( SendDataBytesToTermByMRUDP(tid, pBuffer, msg_size) == false )
      {
          printf("发送失败，1ms后重发...\n");
          fslogger.Write("发送失败，1ms后重发...\n");
          //不能太短，当发送失败后，要等待适当时间，
          //因为底层在MRUDP传输,这里sleep太短，
          //持续循环挤占底层MRUDP的CPU，导致MRUDP速率低
          usleep(500*1000);
      };

      rdsn_size += msg_size;

      //usleep(1);
  }

  unsigned long endTime = GetTimeStampNow_us();

  unsigned long passTime_ms = (endTime - beginTime ) / 1000;

  fslogger.Write("Finish read and mrudpSend, use time ms:%d.\n", 
                                                            passTime_ms);
  printf("Finish read and mrudpSend, use time ms:%d.\n",
                                                            passTime_ms);

  unsigned long sum_MB = rdsn_size / 1000 / 1000 ;
  unsigned long sum_Mb = sum_MB*8 ;

  fslogger.Write("-------total size Mb:%d, MB:%d.\n"
                           "-------total rate Mbps:%lf\n",
                           sum_Mb, sum_MB, 1.0*sum_Mb / (1.0*passTime_ms/1000.0));
  printf("--------total size Mb:%d, MB:%d.\n" 
         "-------total rate Mbps:%lf\n",
         sum_Mb, sum_MB, 1.0*sum_Mb / (1.0*passTime_ms/1.0*1000));

}

}// namespace FileSender

#endif // _FILE_SENDER_H__
