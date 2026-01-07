//
// Created by 王炳棋 on 2022/12/21.
//
#include "AbstractStorage_H/FileStorage.h"

namespace MRUDP {
    FileStorage::FileStorage(int nCapacity, std::string fileAbsolutePath, int dataLength) :
            m_sFileAbsolutePath(fileAbsolutePath),
            m_dDataLength(dataLength), m_dCapacity(nCapacity),
            m_dFileState(STORAGE_STATE_UNINIT), m_File(nullptr) {

        pthread_mutex_init(&Mutex_, nullptr);
        m_File = fopen(m_sFileAbsolutePath.c_str(), "wb+");    //打开指定文件，如果文件已经存在，那么就先删除该文件后再打开
        if (nullptr == m_File) {
            return;
        }

        std::shared_ptr<char> pTemp(new char[m_dDataLength + 1], releaseArrays<char>);
        //把文件的长度扩展到数据包定长乘以包数目
        for (DWORD i = 0; i <= m_dCapacity; i++) {
            fwrite(pTemp.get(), m_dDataLength, 1, m_File);
            fflush(m_File);
        }
        //查看文件是否打开失败
        if (nullptr == m_File) {
            m_dFileState = STORAGE_STATE_WRONG;
        } else {
            m_dFileState = STORAGE_STATE_OPEN;
        }
    }

    FileStorage::~FileStorage() {

        switch (m_dFileState) {
            case STORAGE_STATE_OPEN:
                if (nullptr != m_File) {
                    fclose(m_File);
                    m_File = nullptr;
                }
                m_dFileState = STORAGE_STATE_CLOSE;
                break;
            default:
                break;
        }
        pthread_mutex_destroy(&Mutex_);
    }

    DWORD FileStorage::GetStorageState() {
        pthread_mutex_lock(&Mutex_);
        bool bResult = m_dFileState;
        pthread_mutex_unlock(&Mutex_);
        return bResult;
    }

    std::shared_ptr<BYTE> FileStorage::GetDataByIndex(DWORD dIndex, DWORD &dDataLength) {
        pthread_mutex_lock(&Mutex_);
        //计算索引值
        const DWORD dFileSeekOffset = dIndex * m_dDataLength;
        std::shared_ptr<BYTE> fileContent(new BYTE[m_dDataLength], releaseArrays<BYTE>);
        //移动文件指针位置
        fseek(m_File, dFileSeekOffset, SEEK_SET);
        //读取这一数据块的内容
        fread(fileContent.get(), m_dDataLength, 1, m_File);
        //查看有效数据部分占据了多少字节
        const DWORD nValidDataLength = ReadData(fileContent.get() + 9);
        //开始摘取所有的字节
        dDataLength = ReliableUdpData::GetPacketHeaderLength() + nValidDataLength;

        const DWORD dTemp = ReadData(fileContent.get() + 1);
        if (dTemp != dIndex) {
            pthread_mutex_unlock(&Mutex_);
            return nullptr;
        }
        pthread_mutex_unlock(&Mutex_);
        return fileContent;
    }

    bool FileStorage::StorageData(std::shared_ptr<const ReliableUdpData> reliableUdpData) {
        pthread_mutex_lock(&Mutex_);
        if (m_dFileState == STORAGE_STATE_OPEN) {
            DWORD dFileSeekOffset = (reliableUdpData->GetPacketIndex()) * m_dDataLength;
            fseek(m_File, dFileSeekOffset, SEEK_SET);
            DWORD WriteLength = 0;
            std::shared_ptr<BYTE> pData = reliableUdpData->Encode(WriteLength);
            DWORD dPacketIndex = ReadData(pData.get() + 1);
            if (dPacketIndex != reliableUdpData->GetPacketIndex()) {
                pthread_mutex_unlock(&Mutex_);
                return false;
            }
            fwrite(pData.get(), WriteLength, 1, m_File);
            pthread_mutex_unlock(&Mutex_);
            return true;
        } else {
            pthread_mutex_unlock(&Mutex_);
            return false;
        }
    }


}