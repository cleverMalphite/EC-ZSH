//
// Created by 王炳棋 on 2022/9/30.
//

#ifndef __EPOLLCOMM_EPOLLTHREADS_H
#define __EPOLLCOMM_EPOLLTHREADS_H

#include "CIPSOCKET.h"
#include <pthread.h>

#ifndef CHANNELNUM_RANGE
#define CHANNELNUM_RANGE
#define TCP_CHANNEL_FIRST_ID        3000    //TCP 通道分配开始号码
#define TCP_CHANNEL_FIRST_ID_MAX    3999    //TCP 通道分配最大号码
#define UDP_CHANNEL_FIRST_ID        4000    //UDP 通道分配开始号码
#define UDP_CHANNEL_FIRST_ID_MAX    4999    //UDP 通道分配最大号码
#endif

#define UDP_MAX_THREADNUM 10
#define RX_CHANNEL_READY 1000
#define TX_CHANNEL_READY 2000
#define MCU_RECEIVE_CHANNEL 3000
#define TX_RX_CHANNEL_CLOSE 4000
#define ADD_NEW_TCP_CHANNEL 5000



//----------------------回调函数定义---------------------------------
typedef bool (*PMESSAGECALLBACK)(DWORD nChannel, bool nCreate);    //通道建立回调函数
typedef bool (*PDATACALLBACK)(DWORD nChannel, const std::shared_ptr<BYTE> &pBuffer, DWORD nLength, bool isNeedSlit);

bool
Init_NetComm(PMESSAGECALLBACK pMESSAGECALLBACK, PMESSAGECALLBACK pMESSAGECALLBACK2, PDATACALLBACK pDATACALLBACK,
             unsigned UdpThreadNum = UDP_MAX_THREADNUM);

bool UnInit_NetComm();

//----------------------创建TCP通道---------------------------------
bool 
CreateTcpServerChannel(int &nLocalPort, const std::string &szLocalAddress, 
                       DWORD ChannelHead, DWORD ChannelNumber, bool isForceLocalPort);

bool
CreateTcpClientChannel(int &nLocalPort, int nRemotePort, const std::string &szLocalAddress, const std::string &szRemoteAddress,
                       DWORD Channel, bool isForceLocalPort = false/*是否强制要求使用参数中的本地端口号*/);

//----------------------创建DTU-TCP通道---------------------------------
bool 
CreateDtuTcpServerChannel(int &nLocalPort, const std::string &szLocalAddress, 
                       DWORD ChannelHead, DWORD ChannelNumber);

bool
CreateDtuTcpClientChannel(int &nLocalPort, int nRemotePort, const std::string &szLocalAddress, const std::string &szRemoteAddress,
                       DWORD Channel);


//----------------------创建UDP通道---------------------------------
bool CreateUdpServerChannel(DWORD &nLocalPort, const std::string &szLocalAddress, DWORD dwChannel,
                            bool isForceLocalPort = false/*如果为true，那么强制绑定参数中的本地端口*/);

bool
CreateUdpClientChannel(DWORD &nLocalPort/*isForceLocalPort为true时才有效，且均返回实际使用的本地端口*/, UINT nRemotePort,
                       const std::string &szLocalAddress,
                       const std::string &szRemoteAddress, DWORD dwChannel,
                       bool isForceLocalPort = false/*如果为true，那么强制绑定参数中的本地端口*/);

//----------------------创建BCast通道---------------------------------
bool CreateBCastServerChannel(UINT nBCastPort, std::string szLocalIPAddress, std::string szBCastAddr, DWORD dwChannel);

//----------------------关闭通道--------------------------------------
bool CloseTcpChannel(DWORD dwChannel);
bool CloseUdpChannel(DWORD dwChannel);
bool CloseBCastChannel(DWORD dwChannel);

//----------------------向通道写数据(发送数据)------------------------
bool WriteTcpChannel(DWORD dwChannel, const std::shared_ptr<BYTE>& pBuffer, DWORD Length);
bool WriteUdpChannel(DWORD dwChannel, const std::shared_ptr<BYTE> &pBuffer, DWORD Length);
bool WriteBCastChannel(DWORD dwChannel, std::shared_ptr<BYTE> pBuffer, DWORD Length);


//----------------------参数设置-----------------------------------
void SetUdpServerChannelRemoteAddress(DWORD CID, std::string remote_address, DWORD remotePort);

//----------------------参数获取-----------------------------------
DWORD GetUDPLocalPort(DWORD dwChannel);
std::string GetLocalHostAddress(DWORD dwChannel, enumChannelType eNetType = TCPChannel);
std::string GetTCPLocalHostAddress(DWORD dwChannel);
unsigned int GetTCPLocalHostPort(DWORD dwChannel);
std::string GetTcpRemoteAddress(DWORD dwChannel);
bool GetChannelDtuType(DWORD dwChannel);
//server端获取和客户端建立连接的通道
std::vector<unsigned int > getConnectedChannel(unsigned int channelHead,unsigned int channelNum);
//确认server和client是否创建成功
bool confirmCreateTCPServer(unsigned int channelHead,unsigned int channelNum);
//用于确认client创建连接成功
bool confirmCreateTCPClient(unsigned int channel);

#endif //EPOLLCOMM_EPOLLTHREADS_H
