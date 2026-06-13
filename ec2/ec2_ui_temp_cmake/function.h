#ifndef FUNCTION_H
#define FUNCTION_H

#include <string>
#include <iostream>
 #include "../GlobalMessage.h"
#include "../epollComm/EpollCommApi.h"
#include "SerializeUtil.h"
#include "../epollComm/CIPSOCKET.h"
#include "../RBUDP/RBUDPApi.h"
#include "../MRUDP/MRUDPApi.h"
#include "../Util/LockGuard.h"
#include "../NetCombTransfer/NetCombTransferApi.h"
#include "../Util/DataUpdater.h"
#include "../BigDataTransfer/BigDataTransferApi.h"
#include "../NetCombTransfer/CNetTerminalManager.h"
#include "../SpeedControl/SpeedControlManager.h"
#include "../Term2TermQos/TermToTermQosApi.h"
#include "time.h"
#include "../public/_public.h"
//#include "../DataTransfer/FileSender.h"
//#include "../DataTransfer/FileReceiver.h"
//#include "public/utility.h"
//#include "public/_public.h"
//#include "../DataTransfer/fileblock/FileBlock.h"
//#include "fileblock/ReadFileBlock.h"
//#include "../DataTransfer/fileblock/WriteFileBlock.h"
//#include "../DataTransfer/fileblock/msg_cpp/FileBlockMsg.pb.h"


 // 结构体 st_arg 的声明
 struct st_arg;  // 前向声明

typedef struct TemplateForCreateServer {
 bool _bRunning;
 unsigned int _serverPort;
 unsigned int _channelNum;
 unsigned int _channelHead;
 std::string _serverAddress;
} ServerTemplate;

typedef struct TemplateForCreateClient {
 int _bRunning;
 unsigned int _channelNum;
 unsigned int _localPort;
 std::string _localAddress;
 unsigned int _remotePort;
 std::string _remoteAddress;
 unsigned int _TID;
} ClientTemplate;

typedef struct TemplateForTransferTask {
 unsigned int _dTermID;
 unsigned int _taskID;
 std::string _fileName;
 std::string _fileAddress;
} TransferTaskTemplate;

typedef struct SendTaskState {
 unsigned int _TID;
 std::string _taskName;
 std::string _taskFilePath;
 unsigned int _taskID;
 unsigned int _process;
 float _transferSpeed;
 unsigned int _transferTime;
 unsigned int _taskState;
 quint64 _fileSize = 0;
 vector<map<unsigned int ,float>> _historySpeed;
} TaskSendState;

typedef struct RecvTaskState {
 unsigned int _TID;
 std::string _taskName;
 std::string _taskFilePath;
 unsigned int _taskID;
 unsigned int _process;
 float _transferSpeed;
 unsigned int _transferTime;
 unsigned int _taskState;
 quint64 _fileSize = 0;
 float _loseRate;    //丢包率
 vector<map<unsigned int ,float>> _historySpeed;
} TaskRecvState;

typedef struct ChannelInfo{
 unsigned int port;
 std::string address;
} ChannelTemplate;


extern pthread_mutex_t _sendTask_Mutex;  //_sendTaskState的锁
extern pthread_mutex_t _recvTask_Mutex;  //_recvTaskState的锁
extern pthread_mutex_t _terminal_Mutex;  //_clientTemplate的锁
extern std::vector<std::string>IPList;
extern std::vector<std::shared_ptr<ChannelTemplate>> _channelTemplate;//QT的UI里面没有使用，是命令行UI使用的
extern std::vector<std::shared_ptr<ServerTemplate>> _serverTemplate; //QT的UI里面没有使用，是命令行UI使用的
extern std::vector<std::shared_ptr<ClientTemplate>> _clientTemplate; //成功连接的终端列表
extern std::vector<std::shared_ptr<TransferTaskTemplate>> _transferTaskTemplate;//QT的UI里面没有使用，是命令行UI使用的
extern std::vector<std::shared_ptr<SendTaskState>> _sendTaskState;  //发送任务列表
extern std::vector<std::shared_ptr<RecvTaskState>> _recvTaskState;  //接收任务列表
extern std::shared_ptr<BigDataTransfer::BigDataTransferManager> gBigDataTransferManager;

//定义DateUpdater的对象，用来发送数据更新信号
extern DataUpdater *dataUpdater;
extern pthread_mutex_t _Mutex;  //NOTE:没用

extern int gUiRole;


//系统启动
void System_start(const char *config_path, bool QosOpen = true, bool IsRBUDP = false) ;

//系统关闭
void System_shutdown(bool QosOpen = true) ;

DWORD UpdateSendTaskProgressState(const std::shared_ptr<FileTaskSendProgressInfo> &pInfo);

// void UpdateSendTaskStatusState(const std::shared_ptr<FileTaskSendStatusInfo> &pInfo);
DWORD UpdateSendTaskStatusState(const std::shared_ptr<FileTaskSendStatusInfo> &pInfo);
DWORD UpdateRecvTaskProgressState(const std::shared_ptr<FileTaskRecvProgressInfo> &pInfo);

void UpdateRecvTaskStatusState(const std::shared_ptr<FileTaskRecvStatusInfo> &pInfo);

void UpdateRecvTaskStatusState(const std::shared_ptr<FileTaskRecvStatusInfo> &pInfo);

void SendTaskProgressCallBack(const std::shared_ptr<FileTaskSendProgressInfo> &pInfo);

void SendTaskStatusCallBack(const std::shared_ptr<FileTaskSendStatusInfo> &pInfo);

void RecvTaskProgressCallBack(const std::shared_ptr<FileTaskRecvProgressInfo> &pInfo);

void RecvTaskStatusCallBack(const std::shared_ptr<FileTaskRecvStatusInfo> &pInfo);

void TerminalListCallBack(std::vector<Terminal> terminalList);

////自送开启发送后，将发送任务添加到发送任务列表中
//void AutoSendFileStatusCallBack(unsigned int TID,
//                                unsigned int taskID,
//                                const string& filePath,
//                                const string& fileName,
//                                unsigned int taskState);


//通道类型枚举：与 EpollComm channelHead 解耦
#ifndef CONN_MODE_ENUM_DEFINED
#define CONN_MODE_ENUM_DEFINED
enum class ConnMode {
    MeshOnly  = 0,  // 仅自组网
    DTUOnly   = 1,  // 仅5G DTU
    DualPath  = 2   // 双链路带宽叠加
};
#endif

//創建服務器（增加 channelHead 参数,默认 3020 保持兼容）
bool createServer(std::string localAddr, int localPort, bool isDTU,
                  DWORD channelHead = 3020, DWORD channelNumber = 100);
//創建客戶端（增加 channel 参数,默认 3200 保持兼容）
bool createClient(std::string localAddr, int localPort,
                  std::string remoteAddr, unsigned int remotePort, bool isDTU,
                  DWORD channel = 3200);

//创建双路径服务器（自组网 + DTU 同时监听）
bool createDualServer(std::string meshAddr, int meshPort,
                      std::string dtuAddr, int dtuPort);
//创建双路径客户端（自组网 + DTU 同时连接）
bool createDualClient(std::string meshAddr, int meshPort,
                      std::string meshRemoteAddr, int meshRemotePort,
                      std::string dtuAddr, int dtuPort,
                      std::string dtuRemoteAddr, int dtuRemotePort);
// bool createServer(st_arg starg);

//创建发送任务
bool createTransferSendTask(unsigned int TID,std::string fileName,std::string filePath);

bool createTransferSendTask(unsigned int TID,std::string fileName,std::string filePath,bool is_RBUDP);

//执行自动发送任务

bool startAutoSend(unsigned int TID,bool role);
bool SendStartAutoSendMessage_receiver(DWORD remote_tid = 0);  //发送信令，让对端开始自动发送
bool SendStartAutoSendMessage_sender(DWORD remote_tid = 0);

//MRUDP数据服务a回调函数
void mrudp_dataserver_startAutoSend_callback(
        DWORD dTermId,
        const std::shared_ptr<BYTE> &recv_data,
        DWORD dRecvDataByteLength);

void mrudp_dataserver_startVideoTrans_callback(
        DWORD dTermId,
        const std::shared_ptr<BYTE> &recv_data,
        DWORD dRecvDataByteLength);


 //发送请求视频传输信令
 bool askStartVideoTrans(unsigned int remote_tid,unsigned int remote_videoPort);
 bool AskStartVideoTrans(DWORD remote_tid);


//获取TID
unsigned int getTID();

//取消传输任务
bool transferTaskCancel(DWORD task_id);

//终端执行指令，获取结果
//这里主要用于获取本机IP，指令为“hostname -I”
std::string exec_command(const char* cmd);

//从配置文件中读取相关信息
std::string getStringFromIni(std::string section,std::string key,std::string defaultValue);
int getIntFromIni(std::string section,std::string key,int defaultValue);
 bool getBoolFromIni(std::string section,std::string key,int defaultValue);

//截取文件路径中的文件名
std::string extractFileName(const std::string& filePath);

vector<string> splitString(const string& s,const string& delim);  //分割字符串


#endif // FUNCTION_H










