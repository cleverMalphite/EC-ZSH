//
// Created by Kong on 2023/6/9.
//
#include "InitHelp.h"
#include "RBUDPApi.h"
#include "RBUDPManager.h"
#include "utility"
#include "queue"
#include "memory"
#include "../Util/LockGuard.h"
#include "../SpeedControl/SpeedControlApi.h"

#include "../public/_public.h"
#include "../msg_cpp/msg.pb.h"

//---------------------RBUDP模块的内部全局单例--------------
namespace RBUDP {
    /*===========RBUDPManager的模块单例===========*/
    std::shared_ptr<RBUDPManager> g_pRBUDPManager;

    /*===========数据回调函数列表===========*/
    std::vector<PGETDATAFROMRBUDPFORBIGDATATRANSFERCALLBACK> \
                                        g_rbudp_data_func_vector;

//    /*===========数据服务回调表===========*/
//    //20240911-houlc g_rbudp_dataserver_callback_map                
//    std::map<std::string, RBUDP_DATASERVER_CALLBACK> \
//                                g_rbudp_dataserver_callback_map;

    /*===============线程开关=========*/
    void SetThreadStop(bool bTemp) {
        MutexLockGuard gStopLockGuard(gStopMutex_);
        g_bStop = bTemp;
    }

    bool GetThreadStop() {
        MutexLockGuard gStopLockGuard(gStopMutex_);
        return g_bStop;
    }

    pthread_t NAKSendThread;
    pthread_t UploadAllEnd2EndTransmissionRecvQueueThread_;

    void UploadAllEnd2EndTransmissionRecvQueue() {
        g_pRBUDPManager->UploadTempDataQueueForAllEnd2EndTransmission();
    }

    void *UploadAllEnd2EndTransmissionRecvQueueThread(void *argNeeded) {
        while (!GetThreadStop()) {
            UploadAllEnd2EndTransmissionRecvQueue();
            usleep(2000);
        }
        return nullptr;
    }

    void RBUDP_NAK_SEND() {
        g_pRBUDPManager->SendNAKForAllEnd2EndReliableTransmission();
    }   //用于向所有端到端实体发送一次NAK，但是由于各个通道传输效率不同，目前先设计向一个发送

    void *RBUDP_NAK_SEND_THREAD(void *argNeeded) {
        //DWORD startime, endtime, total, sleeptime;
        while (!GetThreadStop()) {
            //startime = GetTickCount();
            RBUDP_NAK_SEND();
            //endtime = GetTickCount();
            usleep(5000);
        }
        return nullptr;
    }

}

//---------------------------RBUDP接口函数的定义----------------------


/*==============注册回调函数===================*/
//注册数据回调函数
bool RegisterRBUDPFunc(PGETDATAFROMRBUDPFORBIGDATATRANSFERCALLBACK pFunc) {
    if (nullptr == RBUDP::g_pRBUDPManager || nullptr == pFunc) {
        return false;
    }
    RBUDP::g_rbudp_data_func_vector.push_back(pFunc);
    return true;
}

////对外通用的注册数据服务回调的注册函数
////注意：需要注册服务类型的名称+回调函数
////  以便接收数据时根据名称判断数据类型后,
////  分发到的响应的回调函数中
//bool register_rbudp_dataserver_callback(string server_name, RBUDP_DATASERVER_CALLBACK p_callback){
//    printf("注册 数据 服务 回调 1 \n");
//    //空参数判别
//    if( "" == server_name || nullptr == p_callback ) return false;
//    //重复服务名判别
//    auto it = RBUDP::g_rbudp_dataserver_callback_map.find(server_name);
//    if( it != RBUDP::g_rbudp_dataserver_callback_map.end() ) return false;
//    //将键值插入服务回调表
//    printf("注册 数据 服务 回调 5 \n");
//    RBUDP::g_rbudp_dataserver_callback_map[server_name] = p_callback;
//    return true;
//}

/*=============模块核心函数====================*/
//初始化RBUDP模块
void InitRBUDP(std::string sSendFileDirPath) {
    pthread_mutex_init(&RBUDP::gStopMutex_, nullptr);
    RBUDP::SetThreadStop(false);


    RBUDP::g_pRBUDPManager = std::make_shared<RBUDP::RBUDPManager>(sSendFileDirPath);
    //开启两个线程，一个用于处理消息，一个用于取数据进行发送
    pthread_create(&RBUDP::NAKSendThread, nullptr, RBUDP::RBUDP_NAK_SEND_THREAD, nullptr);
    pthread_create(&RBUDP::UploadAllEnd2EndTransmissionRecvQueueThread_, nullptr,
                   RBUDP::UploadAllEnd2EndTransmissionRecvQueueThread,
                   nullptr);

    printf(">> >> START RBUDP  ...\n");
}
//释放资源函数
void UnInitRBUDP() {
    //关闭各个线程
    //我们采用优雅关闭线程的办法，即让其它线程的线程函数优雅地return
    //为了避免纷繁复杂的临界区问题，我们在InitHelper中定义了一个boolean，然后设置其GET和SET方法，在这两个方法内对其进行临界区设置，
    //从而避免了多个线程内对其操作的尴尬局面
    RBUDP::SetThreadStop(true);

    //Sleep(10000);    //留给其它线程优雅退出的时间
    pthread_join(RBUDP::NAKSendThread, nullptr);
    pthread_join(RBUDP::UploadAllEnd2EndTransmissionRecvQueueThread_,nullptr);
    pthread_mutex_destroy(&RBUDP::gStopMutex_);
}

//int totallength = 0;
int times = 0;
//DWORD start = GetTickCount(), end_t = 0;
//可靠传输的发送函数
bool SendDataBytesToTermByRBUDP(DWORD dTermId, const std::shared_ptr<BYTE> &bytes, DWORD nLength) {

    if (nullptr != RBUDP::g_pRBUDPManager) {
        printf("___---+++| SendDataBytesToTermByRBUDP\n");
        const bool bResult = RBUDP::g_pRBUDPManager->SendDataBytesToEndByRBUDP(dTermId,bytes,nLength);
        return bResult;
    }
    return false;
}
//不可靠传输的发送函数
bool SendDataBytesToTermByRBUDPWithoutReliable(DWORD dTermId, std::shared_ptr<BYTE> bytes, DWORD nLength) {
    if (nullptr != RBUDP::g_pRBUDPManager) {
        return RBUDP::g_pRBUDPManager->SendDataBytesToTermByRBUDPWithoutReliable(dTermId, bytes, nLength);
    }
    return false;
}
/*====================模块内工具函数=====================*/
//调用BigDataTransfer注册的回调函数
void CallBackDataFunc(DWORD dTermId, std::shared_ptr<BYTE> pRecvDataBytes, DWORD dRecvDataByteLength) {
    for (PGETDATAFROMRBUDPFORBIGDATATRANSFERCALLBACK data_func_vector: RBUDP::g_rbudp_data_func_vector) {
        data_func_vector(dTermId, pRecvDataBytes, dRecvDataByteLength);
    }
}
////数据服务回调的数据检查函数
//bool rbudp_dataserver_check(
//       const std::shared_ptr<BYTE> &recv_data,
//       DWORD dRecvDataByteLength )
//{
//    /***从BYTE数据中解析判断数据类型***/
//    //解析数据头,获取消息类型
//    msg::msg_head mh;
//    //mh.ParseFromString(strdata);
//    if(!mh.ParseFromArray(recv_data.get(),
//                            dRecvDataByteLength)){
//        printf("[dataserver check] => not dataserver data!!!\n");
//        //解析出错
//        return false;
//    };
//    printf("[dataserver check] => is dataserver data.\n");
//    std::string server_type = mh.server_type();
//    printf("[dataserver check] => server type:%s\n",
//                                    server_type.c_str());
//}

//数据回调服务对应的回调调用
//20240909-houlc spin_server_callback
//typedef void(*RBUDP_DATASERVER_CALLBACK)(
//        DWORD dTermId,
//        const std::shared_ptr<BYTE> &recv_data,
//        DWORD dRecvDataByteLength
//);
//bool rbudp_spin_server_callback(DWORD dTermId,
//                        const std::shared_ptr<BYTE> &recv_data,
//                        DWORD dRecvDataByteLength) {
//    printf("in spin_server_callback...\n");
//    /***从BYTE数据中解析判断数据类型***/
//    //解析数据头,获取消息类型
//    msg::msg_head mh;
//    //mh.ParseFromString(strdata);
//    if(!mh.ParseFromArray(recv_data.get(), dRecvDataByteLength)){
//      printf("!!! !!! msg_head parse wrong !!! !!!\n");
//      //解析出错
//      return false;
//    };
//    std::string server_type = mh.server_type();
//
//    auto it = RBUDP::g_rbudp_dataserver_callback_map.find(server_type);
//    //if(it!=RBUDP::g_rbudp_dataserver_callback_map.end()){
//    //  printf("has not register this dataserver : %s \n"\
//    //         , server_type.c_str());
//    //  return ;
//    //}
//    
//    if(it == RBUDP::g_rbudp_dataserver_callback_map.end()){
//      printf("has not register this dataserver : %s \n"\
//                                        , server_type.c_str());
//      return false;
//    }
//
//    printf("call rbudp dataserver callback(%s) !\n",\
//                server_type.c_str());
//    RBUDP::g_rbudp_dataserver_callback_map[server_type](dTermId, recv_data, dRecvDataByteLength);
//    printf("call rbudp dataserver callback ok !\n");
//    return true;
//}



