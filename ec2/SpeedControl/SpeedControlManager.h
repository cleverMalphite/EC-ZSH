//
// Created by 王炳棋 on 2022/12/10.
//

#include <map>
#include <memory>
#include "SpeedControlApi.h"
#include "Term2TermTransmission.h"

#ifndef NETCOMBTRANSFER_SPEEDCONTROLMANAGER_H
#define NETCOMBTRANSFER_SPEEDCONTROLMANAGER_H

namespace SpeedControl {
    class SpeedControlManager {
    public:
        explicit SpeedControlManager(DWORD d_self_tid);

        ~SpeedControlManager();


    private:
        //为方便测试先调成public
        std::map<DWORD, std::shared_ptr<Term2TermTransmission>> m_map_term_to_term_transmission;
        std::map<DWORD, std::shared_ptr<Term2TermTransmission>> m_map_term_to_term_transmission_tmp;

        std::queue<std::shared_ptr<DataWithPacketInfo>> m_packet_need_recv;
        std::queue<std::shared_ptr<TermOnOffInfo>> m_term_on_or_off_info;

        const DWORD MIN_DATA_LENGTH_FEEDBACK_FOR_QOS = 1300;
        pthread_mutex_t Mutex_;

        const DWORD m_self_tid;
        std::shared_ptr<TermSpeedInfo> p_temp_term_speed_info;

        pthread_t SpeedControlPID = 0;

        bool ChannelUpdateFlagForSendData = false;

        bool ChannelUpdateFlagForSendDataPri = false;

        bool ChannelUpdateFlagForSendDataWithoutSC = false;

    public:
        bool Init();//初始化

        bool Uninit();//模块卸载方法

        bool DoCycle();//被线程循环执行的方法
        //本类的功能性方法
        void DoRecvData(DWORD dwTID, const std::shared_ptr<BYTE> &pBuffer, DWORD dLength, int64_t &recvtime);

        void DoRecvTermOnOrOff(DWORD dwTID, bool bCreate);

        void DoRecvSpeedInfo(TermSpeedInfo term_speed_info);

        bool DoSendData(DWORD dwTID, const std::shared_ptr<BYTE> &pBuffer, DWORD dLength, bool isSleep, bool isRBUDP);

        bool DoSendDataPri(DWORD dwTID, const std::shared_ptr<BYTE> &pBuffer, DWORD dLength, bool isSleep, bool isRBUDP);

        bool DoSendDataWithoutSpeedControl(DWORD dwTID, const std::shared_ptr<BYTE> &pBuffer, DWORD dLength, bool isRBUDP);

        bool SendTIDCommandWithoutSpeedControl(DWORD dwTID, const std::shared_ptr<BYTE> &pBuffer, DWORD dLength, bool isRBUDP);

        std::shared_ptr<Term2TermTransmission> GetTerm2TermTransMission(DWORD nTID);

    private:
        //工作线程内调用的方法
        bool DoTermOnOrOff();

        bool DoRealCycleSend();//实际发送数据,如果所有T2T都未发送数据，返回false，否则返回true

        const static DWORD MAX_PACKET_CAN_DEAL = 10000;//接收队列的最大长度
    };
}


#endif //NETCOMBTRANSFER_SPEEDCONTROLMANAGER_H
