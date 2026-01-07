#include "SpeedControlApi.h"

#include <utility>
#include "../NetCombTransfer/NetCombTransferApi.h"
#include "../IniHandle/IniHandleApi.h"
#include "SpeedControlManager.h"
#include "QosStructInterfaceInfo.h"

#include "infoHub/infoHub.h"
#include "infoHub/rateStatistic.h"
#include "infoHub/rateStatisticTable.h"

//----------------------infoHub(信息仓库)初始化---------------------------------
/*SpeedControl模块的信息需求清单
  1. 该层的总的发送速率：
        tx_rate
  2. 各个终端的发送速率：
        map<tid, rate> tx_tid_rate_map
  3. 当前期望的发送速率:
        map<tid, rate> tx_tid_rate_map
  4. QOS参考信息
        map<tid, rate> qos_tx_tid_rate_map
        map<tid, rtt> qos_tid_rtt_map
        map<tid, delay> qos_tid_delay_map
        map<tid, loss> qos_tid_loss_map

  * key名，即为value名，table名
  * 上述table的field名即为tid
  * module名为speedcontrol
*/


std::shared_ptr<ec2::infoHub>\
        _speedcontrol_infohub_instance = \
            ec2::infoHub::getInstance();

//spc means speed control module
ec2::rateStatistic      spc_tx_rate_stat; //当前模块的输出速率
ec2::rateStatistic      spc_rx_rate_stat; //当前模块的输入速率
                                          //
ec2::rateStatistic      spc_mrudp_tx_rate_stat; //当前模块的mrudp输出速率
ec2::rateStatistic      spc_mrudp_rx_rate_stat; //当前模块的mrudp输入速率
                                          //
ec2::rateStatistic      spc_rbudp_tx_rate_stat; //当前模块的rbudp输出速率
ec2::rateStatistic      spc_rbudp_rx_rate_stat; //当前模块的rbudp输入速率
                                                
ec2::rateStatisticTable spc_tx_tid_rate_sttable; //各个终端的输出速率
ec2::rateStatisticTable spc_rx_tid_rate_sttable; //各个终端的输入速率

ec2::rateStatisticTable spc_exp_tx_tid_rate_sttable; //各个终端的期望输出速率
ec2::rateStatisticTable spc_exp_rx_tid_rate_sttable; //各个终端的期望输入效率

//speedcontrol模块的控制速率
//speed control expect speed :  expect_speed_stat



//------------------ infoHub(信息仓库)初始化 End --------------------------

namespace SpeedControl {
    std::shared_ptr<SpeedControlManager> gSpeedControlManager = nullptr;
    bool g_speed_control_stop = false;    //为true时表示线程退出
    /*HANDLE g_speed_control_event = CreateEvent(NULL, 1, 1, NULL); //防止工作线程CPU占比过高*/
    //数据回调函数数组
    const static DWORD SPEED_CONTROL_MAX_REGISTER_DATA_FUNC = 10;
    DWORD g_d_current_register_func_num = 0;    //到目前已经注册了多少个数据回调函数
    pSpeedControlRecvDataCallBack g_p_data_func[SPEED_CONTROL_MAX_REGISTER_DATA_FUNC];
    //仅供QoS模块注册的数据回调函数
    pSpeedControlRemoteToSelfQosInfoCallBack g_p_qos_remote_to_self_func = NULL;
    pSpeedControlSelfToRemoteQosInfoCallBack g_p_qos_self_to_remote_func = NULL;

    //已经注册了多少个终端上下线回调函数
    static DWORD SPEEDCONTROL_TERM_ON_OFF_ALREADY_NUM = 0;
    pSpeedControlTermOnOffCallBack g_p_term_onoff_func[SPEED_CONTROL_MAX_REGISTER_DATA_FUNC];

    //RBUDP模块注册的已发送包的回调函数
    static DWORD SPEEDCONTROL_RBUDP_ALREADY_NUM = 0;
    pSpeedControlRBUDPDataCallBack g_p_rbudp_data_func[SPEED_CONTROL_MAX_REGISTER_DATA_FUNC];
}

bool InitSpeedControl(DWORD n_tid) {
    //MARK:API提供的初始化函数包括对日志的初始化和对Manager的初始化，
    //这里因为还没有对SPDlog库进行封装所以暂时将日志部分注释掉
    if (nullptr == SpeedControl::gSpeedControlManager) {
        SpeedControl::gSpeedControlManager = std::make_shared<SpeedControl::SpeedControlManager>(n_tid);
    }
    //printf("[SpeedControl]::模块初始化完成\n");

    bool ret =  SpeedControl::gSpeedControlManager->Init();
    if(true==ret){
        //-------------------- infohub init --------------------
        spc_tx_rate_stat.init("speedcontrol", "tx_rate_stat");
        spc_rx_rate_stat.init("speedcontrol", "rx_rate_stat");

        spc_mrudp_tx_rate_stat.init("speedcontrol", "mrudp_tx_rate_stat"); 
        spc_mrudp_rx_rate_stat.init("speedcontrol", "mrudp_rx_rate_stat"); 
        
        spc_rbudp_tx_rate_stat.init("speedcontrol", "rbudp_tx_rate_stat");
        spc_rbudp_rx_rate_stat.init("speedcontrol", "rbudp_rx_rate_stat"); 


        spc_tx_tid_rate_sttable.init("speedcontrol", "tx_tid_rate_sttable");
        spc_rx_tid_rate_sttable.init("speedcontrol", "rx_tid_rate_sttable");

        spc_exp_tx_tid_rate_sttable.init("speedcontrol", "exp_tx_tid_rate_sttable");
        spc_exp_rx_tid_rate_sttable.init("speedcontrol", "exp_rx_tid_rate_sttable");

        spc_tx_rate_stat.begin();
        spc_rx_rate_stat.begin();

        spc_mrudp_tx_rate_stat.begin(); 
        spc_mrudp_rx_rate_stat.begin(); 

        spc_rbudp_tx_rate_stat.begin();
        spc_rbudp_rx_rate_stat.begin(); 
    }
    return ret;
}

bool UninitSpeedControl() {
    SpeedControl::g_speed_control_stop = true;
    //这里暂时选择使用usleep
    usleep(1000);
    SpeedControl::gSpeedControlManager = nullptr;
    return true;
}

DWORD recv_data_count = 0;
DWORD number_of_thousand = 0;


bool SendTIDDataWithSpeedControl(DWORD dwTID, const std::shared_ptr<BYTE> &pBuffer, DWORD dwLength, bool isUrgent,
                                 bool isSleep, bool isRBUDP) {

    //printf("[SC::AutoTestDebug]::向%d发送数据:%d字节\n", dwTID, dwLength);
    if (SpeedControl::gSpeedControlManager) {
        bool ret = false;
        if (isUrgent) {
            ret =  SpeedControl::gSpeedControlManager->DoSendDataPri(dwTID, pBuffer, dwLength, isSleep, isRBUDP);
        } else {
            ret = SpeedControl::gSpeedControlManager->DoSendData(dwTID, pBuffer, dwLength, isSleep, isRBUDP);
        }
        //------- infohub -------------
        if(true==ret){
            spc_tx_rate_stat.pass(dwLength);
            if(false==isRBUDP) spc_mrudp_tx_rate_stat.pass(dwLength);
            else if(true==isRBUDP) spc_rbudp_tx_rate_stat.pass(dwLength);

            spc_tx_tid_rate_sttable.begin(dwTID);
            spc_tx_tid_rate_sttable.pass(dwTID, dwLength);
        }

        return ret;
    }
    return false;
}

bool SendTIDDataWithoutSpeedControl(DWORD dwTID, const std::shared_ptr<BYTE> &pBuffer, DWORD dwLength, bool isRBUDP) {
    static DWORD number_of_thousand_without_speedcontrol = 0;
    static DWORD send_data_count_without_speedcontrol = 0;
    if (SpeedControl::gSpeedControlManager) {

        SpeedControl::gSpeedControlManager->DoSendDataWithoutSpeedControl(dwTID, pBuffer, dwLength, isRBUDP);

        //------- infohub -------------
        spc_tx_rate_stat.pass(dwLength);
        if(false==isRBUDP) spc_mrudp_tx_rate_stat.pass(dwLength);
        else if(true==isRBUDP) spc_rbudp_tx_rate_stat.pass(dwLength);

        spc_tx_tid_rate_sttable.begin(dwTID);
        spc_tx_tid_rate_sttable.pass(dwTID, dwLength);

        return true;
    }
    return false;
}

bool SendTIDDataWithSpeedControlPri(DWORD dwTID, const std::shared_ptr<BYTE> &pBuffer, DWORD dwLength, bool isSleep,
                                    bool isRBUDP) {
    if (SpeedControl::gSpeedControlManager) {
        SpeedControl::gSpeedControlManager->DoSendDataPri(dwTID, pBuffer, dwLength, isSleep, isRBUDP);

        //------- infohub -------------
        spc_tx_rate_stat.pass(dwLength);
        if(false==isRBUDP) spc_mrudp_tx_rate_stat.pass(dwLength);
        else if(true==isRBUDP) spc_rbudp_tx_rate_stat.pass(dwLength);

        spc_tx_tid_rate_sttable.begin(dwTID);
        spc_tx_tid_rate_sttable.pass(dwTID, dwLength);

        return true;
    }
    return false;
}

bool
SendTIDCommandWithoutSpeedControl(DWORD dwTID, const std::shared_ptr<BYTE> &pBuffer, DWORD dwLength, bool isRBUDP) {
    static DWORD send_command_count = 0;
    static DWORD number_of_send_thousand = 0;
    if (SpeedControl::gSpeedControlManager) {
        bool bResult = SpeedControl::gSpeedControlManager->SendTIDCommandWithoutSpeedControl(dwTID, pBuffer, dwLength,
                                                                                             isRBUDP);
        //------- infohub -------------
        if(true == bResult) {
            spc_tx_rate_stat.pass(dwLength);
            if(false==isRBUDP) spc_mrudp_tx_rate_stat.pass(dwLength);
            else if(true==isRBUDP) spc_rbudp_tx_rate_stat.pass(dwLength);

            spc_tx_tid_rate_sttable.begin(dwTID);
            spc_tx_tid_rate_sttable.pass(dwTID, dwLength);
        }

        return bResult;
    }
    return false;
}


bool RegisterSpeedControlQoSSelf2RemoteInfoCallBack(pSpeedControlSelfToRemoteQosInfoCallBack pFunc) {
    if (nullptr != pFunc) {
        SpeedControl::g_p_qos_self_to_remote_func = pFunc;
        return true;
    }
    return false;
}


bool RegisterSpeedControlQoSRemote2SelfInfoCallBack(pSpeedControlRemoteToSelfQosInfoCallBack pFunc) {
    if (nullptr != pFunc) {
        SpeedControl::g_p_qos_remote_to_self_func = pFunc;
        return true;
    }
    return false;
}

bool RegisterSpeedControlTermOnOffCallBack(pSpeedControlTermOnOffCallBack pFunc) {
    if (nullptr == pFunc ||
        (SpeedControl::SPEEDCONTROL_TERM_ON_OFF_ALREADY_NUM >= SpeedControl::SPEED_CONTROL_MAX_REGISTER_DATA_FUNC)) {
        return false;
    }
    SpeedControl::g_p_term_onoff_func[SpeedControl::SPEEDCONTROL_TERM_ON_OFF_ALREADY_NUM] = pFunc;
    SpeedControl::SPEEDCONTROL_TERM_ON_OFF_ALREADY_NUM++;
    return true;
}

using SpeedControl::SPEEDCONTROL_RBUDP_ALREADY_NUM;
using SpeedControl::SPEED_CONTROL_MAX_REGISTER_DATA_FUNC;

bool RegisterSpeedControlRBUDPDataCallBack(pSpeedControlRBUDPDataCallBack pFunc) {
    if (nullptr == pFunc || (SPEEDCONTROL_RBUDP_ALREADY_NUM >= SPEED_CONTROL_MAX_REGISTER_DATA_FUNC)) {
        return false;
    }
    SpeedControl::g_p_rbudp_data_func[SPEEDCONTROL_RBUDP_ALREADY_NUM] = pFunc;
    SPEEDCONTROL_RBUDP_ALREADY_NUM++;
    return true;
}

bool Register_SpeedControl_RecvDataCallBack(pSpeedControlRecvDataCallBack pFunc) {

    if (SpeedControl::g_d_current_register_func_num > SpeedControl::SPEED_CONTROL_MAX_REGISTER_DATA_FUNC ||
        nullptr == pFunc) {
        return false;    //数据回调函数已经注册满了
    }

    SpeedControl::g_p_data_func[SpeedControl::g_d_current_register_func_num] = pFunc;
    SpeedControl::g_d_current_register_func_num++;
    return true;
}

/**
 * 下面是一些调用全局回调函数的全局方法
 */
//调用所有的数据回调函数

void CallAllDataFunc(DWORD dwTID, const std::shared_ptr<BYTE> &pBuffer, DWORD dwLength, int64_t &recvtime,
                     int64_t &fb_Send_time,bool isRBUDP) {
/*
#ifdef CallAllDataFunc_FREQ
    g_speedcontrol_data_callback_statistics.ComputeOnce();
#endif // CallAllDataFunc_FREQ
*/
    //------- infohub -------------
    spc_rx_rate_stat.pass(dwLength);
    if(false==isRBUDP) spc_mrudp_rx_rate_stat.pass(dwLength);
    else if(true==isRBUDP) spc_rbudp_rx_rate_stat.pass(dwLength);

    spc_rx_tid_rate_sttable.begin(dwTID);
    spc_rx_tid_rate_sttable.pass(dwTID, dwLength);
    
    if(isRBUDP){
        SpeedControl::g_p_data_func[1](dwTID, pBuffer, dwLength, recvtime, fb_Send_time);
        //printf("<A>|| SpeedControl CallAllDataFunc: RUDP g_p_data_func[1]\n");
    }
    else{
        SpeedControl::g_p_data_func[0](dwTID, pBuffer, dwLength, recvtime, fb_Send_time);
        //printf("<A>|| SpeedControl CallAllDataFunc: MRUDP g_p_data_func[0]\n");
    }
}

//调用QoS注册的两个回调函数的全局函数
void CallQoSSelf2RemoteInfoFunc(std::shared_ptr<const SelfToRemoteQosInfo> p_self_to_remote_qos_info) {
    if (nullptr != SpeedControl::g_p_qos_self_to_remote_func) {
        SpeedControl::g_p_qos_self_to_remote_func(std::move(p_self_to_remote_qos_info));
    }
}

void CallQoSRemote2SelfInfoFunc(std::shared_ptr<const RemoteToSelfQosInfo> p_remote_to_self_qos_info) {
    if (nullptr != SpeedControl::g_p_qos_remote_to_self_func) {
        SpeedControl::g_p_qos_remote_to_self_func(std::move(p_remote_to_self_qos_info));
    }
}


void QoSSelf2RemoteInfo(std::shared_ptr<const SelfToRemoteQosInfo> p_self_to_remote_qos_info) {
    CallQoSSelf2RemoteInfoFunc(std::move(p_self_to_remote_qos_info));
}

void QoSRemote2SelfInfo(std::shared_ptr<const RemoteToSelfQosInfo> p_remote_to_self_qos_info) {
    CallQoSRemote2SelfInfoFunc(std::move(p_remote_to_self_qos_info));
}

bool SpeedControl_QoSSpeedInfoCallBack(TermSpeedInfo term_speed_info);

bool SpeedControlQoSSpeedInfo(TermSpeedInfo term_speed_info) {
    return SpeedControl_QoSSpeedInfoCallBack(term_speed_info);
}

void SpeedControl_QoSTermOnOffCallBack(DWORD dwTID, bool bCreate) {
    static DWORD i = 0;
    for (i = 0; i < SpeedControl::SPEED_CONTROL_MAX_REGISTER_DATA_FUNC; i++) {
        if (SpeedControl::g_p_term_onoff_func[i]) {
            SpeedControl::g_p_term_onoff_func[i](dwTID, bCreate);
        }
    }
}

void SpeedControl_RBUDPIndexCallBack(DWORD dwTID, DWORD dIndex) {
    static DWORD i = 0;
    for (i = 0; i < SPEEDCONTROL_RBUDP_ALREADY_NUM; i++) {
        SpeedControl::g_p_rbudp_data_func[i](dwTID, dIndex);
    }
}

bool ExChangeTransSpeedOfTID(DWORD nTID, DWORD nExpectSpeed) {
    std::shared_ptr<SpeedControl::Term2TermTransmission> p_temp_t2t;
    p_temp_t2t = SpeedControl::gSpeedControlManager->GetTerm2TermTransMission(nTID);
    if (p_temp_t2t && p_temp_t2t.get()) {
        p_temp_t2t->DoChangeExpectSpeed(nExpectSpeed * 1000000);
        printf("和%d之间的通信链路已经被设置了最新的传输速度\n", nTID);
        return true;
    }
    return false;
}
