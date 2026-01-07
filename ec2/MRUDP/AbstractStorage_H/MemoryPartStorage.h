//
// Created by 王炳棋 on 2022/12/22.
//
#include <utility>

#include "AbstractStorage.h"
#include "Store_H/ReliableUdpDataSenderStore.h"

#ifndef NETCOMBTRANSFER_MEMORYPARTSTORE_H
#define NETCOMBTRANSFER_MEMORYPARTSTORE_H

namespace MRUDP {
#ifndef MemoryPartStorage_StoreElement
#define MemoryPartStorage_StoreElement

    typedef struct StoreElement {
        std::shared_ptr<BYTE> pData;
        DWORD dDataLength = 0;

        StoreElement(std::shared_ptr<BYTE> tmpBytes, DWORD dBytesLength) :
                pData(std::move(tmpBytes)), dDataLength(dBytesLength) {

        }

        StoreElement() : pData(nullptr), dDataLength(0) {

        }
    } StoreElement;
#endif

    /**
	 * TOOD TEST 这个类目前只是用于测试随机文件读写是否是系统效率低下的主要原因
	 */
    class MemoryStorageSender :
            public AbstractStorage {
    public:
        MemoryStorageSender(DWORD dCapacity);

        virtual ~MemoryStorageSender();

        bool StorageData(std::shared_ptr<const ReliableUdpData> reliableUdpData) override;

        std::shared_ptr<BYTE> GetDataByIndex(DWORD dIndex, DWORD &dDataLength) override;

        DWORD GetStorageState() override;

    private:
        pthread_mutex_t Mutex_;    //内部同步用临界区
        std::vector<std::shared_ptr<StoreElement>> m_pDataArrays;   //存储数据的对象
        DWORD m_dStoreState = STORAGE_STATE_UNINIT;     //存储介质的存储状态
        DWORD m_dCapacity = 0;
        DWORD m_dDataLength = 0;    //内存存储介质允许的单个BYTE数组元素的最大长度

        const DWORD m_dPacketHeaderLength = 13;    //reliableUdpData的包头长度，此处只是为了测试
    };
}
#endif //NETCOMBTRANSFER_MEMORYPARTSTORE_H
