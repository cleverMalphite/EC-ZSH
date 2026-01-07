//
// Created by 王炳棋 on 2022/12/30.
//
#include <atomic>
#include <map>
#include "MRUDPApi.h"
#include "MRUDPTerm2TermConfig.h"
#include "MRUDPManager.h"
#include "MRUDPSettingUtil.h"
#include "../Util/LockGuard.h"
#include "Transfer_H/ReliableForceSpeedMessage.h"
#include "Transfer_H/ReliableForceAdjustBandWidthMessage.h"
#include "../SpeedControl/SpeedControlApi.h"
#include "SerializeUtil.h"
#include "./End2EndReliableTransmission.h"

#include "../public/_public.h"
#include "../msg_cpp/msg.pb.h"

#define DEBUG_INITRUDP

//-----------------------MRUDP模块的内部全局单例--------------------
namespace MRUDP {
    /*=============MRUDPManager的模块单例==========*/
    std::shared_ptr<MRUDPManager> gMRUDPManager;

    /*==================数据回调函数列表============*/
    std::vector<PGETDATAFROMMRUDPFORBIGDATATRANSFERCALLBACK> \
                                g_reliable_udp_data_func_vector;

    /*==================数据服务回调表============*/
    //20240909-houlc g_mrudp_dataserver_callback_map
    std::map<std::string, MRUDP_DATASERVER_CALLBACK> \
                                g_mrudp_dataserver_callback_map;

    std::map<std::string, MRUDP_DATASERVER_CALLBACK2> \
                                g_mrudp_dataserver_callback_map2;
    //std::map<std::string, void *handle> g_mrudp_dataserver_handle_map;
    std::map<std::string, int> g_mrudp_handle_index;
    void*  g_mrudp_handle_map[301]; //默认能存储301个回调调用对象
    int g_mrudp_handle_cnt = 0;

    /*==================线程开关============*/
    //线程停止标识
    bool g_bStop;   
    //线程停止标志的互斥锁：配合g_bStop使用
    pthread_mutex_t gStopMutex_;    

    //线程标识操作 用于设置线程结束标志
    void SetThreadStop(bool bTemp) {
        MutexLockGuard gStopLockGuard(gStopMutex_);
        g_bStop = bTemp;
    }

    //用于得到线程结束标志的值
    bool GetThreadStop() {
        MutexLockGuard gStopLockGuard(gStopMutex_);
        return g_bStop;
    }

    /*=======3个线程实例及对应的线程函数=======*/
    //【thread-1】 ACK的发送线程，发送FACK确认包
    pthread_t AckSendThread;
    //对应的线程函数
    void *MRUDP_FACK_SEND_THREAD(void *argNeeded) {
        DWORD StartTime = 0;
        DWORD SendTime = 0;
        while (!GetThreadStop()) {
            /*所有的端到端实体发一次FACK*/
            gMRUDPManager->SendFACKForAllEnd2EndReliableTransmission();
            usleep( 2500);
        }
        return nullptr;
    }
    //【thread-2】端到端传输的数据接收线程(收到后存入CircularQueue中缓冲-Update)
    pthread_t DealAllEnd2EndTransmissionTempDataQueueThread_;
    //对应的线程函数
    void *DealAllEnd2EndTransmissionTempDataQueueThread(void *argNeeded) {
        while (!GetThreadStop()) {
            gMRUDPManager->DealTempDataQueueForAllEnd2EndTransmission();
            usleep(2000);
        }
        return nullptr;
    }
    //【thread-3】端到端传输的数据上传线程(从CircularQueue中将数据上传给上层模块-Load)
    pthread_t UploadAllEnd2EndTransmissionRecvQueueThread_;
    //对应的线程函数
    void *UploadAllEnd2EndTransmissionRecvQueueThread(void *argNeeded) {
        while (!GetThreadStop()) {
            gMRUDPManager->UploadTempDataQueueForAllEnd2EndTransmission();
            usleep(2000);
        }
        return nullptr;
    }
}

//---------------------------MRUDP接口函数的定义----------------------


/*==============注册回调函数===================*/
//注册数据回调函数
bool RegisterMRUDPFunc(PGETDATAFROMMRUDPFORBIGDATATRANSFERCALLBACK pFunc) {
    if (nullptr == MRUDP::gMRUDPManager || nullptr == pFunc) {
        return false;
    }
    MRUDP::g_reliable_udp_data_func_vector.push_back(pFunc);
    return true;
}

//对外通用的注册数据服务回调的注册函数
//注意：需要注册服务类型的名称+回调函数
//  以便接收数据时根据名称判断数据类型后,
//  分发到的响应的回调函数中
bool register_mrudp_dataserver_callback(string server_name, MRUDP_DATASERVER_CALLBACK p_callback){
    //空参数判别
    if( "" == server_name || nullptr == p_callback ) return false;
    //重复服务名判别
    auto it = MRUDP::g_mrudp_dataserver_callback_map.find(server_name);
    if( it != MRUDP::g_mrudp_dataserver_callback_map.end() ) return false;
    //将键值插入服务回调表
    printf("注册 数据 服务 Callback\n");
    MRUDP::g_mrudp_dataserver_callback_map[server_name] = p_callback;
    //将键值插入服务回调表

    return true;
}
bool register_mrudp_dataserver_callback(string server_name, MRUDP_DATASERVER_CALLBACK2 p_callback, void* handle){
    //空参数判别
    if( "" == server_name || nullptr == p_callback ) return false;
    //重复服务名判别
    auto it = MRUDP::g_mrudp_dataserver_callback_map.find(server_name);
    if( it != MRUDP::g_mrudp_dataserver_callback_map.end() ) return false;
    //将键值插入服务回调表
    printf("注册 数据 服务 Callback\n");
    MRUDP::g_mrudp_dataserver_callback_map2[server_name] = p_callback;
    //将键值插入服务回调表
    printf("注册 数据 服务 Handle\n");

    if( MRUDP::g_mrudp_handle_cnt>=301 )return false;
    MRUDP::g_mrudp_handle_map[ MRUDP::g_mrudp_handle_cnt ] = handle;
    MRUDP::g_mrudp_handle_index[server_name] = MRUDP::g_mrudp_handle_cnt;
    MRUDP::g_mrudp_handle_cnt++;

    return true;
}


/*=============模块核心函数====================*/
//初始化MRUDP模块
void InitMRUDP(const std::string &sSendFileDirPath) {
    pthread_mutex_init(&MRUDP::gStopMutex_, nullptr);
    MRUDP::SetThreadStop(false);    //设置线程退出标志为false
    //初始化manager
    MRUDP::gMRUDPManager = std::make_shared<MRUDP::MRUDPManager>(sSendFileDirPath);

    //开启另一个线程来处理循环消息
    pthread_create(&MRUDP::AckSendThread, nullptr, MRUDP::MRUDP_FACK_SEND_THREAD, nullptr);
    pthread_create(&MRUDP::DealAllEnd2EndTransmissionTempDataQueueThread_, nullptr,
                   MRUDP::DealAllEnd2EndTransmissionTempDataQueueThread,
                   nullptr);
    pthread_create(&MRUDP::UploadAllEnd2EndTransmissionRecvQueueThread_, nullptr,
                   MRUDP::UploadAllEnd2EndTransmissionRecvQueueThread,
                   nullptr);
    printf(">> >> START MRUDP  ...\n");
}
//释放资源函数
void UnInitMRUDP() {

    MRUDP::SetThreadStop(true);
    //不能用sleep阻塞的方式去等待条件，需要修改,可以使用定时回调
    pthread_join(MRUDP::AckSendThread, nullptr);
    pthread_join(MRUDP::DealAllEnd2EndTransmissionTempDataQueueThread_, nullptr);
    pthread_join(MRUDP::UploadAllEnd2EndTransmissionRecvQueueThread_, nullptr);
    pthread_mutex_destroy(&MRUDP::gStopMutex_);
}
//可靠传输发送函数
bool SendDataBytesToTermByMRUDP(DWORD dTermId, const std::shared_ptr<BYTE> &bytes, DWORD nLength) {

    if (nullptr != MRUDP::gMRUDPManager) {
        const bool bResult = MRUDP::gMRUDPManager->SendDataBytesToEndByMRUDP(dTermId, bytes, nLength);
        if (bResult) {
            //统计上层调用MRUDP发送的数据包数量
        }
        return bResult;
    }
    return false;
}
//不可靠传输发送函数
bool SendDataBytesToTermByMRUDPWithoutReliable(DWORD dTermId, std::shared_ptr<BYTE> bytes, DWORD nLength) {
    if (nullptr != MRUDP::gMRUDPManager) {


        return MRUDP::gMRUDPManager->SendDataBytesToTermByMRUDPWithoutReliable(dTermId, bytes, nLength);
    }
    return false;
}
/*=============辅助功能函数====================*/
bool AdjustselfBandWidth(DWORD remote_tid) {
    if (MRUDP::gMRUDPManager) {
        return MRUDP::gMRUDPManager->AdjustBandWidth(remote_tid);
    }
    return false;
}
bool SendAdjustBandWidthMessage(DWORD remote_tid) {
    DWORD length = 0;
    int DeviceID = GetIntegerKeyIni("Main", "DeviceID", 200);

    std::shared_ptr<MRUDP::ReliableForceAdjustBandWidthMessage> \
    control_msg = std::make_shared<MRUDP::ReliableForceAdjustBandWidthMessage>( DeviceID );

    std::shared_ptr<BYTE> tmp = control_msg->Encode(length);

    return SendTIDCommandWithoutSpeedControl(remote_tid, tmp, length,false);
}
bool ChangeRemoteSendSpeed(DWORD remote_tid, DWORD m_force_speed) {
    DWORD dLength = 0;
    int DeviceID = GetIntegerKeyIni("Main", "DeviceID", 200); 

    std::shared_ptr<MRUDP::ReliableForceSpeedMessage> \
    force_speed_message = std::make_shared<MRUDP::ReliableForceSpeedMessage>(DeviceID, m_force_speed);

    std::shared_ptr<BYTE> pBytes = force_speed_message->Encode(dLength);

    return SendTIDCommandWithoutSpeedControl(remote_tid, pBytes, dLength, false);
}
void PrintMRUDPInfo() {
    if (MRUDP::gMRUDPManager != nullptr) {
        MRUDP::gMRUDPManager->GetAndUpdateMrudpStatusInfo();
    }
}

/*=================模块内工具函数==================*/
void CallBackDataFunc(DWORD dTermId, 
                      const std::shared_ptr<BYTE> &pRecvDataBytes, 
                      DWORD dRecvDataByteLength) {
    for (auto data_func_vector: MRUDP::g_reliable_udp_data_func_vector) {
        data_func_vector(dTermId, pRecvDataBytes, dRecvDataByteLength);
    }
}

//数据服务回调的数据检查函数
bool dataserver_check(
        const std::shared_ptr<BYTE> &recv_data, 
        DWORD dRecvDataByteLength )
{
    /***从BYTE数据中解析判断数据类型***/
    //解析数据头,获取消息类型
    msg::msg_head mh;
    //mh.ParseFromString(strdata);
    if(!mh.ParseFromArray(recv_data.get(), 
                          dRecvDataByteLength)){
      printf("[dataserver check] => not dataserver data!!!\n");
      //解析出错
      return false;
    };
    printf("[dataserver check] => is dataserver data.\n");
    std::string server_type = mh.server_type();
    printf("[dataserver check] => server type:%s\n",
                                    server_type.c_str());
    return true;
}

//数据回调服务对应的回调调用
//20240909-houlc spin_server_callback
//typedef void(*MRUDP_DATASERVER_CALLBACK)(
//        DWORD dTermId,
//        const std::shared_ptr<BYTE> &recv_data,
//        DWORD dRecvDataByteLength
//);
bool spin_server_callback(DWORD dTermId, 
                      const std::shared_ptr<BYTE> &recv_data, 
                      DWORD dRecvDataByteLength) {

    printf("in spin_server_callback...\n");
    /***从BYTE数据中解析判断数据类型***/
    //解析数据头,获取消息类型
    msg::msg_head mh;
    //mh.ParseFromString(strdata);
    if(!mh.ParseFromArray(recv_data.get(), dRecvDataByteLength)){
      printf("!!! !!! msg_head parse wrong !!! !!!\n");
      //解析出错
      return false;
    };
    std::string server_type = mh.server_type();

    auto it = MRUDP::g_mrudp_dataserver_callback_map.find(server_type);
    if(it != MRUDP::g_mrudp_dataserver_callback_map.end()){
        printf("call mrudp dataserver callback(%s) !\n", server_type.c_str());
        MRUDP::g_mrudp_dataserver_callback_map[server_type](dTermId, recv_data, dRecvDataByteLength);
    }
    else{
        auto it2 = MRUDP::g_mrudp_dataserver_callback_map2.find(server_type);
        if(it2 != MRUDP::g_mrudp_dataserver_callback_map2.end()){
            printf("call mrudp dataserver callback2(%s) !\n", server_type.c_str());
            MRUDP::g_mrudp_dataserver_callback_map2[server_type](\
                            dTermId, recv_data, dRecvDataByteLength, \
                            MRUDP:: g_mrudp_handle_map[\
                                MRUDP::g_mrudp_handle_index[server_type]\
                            ]);
        }
        else{
          printf("has not register this dataserver : %s \n", server_type.c_str());
          return false;
        }
    }
    //根据函数指针呢类型判断运行模式
    printf("call mrudp dataserver callback ok !\n");
    return true;
}

float ComputeAndGetRUDPLossRateByTID(DWORD d_remote_tid) {
    if (nullptr != MRUDP::gMRUDPManager) {
        return MRUDP::gMRUDPManager->ComputeAndGetRUDPLossRateByTID(d_remote_tid);
    }
    return 0.0;
}
bool IsRUDPOnByTermId(DWORD term_id) {
    if (nullptr != MRUDP::gMRUDPManager) {
        return MRUDP::gMRUDPManager->IsRUDPOnByTermId(term_id);
    }
    return false;
}


