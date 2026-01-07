#ifndef __BULKDATATRANSFERAPI_H__
#define __BULKDATATRANSFERAPI_H__

#include <hv/hv.h>
#include <hv/EventLoopThreadPool.h>

#include "BigDataTransfer/BigDataTransferApi.h"
#include "public/_public.h"
#include "../DataTransfer/FileSender.h"



CLogFile bdlogger;

class BulkDataTransfer{

public:
    BulkDataTransfer(){
    }

    ~BulkDataTransfer(){
        _loop_thread.join();
    }

    //-----------对外调用接口---------------------------
    void Init(  DWORD localTID, 
                int remotetid, 
                std::string  servicename,
                int nthreads = 4,  
                bool is1by1trans = true,
                std::string localpath = "../../FileSend", 
    		    std::string matchname = "SURF_*.TXT, *.DAT, *.PNG, *.JPEG", 
    		    std::string okfilename = "./ecbulktrans_rsdata.xml", 
    		    int timetvl = 30,
                int blocksize = 5000) ;

    //发送方后台运行批量传输程序
    void Run(int timetvl); 
    
    //发送方后台异步运行批量传输程序
    void AsynRun(int timetvl) ; 


private:
    
    //本程序的业务流程主函数
    bool _ectransfiles();
    
    
    //-----------内部调用函数接口---------------------------
    
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
    
    //把发送成功的文件记录追加到okfilename文件中
    bool AppendToOKFileName(struct st_fileinfo *stfileinfo);


private:
    //批量文件传输参数
    bool        _is_start;
    std::string _localpath;
    std::string _matchname;
    std::string _okfilename;
    int         _timetvl;
    //FileSender_参数
    int         _remotetid; 
    std::string _servicename; 
    int         _blocksize;

    int _localTID        ;                           
    int _nthreads    ;                        

    
    vector<struct st_fileinfo> vlistfile, vlistfile1;
    vector<struct st_fileinfo> vokfilename, vokfilename1;

    std::atomic<hv::TimerID> _nextTimerID;
    hv::EventLoopThreadPool _eloop_thrdpool;

    hv::EventLoopThread _loop_thread;
    //const EventLoopPtr& loop = _loop_thread.loop();
    
    //ec2::FileSender _filesender(TermId, 4);
    ec2::FileSender _filesender;

};



void BulkDataTransfer::Init(  DWORD localTID, 
                                int remotetid, 
                                std::string  servicename,
                                int nthreads ,  
                                bool is1by1trans,
                                std::string localpath, 
    		                    std::string matchname, 
    		                    std::string okfilename, 
    		                    int timetvl ,
                                int blocksize) 
{
    _is_start = false;
    _localpath = localpath;
    _matchname = matchname;
    _okfilename 	 = okfilename;
    _timetvl 		 = timetvl;

    _localTID        = localTID ;                           
    _remotetid       = remotetid ;                           
    _servicename     = servicename;                           
    _nthreads        = nthreads    ;                        
    _blocksize       = blocksize    ;                       


    _filesender.init(localTID, nthreads);
    _filesender. enableTrans1By1(is1by1trans);

    //_eloop_thrdpool.setThreadNum(nthreads);
    //_eloop_thrdpool.start(true);

    _loop_thread.start(true);

    if(bdlogger.Open("../log/BulkSender.log", "a+")==false)
	{ 
        printf("<BulkSender>打开日志文件出错！\n");
        exit(0);
	}
} 


//发送方后台运行批量传输程序
void BulkDataTransfer::Run(int timetvl) 
{
    _timetvl = timetvl;
	while(true)
	{
      bdlogger.Write("BulkDataTransfer::Run::进行一次批量文件传输\n"); ;
	  _ectransfiles();
      if( _filesender.canTransferNextFile() ==false){
	     sleep(_timetvl);
      }
	  sleep(_timetvl);
	}
}

//发送方后台异步运行批量传输程序
void BulkDataTransfer::AsynRun(int timetvl) 
{
	_timetvl = timetvl;

    bdlogger.Write("BulkDataTransfer::AsynRun::\n"); ;
    //hv::EventLoopPtr loop = _eloop_thrdpool.nextLoop();
    //_loop_thread.loop()->queueInLoop([this, timetvl]()
    _loop_thread.loop()->setInterval(timetvl*1000 /*1s*/, [this](hv::TimerID timerID)
    {
         bdlogger.Write("BulkDataTransfer::AsynRun::进行一次批量文件传输\n"); ;
         this->_ectransfiles();

        // sleep(timetvl);

        //_loop_thread.loop()->queueInLoop([this, timetvl]()
        //{
        //    this->AsynRun(timetvl) ;

        //});

         //if( this->_filesender.canTransferNextFile() ==false){
	     //   sleep(this->_timetvl);
         //}
    });
}


// 本程序的业务流程主函数(发送方)
bool BulkDataTransfer::_ectransfiles()
{
  //须确保对端存在指定的接收目录
  
  if(LoadListFile()==false)
  {
    bdlogger.Write("LoadListFile() failed.\n"); return false;
  }

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

  //把客户端的新文件或已经改动后的文件发送给服务端
  for(int ii=0; ii<vlistfile.size(); ii++)
  {
      char strremotefilename[301],strlocalfilename[301];
      SNPRINTF(strlocalfilename,300,"%s/%s",_localpath.c_str(),vlistfile[ii].filename);

      bdlogger.Write("transfer %s ...\n",strlocalfilename);


      //if(Create_BigDataTransferTask(_remotetid, vlistfile[ii].filename, strlocalfilename, TaskID)==false)

      // 发送文件
      DWORD TaskID;
      while(_filesender.CreateTransferFileTask(_remotetid, 
      //filesender.TransferFile(_remotetid, 
                                      _servicename, 
                                      strlocalfilename,
                                      _blocksize,
                                      1)==false)
      {
        usleep(5*1000);
      }


      // 如果，把发送成功的文件记录追加到okfilename文件中
      AppendToOKFileName(&vlistfile[ii]);

  }
}

// 把localpath目录下的文件加载到vlistfile容器中
bool BulkDataTransfer::LoadListFile()
{
  vlistfile.clear();

  CDir Dir;

  // 不包括子目录
  // 注意，如果目录下的总文件数超过50000，增量发送文件功能将有问题
  if (Dir.OpenDir(_localpath.c_str(),_matchname.c_str(),50000,false,false)==false)
  {
    bdlogger.Write("Dir.OpenDir(%s) 失败。\n",_localpath.c_str()); return false;
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

    // bdlogger.Write("vlistfile filename=%s,mtime=%s\n",stfileinfo.filename,stfileinfo.mtime);
  }

  return true;
}

// 把okfilename文件内容加载到vokfilename容器中
bool BulkDataTransfer::LoadOKFileName()
{
  vokfilename.clear();

  CFile File;

  // 注意：如果程序是第一次发送，okfilename是不存在的，并不是错误，所以也返回true。
  if (File.Open(_okfilename.c_str(),"r") == false) return true;

  struct st_fileinfo stfileinfo;

  char strbuffer[301];

  while (true)
  {
    memset(&stfileinfo,0,sizeof(struct st_fileinfo));

    if (File.Fgets(strbuffer,300,true)==false) break;

    GetXMLBuffer(strbuffer,"filename",stfileinfo.filename,300);
    GetXMLBuffer(strbuffer,"mtime",stfileinfo.mtime,20);

    vokfilename.push_back(stfileinfo);

    // bdlogger.Write("vokfilename filename=%s,mtime=%s\n",stfileinfo.filename,stfileinfo.mtime);
  }

  return true;
}


// 把vlistfile容器中的文件与vokfilename容器中文件对比，得到两个容器
// 一、在vlistfile中存在，并已经发送成功的文件vokfilename1
// 二、在vlistfile中存在，新文件或需要重新发送的文件vlistfile1
bool BulkDataTransfer::CompVector()
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
    bdlogger.Write("vokfilename1 filename=%s,mtime=%s\n",vokfilename1[ii].filename,vokfilename1[ii].mtime);
  }

  for (int ii=0;ii<vlistfile1.size();ii++)
  {
    bdlogger.Write("vlistfile1 filename=%s,mtime=%s\n",vlistfile1[ii].filename,vlistfile1[ii].mtime);
  }
  */

  return true;
}

// 把vokfilename1容器中的内容先写入okfilename文件中，覆盖之前的旧okfilename文件
bool BulkDataTransfer::WriteToOKFileName()
{
  CFile File;

  // 注意，打开文件不要采用缓冲机制
  if (File.Open(_okfilename.c_str(),"w",false) == false)
  {
    bdlogger.Write("File.Open(%s) failed.\n",_okfilename.c_str()); return false;
  }
  
  for (int ii=0;ii<vokfilename1.size();ii++)
  {
    File.Fprintf("<filename>%s</filename><mtime>%s</mtime>\n",vokfilename1[ii].filename,vokfilename1[ii].mtime);
  }
  
  return true;
} 

// 如果，把发送成功的文件记录追加到okfilename文件中
bool BulkDataTransfer::AppendToOKFileName(struct st_fileinfo *stfileinfo)
{ 
  CFile File;

  // 注意，打开文件不要采用缓冲机制
  if (File.Open(_okfilename.c_str(),"a",false) == false)
  {
    bdlogger.Write("File.Open(%s) failed.\n",_okfilename.c_str()); return false;
  }
  
  File.Fprintf("<filename>%s</filename><mtime>%s</mtime>\n",stfileinfo->filename,stfileinfo->mtime);
  
  return true;
} 






#endif // __BULKDATATRANSFERAPI_H__
