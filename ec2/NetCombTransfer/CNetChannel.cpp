//
// Created by 王炳棋 on 2022/11/16.
//
#include "CNetChannel.h"

#include "./NetCombTransferApi.h"


#include <utility>

namespace NETCOMBTRANSFER {
}

CNetChannel::CNetChannel(DWORD dwCID) {
    //初始化成员变量
    m_bIsTempChannel = true;    //还未完成认证的临时通道
    m_bUDPServerCreated = false;
    m_bUDPClientCreated = false;
    m_dwCID = dwCID;
    m_dwTID = 0;    //初始化为无效的终端设备号
    m_nLocalUDPPort = 0;
    m_nRemoteUDPPort = 0;
    m_nQoSSend = 0;
    m_nQoSRecv = 0;
    m_nNetChannelStatus = E_NETCOMB_STATUS_CHANNEL_CONNECTED;

    m_szLocalAddress = GetLocalHostAddress(m_dwCID);    //初始化通道对应的TCP连接对应的本地网卡IP信息
    m_szRemoteAddress = GetTcpRemoteAddress(m_dwCID);    //初始化通道对应的TCP连接对应的远端网卡IP信息

#ifdef COMBTRANSFER_CHANNELTRAVERSE
#endif
    //判断本终端（不是本CNetChannel）是否是部署在公网上

    m_bIsPublicServer = GetBoolValueKeyIni("Main", "bIsPublicServer", false);
    //判断本终端（不是本CNetChannel)是否需要连接到公网服务器
    m_bIsClientConnectServer = GetBoolValueKeyIni("Main", "bIsClientConnectServer", false);
    //创建UDP服务端
    printf("CNetChannel.cpp:createUDP执行。\n");
    CreateUDP(true);

    bool bOK = RIDRequest();
    if (bOK) {
#ifdef COMBTRANSFER_CHANNELTRAVERSE
#endif
    } else {
        //打log
    }
}

CNetChannel::CNetChannel(DWORD dwCID, bool isDTUChannel) {
    //初始化成员变量
    m_bIsTempChannel = true;    //还未完成认证的临时通道
    m_bUDPServerCreated = false;
    m_bUDPClientCreated = false;
    m_dwCID = dwCID;
    m_dwTID = 0;    //初始化为无效的终端设备号
    m_nLocalUDPPort = 0;
    m_nRemoteUDPPort = 0;
    m_nQoSSend = 0;
    m_nQoSRecv = 0;
    m_nNetChannelStatus = E_NETCOMB_STATUS_CHANNEL_CONNECTED;

    m_szLocalAddress = GetLocalHostAddress(m_dwCID);    //初始化通道对应的TCP连接对应的本地网卡IP信息
    m_szRemoteAddress = GetTcpRemoteAddress(m_dwCID);    //初始化通道对应的TCP连接对应的远端网卡IP信息

#ifdef COMBTRANSFER_CHANNELTRAVERSE
#endif
    //判断本终端（不是本CNetChannel）是否是部署在公网上

    m_bIsPublicServer = GetBoolValueKeyIni("Main", "bIsPublicServer", false);
    //判断本终端（不是本CNetChannel)是否需要连接到公网服务器
    m_bIsClientConnectServer = GetBoolValueKeyIni("Main", "bIsClientConnectServer", false);
    //创建UDP服务端
    printf("CNetChannel.cpp:createUDP执行。\n");
    //CreateUDP(true);
    
    m_isDTUChannel = isDTUChannel;
    CreateUDP(true, m_isDTUChannel);

    bool bOK = RIDRequest();
    if (bOK) {
#ifdef COMBTRANSFER_CHANNELTRAVERSE
#endif
    } else {
        //打log
    }
}

bool CNetChannel::ChannelStateTraverse() {
    bool bOK = false;
    switch (m_nNetChannelStatus) {
        case E_NETCOMB_STATUS_CHANNEL_INVALID:
            //由于是在两终端已经建立TCP连接的情况下才有了这个类的实例对象出现，所以不用考虑通道无效的情况
#ifdef COMBTRANSFER_CHANNELTRAVERSE
#endif
            break;
        case E_NETCOMB_STATUS_CHANNEL_CONNECTED:
            //通道刚刚建立了UDP监听，还未通过已经建立的TCP通道获取对端的TID
#ifdef COMBTRANSFER_CHANNELTRAVERSE
#endif
            bOK = RIDRequest();
            //根据bOK打log
            if (!bOK) {
#ifdef COMBTRANSFER_CHANNELTRAVERSE
#endif
            } else {
#ifdef COMBTRANSFER_CHANNELTRAVERSE
#endif
            }
            break;
        case E_NETCOMB_STATUS_CHANNEL_RID_CONFIRMED:
            //已经得知对端的TID号，下一步要协商UDP端口号
#ifdef COMBTRANSFER_CHANNELTRAVERSE
#endif
            bOK = RUDPPortRequest();
            if (!bOK) {
#ifdef COMBTRANSFER_CHANNELTRAVERSE
#endif
            } else {
#ifdef COMBTRANSFER_CHANNELTRAVERSE
#endif
            }
            break;
        case E_NETCOMB_STATUS_RUDPPORT_CONFIRMED:
            //对端端口号已知
#ifdef COMBTRANSFER_CHANNELTRAVERSE
#endif
            bOK = true;
            break;
        case E_NETCOMB_STATUS_CHANNEL_NEGOTIATED:
#ifdef COMBTRANSFER_CHANNELTRAVERSE
#endif
            break;
        default:
            break;
    }
    return bOK;
}

void CNetChannel::CreateUDP(bool bIsServer) {
    printf("CNetChannel:CreateUDP.1\n");
    bool bOK = false;
    //根据预设的UDP链路尝试建立的最大次数尝试建立链路
    for (int i = 0; i < CREATE_UDP_CHANNEL_MAX_TRY_TIMES; ++i) {
        if (bIsServer) {
            printf("CNetChannel:CreateUDP.2\n");
            //判断本地UDP服务器监听是否已经建立
            if (m_bUDPServerCreated) {
                bOK = true;
                break;
            }
            //计算UDP通道号
            DWORD dwUDPChannel = m_dwCID + TCP_UDP_SERVER_CHANNEL_INTERVAL;
            printf("create UDP channel:%d\n",dwUDPChannel);
            //mark端口白名单
            //TODO:整合NetSPY到项目中，使用GetAvailableLocalPort函数。
            //GetAvailableLocalPort属于NetSPY模块下的一个接口,函数描述为:返回值为false时表示采用提前申请套接字方式占用端口，
            //为true时表示通过白名单获取端口，端口号从引用返回，分配失败就返回端口号为0。
            //此函数即从Port白名单里取出来一个Port给m_nLocalPort，里边儿没了就不取，给其赋0，这里我们先给isPortWhiteList置true，m_nLocalUDPPort置为9001；
            /*bool isPortWhiteList = GetAvailableLocalPort(m_nLocalUDPPort);*/
            /*bool isPortWhiteList = true;*/
            bool isPortWhiteList = false;
            m_nLocalUDPPort = 0;
            if (isPortWhiteList && 0 != m_nLocalUDPPort) {
                printf("CNetChannel:CreateUDP.3\n");
                bOK = CreateUdpServerChannel(m_nLocalUDPPort, m_szLocalAddress, dwUDPChannel, true);
                if (bOK) {
                    printf("CNetChannel:CreateUDP.4\n");
                    m_bUDPServerCreated = true;
                    break;
                } else {
                    printf("CNetChannel:CreateUDP.5\n");
                }
            } else {
                printf("CNetChannel:CreateUDP.6\n");
                bOK = CreateUdpServerChannel(m_nLocalUDPPort, m_szLocalAddress, dwUDPChannel, false);
                if (bOK) {
                    printf("CNetChannel:CreateUDP.7\n");
                    m_nLocalUDPPort = GetUDPLocalPort(dwUDPChannel);
                    m_bUDPServerCreated = true;
                    break;
                } else {
                    printf("CNetChannel:CreateUDP.8\n");
                    //打log[Warn]
                }
            }
        }
    }
}

void CNetChannel::CreateUDP(bool bIsServer, bool isDTUChannel) {
    printf("<DTU>CNetChannel:CreateUDP.1\n");
    bool bOK = false;
    //根据预设的UDP链路尝试建立的最大次数尝试建立链路
    for (int i = 0; i < CREATE_UDP_CHANNEL_MAX_TRY_TIMES; ++i) {
        if (bIsServer) {
            printf("DTUCNetChannel:CreateUDP.2\n");
            //判断本地UDP服务器监听是否已经建立
            if (m_bUDPServerCreated) {
                bOK = true;
                break;
            }
            //计算UDP通道号
            DWORD dwUDPChannel = m_dwCID + TCP_UDP_SERVER_CHANNEL_INTERVAL;
            printf("DTUcreate DTU UDP channel:%d, DTU:%d\n",dwUDPChannel, isDTUChannel);
            //mark端口白名单
            //TODO:整合NetSPY到项目中，使用GetAvailableLocalPort函数。
            //GetAvailableLocalPort属于NetSPY模块下的一个接口,函数描述为:返回值为false时表示采用提前申请套接字方式占用端口，
            //为true时表示通过白名单获取端口，端口号从引用返回，分配失败就返回端口号为0。
            //此函数即从Port白名单里取出来一个Port给m_nLocalPort，里边儿没了就不取，给其赋0，这里我们先给isPortWhiteList置true，m_nLocalUDPPort置为9001；
            /*bool isPortWhiteList = GetAvailableLocalPort(m_nLocalUDPPort);*/
            /*bool isPortWhiteList = true;*/

            //bool isPortWhiteList = false;
            bool isPortWhiteList;
            m_nLocalUDPPort = 0;
            
            if( isDTUChannel==true){
                printf("<DTU>[DEBUG]isDTUChannel = true\n");
                isPortWhiteList = true; // force local port for "CreateUdpServerChannel"
                printf("<DTU>[DEBUG]try to get local port from DTU virtual server.\n");
                m_nLocalUDPPort = Manager_GetLocalPort();
                printf("... ... ... \n");
                if(m_nLocalUDPPort==-1){
                    std::cerr << "GetLocalPort failed: " << strerror(errno) << std::endl;
                    return ;
                }
                printf("<DTU>[DEBUG]successfully to get local port from DTU virtual server\n");
            }
            else{
                printf("[DEBUG]isDTUChannel = false\n");
                isPortWhiteList = false;
                m_nLocalUDPPort = 0;
            }


            printf("CreateDTU UDP: local udp port:%d, local ip:%s, channelNum:%d\n",
                   m_nLocalUDPPort, m_szLocalAddress.c_str(), dwUDPChannel);


            if (isPortWhiteList==true && m_nLocalUDPPort != 0) {
                /*
                 * ture means forceLocalPort
                 */
                bOK = CreateUdpServerChannel(m_nLocalUDPPort, m_szLocalAddress, dwUDPChannel, true);
                if (bOK) {
                    m_bUDPServerCreated = true;
                    break;
                } else {
                }
            } else {
                bOK = CreateUdpServerChannel(m_nLocalUDPPort, m_szLocalAddress, dwUDPChannel, false);
                if (bOK) {
                    m_nLocalUDPPort = GetUDPLocalPort(dwUDPChannel);
                    m_bUDPServerCreated = true;
                    break;
                } else {
                }
            }
        }
    }
}

bool CNetChannel::CloseUDP() const {
    bool bOKServerClose = false;
    bool bOKClientClose = false;
    //关闭服务端UDP通道
    DWORD dwUDPServerCID = m_dwCID + TCP_UDP_SERVER_CHANNEL_INTERVAL;
    bOKServerClose = CloseUdpChannel(dwUDPServerCID);
    //关闭客户端UDP通道
    //DWORD dwUDPClientCID = m_dwCID + TCP_UDP_CLIENT_CHANNEL_INTERVAL;
    //bOKClientClose = CloseUdpChannel(dwUDPClientCID);
    return bOKServerClose/* & bOKClientClose*/;
}

//发送请求以获取对端TID
bool CNetChannel::RIDRequest() const {
    bool bOK = false;
    std::shared_ptr<CIMsgRemoteIDRequest> pMsg = std::make_shared<CIMsgRemoteIDRequest>();
    bOK = globalNetCombTransferSendCommand(pMsg, m_dwCID);
    return bOK;
}

//收到对端TID的回复
void CNetChannel::RIDNotify(DWORD nTID) {
    m_dwTID = nTID;
    m_nNetChannelStatus = E_NETCOMB_STATUS_CHANNEL_RID_CONFIRMED;
}

//发送请求以端的UD获取对PPort
bool CNetChannel::RUDPPortRequest() const {
    bool bOK = false;
    std::shared_ptr<CIMsgRemoteUDPPortRequest> pMsg = std::make_shared<CIMsgRemoteUDPPortRequest>();
    bOK = globalNetCombTransferSendCommand(pMsg, m_dwCID);
    return bOK;
}

//收到对端UDP PORT的回复，更新远端端口号和对应网卡信息
void CNetChannel::RUDPPortNotify(DWORD nRemotePort) {
    
    printf("[CNetChannel] RUDPPortNotify nRemotePort is %d\n", nRemotePort);
    m_nRemoteUDPPort = nRemotePort;

    if (!m_isNat) {
        SetUdpServerChannelRemoteAddress(m_dwCID + TCP_UDP_SERVER_CHANNEL_INTERVAL, 
                                         m_szRemoteAddress,
                                         m_nRemoteUDPPort);
        printf("[CNetChannel] m_isNat = false\n");
        printf("[CNetChannel] SetUdpServerChannelRemoteAddress\n");
    }
    else{
        printf("[CNetChannel] m_isNat = true\n");
        printf("[CNetChannel] do not SetUdpServerChannelRemoteAddress\n");
        printf("[CNetChannel] CUDP remoteAddr will be set from recvfrom()\n");
    }
    m_nNetChannelStatus = E_NETCOMB_STATUS_RUDPPORT_CONFIRMED;
}

bool CNetChannel::SendCommand(const std::shared_ptr<BYTE>& buf, DWORD length) const {
    bool bOK = false;
    bOK = WriteTcpChannel(m_dwCID, buf, length);
    return bOK;
}

bool CNetChannel::SendData(const std::shared_ptr<BYTE> &buf, DWORD length) const {
    bool bOK = false;

    //printf("<CNetChannel> SendData::tcp localaddress:%s\n", m_szLocalAddress.c_str());
    //printf("<CNetChannel> SendData::tcp remoteddress:%s\n", m_szRemoteAddress.c_str());
    //printf("<CNetChannel> SendData::m_nLocalUDPPort = %d\n", m_nLocalUDPPort);
    //printf("<CNetChannel> SendData::m_nRemoteUDPPort = %d\n", m_nRemoteUDPPort);
    //printf("<CNetChannel> SendData::WriteUdpChannel!\n");

    bOK = WriteUdpChannel(m_dwCID + TCP_UDP_SERVER_CHANNEL_INTERVAL, buf, length);
    //printf("[AutoTestDebug]::向UDP通道:%d写数据,结果:%d\n", m_dwCID + TCP_UDP_SERVER_CHANNEL_INTERVAL, bOK);
    return bOK;
}

bool CNetChannel::initIsConnectPublicServer() {
    //判断本Channel是否需要连接远程网卡
    if (m_bIsClientConnectServer) {
        //目前不需要干什么
        return true;
    }
    return true;
}

void CNetChannel::UpdateChannelStatus(bool isSuccess) {
    m_total_count++;
    m_current_count++;
    if(isSuccess){
        m_success_count++;
    }
    if(m_current_count==m_initial_send_quantity){
        m_current_count=0;
        AdjustChannelSendQuantity();
    }
}

void CNetChannel::AdjustChannelSendQuantity() {
    double success_rate = static_cast<double>(m_success_count) / m_total_count;

    int adjustment = 0;
    if(success_rate == 1){
        adjustment = 1;
    }else if(success_rate < min_success_rate){
        if(m_initial_send_quantity > 1){
            adjustment = -1;
        }
    }
    m_initial_send_quantity += static_cast<int>(smoothing_factor * adjustment);
    if(m_total_count >= 10000){
        m_total_count = 0;
        m_success_count = 0;
    }
}


