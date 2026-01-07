//
// Created by 王炳棋 on 2022/11/19.
//
//
//
#include "NetCombTransferApi.h"
#include <utility>
#include "CNetTerminalManager.h"

#include "infoHub/infoHub.h"
#include "infoHub/rateStatistic.h"
#include "infoHub/rateStatisticTable.h"

//----------------------infoHub(信息仓库)初始化---------------------------------
/*netcombtransfer模块的信息需求清单
  1. 该层总的传输速率：
        nct_rx_rate,                            (value名, 类型为i, 单位)
        nct_tx_rate                             (value名, 类型为i, 单位)
  2. 各个通道的传输速率:
        map<tid, rate> rx_tid_rate_map,     (table名, 类型i2i, 单位)
        map<tid, rate> tx_tid_rate_map      (table名, 类型i2i, 单位)
  3. 各个终端对应的通道:
        map<tid, list<cid>> tid_cids_map;     (table名, 类型i2s, 单位)
  4. 当前的路由表信息
        1）源能到哪些目的终端：
           map<src_tid, dst_tids> router_src_dsts_map
        2）能到目的的有哪些源：
           map<dst_tid, src_tids> router_dst_srcs_map
        3）特定源到目的终端的跳数
           map<pair<src_tid, dst_tid>, nop> router_nop_map
  4. QOS参考信息
        map<tid, rate> qos_tx_tid_rate_map  (table名, 类型i2i, 单位)
        map<tid, rtt> qos_tid_rtt_map       (table名, 类型i2i, 单位)
        map<tid, delay> qos_tid_delay_map   (table名, 类型i2i, 单位)
        map<tid, loss> qos_tid_loss_map     (table名, 类型i2i, 单位)

  * key名，即为value名，table名
  * 上述table的field名即为tid
  * module名为netcombtransfer
*/
std::shared_ptr<ec2::infoHub>\
        _netcombtransfer_infohub_instance = \
            ec2::infoHub::getInstance();

// tid cids table : tid_cids_sttable

ec2::rateStatistic      nct_rx_rate_stat; //nct means net comb transfer
ec2::rateStatistic      nct_tx_rate_stat;

ec2::rateStatistic      nct_msg_rx_rate_stat; //nct means net comb transfer
ec2::rateStatistic      nct_data_rx_rate_stat; //nct means net comb transfer

ec2::rateStatistic      nct_msg_tx_rate_stat;
ec2::rateStatistic      nct_data_tx_rate_stat;


ec2::rateStatisticTable rx_tid_rate_sttable;
ec2::rateStatisticTable tx_tid_rate_sttable;

//------------------ infoHub(信息仓库)初始化 End --------------------------


namespace NETCOMBTRANSFER {
}

std::shared_ptr<CNetTerminalManager> gNetTerminalManager = nullptr; 


bool Init_NetCombTransfer(DWORD dwTID) {
    try {

        bool ret = false;

        gNetTerminalManager = std::make_shared<CNetTerminalManager>(dwTID);
        if (nullptr == gNetTerminalManager) {
            //说明初始化失败了，就直接返回false
            ret = false;
        } else {
            printf("transferAPI:init\n");
            gNetTerminalManager->Init();

            // ---- infohub -----
            nct_rx_rate_stat.init("netcombtransfer", "rx_rate_stat");
            nct_tx_rate_stat.init("netcombtransfer", "tx_rate_stat");

            nct_msg_rx_rate_stat.init("netcombtransfer", "msg_rx_rate_stat"); 
            nct_data_rx_rate_stat.init("netcombtransfer", "data_rx_rate_stat"); 

            nct_msg_tx_rate_stat.init("netcombtransfer", "msg_tx_rate_stat");
            nct_data_tx_rate_stat.init("netcombtransfer", "data_tx_rate_stat");

            rx_tid_rate_sttable.init("netcombtransfer", "rx_tid_rate_sttable");
            tx_tid_rate_sttable.init("netcombtransfer", "tx_tid_rate_sttable");

            nct_rx_rate_stat.begin();
            nct_tx_rate_stat.begin();

            nct_msg_rx_rate_stat.begin(); 
            nct_data_rx_rate_stat.begin(); 

            nct_msg_tx_rate_stat.begin();
            nct_data_tx_rate_stat.begin();

            ret = true;
        }
        return ret;


    } catch (...) {
        gNetTerminalManager = nullptr;
        return false;
    }
}

bool UnInit_NetCombTransfer() {
    bool bOK = true;
    //printf("[NetCombTransfer]::开始注销模块...\n");
    if (gNetTerminalManager) {
        bOK = gNetTerminalManager->UnInit();
        gNetTerminalManager = nullptr;
    }


    //printf("\nNetCombTransfer模块注销完成!!!\n\n");
    return bOK;
}

//TODO:这里是修改
//供其他模块调用以注册终端列表回调函数
bool Register_TerminalListCallback(pNetMessageCallback p_net_message_callback){
    gNetTerminalManager->DoRegister_TerminalCallBack(p_net_message_callback);
    return true;
}

bool Register_NetCombTransferCallback(pNetCombTransferMessageCallback p_msg_callback,
                                      pNetCombTransferBufferCallback p_buffer_callback) {
    bool ret = false;

    if (gNetTerminalManager) {
        ret = gNetTerminalManager->DoRegister_NetCombTransferCallBack(p_msg_callback, p_buffer_callback);
    }

    return ret;
}

bool Register_NetCombTransferRouterCallback(pNetCombTransferRouterNotify p_router_notify) {
    bool ret = false;

    if (gNetTerminalManager) {
        ret = gNetTerminalManager->DoRegister_NetCombTransferRouterCallBack(p_router_notify);
    }

    return ret;
}

bool SendTIDData(DWORD TID, const std::shared_ptr<BYTE> &buffer, DWORD length, bool bBeTransmitted) {
    bool bOK = false;
    if (gNetTerminalManager) {
        //跨tid的传输
        bOK = gNetTerminalManager->SendData(TID, buffer, length, bBeTransmitted);

        //-----infohub ---------
        nct_tx_rate_stat.pass(length);
        nct_data_tx_rate_stat.pass(length);
    }

    return bOK;
}

bool SendTIDCommand(DWORD TID, const std::shared_ptr<BYTE> &buffer, DWORD length) {
    bool bOK = false;
    if (gNetTerminalManager) {
        //跨tid的传输
        bOK = gNetTerminalManager->SendMsg(TID, buffer, length, false);

        //-----infohub ---------
        nct_tx_rate_stat.pass(length);
        nct_msg_tx_rate_stat.pass(length);
    }
    return bOK;
}

//全局命令发送函数
bool globalNetCombTransferSendCommand(const std::shared_ptr<CIMsg> &pMsg, DWORD nCID) {
    bool bOK = false;
    if (gNetTerminalManager) {
        //跨cid的传输
        bOK = gNetTerminalManager->SendCommand(pMsg, nCID);
    }
    return bOK;
}

//全局消息通知函数
bool globalNetCombTransferRecvMessage(DWORD nCID, bool bCreate) {
    bool bOK = false;
    if (gNetTerminalManager) {
        printf("transferAPI:globalNetCombTransferRecvMessage.1\n");
        bOK = gNetTerminalManager->Recv_NetCommMessageNotify(nCID, bCreate);
    }
    return bOK;
}

bool NetCombTransferGetTcpChannelClose(DWORD nCID, bool bCreate) {
    bool bOK = false;
    if (gNetTerminalManager) {
        gNetTerminalManager->TcpChannelActiveCloseByNetComm(nCID, bCreate);
    }
    return bOK;
}

//全局数据通知函数
bool
globalNetCombTransferRecvCommand(DWORD nCID, std::shared_ptr<BYTE> pBuffer, DWORD nLength, bool isNeedSlit) {
    bool bOK = false;
    if (gNetTerminalManager) {
        bOK = gNetTerminalManager->Recv_NetCommBufferNotify(nCID, pBuffer, nLength, isNeedSlit);

        //-----infohub ---------
        //nct_rx_rate_stat.pass(nLength);
    }
    return bOK;
}

//全局函数，临时终端状态遍历
bool globalNetCombTransferTerminalStateTraverse() {
    bool bOK = false;
    if (gNetTerminalManager) {
        bOK = gNetTerminalManager->TmpTerminalStateTraverse();
    }
    return bOK;
}

void PrintAllRouters() {
    if (gNetTerminalManager) {
        gNetTerminalManager->m_RouterInfoList.Lock();
        for (const auto &pRouterInfo: gNetTerminalManager->m_RouterInfoList.node_list) {
            if (pRouterInfo && pRouterInfo.get()) {
            } else {
                gNetTerminalManager->m_RouterInfoList.UnLock();
                return;
            }
        }
        gNetTerminalManager->m_RouterInfoList.UnLock();
    }
}

//test:遍历终端列表，看创建server或client时，是否加入终端列表
void traverseAllTerminals(){
    printf("API:执行。\n");
    if(!gNetTerminalManager->m_TerminalList.GetMyHead()){
        printf("不存在终端。\n");
        return;
    }
    for(const auto pTerminal:gNetTerminalManager->m_TerminalList.node_list){
        if(pTerminal){
            std::cout<<"存在终端，本地地址为："<<pTerminal->NetChannelList.GetMyHead()->m_szLocalAddress<<std::endl;
        }
    }
    return;
}

int Manager_GetLocalPort(){
    return gNetTerminalManager->m_pDTUVirtualServer->GetLocalPort();
}
/*
//获取全部连接终端
std:vector<std::shared_ptr<CNetTerminal>> getAllClient(){
    std::vector<std::shared_ptr<CNetTerminal>> terminals;
    gNetTerminalManager->m_TerminalList.Lock();
    for(std::shared_ptr<CNetTerminal> tempTerminal:gNetTerminalManager->m_TerminalList.node_list){
        terminals.push_back(tempTerminal);
    }
    gNetTerminalManager->m_TerminalList.UnLock();
    return terminals;
}*/
