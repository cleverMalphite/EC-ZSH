//
// Created by 王炳棋 on 2022/12/28.
//

#ifndef NETCOMBTRANSFER_MRUDPMANAGER_H
#define NETCOMBTRANSFER_MRUDPMANAGER_H

#include "MRUDPTerm2TermConfig.h"
#include <utility>
#include <vector>
#include <map>
#include <string>
#include "Transfer_H/ReliableUdpMessage.h"
#include "End2EndReliableTransmission.h"


namespace MRUDP {
    struct MrudpUndealData {
        DWORD dwTID;
        std::shared_ptr<BYTE> pBuffer;
        DWORD dwLength;

        MrudpUndealData(DWORD tid, std::shared_ptr<BYTE> Buffer, DWORD nLength) {
            this->dwTID = tid;
            this->pBuffer = std::move(Buffer);
            this->dwLength = nLength;
        }
    };

    struct MrudpPacketStatusInfo {
        //PacketTrans
        DWORD64 RealDealTransmissionDataPackNum = 0;
        DWORD64 RealDealReTransmissionPackNum_TimeOut1 = 0;
        DWORD64 RealDealReTransmissionPackNum_TimeOut2 = 0;
        DWORD64 RealDealReTransmissionPackNum_SACK1 = 0;
        DWORD64 RealDealReTransmissionPackNum_SACK2 = 0;
        DWORD64 RecvUrgencyReTransmissionPackNum = 0;
        DWORD64 TotalRecvTransmissionDataPackNum = 0;
    public:
        MrudpPacketStatusInfo() {
            TotalRecvTransmissionDataPackNum = 0;
            RecvUrgencyReTransmissionPackNum = 0;
            RealDealReTransmissionPackNum_SACK1 = 0;
            RealDealReTransmissionPackNum_SACK2 = 0;
            RealDealReTransmissionPackNum_TimeOut1 = 0;
            RealDealReTransmissionPackNum_TimeOut2 = 0;
            RealDealTransmissionDataPackNum = 0;
        }

        ~MrudpPacketStatusInfo() {
            TotalRecvTransmissionDataPackNum = 0;
            RecvUrgencyReTransmissionPackNum = 0;
            RealDealReTransmissionPackNum_SACK1 = 0;
            RealDealReTransmissionPackNum_SACK2 = 0;
            RealDealReTransmissionPackNum_TimeOut1 = 0;
            RealDealReTransmissionPackNum_TimeOut2 = 0;
            RealDealTransmissionDataPackNum = 0;
        }
        //More Other Info...
    };

    /**
	 * 这个类用于管理抽象实时UDP可靠传输通道
	 */
    class MRUDPManager {
    public:
        /**
		 * 注意，原则上禁止使用此类中没有任何参数的构造函数
		 */
        explicit MRUDPManager(std::string filePath);

        virtual ~MRUDPManager();

    public:
        pthread_mutex_t MapMutex_;
        pthread_mutex_t RecvMutex_;
        pthread_mutex_t UndealDataQueueMutex_;

    public:
        MrudpPacketStatusInfo mStatus;

        void GetAndUpdateMrudpStatusInfo() {
            /*printf("\n>>*********************************<<\nRealDealTransmissionDataPackNum == %lu\n",
                   mStatus.RealDealTransmissionDataPackNum);
            //printf("RealDealReTransmissionPackNum_TimeOut1 == %lu\n",mStatus.RealDealReTransmissionPackNum_TimeOut1);
            printf("RealDealReTransmissionPackNum_TimeOut1 == %lu\n", mStatus.RealDealReTransmissionPackNum_TimeOut1);
            printf("RealDealReTransmissionPackNum_TimeOut2 == %lu\n", mStatus.RealDealReTransmissionPackNum_TimeOut2);
            printf("RealDealReTransmissionPackNum_SACK1 == %lu\n", mStatus.RealDealReTransmissionPackNum_SACK1);
            printf("RealDealReTransmissionPackNum_SACK2 == %lu\n", mStatus.RealDealReTransmissionPackNum_SACK2);
            printf("RecvUrgencyReTransmissionPackNum == %lu\n", mStatus.RecvUrgencyReTransmissionPackNum);
            printf("TotalRecvTransmissionDataPackNum == %lu\n>>*********************************<<\n\n",
                   mStatus.TotalRecvTransmissionDataPackNum);*/
            mStatus.TotalRecvTransmissionDataPackNum = 0;
            mStatus.RecvUrgencyReTransmissionPackNum = 0;
            mStatus.RealDealReTransmissionPackNum_SACK1 = 0;
            mStatus.RealDealReTransmissionPackNum_SACK2 = 0;
            mStatus.RealDealReTransmissionPackNum_TimeOut1 = 0;
            mStatus.RealDealReTransmissionPackNum_TimeOut2 = 0;
            mStatus.RealDealTransmissionDataPackNum = 0;
        };
    public:

        //const DWORD m_undealDataLength = 20000;
        const DWORD m_undealDataLength = GetIntegerKeyIni("MRUDP", "RUDP_UNDEAL_DATA_NUMBER", 20000);
        std::deque<std::shared_ptr<MrudpUndealData>> m_undealData;
        std::atomic_flag undeal_data_thread_flag = ATOMIC_FLAG_INIT;//此宏将flag初始化为false
        //存储端到端实时UDP信息可靠传输通道
        std::map<DWORD, std::shared_ptr<End2EndReliableTransmission>> m_mapReliableUdpSocket;
        //存储端到端最新的ACK
        std::unordered_map<DWORD, std::shared_ptr<Force_ACK_Message>> m_mapTermToAck;
        std::string m_filePath;

        
        /**
         * 根据一条配置来初始化这一条配置对应的通道，注意方法内自动实现了防重复配置的功能
         * 参数是一条配置项
         * 如果配置项生效（不管是调用此方法前已经生效还是调用此方法后生效，或者配置项本身无效），那么返回TRUE；否则返回FALSE
         */

    public:
        static bool IsAckMessage(const std::shared_ptr<BYTE> &pBuffer, DWORD dwLength) {
            if (!pBuffer || dwLength <= 1) {
                return false;
            } else {
                //printf("pBuffer.get()[0] = %d\n", pBuffer.get()[0]);
                return (pBuffer.get()[0] == MRUDP_MESSAGE_FIRST_BYTE_FLAG);
            }
        }

        static bool IsForceSpeedMessage(const std::shared_ptr<BYTE> &pBuffer, DWORD dwLength) {
            if (!pBuffer || dwLength <= 1) {
                return false;
            } else {
                return (pBuffer.get()[0] == MRUDP_FORCE_SPEED_MESSAGE);
            }
        }

        static bool IsForceAdjustBandWidthMessage(const std::shared_ptr<BYTE> &pBuffer, DWORD dwLength) {
            if (!pBuffer || dwLength <= 1) {
                return false;
            } else {
                return (pBuffer.get()[0] == MRUDP_FORCE_ADJUSTBANDWIDTH_MESSAGE);
            }
        }

        static bool IsUnreliableData(const std::shared_ptr<BYTE> &pBuffer,DWORD dwLength){
            if(!pBuffer||dwLength<=1){
                return false;
            }else{
                return (pBuffer.get()[0]==MRUDP_DATA_FIRST_BYTE_FLAG_WITHOUT_RELIABLE);
            }
        }

    public:
        /**
		 * 根据一条配置来初始化这一条配置对应的通道，注意方法内自动实现了防重复配置的功能
		 * 参数是一条配置项
		 * 如果配置项生效（不管是调用此方法前已经生效还是调用此方法后生效，或者配置项本身无效），那么返回TRUE；否则返回FALSE
		 */
        bool InitSingleEnd2EndReliableTransmission(std::shared_ptr<MRUDPTerm2TermConfig> m_role_with_channel);

        bool SendDataBytesToEndByMRUDP(DWORD dTermId, const std::shared_ptr<BYTE> &pData, DWORD nLength);

        bool SendFACKForAllEnd2EndReliableTransmission();

        bool DealTempDataQueueForAllEnd2EndTransmission();

        bool UploadTempDataQueueForAllEnd2EndTransmission();

        bool DoRecvDataOrMessageForMRUDP(DWORD dwTID, const std::shared_ptr<BYTE> &pBuffer, DWORD dwLength);

        //删除一个端到端实时通信实体
        bool RemoveSingleEnd2EndReliableTransmission(DWORD dwTID);

        //根据对端TID获得到对端的丢包率
        float ComputeAndGetRUDPLossRateByTID(DWORD remote_tid);

        //如果对应终端在线就返回true，否则返回false
        bool IsRUDPOnByTermId(DWORD dTermId);

        //不可靠地传输数据，如果dTermId在线就返回true，否则返回false
        bool
        SendDataBytesToTermByMRUDPWithoutReliable(DWORD dTermId, const std::shared_ptr<BYTE> &bytes, DWORD nLength);

        bool AdjustBandWidth(DWORD dTermId);

        bool DealMessageWork();

        bool DealDataWork();

        void DoCycle();
    };
}

#endif //NETCOMBTRANSFER_MRUDPMANAGER_H
