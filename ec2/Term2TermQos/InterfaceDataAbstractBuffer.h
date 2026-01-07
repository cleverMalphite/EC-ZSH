#pragma once
#include <memory>
#include "../mGlobalDef.h"
using std::shared_ptr;
namespace Term2TermQos
{
    /**
     * InterfaceDataAbstractBuffer是一个纯抽象类，其子类必须实现Encode和Decode方法
     * 这个类主要用于上下层接口中与数据编解码有关的数据传输
     * 以终端A向终端B发送数据这一传输为例：
     * 对A来说，B反馈的反馈包对于A的Qos模块来说就是一个字节序列和字节序列长度这一二元组，这个类的子类的实例就可以对这一数据进行解码；
     * 同理B端的Qos模块也可以通过这个类的子类的实例将要发给A的反馈包编码成字节序列，然后交给B端的NetCombTransfer模块，由B的NetCombTransfer模块发送给A。
     */
    class InterfaceDataAbstractBuffer
    {
    public:
        virtual shared_ptr<BYTE> Encode(DWORD& d_data_length) = 0;
        virtual bool Decode(shared_ptr<BYTE> p_buffer, DWORD d_data_length) = 0;

        virtual ~InterfaceDataAbstractBuffer() {}
    };
};