//
// Created by Kong on 2023/6/17.
//
//
// Created by Kong on 2023/6/17.
//

#include "AbstractStorage_RBUDP_H/FileStorage.h"

#define _FILE_OFFSET_BITS 64
namespace RBUDP {
    FileStorage::FileStorage(std::string fileAbsolutePath, int dataLength) {
        //初始化临界区

        pthread_mutex_init(&Mutex_, nullptr);
        m_sFileAbosultePath = fileAbsolutePath;
        m_dDataLength = dataLength;
        //m_dCapacity = nCapacity;
        m_dFileState = STORAGE_STATE_UNINIT;
        m_File = nullptr;
        m_File = fopen(fileAbsolutePath.c_str(), "wb+");    //打开指定文件，如果文件已经存在，那么就先删除该文件后再打开

        //查看文件是否打开失败
        if (nullptr == m_File) {
            m_dFileState = STORAGE_STATE_WRONG;
        } else {
            m_dFileState = STORAGE_STATE_OPEN;
            char end = EOF;
            fseeko64(m_File, 1, SEEK_SET);
            fwrite(&end, 1, 1, m_File);
            fflush(m_File);
        }
        std::cout << "文件初始状态：" << m_dFileState << std::endl;
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

        //销毁临界区对象
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
        const DWORD dFileSeekIndex = dIndex * m_dDataLength;
        std::shared_ptr<BYTE> fileContent(new BYTE[m_dDataLength], releaseArrays<BYTE>);
        //移动文件指针位置
        fseeko64(m_File, dFileSeekIndex, SEEK_SET);
        //读取这一数据块的内容
        fread(fileContent.get(), m_dDataLength, 1, m_File);

        //查看有效数据部分占据了多少字节
        const int nValidDataLength = ReadData(fileContent.get() + 14);
        //开始摘取所有的字节
        dDataLength = RBUDPdata::GetPacketHeaderLength() + nValidDataLength;

        const DWORD dTemp = ReadData(fileContent.get() + 6);
        if (dTemp != dIndex) {
            pthread_mutex_unlock(&Mutex_);
            return NULL;
        }
        pthread_mutex_unlock(&Mutex_);
        return fileContent;
    }

    bool FileStorage::StorageData(std::shared_ptr<const RBUDPdata> reliableUdpData) {
        pthread_mutex_lock(&Mutex_);

        if (m_dFileState == STORAGE_STATE_OPEN) {
            DWORD m_nIndex = (reliableUdpData->GetPacketIndex()) * m_dDataLength;
            fseeko64(m_File, m_nIndex, SEEK_SET);
            DWORD writeLength = 0;
            std::shared_ptr<BYTE> temp = reliableUdpData->Encode(writeLength);

            DWORD dTemp = ReadData(temp.get() + 6);
            if (dTemp != reliableUdpData->GetPacketIndex()) {
                pthread_mutex_unlock(&Mutex_);
                return false;
            }
            fwrite(temp.get(), writeLength, 1, m_File);    //向文件中写入数据
            pthread_mutex_unlock(&Mutex_);
            m_lastIndex = m_nIndex;
            return true;
        } else {
            pthread_mutex_unlock(&Mutex_);
            return false;
        }
    }

    //bool FileStorage::StorageData(shared_ptr<const RBUDPdata> reliableUdpData)
    //{
    //	pthread_mutex_lock(&Mutex_);

    //	if (m_dFileState == STORAGE_STATE_OPEN)
    //	{
    //		ftell(m_File);
    //		DWORD32 m_nIndex = (reliableUdpData->GetPacketIndex()) * m_dDataLength;
    //		if (m_nIndex == 0) {
    //			_fseeki64(m_File, m_nIndex, SEEK_SET);
    //		}
    //		else {
    //			long long m_seg = (long long)m_nIndex - m_lastIndex - 1;
    //			_fseeki64(m_File, m_seg, SEEK_CUR);	//移动位置指针
    //		}
    //		DWORD32 writeLength = 0;
    //		std::shared_ptr<BYTE> temp = reliableUdpData->Encode(writeLength);

    //		DWORD32 dTemp = ReadData(temp.get() + 6);
    //		if (dTemp != reliableUdpData->GetPacketIndex())
    //		{
    //			pthread_mutex_unlock(&Mutex_);
    //			return false;
    //		}
    //		fwrite(temp.get(), writeLength, 1, m_File);	//向文件中写入数据
    //		pthread_mutex_unlock(&Mutex_);
    //		m_lastIndex = m_nIndex;
    //		return true;
    //	}
    //	else
    //	{
    //		pthread_mutex_unlock(&Mutex_);
    //		return false;
    //	}
    //}
}