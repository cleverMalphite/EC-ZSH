//
// Created by 王炳棋 on 2022/9/30.
//

#include "./EpollCommApi.h"
#include <utility>
#include "CNetCommEpoll.h"
#include <pthread.h>

#include "infoHub/infoHub.h"
#include "infoHub/rateStatistic.h"
#include "infoHub/rateStatisticTable.h"

//----------------------infoHub(信息仓库)初始化---------------------------------
/*epollcomm模块的信息需求清单
  1. 该层总的传输速率：
        rx_rate,                            (value名, 类型为i, 单位)
        tx_rate                             (value名, 类型为i, 单位)
  2. 各个通道的传输速率:
        map<cid, rate> rx_cid_rate_map,     (table名, 类型i2i, 单位)
        map<cid, rate> tx_cid_rate_map      (table名, 类型i2i, 单位)
  3. 各个通道对应的ip地址:
        map<cid, str_ip> cid_strip_map;     (table名, 类型i2s, 单位)
  4. QOS参考信息
        map<cid, rate> qos_tx_cid_rate_map  (table名, 类型i2i, 单位)
        map<cid, rtt> qos_cid_rtt_map       (table名, 类型i2i, 单位)
        map<cid, delay> qos_cid_delay_map   (table名, 类型i2i, 单位)
        map<cid, loss> qos_cid_loss_map     (table名, 类型i2i, 单位)

  * key名，即为value名，table名
  * 上述table的field名即为cid
  * module名为epollcomm
*/
std::shared_ptr<ec2::infoHub>\
        _epollcomm_infohub_instance = \
            ec2::infoHub::getInstance();

// cid ip table : cid_strip_sttable

ec2::rateStatistic      rx_rate_stat;
ec2::rateStatistic      tx_rate_stat;

ec2::rateStatisticTable rx_cid_rate_sttable;
ec2::rateStatisticTable tx_cid_rate_sttable;

//qos_tx_rx_rtt_stt     :   qos_tx_rx_rtt_stat
//qos_tx_rx_loss_stat   :   qos_tx_rx_loss_stat

//qos tx cid rtt table:  qos_tx_cid_rtt_sttable;
//qos tx cid loss table: qos_tx_cid_loss_sttable;

//qos rx cid rtt table:  qos_rx_cid_rtt_sttable;
//qos rx cid loss table: qos_rx_cid_loss_sttable;



//----------------- infoHub(信息仓库)初始化 End -------------------------



/*std::shared_ptr<CNetCommEpoll> NetCommManager = std::make_shared<CNetCommEpoll>();
CNetCommEpoll *gNetCommManager = NetCommManager.get();*/
CNetCommEpoll *gNetCommManager = nullptr;
static pthread_t gTcpWorker;
static pthread_t gUdpWorker[UDP_MAX_THREADNUM];
static pthread_t gBCastWorker;

//todo:worker线程，修改成：事件线程+处理线程
void *TcpWorkerThread(void *argNeeded);
void *UdpWorkerThread(void *argNeeded);
void *BCastWorkerThread(void *argNeeded);

bool
Init_NetComm(PMESSAGECALLBACK pMESSAGECALLBACK, PMESSAGECALLBACK pMESSAGECALLBACK2, PDATACALLBACK pDATACALLBACK,
             unsigned UdpThreadNum) {
    if (gNetCommManager) {
        printf("EPollThreads.cpp:Init_NetCOmm.3\n");
        return true;//判断Manager是否初始化了，如果已经初始化则直接返回true；
    }
    int CreateErr = 0;
    gNetCommManager = new CNetCommEpoll();
    if (gNetCommManager) {
        printf("EPollThreads.cpp:Init_NetCOmm.1\n");

        //------------ infohub init -----------------
        rx_rate_stat.init("epollcomm", "rx_rate_stat");
        tx_rate_stat.init("epollcomm", "tx_rate_stat");

        rx_cid_rate_sttable.init("epollcomm", "rx_cid_rate_sttable");
        tx_cid_rate_sttable.init("epollcomm", "tx_cid_rate_sttable");

        rx_rate_stat.begin();
        tx_rate_stat.begin();

        gNetCommManager->SetInterfaceParam(pMESSAGECALLBACK, pMESSAGECALLBACK2, pDATACALLBACK);
        //线程创建成功返回0,任何线程创建失败时都会返回false,错误代码暂存在CreateErr
        CreateErr = pthread_create(&gTcpWorker, nullptr, TcpWorkerThread, gNetCommManager);
        if (CreateErr) { printf("EpollThreas.cpp:Init_NetCOmm.2\n"); return false;}
        for (int i = 0; i < UdpThreadNum; i++) {
            CreateErr = pthread_create(&gUdpWorker[i], nullptr, UdpWorkerThread, gNetCommManager);
            if (CreateErr) return false;
        }
        CreateErr = pthread_create(&gBCastWorker, nullptr, BCastWorkerThread, gNetCommManager);
        if (CreateErr) return false;
    } else {
        return false;
    }
    return true;
}

bool UnInit_NetComm() {
    DWORD dwExitCode = 0;
    if (gNetCommManager) {
        gNetCommManager->m_bInit = false;
        usleep(500 * 1000);
        gNetCommManager->CloseAllSocket();
    }
    if (gTcpWorker != 0) {
        pthread_cancel(gTcpWorker);
        //printf("\n等待Tcp线程终止...\n");
        pthread_join(gTcpWorker, nullptr);
        gTcpWorker = 0;
    }
    for (auto &TempUdpWorker: gUdpWorker) {
        if (TempUdpWorker != 0) {
            pthread_cancel(TempUdpWorker);
            //printf("等待Udp线程终止...\n");
            pthread_join(TempUdpWorker, nullptr);
            TempUdpWorker = 0;
        }
    }
    if (gBCastWorker != 0) {
        pthread_cancel(gBCastWorker);
        //printf("等待BCast线程终止...\n");
        pthread_join(gBCastWorker, nullptr);
        gBCastWorker = 0;
    }
    close(gNetCommManager->TcpEpollFd);
    close(gNetCommManager->UdpEpollFd);
    close(gNetCommManager->BCastEpollFd);
    delete gNetCommManager;
    return true;
}

/*
 * Tcp Channel create
 */

bool
CreateTcpServerChannel(int &nLocalPort, const std::string &szLocalAddress, DWORD ChannelHead, DWORD ChannelNumber,
                       bool isForceLocalPort) {
    bool ret = false;
    if (ChannelNumber < 1 || ChannelHead < TCP_CHANNEL_FIRST_ID || ChannelHead > TCP_CHANNEL_FIRST_ID_MAX) {
//        printf("Server Channel Created Failed\n");
        return ret;
    }
    if (gNetCommManager) {
        //注释部分为原NetComm通道管理机制
        /*shared_ptr<CTCP> TcpOld = gNetCommManager->mTcpList.GetByChannelNum(ChannelHead);
        if (TcpOld) {
            printf("Channel {%ld} has existed\n", ChannelHead);
            return true;
        }*/
        /*new channel manager*/
        //todo:22/11/04
        for (DWORD i = ChannelHead; i != (ChannelHead + ChannelNumber); i++) {
            if (!gNetCommManager->gChannelManager->IsServerAvailable(i)) {
//                printf("Server does not have enough channels\n");
                return false;
            }
        }
        //划定了Server的管理范围
        gNetCommManager->gChannelManager->SetChannelStat(ChannelHead, (ChannelHead + ChannelNumber), ServerChannel);
        gNetCommManager->gChannelManager->SetChannelStat(ChannelHead, ServerListenChannel);
        //判断server设定的通道范围内通道是否全部可用，并设置其为ServerChannel
        std::shared_ptr<CTCP> TempTcpServer = gNetCommManager->CreateTCP();
        if (TempTcpServer) {
//            printf("Create TCP Server\n");
            if (isForceLocalPort) {
                TempTcpServer->tcp_init(nLocalPort, 0, szLocalAddress, "");
            } else {
                TempTcpServer->tcp_init(0, 0, szLocalAddress, "");
            }
            TempTcpServer->SetChannel_TcpList(ChannelHead, ChannelNumber);//设置了Server内部的mChannelList
            ret = TempTcpServer->tcp_open(true);
            if (!ret) {
                /*if (TempTcpServer->tcp_close()) {
                    gNetCommManager->DeleteTCP(TempTcpServer);
                }*/
                TempTcpServer->tcp_close();
                gNetCommManager->DeleteTCP(TempTcpServer);
            }
        }
    }
    if (!ret) {
//        printf("CloseTcpChannel\n");
        CloseTcpChannel(ChannelHead);
    }
    return ret;
}

bool CreateTcpClientChannel(int &nLocalPort, int nRemotePort,
                            const std::string &szLocalAddress,
                            const std::string &szRemoteAddress, DWORD dwChannel,
                            bool isForceLocalPort) {
    bool ret = false;
    if (dwChannel < TCP_CHANNEL_FIRST_ID || dwChannel > TCP_CHANNEL_FIRST_ID_MAX) {
        return ret;
    }
    if (gNetCommManager) {
        /*shared_ptr<CTCP> TcpOld = gNetCommManager->mTcpList.GetByChannelNum(dwChannel);
        if (TcpOld) {
            printf("Channel {%ld} has existed\n", dwChannel);
            return true;
        }*/
        //todo:22/11/04
        if (!gNetCommManager->gChannelManager->IsClientAvailable(dwChannel)) {
            //printf("[AutoTestDebug]::here is already a ClientChannel or ServerChannel\n");
            return false;
        }
        gNetCommManager->gChannelManager->SetChannelStat(dwChannel, ClientChannel);
        //判断是否可用为ClientChannel，如果可用则设置成clientchannel
        shared_ptr<CTCP> TempTcpClient = gNetCommManager->CreateTCP();
        if (TempTcpClient) {
            if (isForceLocalPort) {
                TempTcpClient->tcp_init(nLocalPort, nRemotePort, szLocalAddress, szRemoteAddress);
            } else {
                //printf("LocalPort is Not Forced\n");
                TempTcpClient->tcp_init(0, nRemotePort, szLocalAddress, szRemoteAddress);
            }
            TempTcpClient->SetChannel(dwChannel);
            ret = TempTcpClient->tcp_open(false);
            //printf("[AutoTestDebug]::TcpChannelClose, bRet is %d\n", ret);
            if (!ret) {
                if (TempTcpClient->tcp_close()) {
                    gNetCommManager->DeleteTCP(TempTcpClient);
                    //printf("[Warn]::Created TcpClientChannel Failed {%d}\n", dwChannel);
                }
            } else {
                //printf("[Info]::Created TcpClientChannel Success {%d}\n", dwChannel);

                // ----------- infohub -------------
                rx_cid_rate_sttable.begin(dwChannel);
                tx_cid_rate_sttable.begin(dwChannel);
            }
        }
    }
    if (!ret) {
        CloseTcpChannel(dwChannel);
    }
    return ret;
}


/*
 * DTU Tcp Channel create
 */
bool
CreateDtuTcpServerChannel(int &nLocalPort, const std::string &szLocalAddress, 
                          DWORD ChannelHead, DWORD ChannelNumber) {
    bool ret = false;
    if (ChannelNumber < 1 || ChannelHead < TCP_CHANNEL_FIRST_ID || ChannelHead > TCP_CHANNEL_FIRST_ID_MAX) {
//        printf("Server Channel Created Failed\n");
        return ret;
    }
    if (gNetCommManager) {
        //注释部分为原NetComm通道管理机制
        /*shared_ptr<CTCP> TcpOld = gNetCommManager->mTcpList.GetByChannelNum(ChannelHead);
        if (TcpOld) {
            printf("Channel {%ld} has existed\n", ChannelHead);
            return true;
        }*/
        /*new channel manager*/
        //todo:22/11/04
        for (DWORD i = ChannelHead; i != (ChannelHead + ChannelNumber); i++) {
            if (!gNetCommManager->gChannelManager->IsServerAvailable(i)) {
//                printf("Server does not have enough channels\n");
                return false;
            }
        }
        //划定了Server的管理范围
        gNetCommManager->gChannelManager->SetChannelStat(ChannelHead, (ChannelHead + ChannelNumber), ServerChannel);
        gNetCommManager->gChannelManager->SetChannelStat(ChannelHead, ServerListenChannel);
        //判断server设定的通道范围内通道是否全部可用，并设置其为ServerChannel
        std::shared_ptr<CTCP> TempTcpServer = gNetCommManager->CreateTCP();
        if (TempTcpServer) {
//            printf("Create TCP Server\n");
            TempTcpServer->tcp_init(nLocalPort, 0, szLocalAddress, "");
            TempTcpServer->SetChannel_TcpList(ChannelHead, ChannelNumber);//设置了Server内部的mChannelList

            TempTcpServer->SetIsDTUChannel(true);

            ret = TempTcpServer->tcp_open(true);
            if (!ret) {
                /*if (TempTcpServer->tcp_close()) {
                    gNetCommManager->DeleteTCP(TempTcpServer);
                }*/
                TempTcpServer->tcp_close();
                gNetCommManager->DeleteTCP(TempTcpServer);
            }
            else{
                // ----------- infohub -------------
                rx_cid_rate_sttable.begin(ChannelHead);
                tx_cid_rate_sttable.begin(ChannelHead);
            }
        }
    }
    if (!ret) {
//        printf("CloseTcpChannel\n");
        CloseTcpChannel(ChannelHead);
    }
    return ret;
}

bool CreateDtuTcpClientChannel(int &nLocalPort, int nRemotePort,
                            const std::string &szLocalAddress,
                            const std::string &szRemoteAddress, DWORD dwChannel) {
    bool ret = false;
    if (dwChannel < TCP_CHANNEL_FIRST_ID || dwChannel > TCP_CHANNEL_FIRST_ID_MAX) {
        return ret;
    }
    if (gNetCommManager) {
        /*shared_ptr<CTCP> TcpOld = gNetCommManager->mTcpList.GetByChannelNum(dwChannel);
        if (TcpOld) {
            printf("Channel {%ld} has existed\n", dwChannel);
            return true;
        }*/
        //todo:22/11/04
        if (!gNetCommManager->gChannelManager->IsClientAvailable(dwChannel)) {
            //printf("[AutoTestDebug]::here is already a ClientChannel or ServerChannel\n");
            return false;
        }
        gNetCommManager->gChannelManager->SetChannelStat(dwChannel, ClientChannel);
        //判断是否可用为ClientChannel，如果可用则设置成clientchannel
        shared_ptr<CTCP> TempTcpClient = gNetCommManager->CreateTCP();
        if (TempTcpClient) {
            TempTcpClient->tcp_init(nLocalPort, nRemotePort, szLocalAddress, szRemoteAddress);
            TempTcpClient->SetChannel(dwChannel);
            
            TempTcpClient->SetIsDTUChannel(true);

            ret = TempTcpClient->tcp_open(false);
            //printf("[AutoTestDebug]::TcpChannelClose, bRet is %d\n", ret);
            if (!ret) {
                if (TempTcpClient->tcp_close()) {
                    gNetCommManager->DeleteTCP(TempTcpClient);
                    //printf("[Warn]::Created TcpClientChannel Failed {%d}\n", dwChannel);
                }
            } else {
                //printf("[Info]::Created TcpClientChannel Success {%d}\n", dwChannel);
                // ----------- infohub -------------
                rx_cid_rate_sttable.begin(dwChannel);
                tx_cid_rate_sttable.begin(dwChannel);
            }
        }
    }
    if (!ret) {
        CloseTcpChannel(dwChannel);
    }
    return ret;
}




bool
CreateUdpServerChannel(DWORD &nLocalPort, const std::string &szLocalAddress, DWORD dwChannel, bool isForceLocalPort) {
    bool ret = false;
    if (gNetCommManager) {
        //先看看已有的UDP队列里有没有已经生成了的udp数据链路
        gNetCommManager->gChannelManager->lock();
        if (!gNetCommManager->gChannelManager->IsClientAvailable(dwChannel)) {
            gNetCommManager->gChannelManager->unlock();
            //printf("This Channel Cannot use by UDPServer\n");
            return false;//创建失败
        }
        /*std::shared_ptr<CUDP> pUdpOld = gNetCommManager->GetUDP(dwChannel);
        if (pUdpOld) {
            //如果有已经生成了的链路，那么直接返回true
            printf("this channel has already exist\n");
            return true;
        }*/
        gNetCommManager->gChannelManager->SetChannelStat(dwChannel, ClientChannel);
        gNetCommManager->gChannelManager->unlock();
        std::shared_ptr<CUDP> UdpChannel = gNetCommManager->CreateUDP();
        if (UdpChannel) {
            if (isForceLocalPort) {
                UdpChannel->udp_init(nLocalPort, 0, szLocalAddress, "");
            } else {
                nLocalPort = 0;
                UdpChannel->udp_init(nLocalPort, 0, szLocalAddress, "");
            }
            UdpChannel->SetChannel(dwChannel);
            UdpChannel->m_bIsUdpServer = true;
            ret = UdpChannel->udp_open(true);
            if (ret /*&& 0 == nLocalPort*/) {
                getsockname(UdpChannel->mLocalSock, (sockaddr *) &UdpChannel->mLocalAddr, &UdpChannel->mLocalAddrLen);
                UdpChannel->mLocalPort = ntohs(UdpChannel->mLocalAddr.sin_port);
                /*nLocalPort = UdpChannel->mLocalAddr.sin_port;*/
            }
            if (!ret) {
                //printf("Net open failed, start close...\n");
                if (UdpChannel->udp_close()) {
                    gNetCommManager->gChannelManager->SetChannelStat(dwChannel, UnKnowChannel);
                    gNetCommManager->DeleteUDP(UdpChannel);
                }
            }
        }
    } else {
        //todo:CloseUDPChannel();
        CloseUdpChannel(dwChannel);
    }
    //printf("[AutoTestDebug]::创建了通道号为%d的UDP通道\n", dwChannel);
    return ret;
}

//todo使用新的通道管理机制进行改进
bool CreateUdpClientChannel(DWORD &nLocalPort, UINT nRemotePort,
                            const std::string &szLocalAddress,
                            const std::string &szRemoteAddress, DWORD dwChannel,
                            bool isForceLocalPort) {
    bool ret = false;
    if (gNetCommManager) {
        //先看看已有的UDP队列里有没有已经生成了的udp数据链路
        std::shared_ptr<CUDP> pUdpOld = gNetCommManager->GetUDP(dwChannel);
        if (pUdpOld) {
            //如果有已经生成了的链路，那么直接返回true
            //printf("this channel has already exist\n");
            return true;
        }
        std::shared_ptr<CUDP> UdpChannel = gNetCommManager->CreateUDP();
        if (UdpChannel) {
            if (isForceLocalPort) {
                UdpChannel->udp_init(nLocalPort, nRemotePort, szLocalAddress, szRemoteAddress);
            } else {
                nLocalPort = 0;
                UdpChannel->udp_init(nLocalPort, nRemotePort, szLocalAddress, szRemoteAddress);
            }
            UdpChannel->SetChannel(dwChannel);
            UdpChannel->m_bIsUdpClient = true;
            ret = UdpChannel->udp_open(false);
            if (!ret) {
                //printf("Net open failed, start close...\n");
                if (UdpChannel->udp_close()) {
                    //这里的udp_close()没写完善，在其中并没有关闭udpsock的操作，仅仅改变了一些状态量
                    gNetCommManager->DeleteUDP(UdpChannel);
                }
            }
        }
    } else {
        //todo:CloseUDPChannel();
        CloseUdpChannel(dwChannel);
    }
    return ret;
}

//todo:needtest：22/11/02
bool CreateBCastServerChannel(UINT nBCastPort, std::string szLocalIPAddress, std::string szBCastAddr, DWORD dwChannel) {
    bool ret = false;
    if (gNetCommManager) {
        std::shared_ptr<CBCast> pBCastOld = gNetCommManager->GetBCast(dwChannel);
        if (pBCastOld) {
            return true;
        }
        std::shared_ptr<CBCast> BCastChannel = gNetCommManager->CreateBCast();
        if (BCastChannel) {
            BCastChannel->bcast_init(nBCastPort, std::move(szLocalIPAddress), std::move(szBCastAddr));
            BCastChannel->SetChannel(dwChannel);
            BCastChannel->m_bIsUdpServer = true;
            ret = BCastChannel->bcast_open(true);
            if (!ret) {
                if (BCastChannel->bcast_close()) {
                    gNetCommManager->DeleteBCast(BCastChannel);
                }
            }
        } else {
            return false;
        }
        if (!ret) {
            CloseBCastChannel(dwChannel);
        }
    }
    return ret;
}

bool WriteTcpChannel(DWORD dwChannel, const std::shared_ptr<BYTE> &pBuffer, DWORD Length) {
    if (RESERVE_CHANNEL_BEGIN <= dwChannel && dwChannel <= RESERVE_CHANNEL_END) {
        return true;
    }
    if (nullptr != gNetCommManager) {

        //-------- infoHub -----
        tx_rate_stat.pass(Length);

        bool ret = gNetCommManager->WriteTcpChannel(dwChannel, pBuffer, Length);

        if(true==ret){
            //-------------- infohub ----------------
            rx_cid_rate_sttable.pass(dwChannel, Length);
            tx_cid_rate_sttable.pass(dwChannel, Length);
        }
        
        return ret;

    } else {
        return false;
    }
}

bool WriteUdpChannel(DWORD dwChannel, const std::shared_ptr<BYTE> &pBuffer, DWORD Length) {
    if (nullptr != gNetCommManager) {

        //-------- infoHub -----
        tx_rate_stat.pass(Length);

        bool ret = gNetCommManager->WriteUdpChannel(dwChannel, pBuffer, Length);

        if(true==ret){
            //-------------- infohub ----------------
            rx_cid_rate_sttable.pass(dwChannel, Length);
            tx_cid_rate_sttable.pass(dwChannel, Length);
        }

        return ret;
    } else {
        return false;
    }
}

bool WriteBCastChannel(DWORD dwChannel, std::shared_ptr<BYTE> pBuffer, DWORD Length) {
    if (nullptr != gNetCommManager) {

        //-------- infoHub -----
        tx_rate_stat.pass(Length);

        bool ret = gNetCommManager->WriteBCastChannel(dwChannel, std::move(pBuffer), Length);

        if(true==ret){
            //-------------- infohub ----------------
            rx_cid_rate_sttable.pass(dwChannel, Length);
            tx_cid_rate_sttable.pass(dwChannel, Length);
        }

        return ret;
    } else {
        return false;
    }
}

//todo:needTest：22/10/26
bool CloseTcpChannel(DWORD dwChannel) {
    bool ret = false;
    if (nullptr != gNetCommManager) {
        /*ret = gNetCommManager->CloseTcpChannel(dwChannel);*/
        ret = gNetCommManager->NewCloseTcpChannel(dwChannel);
    }
    if (!ret) {
        //printf("[NetCommEpoll API]:Close TCP Channel Failed\n");
        return false;
    }
    return true;
}

//todo:needtest：22/11/02
bool CloseUdpChannel(DWORD dwChannel) {
    //printf("[AutoTestDebug]::关闭UDP通道:%d\n", dwChannel);
    bool ret = false;
    if (nullptr != gNetCommManager) {
        ret = gNetCommManager->CloseUdpChannel(dwChannel);
    }
    if (!ret) {
        //printf("[NetCommEpoll API]:Close UDP Channel failed\n");
    }
    //gNetCommManager不存在，返回false
    return ret;
}

//todo:needtest：22/11/02
bool CloseBCastChannel(DWORD dwChannel) {
    if (nullptr != gNetCommManager) {
        std::shared_ptr<CBCast> BCastChannel = gNetCommManager->GetBCast(dwChannel);
        if (BCastChannel) {
            if (BCastChannel->bcast_close()) {
                gNetCommManager->DeleteBCast(BCastChannel);
                return true;
            }
            return false;
        }
        return true;
    }
    return false;
}

std::string GetLocalHostAddress(DWORD dwChannel, enumChannelType eNetType) {
    std::string ss;
    std::shared_ptr<CBCast> pBCastFind;
    if (nullptr != gNetCommManager) {
        switch (eNetType) {
            case InvalidChannel:
                break;
            case TCPChannel:
                ss = GetTCPLocalHostAddress(dwChannel);
                break;
            case UDPChannel:
                break;
            case MultiCastChannel:
                break;
            case CBCastChannel:
                pBCastFind = gNetCommManager->GetBCast(dwChannel);    //注意按照C++ 11及以上的标准，pBCastFind不能在switch里进行声明
                if (pBCastFind) {
                    ss = pBCastFind->GetLocalAddress();
                }
                break;
            default:
                break;
        }
    }
    return ss;
}

std::string GetTCPLocalHostAddress(DWORD dwChannel) {
    std::string ss;
    if (nullptr != gNetCommManager) {
        std::shared_ptr<CTCP> pTcpFind = gNetCommManager->GetTCP(dwChannel);
        if (pTcpFind && pTcpFind.get()) {
            ss = pTcpFind->GetLocalAddress();
        }
    }
    return ss;
}

unsigned int GetTCPLocalHostPort(DWORD dwChannel){
    unsigned int port;
    if(nullptr!=gNetCommManager){
        std::shared_ptr<CTCP> pTcpFind=gNetCommManager->GetTCP(dwChannel);
        if(pTcpFind&&pTcpFind.get()){
            port=pTcpFind->GetLocalPort();
            std::cout<<"localPort:"<<port<<std::endl;
        }
    }
    return port;
}

std::string GetTcpRemoteAddress(DWORD dwChannel) {
    std::string ss;
    if (nullptr != gNetCommManager) {
        std::shared_ptr<CTCP> pTcpFind = gNetCommManager->GetTCP(dwChannel);
        if (pTcpFind && pTcpFind.get()) {
            ss = pTcpFind->GetRemoteAddress();
        }
    }
    return ss;
}

DWORD GetUDPLocalPort(DWORD dwChannel) {
    if (nullptr != gNetCommManager) {
        std::shared_ptr<CUDP> TempUdp = gNetCommManager->GetUDP(dwChannel);
        if (TempUdp && TempUdp.get()) {
            //TODO：在不使用强制端口号时，NetComm能否及时更新mLocalPort为最新的端口？有待验证
            return TempUdp->GetLocalPort();
        }
        return -1;
    }
    return -1;
}

bool GetChannelDtuType(DWORD dwChannel) {
    bool ss;
    if (nullptr != gNetCommManager) {
        std::shared_ptr<CTCP> pTcpFind = gNetCommManager->GetTCP(dwChannel);
        if (pTcpFind && pTcpFind.get()) {
            ss = pTcpFind->GetIsDTUChannel();
        }
    }
    return ss;
}

void SetUdpServerChannelRemoteAddress(DWORD CID, std::string remote_address, DWORD remotePort) {
    if (nullptr != gNetCommManager) {
        gNetCommManager->SetUdpServerChannelRemoteAddress(CID, std::move(remote_address), remotePort);
    }
}

/*定义线程因非返回而终止时的清理函数*/
void TcpWorkerCleanUp(void *arg) {
    //printf("TCP线程的清理函数调用，线程终止\n");
}

void *TcpWorkerThread(void *argNeeded) {

    CNetCommEpoll *NetCommManager = (CNetCommEpoll *) argNeeded;
    //printf("[NetComm]:TcpWorker created\n");
    /*pthread_detach(pthread_self());*/
    int TcpEpollFd = NetCommManager->TcpEpollFd;
    /*std::unique_ptr<epoll_event[]> TcpEventList(new epoll_event[TcpMaxChannelNum]);*/
    std::unique_ptr<epoll_event[]> TcpEventList = std::make_unique<epoll_event[]>(TcpMaxChannelNum);
    int Event2Solve = 0;
    pthread_cleanup_push(TcpWorkerCleanUp, nullptr);
        while (1) {
            /*自定义函数取消点*/
            pthread_testcancel();
            pthread_setcanceltype(PTHREAD_CANCEL_DISABLE, nullptr);
            usleep(100);
            //printf("\nWait for an new event\n");
            Event2Solve = epoll_wait(TcpEpollFd, TcpEventList.get(), TcpMaxChannelNum, 1000);
            //printf("Get an new event\n\n");
            //printf("NewEventNum{%d},Events:%d\n", Event2Solve, TcpEventList[0].events);
            if (0 == Event2Solve) {
                pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, nullptr);
                continue;
            }
            NetCommManager->HandleTcpEvents(TcpEventList.get(), Event2Solve);
            pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, nullptr);
        }
    pthread_cleanup_pop(0);
    return nullptr;
}

/*定义线程因非返回而终止时的清理函数*/
void UdpWorkerCleanUp(void *arg) {
    //printf("UDP线程的清理函数调用，线程终止\n");
}

void *UdpWorkerThread(void *argNeeded) {

    CNetCommEpoll *NetCommManager = (CNetCommEpoll *) argNeeded;
    /*printf("[NetComm]:UdpWorker created,PID is %ld\n", pthread_self());*/
    /*pthread_detach(pthread_self());*/
    int UdpEpollFd = NetCommManager->UdpEpollFd;
    std::unique_ptr<epoll_event[]> UdpEventList = std::make_unique<epoll_event[]>(UdpMaxChannelNum);
    int Event2Solve = 0;
    pthread_cleanup_push(UdpWorkerCleanUp, nullptr);
        while (1) {
            /*自定义函数取消点*/
            pthread_testcancel();
            pthread_setcanceltype(PTHREAD_CANCEL_DISABLE, nullptr);
            //usleep(10);
            //printf("wait for new event\n");
            Event2Solve = epoll_wait(UdpEpollFd, UdpEventList.get(), UdpMaxChannelNum, 1000);
            if (0 == Event2Solve) {
                pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, nullptr);
                continue;
            }
            //printf("ThreadId:{%ld} NewEventNum{%d},Events:%d\n", pthread_self(), Event2Solve, UdpEventList[0].events);
            NetCommManager->HandleUdpEvents(UdpEventList.get(), Event2Solve);
            pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, nullptr);
        }
    pthread_cleanup_pop(0);
    return nullptr;
}

/*定义线程因非返回而终止时的清理函数*/
void BCastWorkerCleanUp(void *arg) {
    //printf("BCast线程的清理函数调用，线程终止\n");
}

void *BCastWorkerThread(void *argNeeded) {
    CNetCommEpoll *NetCommManager = (CNetCommEpoll *) argNeeded;
    /*printf("[Thread]:BCastWorker created,PID is %ld\n", pthread_self());*/
    /*pthread_detach(pthread_self());*/
    pthread_setcanceltype(PTHREAD_CANCEL_DISABLE, nullptr);
    int BCastEpollFd = NetCommManager->BCastEpollFd;
    std::unique_ptr<epoll_event[]> BCastEventList = std::make_unique<epoll_event[]>(BcastMaxChannelNum);
    int Event2Solve = 0;
    pthread_cleanup_push(BCastWorkerCleanUp, nullptr);
        while (1) {
            /*自定义函数取消点*/
            pthread_testcancel();
            pthread_setcanceltype(PTHREAD_CANCEL_DISABLE, nullptr);
            usleep(100);
            Event2Solve = epoll_wait(BCastEpollFd, BCastEventList.get(), BcastMaxChannelNum, 1000);
            if (0 == Event2Solve) {
                pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, nullptr);
                continue;
            }
            NetCommManager->HandleBCastEvents(BCastEventList.get(), Event2Solve);
            pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, nullptr);
        }
    pthread_cleanup_pop(0);
    return nullptr;
}


bool confirmCreateTCPServer(unsigned int channelHead,unsigned int channelNum){
    //遍历channel范围，确认每一个channel状态是否为ServerChannel
    //特例，channelHead在创建时设置的为ServerListenChannel
    if(gNetCommManager->gChannelManager->GetChannelStat(channelHead)!=ServerListenChannel){
        return false;
    }
    for(int i=channelHead+1;i<channelHead+channelNum;i++){
        if(gNetCommManager->gChannelManager->GetChannelStat(i)!=ServerChannel){
            return false;
        }
    }
    return true;
}

//用于确认client创建连接成功
bool confirmCreateTCPClient(unsigned int channel){
    //client这边确认连接成功是channel状态为ClientChannel
    if(gNetCommManager->gChannelManager->GetChannelStat(channel)==ClientChannel){
        return true;
    }else{
        return false;
    }
}

std::vector<unsigned int > getConnectedChannel(unsigned int channelHead,unsigned int channelNum){
    vector<unsigned int > channeList;
    //遍历server的通道范围，看哪一个通道建立了连接，获取所有建立连接的通道
    for(int i=channelHead;i<channelHead+channelNum;i++){
        if(gNetCommManager->gChannelManager->GetChannelStat(i)==ServerTransChannel){
            channeList.push_back(i);
        }else{
            continue;
        }
    }
    return channeList;
}
