//
// Created by Kong on 2023/6/17.
//

#ifndef EMERGENCYCOMMUNICATION_ABSTRACTSTORAGE_H
#define EMERGENCYCOMMUNICATION_ABSTRACTSTORAGE_H
#include "Transfer_H/RBUDPdata.h"
namespace RBUDP
{
    /**
     * 这个抽象类用来做实时UDP信息可靠传输时的存储介质抽象
     */
    class AbstractStorage
    {
    public:
        virtual ~AbstractStorage() {};

        virtual bool StorageData(std::shared_ptr<const RBUDPdata> reliableUdpData) = 0;	//向存储介质写入一个UdpData，如果写入成功则返回true，否则返回false

        virtual std::shared_ptr<BYTE> GetDataByIndex(DWORD dIndex, DWORD& dDataLength) = 0;			//从存储介质中读取第dIndex个UdpData，返回已经Encode好的UdpData元素智能指针

        virtual DWORD GetStorageState() = 0;									//获取存储介质的状态

    public:
        const static DWORD STORAGE_STATE_UNINIT = 0;									//存储介质还没初始化
        const static DWORD STORAGE_STATE_OPEN = 1;									//存储介质已经被成功打开
        const static DWORD STORAGE_STATE_WRONG = 2;									//存储介质打开失败
        const static DWORD STORAGE_STATE_CLOSE = 3;									//存储介质已经关闭

    };
};
#endif //EMERGENCYCOMMUNICATION_ABSTRACTSTORAGE_H
