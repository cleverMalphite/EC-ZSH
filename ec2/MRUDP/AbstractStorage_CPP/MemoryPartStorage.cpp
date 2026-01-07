//
// Created by 王炳棋 on 2022/12/22.
//
#include "AbstractStorage_H/MemoryPartStorage.h"
#include "IniHandleApi.h"

namespace MRUDP {
    const DWORD RUDP_FILE_PACKET_ENCODE_LENGTH = GetIntegerKeyIni("MRUDP", "RUDP_FILE_PACKET_ENCODE_LENGTH", 1400);
    //const DWORD RUDP_FILE_PACKET_ENCODE_LENGTH = 1400;

    MemoryStorageSender::MemoryStorageSender(DWORD dCapacity) : m_dCapacity(dCapacity) {
        m_dDataLength = RUDP_FILE_PACKET_ENCODE_LENGTH;    //TODO 配置文件里读
        m_pDataArrays.resize(dCapacity);    //一定不要忘了先给vector扩容
        for (DWORD i = 0; i < dCapacity; i++) {
            m_pDataArrays[i] = std::make_shared<StoreElement>();
        }
        pthread_mutex_init(&Mutex_, nullptr);
        m_dStoreState = STORAGE_STATE_OPEN;
        //printf("MemoryStorage Init Success\n");
    }

    MemoryStorageSender::~MemoryStorageSender() {
        m_dStoreState = STORAGE_STATE_CLOSE;
        pthread_mutex_destroy(&Mutex_);
    }

    bool MemoryStorageSender::StorageData(std::shared_ptr<const ReliableUdpData> reliableUdpData) {
        pthread_mutex_lock(&Mutex_);
        DWORD dDataLength = 0;
        const std::shared_ptr<BYTE> pData = reliableUdpData->Encode(dDataLength);
        std::shared_ptr<StoreElement> p_sender_data = std::make_shared<StoreElement>(pData, dDataLength);

        if (dDataLength < ReliableUdpData::GetPacketHeaderLength() || dDataLength > m_dDataLength) {
            pthread_mutex_unlock(&Mutex_);
            return false;
        }
        m_pDataArrays[reliableUdpData->GetPacketIndex()] = p_sender_data;
        pthread_mutex_unlock(&Mutex_);
        return true;
    }

    std::shared_ptr<BYTE> MemoryStorageSender::GetDataByIndex(DWORD dIndex, DWORD &dDataLength) {
        pthread_mutex_lock(&Mutex_);
        std::shared_ptr<StoreElement> pData = m_pDataArrays[dIndex];
        dDataLength = pData->dDataLength;
        std::shared_ptr<BYTE> pDataBytes = pData->pData;
        std::shared_ptr<BYTE> pResult(new BYTE[dDataLength], releaseArrays<BYTE>);
        //复制pDataBytes的前一部分内容
        memcpy(pResult.get(), pDataBytes.get(), dDataLength);
#ifdef MRUDP_TEST
        //测试上层大文件传输模块的数据是否正确
        //看第25个字节的长度
        DWORD32 d_packet_number = ReadData((pResult.get() + 38));
        if (d_packet_number == 1130749)
        {
            d_packet_number = d_packet_number;
        }
#endif // MRUDP_TEST
        pthread_mutex_unlock(&Mutex_);
        return pResult;
    }

    DWORD MemoryStorageSender::GetStorageState() {
        pthread_mutex_lock(&Mutex_);
        bool bResult = m_dStoreState;
        pthread_mutex_unlock(&Mutex_);

        return bResult;
    }
}
