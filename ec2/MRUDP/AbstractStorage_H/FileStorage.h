//
// Created by 王炳棋 on 2022/12/21.
//
#include "AbstractStorage.h"

#ifndef NETCOMBTRANSFER_FILESTORAGE_H
#define NETCOMBTRANSFER_FILESTORAGE_H
namespace MRUDP {
    class FileStorage : public AbstractStorage {
    public:
        FileStorage(int nCapacity, std::string fileAbsolutePath, int dataLength);

        ~FileStorage() override;

    private:

        /**
         * nCapacity： 存储介质的容量
         * fileAbsolutePath： 存储介质的实际存储路径，指的是文件的完全路径
         * dataLength: 文件中每一个块的最大存储长度
         * 注意文件里能存储的实际元素数目应该是nCapacity+1
         */

        std::string m_sFileAbsolutePath;    //文件的实际完全存储路径
        FILE *m_File;    //文件句柄
        DWORD m_dFileState;
        DWORD m_dDataLength;    //文件中一个存储单位的长度
        DWORD m_dCapacity;    //文化的存储容量
        pthread_mutex_t Mutex_;    //同步临界区

    public:

        DWORD GetStorageState() override;

        std::shared_ptr<BYTE> GetDataByIndex(DWORD dIndex, DWORD &dDataLength) override;

        bool StorageData(std::shared_ptr<const ReliableUdpData> reliableUdpData) override;

    };
}
#endif //NETCOMBTRANSFER_FILESTORAGE_H
