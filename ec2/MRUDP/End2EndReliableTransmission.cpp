//
// Created by 王炳棋 on 2022/12/28.
//
#include "../SpeedControl/SpeedControlApi.h"
#include "End2EndReliableTransmission.h"

#include <utility>

namespace MRUDP {
    extern bool GetThreadStop();

    //extern pthread_mutex_t MRUDPMutex_;


    End2EndReliableTransmission::End2EndReliableTransmission(DWORD dSelfTermId, DWORD dRemoteTermId,
                                                             const std::string &SendCycleFileName) :
            m_dSelfTermId(dSelfTermId), m_dRemoteTermId(dRemoteTermId) {
        pthread_mutex_init(&Mutex_, nullptr);
        std::string sendFilePath = GetSendFilePath(SendCycleFileName);
        std::string receiveFilePath = GetReceiveFilePath(SendCycleFileName);

        //创建循环队列文件（发送）
        m_send_circular_queue = std::make_shared<SendCircularDataQueue>(m_dSelfTermId, m_dRemoteTermId);
        //创建循环队列文件（接收）
        m_receive_circular_queue = std::make_shared<RecvCircularDataQueue>(m_dSelfTermId, m_dRemoteTermId);

        m_b_state = true;
        m_b_write_able = true;
    }

    std::string End2EndReliableTransmission::GetSendFilePath(const std::string &filePathName) {
        std::string str2("Receive.txt");
        std::string resultString = filePathName + str2;
        return resultString;
    }

    std::string End2EndReliableTransmission::GetReceiveFilePath(const std::string &filePathName) {
        std::string str2("Send.txt");
        std::string resultString = filePathName + str2;
        return resultString;
    }

    End2EndReliableTransmission::~End2EndReliableTransmission() {
        pthread_mutex_destroy(&Mutex_);
    }

    bool End2EndReliableTransmission::SendUserData(const std::shared_ptr<BYTE> &data, DWORD dLength) {
        //1. 尝试将数据加到sendCycleFile中
        auto RUDPData = std::make_shared<ReliableUdpDataSenderStore>(data, dLength);
        //获取ENCODE序列和长度
        DWORD nTempLength = 0;
        //AddItemToQueue会将编码写到文件中
        bool bSuccess = this->m_send_circular_queue->AddItem(RUDPData);
        //2. 根据上一步回来的Reliable，发送数据
        if (bSuccess) {
            std::shared_ptr<BYTE> sendBytes = m_send_circular_queue->SendDataByIndex(RUDPData->GetPacketIndex(),
                                                                                     nTempLength);
            RUDPData->SetEncodeBytes(nTempLength);
            bool bRet = this->SendByteData(sendBytes, nTempLength);
            //reliableUdpData->SetLastRetransferTime(GetTickCount());
            //统计尝试发送包个数，即应该成功发送的数据包个数
            AddTrySendNumber();
            return bRet;
        }

        return false;
    }

    bool End2EndReliableTransmission::SendByteData(const std::shared_ptr<BYTE> &data, DWORD nLength) const {
        bool ret = false;
        DWORD ret_fail_count = 0;
        if (data) {
            while (!ret && ret_fail_count < 5) {
                ret = SendTIDDataWithSpeedControl(m_dRemoteTermId, data, nLength);
            }
            if (!ret && ret_fail_count >= 5) {
                /*SPDLOG_LOGGER_WARN(MRUDPLogger, "MRUDP send data fail after five try.");*/
                return false;
            }
            return true;
        }
        return false;
    }

    void End2EndReliableTransmission::HandleIO(const std::shared_ptr<BYTE> &pData, DWORD nDataLength) {
        printf("[MRUDPDebug]::Docycle---->HandleIO,pData[0] = %d\n",pData.get()[0]);
        //TODO 处理收到的数据或者信令
        switch (pData.get()[0]) {
            case MRUDP_DATA_FIRST_BYTE_FLAG: {
                //表示收到的是数据
                printf("[MRUDPDebug]::DealDataWork,处理Data\n");
                OnReceiveData(pData, nDataLength);
            }
                break;
            case MRUDP_MESSAGE_FIRST_BYTE_FLAG: {
                //表示收到的是命令
                //printf("[MRUDPDebug]::DealDataWork,处理Massage\n");
                OnReceiveMessage(pData, nDataLength);
            }
                break;
            case MRUDP_DATA_FIRST_BYTE_FLAG_WITHOUT_RELIABLE: {
                //表示收到的是不可靠传输数据
                //printf("[MRUDPDebug]::DealDataWork,处理不可靠数据包\n");
                OnReceiveWithoutReliableData(pData, nDataLength);
            }
            default:
                break;
        }

    }

    void End2EndReliableTransmission::UpdateRecvQueue() {
        m_receive_circular_queue->Update();
    }

    void End2EndReliableTransmission::UploadRecvQueue() {
        m_receive_circular_queue->UploadDataPacket(m_dRemoteTermId);
    }

    void End2EndReliableTransmission::SendFACK() {
        //首先需要获得TACK对应的ReliableUdpMessage
        //printf("[MRUDPDebug]::向终端%d发送FACK信息\n",m_dRemoteTermId);
        //std::shared_ptr<Force_ACK_Message> reliableUdpMessage = m_receive_circular_queue->TraverseForceACK(m_dRemoteTermId);
        std::shared_ptr<Force_ACK_Message> reliableUdpMessage = m_receive_circular_queue->NewTraverseForceACK(
                m_dRemoteTermId);
        //获得编码值
        std::shared_ptr<BYTE> message = nullptr;
        DWORD messageLength = 0;    //表示MESSAGE的实际数据长度
        if (reliableUdpMessage) {
            message = reliableUdpMessage->Encode(messageLength);
            //下面就要发送数据
            SendTIDCommandWithoutSpeedControl(m_dRemoteTermId, message, messageLength, false);
        }
        //发送一个心跳连接包,MARK,暂时取消了心跳包,后续需要修改.
        /*shared_ptr<KeepAlive> keepAliveMessage = make_shared<KeepAlive>();
        shared_ptr<BYTE> messageBytes = keepAliveMessage->Encode(messageLength);
        SendTIDDataWithoutSpeedControl(m_dRemoteTermId, messageBytes, messageLength);*/
    }

    void End2EndReliableTransmission::OnReceiveData(const std::shared_ptr<BYTE> &szBytes, int nRecv) {
        //当收到数据时，需要先把数据解析出来
        //而后将之交付上层应用
        //并将循环队列中相应包置为已接收
        std::shared_ptr<ReliableUdpDataReceiverStore> p_reliable_data = std::make_shared<ReliableUdpDataReceiverStore>();
        bool result = p_reliable_data->Decode(szBytes, nRecv);
        if (result) {
            printf("[MRUDPDebug]::E2ERTransmission OnReceiveData \n");
            m_receive_circular_queue->OnRecvDataFromLowerLayers(p_reliable_data);
        }
    }

    void End2EndReliableTransmission::OnReceiveMessage(const std::shared_ptr<BYTE> &szBytes, int nRecv) {
        /**
		 * 对于FACK和TACK，我们如何判断它们是有效的呢？
		 * 一个原则：能让队列长度变小的就是可靠的，我们只需要判断尾包序号是否在循环队列内，如果在的话，
		 * 那么更新发送循环队列，并重传丢包区间。
		 */
        //先把信令解析出来
        std::shared_ptr<Force_ACK_Message> pMessage = std::make_shared<Force_ACK_Message>();
        bool b_decode = pMessage->Decode(szBytes, nRecv);
        if (b_decode) {
            if (nullptr == force_ack_message_we_receive_last_time) {
                m_send_circular_queue->OnRecvMessageFromRecvTerm(pMessage);

                force_ack_message_we_receive_last_time = pMessage;
            } else {
                if (force_ack_message_we_receive_last_time->GetTimeStamp() < pMessage->GetTimeStamp()) {

                    AddRetrySendNumber(m_send_circular_queue->OnRecvMessageFromRecvTerm(pMessage));

                    force_ack_message_we_receive_last_time = pMessage;
                }
            }
        }
    }

    //TODO:Conditional Jump or move depends on uninitialised value(s)
    void End2EndReliableTransmission::OnReceiveMessage(const std::shared_ptr<Force_ACK_Message> &pMessage) {
        if (nullptr == force_ack_message_we_receive_last_time) {
            m_send_circular_queue->OnRecvMessageFromRecvTerm(pMessage);

            force_ack_message_we_receive_last_time = pMessage;
        } else {
            if (force_ack_message_we_receive_last_time->GetTimeStamp() < pMessage->GetTimeStamp()) {

                AddRetrySendNumber(m_send_circular_queue->OnRecvMessageFromRecvTerm(pMessage));

                force_ack_message_we_receive_last_time = pMessage;
            }
        }
    }

    void End2EndReliableTransmission::OnReceiveWithoutReliableData(std::shared_ptr<BYTE> szBytes, int nRecv) const {
        //直接反馈给上层模块相关数据
        //解码不可靠传输消息
        DWORD new_length = 0;
        std::shared_ptr<BYTE> new_bytes = MRUDPDecodeDataWithoutReliable(std::move(szBytes), new_length);

        printf("_________________________\n");
        printf("+++++++++++++++++++++++++\n");
        printf("^^^^^^^^^^^^^^^^^^^^^^^^^\n");
        printf("MRUDP:recv unreliable data...\n");
        printf("Check Data type:\n");
        //检查数据类型，如果不是数据服务类型数据，则交给BigDataTransfer的回调
        if(dataserver_check(new_bytes, new_length)==false){
            printf("CallBackDataFunc => BigDataTransfer\n");
            CallBackDataFunc(m_dRemoteTermId, new_bytes, new_length);
            printf("...CallBackDataFunc...ok\n");
        }
        else { //如果是数据服务类型数据，则交给DATA SERVER的回调
            //MRUDP_DATASERVER_CALLBACK
            //数据回调服务对应的回调调用 240910-houlc spin_server_callback
            printf("spin_server_callback => DATA SERVER\n");
            bool res = spin_server_callback(m_dRemoteTermId, new_bytes, new_length);
            printf("...spin_server_callback...ok(res:%d)\n", res);
        }
        printf("#######################\n");
    }

    float End2EndReliableTransmission::ComputeRetryRate() {
        //避免除数为0
        if (0 == m_try_send_number) {
            m_try_send_number++;
        }
        m_retry_rate = m_retry_send_number * 1.0 / (m_retry_send_number + m_try_send_number * 1.0);
        //SPDLOG_LOGGER_WARN(MRUDPLogger, "Remote TID {} loss rate: {}", m_retry_rate);

        m_try_send_number = 0;
        m_retry_send_number = 0;
        return m_retry_rate;
    }

    bool End2EndReliableTransmission::AdjustBandWidth() {
        return m_send_circular_queue->SendAllUnACKPackets();
    }

}
