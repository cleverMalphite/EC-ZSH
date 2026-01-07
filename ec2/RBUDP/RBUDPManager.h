//
// Created by Kong on 2023/6/9.
//

#ifndef RBUDP_RBUDPMANAGER_H
#define RBUDP_RBUDPMANAGER_H

#endif //RBUDP_RBUDPMANAGER_H

#include "vector"
#include "map"
#include "string"
#include "set"
#include "RBUDPTempDataQueue.h"
#include "End2EndReliableTransmission.h"
#include "CRBUDPTerm2TermConfig.h"
#include "../IniHandle/IniHandleApi.h"

namespace RBUDP {

    struct RBUDPUndealData {
        DWORD dwTID;
        std::shared_ptr<BYTE> pBuffer;
        DWORD dwLength;

        RBUDPUndealData(DWORD tid, std::shared_ptr<BYTE> pBuffer, DWORD length) {
            this->dwTID = tid;
            this->pBuffer = pBuffer;
            this->dwLength = length;
        }
    };

    struct RBUDPIndex {
        DWORD TID;
        DWORD Index;

        RBUDPIndex(DWORD tid, DWORD index) {
            this->TID = tid;
            this->Index = index;
        }
    };

    class RBUDPManager {
    public:

        static bool IsAckMessage(std::shared_ptr<BYTE> pBuffer, DWORD dwLength) {
            if (!pBuffer || dwLength <= 1) {
                return false;
            } else {
                return (pBuffer.get()[0] == MRUDP_MESSAGE_FIRST_BYTE_FLAG) ? true : false;
            }
        }


        void IndexquePush(RBUDPIndex index) {
            pthread_mutex_lock(&m_temp_index_queue_cs);
            tmp_sendIndex.push(index);
            sendindexLength++;
            pthread_mutex_unlock(&m_temp_index_queue_cs);
        }

        RBUDPIndex IndexqueFront() {

            pthread_mutex_lock(&m_temp_index_queue_cs);
            RBUDPIndex index = tmp_sendIndex.front();
            pthread_mutex_unlock(&m_temp_index_queue_cs);
            return index;
        }

        bool IndexIsEmpty() {
            bool isEmpty = false;
            pthread_mutex_lock(&m_temp_index_queue_cs);
            isEmpty = tmp_sendIndex.empty();
            pthread_mutex_unlock(&m_temp_index_queue_cs);
            return isEmpty;
        }

        void IndexquePop() {
            pthread_mutex_lock(&m_temp_index_queue_cs);
            if (sendindexLength > 0) {
                tmp_sendIndex.pop();
                sendindexLength--;
            }
            pthread_mutex_unlock(&m_temp_index_queue_cs);
        }

        RBUDPManager(std::string filePath);

        virtual ~RBUDPManager();

        RBUDPTempDataQueue m_TempData;        //用以临时存放一些上层传输的数据，用以下面的传输
        const DWORD m_TempDataLength = 200000;
        std::map<DWORD, std::shared_ptr<End2EndReliableTransmission>> m_mapReliableUdpSocket;    //存储端到端实时UDP信息可靠传输通道
        std::unordered_map<DWORD, std::shared_ptr<NAK_Message>> m_mapTermNak;
        std::queue<std::shared_ptr<NAK_Message>> m_Nak_que;
        shared_ptr<End2EndReliableTransmission> pSocket_hasFind = nullptr;
        std::deque<std::shared_ptr<RBUDPUndealData> > m_undealData;
        std::queue<std::shared_ptr<RBUDPUndealData> > m_dealindexData;
        std::set<DWORD> m_RecvIndex;
        DWORD current_index;                //用于指示当前接收了哪个数据包
        //const DWORD m_undealDataLength = GetIntegerKeyIni("MRUDP", "RUDP_UNDEAL_DATA_NUMBER", 20000);
        std::queue<RBUDPIndex> tmp_sendIndex;    //存放反馈来的已发送的数据
        DWORD sendindexLength = 0;
        std::unordered_map<DWORD, std::shared_ptr<NAK_Message>> tmp_mapTermToNAK;        //存储对应终端最新NAK
        pthread_mutex_t m_cs_map;
        pthread_mutex_t m_temp_data_queue_cs;
        pthread_mutex_t m_undeal_data_queue_cs;
        pthread_mutex_t m_recv_cs;
        pthread_mutex_t m_temp_index_queue_cs;
        std::string m_filePath;
        DWORD m_fileSize;
        bool Is_End = false;
        std::atomic_flag undeal_data_thread_flag = ATOMIC_FLAG_INIT;//为false则停止
        //TODO

        /**
         * 根据一条配置来初始化这一条配置对应的通道，注意方法内自动实现了防重复配置的功能
         * 参数是一条配置项
         * 如果配置项生效（不管是调用此方法前已经生效还是调用此方法后生效，或者配置项本身无效），那么返回TRUE；否则返回FALSE
         */
        bool InitSingleEnd2EndReliableTransmission(std::shared_ptr<CRBUDPTerm2TermConfig> m_role_with_channel);

        //删除一个端到端实时通信实体
        bool RemoveSingleEnd2EndReliableTransmission(DWORD dwTID);

        bool SendNAKForAllEnd2EndReliableTransmission();

        bool DoRecvDataOrMessageForRBUDP(DWORD dwTID, std::shared_ptr<BYTE> pBuffer, DWORD dwLength);

        bool SendDataBytesToEndByRBUDP(DWORD dTermId, const std::shared_ptr<BYTE> &pData, DWORD nLength);

        bool DoRecvdataFromSpeedControl();

        bool IsRUDPOnByTermId(DWORD term_id);    //如果对应终端在线就返回true，否则返回false

        //不可靠地传输数据，如果dTermId在线就返回true，否则返回false
        bool SendDataBytesToTermByRBUDPWithoutReliable(DWORD dTermId, std::shared_ptr<BYTE> bytes, DWORD nLength);


        bool DealDataWork();

        bool DealIndexWork();

        bool DealMessageWork();

        bool DealTimeOut();

        void DoCycle();

        void DoNAK();

        bool DealNak();

        void DoIndex();

        void DoTimeout();

        bool UploadTempDataQueueForAllEnd2EndTransmission();
    };
}
