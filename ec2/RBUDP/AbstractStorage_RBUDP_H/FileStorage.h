//
// Created by Kong on 2023/6/17.
//

#include "AbstractStorage.h"
#ifndef EMERGENCYCOMMUNICATION_FILESTORAGE_H
#define EMERGENCYCOMMUNICATION_FILESTORAGE_H
namespace RBUDP
{
    class FileStorage : public AbstractStorage
    {
    public:
        /**
         * nCapacity： 存储介质的容量
         * fileAbsolutePath： 存储介质的实际存储路径，指的是文件的完全路径
         * dataLength: 文件中每一个块的最大存储长度
         * 注意文件里能存储的实际元素数目应该是nCapacity+1
         */
        FileStorage(std::string fileAbsolutePath, int dataLength);
        ~FileStorage();

        DWORD GetStorageState() override;
        std::shared_ptr<BYTE> GetDataByIndex(DWORD dIndex, DWORD& dDataLength) override;
        bool StorageData(std::shared_ptr<const RBUDPdata> reliableUdpData) override;
    private:
        std::string m_sFileAbosultePath;	//文件的实际完全存储路径
        FILE* m_File;	//文件句柄
        DWORD m_dFileState;
        DWORD m_dDataLength;	//文件中一个存储单位的长度
        DWORD m_dCapacity;	//文化的存储容量
        DWORD m_lastIndex = 0;	//上一个存储下来的索引
        pthread_mutex_t Mutex_;	//同步临界区
    };
}

#endif //EMERGENCYCOMMUNICATION_FILESTORAGE_H
