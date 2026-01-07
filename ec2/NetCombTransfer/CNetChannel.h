//
// Created by 王炳棋 on 2022/11/16.
//

#ifndef NETCOMBTRANSFER_CNETCHANNEL_H
#define NETCOMBTRANSFER_CNETCHANNEL_H

/*#include "NetSPY.h"*/
#include "EpollCommApi.h"
#include "../mGlobalDef.h"
#include "CIMsgRemoteIDRequest.h"
#include "CIMsgRemoteIDNotify.h"
#include "CIMsgRemoteUDPPortRequest.h"
#include "CIMsgRemoteUDPPortNotify.h"
#include "NetCombTransferApi.h"

/*此类是对认证过程中和认证过程后两设备间UDP通道的抽象。
注意在本系统的抽象中，此类的实例关联着两终端间的一条TCP连接（使用m_dwCID关联着一条TCP连接），m_dwCID直接标识了
两终端间TCP连接在本机的通道号，更间接的标识了本机之间要建立的UDP通道的软件实体（借助所关联的TCP链路通道号计算出认
证成功后两者之间建立的UDP连接在本机的通道号）*/

class CNetChannel {
public:

    CNetChannel(DWORD dwCID);
    CNetChannel(DWORD dwCID, bool isDTUChannel);

    ~CNetChannel() = default;

public:
    bool m_bIsTempChannel{};      //指示是否是临时通道
    bool m_bUDPServerCreated{};   //服务端UDP监听是否建立
    bool m_bUDPClientCreated{};   //客户端UDP监听是否建立
    DWORD m_dwCID{};              //CID通道号
    DWORD m_nLocalUDPPort{};      //本地UDP端口号
    DWORD m_nRemoteUDPPort{};     //远端UDP端口号
    DWORD m_dwTID{};              //TID归属
    DWORD m_nNetChannelStatus{};  //通道状态
    DWORD m_nQoSSend{};           //本机通过此通道向外发送数据的链路质量
    DWORD m_nQoSRecv{};           //本机通过此通道接受数据的链路质量
    DWORD m_initial_send_quantity=100;
    DWORD m_success_count=0;
    DWORD m_total_count=0;
    DWORD m_current_count=0;

    std::string m_szLocalAddress;//通道对应的本机网卡信息
    std::string m_szRemoteAddress;//通道对应的远端网卡信息

    const double min_success_rate=0.95;
    const double smoothing_factor=0.5;

private:

    bool m_bIsPublicServer = false;//如果为true,代表本端部署在公网上，拥有公网IP
    bool m_bIsClientConnectServer = false;//如果为true，表示本抽象终端需要连接到公网IP上
    bool m_isNat = GetBoolValueKeyIni("Main", "IsNat", false);
    DWORD m_nLocalPortAsUdpClient = 0;

    bool m_isDTUChannel = false;

public:
    /*通道状态机流转*/
    bool ChannelStateTraverse();

    /*创建相应的UDP通道*/
    void CreateUDP(bool bIsServer);

    void CreateUDP(bool bIsServer, bool isDTUChannel);

    /*关闭相应的UDP通道*/
    bool CloseUDP() const;

    /*获取对端TID请求*/
    bool RIDRequest() const;

    /*发送获取对端发来的TID通知*/
    void RIDNotify(DWORD nTID);

    /*发送获取对端UDP Port的请求*/
    bool RUDPPortRequest() const;

    /*收到对端发来的UDP Port*/
    void RUDPPortNotify(DWORD nRemotePort);

    void UpdateChannelStatus(bool isSuccess);

    void AdjustChannelSendQuantity();

    /*发送数据*/
    bool SendData(const std::shared_ptr<BYTE>& buf, DWORD length) const;

    /*发送指令*/
    bool SendCommand(const std::shared_ptr<BYTE>& buf, DWORD length) const;

    /*如果本Channel需要连接公网网卡，进行相关初始化*/
    bool initIsConnectPublicServer();

    /*获取通道状态*/
    DWORD GetChannelState() const { return m_nNetChannelStatus; }

public:
    bool operator==(const CNetChannel &hs) const {
        if (m_dwCID == hs.m_dwCID) {
            return true;
        }
        return false;
    }

    bool operator!=(const CNetChannel &hs) const {
        return !(operator==(hs));
    }
};


#endif //NETCOMBTRANSFER_CNETCHANNEL_H
