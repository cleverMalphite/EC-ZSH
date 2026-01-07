#include "function.h"
#include "public/_public.h"
#include "msg_cpp/msg.pb.h"
#include "../infoHub/infoHub.h"
#include "../infoHub/infoHubRpcServer.h"
#include "../infoHub/infoHubRpcClient.h"
#include "../DataTransfer/FileReceiver.h"
#include "../DataTransfer/FileSender.h"

#include <QDebug>
#include <thread>
#include <iostream>
#include <unistd.h>
#include <condition_variable>
#include <atomic>

std::vector<std::string> IPList;  //记录所有创建过监听的IP地址
std::vector<std::shared_ptr<ServerTemplate>> _serverTemplate;
std::vector<std::shared_ptr<ClientTemplate>> _clientTemplate;
std::vector<std::shared_ptr<TransferTaskTemplate>> _transferTaskTemplate;
//TODO:后续需要将当前正在传输的任务和传输完成的任务分开存储，这样可以节省查询时间
std::vector<std::shared_ptr<SendTaskState>> _sendTaskState;
std::vector<std::shared_ptr<RecvTaskState>> _recvTaskState;
std::vector<std::shared_ptr<ChannelTemplate>> _channelTemplate;
std::shared_ptr<BigDataTransfer::BigDataTransferManager> gBigDataTransferManager;

int channelHead=3020;
int perChannelsNumer=100;
int channelNum=0;
int clientChannelNum=0;
int clientChannelHead=3200;

bool isrbudp;

//定义DateUpdater的对象，用来发送数据更新信号
DataUpdater *dataUpdater;

pthread_mutex_t _sendTask_Mutex;
pthread_mutex_t _recvTask_Mutex;
pthread_mutex_t _terminal_Mutex;

unsigned int TID;
std::string localAddr;
int localPort;
int autolocalPort=3030;

//vector<struct st_fileinfo> vlistfile, vlistfile1;
//vector<struct st_fileinfo> vokfilename, vokfilename1;
// 用于控制文件发送线程的启动
std::atomic<bool> startAutoSendFlag(false);
std::atomic<bool> startVideoTransFlag(false);
//批量文件传输参数
struct st_arg{
    char localpath[301]="../../FileSend/";
    char matchname[301]="SURF_*.txt,*.DAT";
    int ptype=1;
    char localpathbak[301];
    char okfilename[301]="../BulkDataTransfer/ecbulktrans_rsdata.xml";
    int timetvl=30;
} starg;

std::shared_ptr<ec2::infoHub> infohub_instance =\
		      ec2::infoHub::getInstance();

infoHubRpcServer sender_infohub_rpcserver;
infoHubRpcServer receiver_infohub_rpcserver;



void SendTaskProgressCallBack(const std::shared_ptr<FileTaskSendProgressInfo> &pInfo){
    printf("发送调用回调1.\n");
    _sendTaskState;
    DWORD task_id=UpdateSendTaskProgressState(pInfo);
    printf("task_id:%d\n",task_id);
    emit dataUpdater->updateSendProgressSignal(task_id);
    qDebug() << "updateSendProgressSignal emitted"; // 确保信号被发射

}

void SendTaskStatusCallBack(const std::shared_ptr<FileTaskSendStatusInfo> &pInfo){
    printf("发送调用回调2.\n");
    _sendTaskState;
    DWORD task_id=UpdateSendTaskStatusState(pInfo);
    if (!dataUpdater) {
//         qDebug() << "dataUpdater is nullptr!";
    } else{
        emit dataUpdater->updateSendStatusSignal();
    }
//    qDebug() << "SendTaskStatusCallBack ok";
}

void RecvTaskProgressCallBack(const std::shared_ptr<FileTaskRecvProgressInfo> &pInfo){
    printf("接收调用回调1.\n");
    DWORD task_id=UpdateRecvTaskProgressState(pInfo);
    // emit dataUpdater->updateRecvProgressSignal(task_id);
    if (!dataUpdater) {
        // qDebug() << "dataUpdater is nullptr!";
    } else {
        emit dataUpdater->updateRecvProgressSignal(task_id);
        // qDebug() << "Emitting updateSendProgressSignal with task_id:" << task_id;
    }
    qDebug() << "RecvTaskProgressCallBack ";
}

void RecvTaskStatusCallBack(const std::shared_ptr<FileTaskRecvStatusInfo> &pInfo){
    printf("接收调用回调2.\n");
    UpdateRecvTaskStatusState(pInfo);
    emit dataUpdater->updateRecvStatusSignal();
    qDebug() << "RecvTaskStatusCallBack ";
}

void TerminalListCallBack(std::vector<Terminal> terminalList){
    printf("开始执行回调函数。。。。\n");
    MutexLockGuard guard(_terminal_Mutex);
    _clientTemplate.clear();
    vector<Terminal>::iterator temp;
    for(temp=terminalList.begin();temp!=terminalList.end();temp++){
        std::shared_ptr<ClientTemplate> client=std::make_shared<ClientTemplate>();
        client->_TID=temp->TID;
        client->_channelNum=temp->channelNum;
        client->_localPort=temp->localPort;
        client->_localAddress=temp->localAddress;
        client->_remotePort=temp->remotePort;
        client->_remoteAddress=temp->remoteAddress;
        client->_bRunning=temp->state;
        _clientTemplate.push_back(client);
    }
    //发送终端列表数据更新信号
    emit dataUpdater->updateTerminalSignal();
}


//系统启动
void System_start(const char *config_path, bool QosOpen , bool IsRBUDP) {
    dataUpdater=new DataUpdater();
    InitIniHandler(config_path);
    printf("system start\n");
    DWORD TermId = GetIntegerKeyIni("Main", "DeviceID", 1);
    // std::cout << "TermId: " << TermId << std::endl; // 输出读取到的值
    TID=TermId;
    bool Is_RBUDP = isrbudp = IsRBUDP;
    // ... ...
    Init_NetCombTransfer(TermId);
    InitSpeedControl(TermId);
    // InitTermToTermQos(TermId);
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
    // SystemControlInit();
    printf("[SystemTest]::Terminal %d had inited\n", TermId);

    //初始化锁
    pthread_mutex_init(&_sendTask_Mutex, nullptr);
    pthread_mutex_init(&_recvTask_Mutex, nullptr);
    pthread_mutex_init(&_terminal_Mutex, nullptr);

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
    // ... ...
    UninitIniHandle();
    printf("[SystemTest]::Terminal %d had shut down\n", TermId);
    delete dataUpdater;
}

DWORD UpdateSendTaskProgressState(const std::shared_ptr<FileTaskSendProgressInfo> &pInfo) {
    MutexLockGuard gGuard(_sendTask_Mutex);
    for (auto &pTaskState: _sendTaskState) {
        // qDebug() << ("UpdateSendTaskProgressState!");
        // qDebug() << "_taskState: " << pTaskState->_taskState;
        // qDebug() << "_taskID: " << pTaskState->_taskID;
        // qDebug() << "_transferSpeed: " << pTaskState->_transferSpeed;
        // qDebug() << "_process: " << pTaskState->_process;
        if (pInfo->m_task_id == pTaskState->_taskID) {
            pTaskState->_transferSpeed=pInfo->m_current_rate;
            pTaskState->_process = pInfo->m_current_proc;

            //get rate from infohub
            double epoll_rate=infohub_instance->value_get<double>("epollcomm", "tx_rate_value");
            printf("infohub: epollcomm tx_rate=%lf\n", epoll_rate);
            printf("\n");


            return pInfo->m_task_id;

        }
    }
}

DWORD UpdateSendTaskStatusState(const std::shared_ptr<FileTaskSendStatusInfo> &pInfo) {
    MutexLockGuard gGuard(_sendTask_Mutex);
    for (auto &pTaskState: _sendTaskState) {
        if (pInfo->m_task_id == pTaskState->_taskID) {
            if(pInfo->m_time_second!=0&&pInfo->m_time_second!=NAN){
                pTaskState->_transferTime = pInfo->m_time_second;
            }
            //计算平均传输速率
            float averageSpeed=pInfo->m_file_size*8.0/pInfo->m_time_second;
//            pTaskState->_transferSpeed = pInfo->m_current_rate;
            pTaskState->_transferSpeed=averageSpeed;
            pTaskState->_taskState = pInfo->m_send_status;
            // qDebug() << "_taskState: " << pTaskState->_taskState; // 输出_taskState的值
            if (pTaskState->_taskState == 1){
                pTaskState->_process = 100;
                time_t t;
                time(&t);
                printf("%s",ctime(&t));
            }   //state==1表示发送成功
            return pInfo->m_task_id;
            // break;
        }
    }
}

DWORD UpdateRecvTaskProgressState(const std::shared_ptr<FileTaskRecvProgressInfo> &pInfo) {
    MutexLockGuard gGuard(_recvTask_Mutex);
    for (auto &pTaskState: _recvTaskState) {
        qDebug() << ("UpdateRecvTaskProgressState!");
        if (pInfo->m_task_id == pTaskState->_taskID) {
            pTaskState->_process = pInfo->m_current_proc;
//            pTaskState->_transferSpeed = pInfo->m_current_rate;
            double current_rate_bytems =  infohub_instance->value_get<double>("bigdatatransfer", "rx_current_speedkBps_stat");
            double current_rate_bps = current_rate_bytems*8;
            pTaskState->_transferSpeed = current_rate_bps;
            printf("UpdateRecvTaskProgressState:pInfo->m_current_rate=%lf\n", pInfo->m_current_rate);
            printf("UpdateRecvTaskProgressState:pInfo->current_rate_bps=%lf\n", pTaskState->_transferSpeed);
            //get rate from infohub
            printf("+++++++++++++++++++++++++++++++++++++++++++\n");
            printf("--------------- epollcomm infohub --------------\n");
            double epollcomm_tx_rate_stat = infohub_instance->value_get<double>("epollcomm", "tx_rate_stat");
            double epollcomm_rx_rate_stat = infohub_instance->value_get<double>("epollcomm", "rx_rate_stat");
            printf("<infohub_rpcclient> epollcomm tx_rate_value = %lf\n", epollcomm_tx_rate_stat);
            printf("<infohub_rpcclient> epollcomm rx_rate_value = %lf\n", epollcomm_rx_rate_stat);
            uint32_t epollcomm_qos_tx_rx_rtt_stat = infohub_instance->value_get<uint32_t>("epollcomm", "qos_tx_rx_rtt_stat");
            uint32_t epollcomm_qos_tx_rx_loss_stat = infohub_instance->value_get<uint32_t>("epollcomm", "qos_tx_rx_loss_stat");
            printf("<infohub_rpcclient> epollcomm qos_tx_rx_rtt_stat  = %u\n", epollcomm_qos_tx_rx_rtt_stat);
            printf("<infohub_rpcclient> epollcomm qos_tx_rx_loss_stat = %u\n", epollcomm_qos_tx_rx_loss_stat);

            printf("--------------- netcombtransfer infohub --------------\n");
            double nct_rx_rate_stat = infohub_instance->value_get<double>("netcombtransfer", "rx_rate_stat");
            double nct_tx_rate_stat = infohub_instance->value_get<double>("netcombtransfer", "tx_rate_stat");
            printf("<infohub_rpcclient> netcombtransfer tx_rate_value = %lf\n", nct_tx_rate_stat);
            printf("<infohub_rpcclient> netcombtransfer rx_rate_value = %lf\n", nct_rx_rate_stat);
            double nct_msg_rx_rate_stat = infohub_instance->value_get<double>("netcombtransfer", "msg_rx_rate_stat");
            double nct_msg_tx_rate_stat = infohub_instance->value_get<double>("netcombtransfer", "msg_tx_rate_stat");
            printf("<infohub_rpcclient> netcombtransfer msg_tx_rate_value = %lf\n", nct_msg_tx_rate_stat);
            printf("<infohub_rpcclient> netcombtransfer msg_rx_rate_value = %lf\n", nct_msg_rx_rate_stat);
            double nct_data_rx_rate_stat = infohub_instance->value_get<double>("netcombtransfer", "data_rx_rate_stat");
            double nct_data_tx_rate_stat = infohub_instance->value_get<double>("netcombtransfer", "data_tx_rate_stat");
            printf("<infohub_rpcclient> netcombtransfer data_tx_rate_value = %lf\n", nct_data_tx_rate_stat);
            printf("<infohub_rpcclient> netcombtransfer data_rx_rate_value = %lf\n", nct_data_rx_rate_stat);

            printf("--------------- speedcontrol infohub --------------\n");
            double spc_expect_speed = infohub_instance->value_get<double>("speedcontrol", "expect_speed_stat");
            double spc_rx_rate_stat = infohub_instance->value_get<double>("speedcontrol", "rx_rate_stat");
            double spc_tx_rate_stat = infohub_instance->value_get<double>("speedcontrol", "tx_rate_stat");
            double spc_mrudp_rx_rate_stat = infohub_instance->value_get<double>("speedcontrol", "mrudp_rx_rate_stat");
            double spc_mrudp_tx_rate_stat = infohub_instance->value_get<double>("speedcontrol", "mrudp_tx_rate_stat");
            double spc_rbudp_rx_rate_stat = infohub_instance->value_get<double>("speedcontrol", "rbudp_rx_rate_stat");
            double spc_rbudp_tx_rate_stat = infohub_instance->value_get<double>("speedcontrol", "rbudp_tx_rate_stat");
            printf("<infohub_rpcclient> speedcontrol spc_expect_speed = %lf\n", spc_expect_speed);
            printf("<infohub_rpcclient> speedcontrol tx_rate_value = %lf\n", spc_tx_rate_stat);
            printf("<infohub_rpcclient> speedcontrol rx_rate_value = %lf\n", spc_rx_rate_stat);
            printf("<infohub_rpcclient> speedcontrol mrudp_tx_rate_value = %lf\n", spc_mrudp_tx_rate_stat);
            printf("<infohub_rpcclient> speedcontrol mrudp_rx_rate_value = %lf\n", spc_mrudp_rx_rate_stat);
            printf("<infohub_rpcclient> speedcontrol rbudp_tx_rate_value = %lf\n", spc_rbudp_tx_rate_stat);
            printf("<infohub_rpcclient> speedcontrol rbudp_rx_rate_value = %lf\n", spc_rbudp_rx_rate_stat);

            printf("--------------- bigdatatransfer infohub --------------\n");
            double bdt_rx_rate_stat = infohub_instance->value_get<double>("bigdatatransfer", "rx_rate_stat");
            double bdt_tx_rate_stat = infohub_instance->value_get<double>("bigdatatransfer", "tx_rate_stat");
            double bdt_cur_spd_stat=  infohub_instance->value_get<double>("bigdatatransfer", "rx_current_speedkBps_stat");
            printf("<infohub_rpcclient> bigdatatransfer tx_rate_value = %lf\n", bdt_tx_rate_stat);
            printf("<infohub_rpcclient> bigdatatransfer rx_rate_value = %lf\n", bdt_rx_rate_stat);
            printf("<infohub_rpcclient> bigdatatransfer rx_current_speedkBps_stat = %lf\n", bdt_cur_spd_stat);
            printf("<infohub_rpcclient> byte/ms\n");
            printf("==============================================\n");


//            //get rate from infohub
//            double bdt_rx_rate_stat=infohub_instance->value_get<double>("bigdatatransfer", "rx_rate_stat");
//            printf("infohub: bigdatatransfer rx_rate_stat=%lf\n", bdt_rx_rate_stat);
//            printf("\n");

            return pInfo->m_task_id;
        }
    }
}

void UpdateRecvTaskStatusState(const std::shared_ptr<FileTaskRecvStatusInfo> &pInfo) {
    MutexLockGuard gGuard(_recvTask_Mutex);
    /*if (_recvTaskState.empty()) {
        std::shared_ptr<RecvTaskState> pTask = std::make_shared<RecvTaskState>();
        pTask->_taskID = pInfo->m_task_id;
        pTask->_process = pInfo->m_current_proc;
        pTask->_taskName = pInfo->m_filename;
        pTask->_taskState = pInfo->m_recv_status;
        _recvTaskState.push_back(pTask);
    }*/
    bool bCreateNewState = true;
    for (auto &pTaskState: _recvTaskState) {
        if (pInfo->m_task_id == pTaskState->_taskID) {
            pTaskState->_transferTime = pInfo->m_time_second;
//            printf("time is:%d\n",pTaskState->_transferTime);
            float averageSpeed=pInfo->m_file_size*8.0/pInfo->m_time_second;
            pTaskState->_transferSpeed=averageSpeed;
//            pTaskState->_transferSpeed = pInfo->m_current_rate;
            pTaskState->_taskState = pInfo->m_recv_status;
            bCreateNewState = false;


        }
    }
    if (!bCreateNewState) {
        return;
    }
    std::shared_ptr<RecvTaskState> pTask = std::make_shared<RecvTaskState>();
    pTask->_TID=pInfo->m_term_id;
    pTask->_taskID = pInfo->m_task_id;
    pTask->_process = pInfo->m_current_proc;
    pTask->_taskFilePath=pInfo->m_file_absolute_path;
    pTask->_taskName = pInfo->m_filename;
    pTask->_taskState = 4;
    _recvTaskState.push_back(pTask);
}

// 创建服务器
bool createServer(std::string localAddr, int localPort, bool isDTU) {
    bool isForceLocalPort = true;
    bool keep_running = true;
    printf("[Debug]funcion.cpp::\n createServer(localip:%s, localport:%d, isDUT:%d)\n",
           localAddr.c_str(), localPort, isDTU);

    if(isDTU == false){
        if (CreateTcpServerChannel(localPort,localAddr,3020,100,isForceLocalPort))
        {
            channelNum += 1;  // 成功创建服务器，更新 channelNum
            register_mrudp_dataserver_callback("AutoSendMessage", mrudp_dataserver_startAutoSend_callback);
            register_mrudp_dataserver_callback("AskStartVideoTrans", mrudp_dataserver_startVideoTrans_callback);
            return true;
        }
        return false;  // 创建失败
     }
    else{
        if (CreateDtuTcpServerChannel(localPort,localAddr,3020,100))
        {
            channelNum += 1;  // 成功创建服务器，更新 channelNum
            register_mrudp_dataserver_callback("AutoSendMessage", mrudp_dataserver_startAutoSend_callback);
            register_mrudp_dataserver_callback("AskStartVideoTrans", mrudp_dataserver_startVideoTrans_callback);
            return true;
        }
        return false;  // 创建失败
    }
}

//創建客戶端
bool createClient(std::string localAddr,int localPort,std::string remoteAddr,unsigned int remotePort, bool isDTU)
{
    bool isForceLocalPort= true;
    printf("[Debug]function.cpp::\n createClient(localip:%s, localport:%d, remoteAddr:%s, remotePort:%d, isDUT:%d)\n",
           localAddr.c_str(), localPort, remoteAddr.c_str(), remotePort, isDTU);

    infohub_instance->value_set("EC", "gui", "Server_infohub");

    //-------infohub rpc server----------
    sender_infohub_rpcserver.init(infohub_instance, 8000);
    sender_infohub_rpcserver.async_run(1);

    if(isDTU == false) {
        if (CreateTcpClientChannel(localPort, remotePort, localAddr, remoteAddr, 3200, isForceLocalPort))
        {
            printf("CreateTcpClientChannel!!!启动普通连接成功!!!");
            clientChannelNum += 1;
            register_mrudp_dataserver_callback("AutoSendMessage", mrudp_dataserver_startAutoSend_callback);
            register_mrudp_dataserver_callback("AskStartVideoTrans", mrudp_dataserver_startVideoTrans_callback);
            //std::thread fileSendThread(FileSendThread);
            //fileSendThread.detach();
            return true;
        }
        return false;
    }
    else{
        if (CreateDtuTcpClientChannel(localPort, remotePort, localAddr, remoteAddr, 3200))
        {
            printf("CreateDtuTcpClientChannel!!!启动DTU成功!!!");
            clientChannelNum += 1;
            //register_mrudp_dataserver_callback("AutoSendMessage", mrudp_dataserver_startAutoSend_callback);

            if(register_mrudp_dataserver_callback("AutoSendMessage", mrudp_dataserver_startAutoSend_callback)==false)
            {
                printf("接收方的mrudp数据服务AutoSendMessage的回调-注册失败！\n");
                return -1;
            }

            register_mrudp_dataserver_callback("AskStartVideoTrans", mrudp_dataserver_startVideoTrans_callback);
            //std::thread fileSendThread(FileSendThread);
            //fileSendThread.detach();
            return true;
        }
        return false;
    }
}

//创建发送任务
bool createTransferSendTask(unsigned int TID,std::string fileName,std::string filePath){
    unsigned int taskID;
    if(Create_BigDataTransferTask(TID,fileName,filePath,taskID)){
        std::shared_ptr<TransferTaskTemplate> tempTask=std::make_shared<TransferTaskTemplate>();
        std::shared_ptr<SendTaskState> tempSend=std::make_shared<SendTaskState>();
        tempTask->_dTermID=TID;
        tempTask->_fileName=fileName;
        tempTask->_fileAddress=filePath;
        tempTask->_taskID=taskID;
        _transferTaskTemplate.push_back(tempTask);
        tempSend->_TID=TID;
        tempSend->_taskID=taskID;
        tempSend->_taskFilePath=filePath;
        tempSend->_taskName=fileName;
        tempSend->_taskState=0;
        _sendTaskState.push_back(tempSend);
        emit dataUpdater->updateSendStatusSignal();
        return true;
    }else{
        printf("没有放入sendlist中\n");
        return false;
    }
}

bool createTransferSendTask(unsigned int TID,std::string fileName,std::string filePath,bool is_RBUDP){
    unsigned int taskID;
    if(Create_BigDataTransferTask(TID,fileName,filePath,taskID,is_RBUDP)){
        std::shared_ptr<TransferTaskTemplate> tempTask=std::make_shared<TransferTaskTemplate>();
        std::shared_ptr<SendTaskState> tempSend=std::make_shared<SendTaskState>();
        tempTask->_dTermID=TID;
        tempTask->_fileName=fileName;
        tempTask->_fileAddress=filePath;
        tempTask->_taskID=taskID;
        _transferTaskTemplate.push_back(tempTask);
        tempSend->_TID=TID;
        tempSend->_taskID=taskID;
        tempSend->_taskFilePath=filePath;
        tempSend->_taskName=fileName;
        tempSend->_taskState=0;
//        qDebug("tempTask->_dTermID: %d", tempTask->_dTermID);
//        qDebug("tempTask->_fileName: %s", tempTask->_fileName.c_str());
//        qDebug("tempTask->_fileAddress: %s", tempTask->_fileAddress.c_str());
//        qDebug("tempTask->_taskID: %d", tempTask->_taskID);
//        qDebug("tempSend->_TID: %d", tempSend->_TID);
//        qDebug("tempSend->_taskID: %d", tempSend->_taskID);
//        qDebug("tempSend->_taskFilePath: %s", tempSend->_taskFilePath.c_str());
//        qDebug("tempSend->_taskName: %s", tempSend->_taskName.c_str());
//        qDebug("tempSend->_taskState: %d", tempSend->_taskState);
//        printf("放入sendlist中了。。。\n");
        _sendTaskState.push_back(tempSend);
        emit dataUpdater->updateSendStatusSignal();
        return true;
    }else{
//        printf("没有放入sendlist中\n");
        return false;
    }
}

//执行自动发送任务
bool startAutoSend(unsigned int TID,bool role){
 //接收端101，发送端102
    if(role){
        SendStartAutoSendMessage_receiver(TID);
        printf("SendStartAutoSendMessage_receiver:%d\n",TID);
    }else{
        SendStartAutoSendMessage_sender(TID);
        printf("SendStartAutoSendMessage_sender:%d\n",TID);
    }
    return role;
}

bool SendStartAutoSendMessage_sender(DWORD remote_tid) {
//    char strLocalTime[21];
//    memset(strLocalTime, 0, sizeof(strLocalTime));
//    LocalTime(strLocalTime, "yyyymmddhh24miss");
//    /*mu_a 用ml发送数*/
//    //生成消息a
//    msg::msg_a ma;
//    ma.set_server_type("AutoSendMessage");
//    //  ma.set_time(strLocalTime);
//    //  ma.set_id(generate_seed);
//    //序列化消息到二进制字符串
//    int msg_size = ma.ByteSizeLong();
//    //std::shared_ptr<BYTE> pBuffer(new BYTE[msg_size], releaseArrays<BYTE>);
//    std::shared_ptr<BYTE> pBuffer(new BYTE[msg_size]);
//    if (ma.SerializeToArray(pBuffer.get(), msg_size) == false) {
//        printf("ma序列化失败!\n");
//    } else {
//        printf("ma序列化成功!\n");
//    }
//    //没发送成功，可能是双端还未连接好
//    while (SendDataBytesToTermByMRUDP(102, pBuffer, msg_size) == false) { sleep(1); };
//    printf("发送ma类型的数据成功！\n");
//    if (remote_tid == 0) {
//    remote_tid = GetIntegerKeyIni("BigDataTransfer", "DEST_TID", 200);
//    }
//    return true;


    DWORD TermId = GetIntegerKeyIni("Main", "DeviceID", 1);
    ec2::FileSender filesender(TermId, 4);

    filesender. enableTrans1By1(false);

    // 创建文件传输任务
    while(filesender.CreateTransferFileTask(102,
                                            "FILE",
                                            "../../FileSend/1test2MB.txt",
                                            5024, 1)==false)
    {
        sleep(1);// 文件传输任务创建失败时，等待并重试
    }

    // 等待文件传输完成

//    while(true){
//        std::this_thread::sleep_for(std::chrono::milliseconds(1));
//    };

    bool transferComplete = false;
    while (!transferComplete) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 适当的休眠

        // 检查文件传输是否完成
        {
            dd::AutoLock autolock(filesender._canTransLock);
            if (filesender._canTransferNextFile && filesender.online_task_num == 0) {
                transferComplete = true;  // 如果没有正在传输的任务，表示传输完成
            }
        }
    }

    // 如果文件传输完成，返回 true
    return true;
}

//想在新线程里等待接收，不知道哪里有问题
//bool SendStartAutoSendMessage_to101(DWORD remote_tid) {
////    char strLocalTime[21]; memset(strLocalTime, 0, sizeof(strLocalTime));
////    LocalTime(strLocalTime, "yyyymmddhh24miss");
////    msg::msg_a ma ;
////    ma.set_server_type("AutoSendMessage");
////    ma.set_time(strLocalTime);
////    ma.set_id(0001);
////    //序列化消息到二进制字符串
////    int msg_size = ma.ByteSizeLong();
////    //std::shared_ptr<BYTE> pBuffer(new BYTE[msg_size], releaseArrays<BYTE>);
////    std::shared_ptr<BYTE> pBuffer(new BYTE[msg_size]);
////    if(ma.SerializeToArray(pBuffer.get(), msg_size)==false)
////    {
////        printf("ma序列化失败!\n");
////    }
////    else{
////        printf("ma序列化成功!\n");
////    }
////    //没发送成功，可能是双端还未连接好
////    while (SendDataBytesToTermByMRUDP(101, pBuffer, msg_size) == false) {sleep(1);};
////
////    //SendDataBytesToTermByMRUDP(102, pBuffer, msg_size);
////    printf("发送ma类型的数据成功！\n");
////    if (remote_tid == 0) {
////        remote_tid = GetIntegerKeyIni("BigDataTransfer", "DEST_TID", 200);
////    }
////    //return SendTIDCommandWithoutSpeedControl(remote_tid, pBuffer, msg_size,false);
////    return true;
//
//    DWORD TermId = GetIntegerKeyIni("Main", "DeviceID", 1);
//    ec2::FileReceiver fileReceiver(TermId, 100);
//    fileReceiver.start("../../FileRecv", "FILE");
//
////    while(true){
////        std::this_thread::sleep_for(std::chrono::milliseconds(200));
////    }
//
////    bool isFinished = false;  // 添加标志，表示文件接收是否完成
//    // 添加标志和条件变量，用于线程之间的同步
//    std::atomic<bool> isFinished(false);  // 标记文件接收是否完成
//    std::condition_variable cv;  // 用于线程同步
//    std::mutex mtx;  // 用于条件变量的互斥锁
//
//    // 启动一个额外的线程检查文件接收状态
//    std::thread statusCheckThread([&]() {
//        while (true) {
//            std::this_thread::sleep_for(std::chrono::seconds(1)); // 每秒检查一次
//
//            // 检查文件接收是否完成，你可以在这里使用适当的判断条件
//            dd::AutoLock autolock(fileReceiver._status_lock);
//            if (fileReceiver._status == "STOP" && fileReceiver._msgqueue.empty()) {
//                isFinished = true;  // 如果接收完成，设置标志为true
//                cv.notify_one();  // 通知主线程可以退出
//                break;  // 跳出循环
//            }
//        }
//    });
//
////    // 等待文件接收完成
////    while (!isFinished) {
////        std::this_thread::sleep_for(std::chrono::milliseconds(200));  // 稍微延时，避免占用过多资源
////    }
//
//    // 等待文件接收完成
//    {
//        std::unique_lock<std::mutex> lock(mtx);
//        cv.wait(lock, [&]{ return isFinished.load(); });  // 等待接收完成的信号
//    }
//
//
//    // 结束文件接收操作
//    fileReceiver.end();
//
//    // 等待状态检查线程结束
//    if (statusCheckThread.joinable()) {
//        statusCheckThread.join();
//    }
//
//    return true;  // 返回true，表示函数执行完毕
//
//}


bool SendStartAutoSendMessage_receiver(DWORD remote_tid) {
//
    DWORD TermId = GetIntegerKeyIni("Main", "DeviceID", 1);
    ec2::FileReceiver fileReceiver(TermId, 100);
    fileReceiver.start("../../FileRecv", "FILE");

    auto startTime = std::chrono::steady_clock::now();
    std::chrono::seconds timeout(3);  // 设置超时时间，假设为3秒
    while(true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        // 超过超时限制，跳出循环
        if (std::chrono::steady_clock::now() - startTime > timeout) {
            break;
        }
    }
//    fileReceiver.end();//这个是在析构函数里面自动调用吗？
    return true;

}

void mrudp_dataserver_startAutoSend_callback(
        DWORD dTermId,
        const std::shared_ptr<BYTE> &recv_data,
        DWORD dRecvDataByteLength)
{
    msg::msg_a ma;
    ma.ParseFromString("");
    printf("mrudp_dataserver_startAutoSend_callback get data\n");
    if(ma.ParseFromArray(recv_data.get(), dRecvDataByteLength)==false) {
        printf("[mrudp_dataserver startAutoSend]::ma.ParseFromArray failed\n");
    }else{
        printf("[mrudp_dataserver startAutoSend]::ma.ParseFromArray succeed!\n");
        printf("[mrudp_dataserver startAutoSend]::recv data: =%s:%s:%d=\n",
               ma.server_type().c_str(), ma.time().c_str(), ma.id());
        printf("[mrudp_dataserver startAutoSend]::recv StartAutoSend Message!!!\n\n");
        startAutoSendFlag = true;
    }
}

void mrudp_dataserver_startVideoTrans_callback(
        DWORD dTermId,
        const std::shared_ptr<BYTE> &recv_data,
        DWORD dRecvDataByteLength)
{
    msg::msg_a ma;
    ma.ParseFromString("");
    printf("mrudp_dataserver_startVideoTrans_callback get data\n");
    if(ma.ParseFromArray(recv_data.get(), dRecvDataByteLength)==false) {
        printf("[mrudp_dataserver startVideoTrans]::ma.ParseFromArray failed\n");
    }else{
        printf("[mrudp_dataserver startVideoTrans]::ma.ParseFromArray succeed!\n");
        printf("[mrudp_dataserver startVideoTrans]::recv data: =%s:%s:%d=\n",
               ma.server_type().c_str(), ma.time().c_str(), ma.id());
        printf("[mrudp_dataserver startVideoTrans]::recv startVideoTrans Message!!!\n\n");
        startVideoTransFlag = true;
    }
}

unsigned int getTID(){
    return TID;
}

//取消传输任务
bool transferTaskCancel(DWORD task_id){
    if(SetFileSendTaskCancel(task_id)){
        return true;
    }
    return false;
}

std::string exec_command(const char* cmd) {
    char buffer[128];
    std::string result = "";
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        return "Command failed";
    }
    while (!feof(pipe.get())) {
        if (fgets(buffer, 128, pipe.get()) != nullptr) {
            result += buffer;
        }
    }
    return result;
}

//从配置文件中读取相关信息
std::string getStringFromIni(std::string section,std::string key,std::string defaultValue){
    return GetStringValueKeyIni(section,key,defaultValue);
}

int getIntFromIni(std::string section,std::string key,int defaultValue){
    return GetIntegerKeyIni(section,key,defaultValue);
}

//截取文件路径中的文件名
std::string extractFileName(const std::string& filePath) {
    size_t pos = filePath.find_last_of("/\\"); // 查找最后一个目录分隔符
    if (pos != std::string::npos) {
        return filePath.substr(pos + 1); // 提取文件名部分
    }
    return filePath; // 如果没有目录分隔符，返回整个路径作为文件名
}

 bool getBoolFromIni(std::string section, std::string key, int defaultValue) {
     return GetBoolValueKeyIni(section,key,defaultValue);
 }

vector<string> splitString(const string& s,const string& delim){
    vector<string> result;
    string temp=s;
    int step=delim.length();
    int last=s.find_first_of(delim,0)+step;
    int index=s.find_first_of(delim,last);
    while(index!=std::string::npos){
        string token=s.substr(last,index-last);
        cout<<"split result:"<<token<<" ";
        result.push_back(token);
        last=index+step;
        index=s.find_first_of(delim,last);
    }
    cout<<endl;
    return result;
}

 //发送请求视频传输信令
 bool askStartVideoTrans(unsigned int remote_tid,unsigned int remote_videoPort){
     return AskStartVideoTrans(remote_tid);
 }
//请求对端传输视频
bool AskStartVideoTrans(DWORD remote_tid){
    char strLocalTime[21]; memset(strLocalTime, 0, sizeof(strLocalTime));
    LocalTime(strLocalTime, "yyyymmddhh24miss");
    msg::msg_a ma ;
    ma.set_server_type("AskStartVideoTrans");
    ma.set_time(strLocalTime); ma.set_id(0001);
    //序列化消息到二进制字符串
    int msg_size = ma.ByteSizeLong();
    //std::shared_ptr<BYTE> pBuffer(new BYTE[msg_size], releaseArrays<BYTE>);
    std::shared_ptr<BYTE> pBuffer(new BYTE[msg_size]);
    if(ma.SerializeToArray(pBuffer.get(), msg_size)==false)
    {
        printf("ma序列化失败!\n");
    }
    else{
        printf("ma序列化成功!\n");
    }
    //没发送成功，可能是双端还未连接好
    while( SendDataBytesToTermByMRUDP(102, pBuffer, msg_size) == false )
    {sleep(1);};
//    SendDataBytesToTermByMRUDP(102, pBuffer, msg_size);
    printf("发送ma类型的数据成功！\n");
    if (remote_tid == 0) {
        remote_tid = GetIntegerKeyIni("BigDataTransfer", "DEST_TID", 200);
    }
    return true;

}
