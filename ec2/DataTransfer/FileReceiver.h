#ifndef _FILE_RECEIVER_H__
#define _FILE_RECEIVER_H__

#include <iostream>
#include <thread>
#include <functional>
#include <filesystem>
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

#include <hv/hv.h>
#include <hv/EventLoopThreadPool.h>


#include "./GlobalTimer.h"

#include "infoHub/infoHub.h"
#include "infoHub/rateStatistic.h"
#include "infoHub/rateStatisticTable.h"
#include "../infoHub/progressStatistic.h"
#include "../infoHub/progressStatisticTable.h"

#include "../Util/SystemTimeFunc.h"


namespace ec2
{

#define MAX_RECV_QUEUE_SIZE 100

std::shared_ptr<ec2::infoHub>\
        _fileReceiver_infoHub_instance =\
            ec2::infoHub::getInstance();

//接收文件上数据的速率
ec2::rateStatistic  recv_rate_stat;

//写文件数据的速率
ec2::rateStatistic  write_rate_stat;


//每个文件对应一个索引号
dd::Lock recv_file_index_lock;
int recv_file_index=0;
//接收文件数据的速率
ec2::rateStatisticTable recv_rate_sttable;
//写文件
ec2::rateStatisticTable write_rate_sttable;


typedef std::queue<hh::FileBlockMsg, std::list<hh::FileBlockMsg>> FileBlockQueue;


CLogFile frlogger;

void FileReceiverMrudpCallback(
              DWORD dTermId, 
              const std::shared_ptr<BYTE> &recv_data, 
              DWORD dRecvDataByteLength,
              void *handle);

class TaskHandle
{
public:
    TaskHandle(){
    }

    TaskHandle(const TaskHandle& handle){
        _writed_cnt   = handle._writed_cnt;
        _writer       = handle._writer;
        _file_name    = handle._file_name;
        _task_name    = handle._task_name;

    }
    uint32_t _writed_cnt = 0;
    WriteFileBlock _writer ;

    std::string _file_name ;
    std::string _task_name ;
};

class FileReceiver
{

public:
    FileReceiver(int local_tid, int nthreads){
        _local_tid = local_tid;
        _nthreads = nthreads;
    }
    ~FileReceiver(){}

    void start(std::string recv_path="../../FileRecv/", 
               std::string server_name="FILE" );
    void end();
    
public:

    // write file thread function 定时器任务
    // 按照一定的时间间隔写文件
    void writefile_task();

public:
    //事件循环线程池
    hv::EventLoopThreadPool             _eventPool;
    std::atomic<hv::TimerID> _nextTimerID;

    std::string                         _recv_path; 
    std::string                         _server_name; 
    int                                 _nthreads;

    //记录该文件是否已经存在接收
    dd::Lock                            _file_recved_lock;
    std::map<std::string, bool>         _file_recved;
    //各个文件任务的写文件Handle以及相应的写文件信息
    dd::Lock                            _handle_lock;
    std::map<std::string, TaskHandle>   _handle_map;

    /* 文件任务标识符：task_name
     * 对应的文件名字查询map：_file_names 
     * 对应的互斥锁map：_queue_locks
     * 对应的fileblock队列map：_queue_locks
    */
    dd::Lock                            _queue_lock;
    FileBlockQueue                      _msgqueue;
    
    //tbox::ObjectPool<hh::FileBlockMsg>  _objpool;
    //

    /* 文件任务和文件名的相互映射
     * task_name -> file_name map
     * file_name -> task_name map
    */
    dd::Lock _file_task_map_lock;
    std::map<std::string, std::string> _file_names;
    std::map<std::string, std::string> _task_names;

    dd::Lock _idx_file_map_lock;;
    std::map<int, std::string> _idx2file;
    std::map<std::string, int> _file2idx;


    int _local_tid;

    dd::Lock    _status_lock;
    std::string _status = "START";// START, STOP
};

//---------------Function Definition-----------------------
    
void FileReceiver::start(std::string recv_path, 
                         std::string server_name){

    _status = "START";// start, stop
                        //
    _recv_path = recv_path; 
    _server_name = server_name; 

    //创建线程池 :TODO 基于这个线程池，创建定时器任务
    _eventPool.setThreadNum(_nthreads);
    _eventPool.start(true);

    for(int i = 0; i < _eventPool.threadNum(); i++){
        hv::TimerID timerID = ++_nextTimerID;
        hv::EventLoopPtr loop = _eventPool.nextLoop();
        loop->setTimerInLoop(1,  [this](hv::TimerID timerID) { 
                                    this->writefile_task();
                                 }, INFINITE, timerID);
    }
    
    //回调里，收到fileblock，存到列表，并创建一个对应的文件任务Handle。
    if(register_mrudp_dataserver_callback(_server_name, FileReceiverMrudpCallback, this)==false) 
    {
        printf("<FileReceiver>接收方的mrudp数据服务回调-注册失败！\n"); 
    }


    //打开文件日志（用于DEBUG）
    if(frlogger.Open("../log/FileReceiver.log", "a+")==false)
    {
        printf("<FileReceiver>打开日志文件出错！\n");
        exit(0);
    }
    recv_rate_stat.init("FileReceiver", "recv_rate_stat");
    write_rate_stat.init("FileReceiver", "write_rate_stat");

    recv_rate_sttable.init("FileReceiver", "recv_rate_sttable");
    write_rate_sttable.init("FileReceiver", "write_rate_sttable");;
}

void FileReceiver::end(){
    //线程池释放线程资源
    _eventPool.join();
}

//recv mrudp callback function
void FileReceiverMrudpCallback(
              DWORD dTermId, 
              const std::shared_ptr<BYTE> &recv_data, 
              DWORD dRecvDataByteLength,
              void *handle
              )
{
    //获取对应的FileReceiver调用对象
    FileReceiver* preceiver = static_cast<FileReceiver*>(handle);

    //TODO:这个东西可以通过对象池技术优化！
    hh::FileBlockMsg filemsg;
    if(filemsg.ParseFromArray(recv_data.get(), dRecvDataByteLength)==false) {
      printf("<FileReceiver>FileReceiverMrudpCallback::filemsg.ParseFromArray failed\n");
    }else{
      printf("<FileReceiver>FileReceiverMrudpCallback::filemsg.ParseFromArray succeed!\n");
    }

    //获取文件任务名：TODO:之后可以转换成对应的哈希值（或按照现在的也行）
    std::string task_name = filemsg.filename_();
    std::string fileName_withoutPath = GetFileNameFromPathStr(filemsg.filename_());
    {
        dd::AutoLock autolock(preceiver->_file_recved_lock);
        if( preceiver->_file_recved.find(task_name) == preceiver->_file_recved.end() ||\
        /*判断是否已经创建过该TaskHandle*/     preceiver->_file_recved[task_name]==false )
        {
            preceiver->_file_recved[task_name] = true;

            printf("<FileReceiver>Create writefile_task with task_name:%s\n", task_name.c_str());
            frlogger.Write("<FileReceiver>Create TaskHandle  with task_name:%s\n", task_name.c_str());

            //从filemsg获取文件全名（含路径），记录task_name和对应的文件名（不含路径）的映射;
            {
            dd::AutoLock autolock(preceiver->_file_task_map_lock);
            preceiver->_file_names[task_name] = fileName_withoutPath;
            preceiver->_task_names[fileName_withoutPath] = task_name;
            }

            {
                //创建task_name对应的TaskHandle，
                //并初始化任务名/文件名/已写数/和WriteFileBlock
                dd::AutoLock autolock2(preceiver->_handle_lock);
                preceiver->_handle_map[task_name]._task_name = task_name;
                preceiver->_handle_map[task_name]._file_name = preceiver-> _file_names[task_name];
                preceiver->_handle_map[task_name]._writed_cnt = 0;
                preceiver->_handle_map[task_name]._writer.open( 
                                    preceiver->_recv_path+"/"+preceiver->_file_names[task_name],
                                                        filemsg.filesize_total_(), 1,
                                                        filemsg.fileblocksize_());
            }
            recv_rate_stat.begin();

            {
            dd::AutoLock autulock(recv_file_index_lock);
            recv_file_index++;
            {
            dd::AutoLock autolock(preceiver->_idx_file_map_lock);
            preceiver->_idx2file[recv_file_index] = fileName_withoutPath;
            preceiver->_file2idx[fileName_withoutPath] = recv_file_index;
            }

            _fileReceiver_infoHub_instance->table_set( "FileReceiver", 
                                                       "FileNameTable",
                                                       recv_file_index,
                                                       fileName_withoutPath);
            recv_rate_sttable.begin(recv_file_index);
            }
            

        }
    }

    recv_rate_stat.pass(filemsg.ByteSizeLong());
    {
    dd::AutoLock autolock(preceiver->_idx_file_map_lock);
    recv_rate_sttable.pass(preceiver->_file2idx[fileName_withoutPath], 
                           filemsg.ByteSizeLong());
    }

    {
        //将回调中收到的数据存到队列中：PUSH->_msgqueue
        dd::AutoLock autolock(preceiver->_queue_lock);
        preceiver->_msgqueue.push(std::move(filemsg));
    }

    //使调用对象指针->空
    preceiver = nullptr;
}

void FileReceiver::writefile_task()
{

    dd::AutoLock autolock(_queue_lock);
    if(_msgqueue.size()<=0) return ;

    //获取队列头的文件块数据 //注意： front返回的是一个引用
    hh::FileBlockMsg blockmsg = _msgqueue.front();


    {
    dd::AutoLock    autolock(_status_lock);
    if(_status=="START" && _msgqueue.size()>MAX_RECV_QUEUE_SIZE){
        _status = "STOP";
        int stid = blockmsg.stid();
        hh::RecvFinishMsg fnmsg;
        fnmsg.set_server_type("FILE_RECV_FINISH_MSG");
        fnmsg.set_stid(stid);//文件从这个tid发来
        fnmsg.set_dtid(_local_tid);//文件发到这个tid
        fnmsg.set_strmsg("FILE_STOP_TRANS");
        int fnmsg_size = fnmsg.ByteSizeLong();
        std::shared_ptr<BYTE> pFnBuffer(new BYTE[fnmsg_size]);
        if(fnmsg.SerializeToArray(pFnBuffer.get(), fnmsg_size)==false)
        {
            frlogger.Write("fnmsg序列化失败!\n");

        }
        frlogger.Write("___send STOP info[]\n");
        printf("___send STOP info[]\n");

        while(SendDataBytesToTermByMRUDP(stid, pFnBuffer, fnmsg_size)==false)
        {
            frlogger.Write("send STOP info to source tid wrong, wait for retry\n");
            printf("send finish STOP to source tid wrong, wait for retry\n");
            usleep(1000);
        }
    }
    if(_status=="STOP" && _msgqueue.size()<=MAX_RECV_QUEUE_SIZE/4){
        _status = "START";
        int stid = blockmsg.stid();
        hh::RecvFinishMsg fnmsg;
        fnmsg.set_server_type("FILE_RECV_FINISH_MSG");
        fnmsg.set_stid(stid);//文件从这个tid发来
        fnmsg.set_dtid(_local_tid);//文件发到这个tid
        fnmsg.set_strmsg("FILE_START_TRANS");
        int fnmsg_size = fnmsg.ByteSizeLong();
        std::shared_ptr<BYTE> pFnBuffer(new BYTE[fnmsg_size]);
        if(fnmsg.SerializeToArray(pFnBuffer.get(), fnmsg_size)==false)
        {
            frlogger.Write("fnmsg序列化失败!\n");

        }
        frlogger.Write("___send START info[]\n");
        printf("___send START info[]\n");

        while(SendDataBytesToTermByMRUDP(stid, pFnBuffer, fnmsg_size)==false)
        {
            frlogger.Write("send START info to source tid wrong, wait for retry\n");
            printf("send START info to source tid wrong, wait for retry\n");
            usleep(1000);
        }

    }
    }// status_lock

            

            

    //获取文件名，对应的任务名，对应的写文件Handle(包含该任务的状态)
    std::string taskName ;
    std::string fileName = GetFileNameFromPathStr(blockmsg.filename_());

    {
    dd::AutoLock autolock(_file_task_map_lock);
    taskName = _task_names[fileName];
    }

    write_rate_stat.begin();
    {
    dd::AutoLock autolock(_idx_file_map_lock);
    write_rate_sttable.begin(_file2idx[fileName]);
    }

    {
        dd::AutoLock autolock2(_handle_lock);
        if( _handle_map.find(taskName) == _handle_map.end() )
        {
            printf("::WriteFileFunc:: not find _handle_map[%s]\n", taskName.c_str());
            frlogger.Write("::WriteFileFunc:: not find _handle_map[%s]\n",
                                                                    taskName.c_str());
        } else {
            _handle_map[taskName]._writer.writeNextBlock(blockmsg);
            _handle_map[taskName]._writed_cnt++;

            //printf("<FileReceiver>write a file block [%d][total block: %ld].\n\n", 
            //                    _handle_map[taskName]._writed_cnt, blockmsg.fileblocknumber_total_());
            //frlogger.Write("<FileReceiver>write a file block [%d][total block: %ld].\n\n", 
            //                    _handle_map[taskName]._writed_cnt, blockmsg.fileblocknumber_total_());
            
            write_rate_stat.pass(blockmsg.ByteSizeLong());
            {
            dd::AutoLock autolock(_idx_file_map_lock);
            write_rate_sttable.pass(_file2idx[fileName], blockmsg.ByteSizeLong());
            }

            //某个文件接收结束的处理
            if(_handle_map[taskName]._writed_cnt==blockmsg.fileblocknumber_total_())
            {
                _handle_map[taskName]._writer.flushWriteFileBlock();
                _handle_map[taskName]._writer.finishWriteFileBlock();

                printf("________________________________________\n" );
                frlogger.Write("________________________________________\n" );

                // byte / ms
                double rateBpms = _fileReceiver_infoHub_instance->value_get<double>\
                                  ( "FileReceiver", "recv_rate_stat");
                double rateMbps = (rateBpms*1000*8)/1000/1000;
                printf("<FileReceiver>finish write file[%s] ---> recv rate: %lf Mbps\n", 
                       fileName.c_str(), rateMbps);
                frlogger.Write("<FileReceiver>finish write file[%s] ---> recv rate: %lf Mbps\n", 
                       fileName.c_str(), rateMbps);

                double write_rateBpms = _fileReceiver_infoHub_instance->value_get<double>\
                                        ( "FileReceiver", "write_rate_stat");
                double write_rateMbps = (write_rateBpms*1000*8)/1000/1000;
                printf("<FileReceiver>finish write file[%s] ---> write rate: %lf Mbps\n", 
                       fileName.c_str(), write_rateMbps);
                frlogger.Write("<FileReceiver>finish write file[%s] ---> write rate: %lf Mbps\n", 
                       fileName.c_str(), write_rateMbps);



                int stid = blockmsg.stid();
                hh::RecvFinishMsg fnmsg;
                fnmsg.set_server_type("FILE_RECV_FINISH_MSG");
                fnmsg.set_stid(stid);//文件从这个tid发来
                fnmsg.set_dtid(_local_tid);//文件发到这个tid
                fnmsg.set_filename(fileName);//文件发到这个tid
                fnmsg.set_strmsg("FILE_RECV_RATE");
                fnmsg.set_dblmsg(rateMbps);
                int fnmsg_size = fnmsg.ByteSizeLong();
                std::shared_ptr<BYTE> pFnBuffer(new BYTE[fnmsg_size]);
                if(fnmsg.SerializeToArray(pFnBuffer.get(), fnmsg_size)==false)
                {
                    frlogger.Write("fnmsg序列化失败!\n");

                }

                frlogger.Write("___send finish info[%s] to src\n", fileName.c_str());
                printf("___send finish info to[%s] src\n", fileName.c_str());

                while(SendDataBytesToTermByMRUDP(stid, pFnBuffer, fnmsg_size)==false)
                {
                    frlogger.Write("send finish info to source tid wrong, wait for retry\n");
                    printf("send finish info to source tid wrong, wait for retry\n");
                    usleep(1000);
                }
                


                printf("---------------------------------------\n" );
                printf("[Show Each File RecvTask StatisticData]:\n" );
                frlogger.Write("---------------------------------------\n" );
                frlogger.Write("[Show Each File RecvTask StatisticData]:\n" );

                int current_file_cnt = 0;
                {
                dd::AutoLock autulock(recv_file_index_lock);
                current_file_cnt = recv_file_index;
                }
                for(int i = 1; i <= current_file_cnt; i++)
                {
                    std::string rcvFileName = \
                        _fileReceiver_infoHub_instance->table_get<std::string>\
                        ( "FileReceiver", "FileNameTable", i);
                    printf("File Name : %s\n", rcvFileName.c_str()); 
                    frlogger.Write("File Name : %s\n", rcvFileName.c_str()); 

                    double rcvFileRateBpms\
                        = _fileReceiver_infoHub_instance->table_get<double>\
                        ("FileReceiver", "recv_rate_sttable", i);
                    double rcvFileRateMbps = (rcvFileRateBpms*1000*8)/1000/1000;
                    printf(">>> recv rate: %lf Mbps\n", rcvFileRateMbps);
                    frlogger.Write(">>> recv rate: %lf Mbps\n", rcvFileRateMbps);

                    double wrtFileRateBpms \
                        = _fileReceiver_infoHub_instance->table_get<double>\
                        ("FileReceiver", "write_rate_sttable", i);
                    double wrtFileRateMbps = (wrtFileRateBpms*1000*8)/1000/1000;
                    printf(">>> write rate: %lf Mbps\n", 
                             wrtFileRateMbps);
                    frlogger.Write(">>> write rate: %lf Mbps\n", wrtFileRateMbps);

                }
                printf("===========================================\n" );
                frlogger.Write("===========================================\n" );

                
            }//end if

        }//end else
    }//end _handle_lock
    //--------------------------------
    _msgqueue.pop();
    return ;
    //end _queue_lock
}

}// namespace ec2::FileReceiver

#endif //_FILE_RECEIVER_H__
