//
// Created by Kong on 2023/6/9.
//

#include "End2EndReliableTransmission.h"
#include "../SpeedControl/SpeedControlApi.h"
#include "memory"
#include "Store_H/RBUDPDataSenderStore.h"

using namespace std;

namespace RBUDP
{
    extern bool GetThreadStop();
    extern pthread_mutex_t g_cs_for_rbudp;


    End2EndReliableTransmission::End2EndReliableTransmission(DWORD dSeldTermId, DWORD dRemoteTermId,
                                                             std::string sendCycleFileName)
    {
        pthread_mutex_init(&m_cs, nullptr);

        m_dSelfTermId = dSeldTermId;
        m_dRemoteTermId = dRemoteTermId;
        //m_dFileSize = dFileSize;

        std::string sendFilepath = GetSendFilepath(sendCycleFileName);
        std::string receiveFilePath = GetReceiveFilePath(sendCycleFileName);

        //创建循环队列文件（发送）
        m_send_queue_map = make_shared<SendDataQueueMap>(m_dSelfTermId, m_dRemoteTermId);
        //创建循环队列文件（接收）
        m_recv_queue_map = make_shared<RecvDataQueueMap>(m_dSelfTermId, m_dRemoteTermId);

        m_b_state = true;
        m_b_write_able = true;

    }

    std::string End2EndReliableTransmission::GetSendFilepath(std::string filePathName)
    {
        std::string str2("Receive.txt");
        std::string resultString = filePathName + str2;
        return resultString;
    }

    std::string End2EndReliableTransmission::GetReceiveFilePath(std::string filePathName)
    {
        std::string str2("Send.txt");
        std::string resultString = filePathName + str2;
        return resultString;
    }

    End2EndReliableTransmission::~End2EndReliableTransmission(void)
    {
        pthread_mutex_destroy(&m_cs);
    }


    /*shared_ptr<const NAK_Message> End2EndReliableTransmission::TraverseNAK()
    {
        return m_recv_queue_map->TraverseNAK(m_dRemoteTermId);
    }*/

    //int count = 0;
    void End2EndReliableTransmission::HandleIO(std::shared_ptr<BYTE> szBytes, DWORD nRecv)
    {
        //EnterCriticalSection(&g_cs_for_mrudp);
        //cout << "{" << count++ << "," << nRecv << "}";
        //TODO 处理收到的数据或者信令
        switch (szBytes.get()[0])
        {
            case MRUDP_DATA_FIRST_BYTE_FLAG:
            {
                //表示收到的是数据
                OnReceiveData(szBytes, nRecv);

            }
                break;
            case MRUDP_MESSAGE_FIRST_BYTE_FLAG:
            {
                //表示收到的是命令
                OnReceiveMessage(szBytes, nRecv);
            }
                break;
            case MRUDP_DATA_FIRST_BYTE_FLAG_WITHOUT_RELIABLE:
            {
                //表示收到的是不可靠传输数据
                OnReceiveWithoutReliableData(szBytes, nRecv);
            }
            default:
                break;
        }

        //LeaveCriticalSection(&g_cs_for_mrudp);
    }

    void End2EndReliableTransmission::SendNAK()
    {
        //获得NAK对应的RBUDPmessage

        shared_ptr<NAK_Message> RbudpMessage = m_recv_queue_map->TraverseNAK_WithoutSeg(m_dRemoteTermId);

        if (RbudpMessage != nullptr) {
            //获得编码值
            std::shared_ptr<BYTE> message = nullptr;
            DWORD messageLength = 0;	//保存Message的实际长度
            message = RbudpMessage->Encode(messageLength);
            shared_ptr<NAK_Message> tmp = make_shared<NAK_Message>();
            tmp->Decode(message, messageLength);
            SendTIDCommandWithoutSpeedControl(m_dRemoteTermId, message, messageLength, true);
            //SPDLOG_LOGGER_WARN(rbudp_logger, "The nak is send, the length is {}", m_dRemoteTermId, messageLength);
            /*shared_ptr<KeepAlive> keepAliveMessage = make_shared<KeepAlive>();
            shared_ptr<BYTE> messageBytes = keepAliveMessage->Encode(messageLength);
            SendTIDCommandWithoutSpeedControl(m_dRemoteTermId, messageBytes, messageLength);*/
        }
    }

    bool End2EndReliableTransmission::SendUserData(const std::shared_ptr<BYTE> &data, const DWORD dLength)
    {
        shared_ptr<RBUDPDataSenderStore> RBUdpData;
        DWORD nTmpLength = 0;
        bool bSuccess = true;
        if (m_add_able) {
            RBUdpData = make_shared<RBUDPDataSenderStore>(data, dLength);
            //bSuccess = this->m_send_queue_map->AddItem(RBUdpData, m_dFileSize);
            bSuccess = this->m_send_queue_map->AddItemInStorage(RBUdpData, m_dFileSize);
            //this->m_send_queue_map->AddItem(RBUdpData);
        }
        else {
            //bSuccess = this->m_send_queue_map->AddItem(RBUdpData, m_dFileSize);
            RBUdpData = Last_Store;
        }
        if (bSuccess) {
            std::shared_ptr<BYTE> sendBytes = m_send_queue_map->SendDataByIndex(RBUdpData->GetPacketIndex(), nTmpLength);
            /*if (RBUdpData->GetPacketIndex() <= 10) {
                cout << RBUdpData->GetPacketIndex() << " ";
            }*/
            RBUdpData->SetEncodeBytes(nTmpLength);
            //this->m_send_queue_map->SetMaxIndex(dLength);
            bool res = false;
            if (RBUdpData->GetPacketIndex() == 0) {
                res = this->SendByteData(sendBytes, nTmpLength);
            }
            res = this->SendByteData(sendBytes, nTmpLength);
            //DWORD* pdata = (DWORD*)((BYTE*)sendBytes.get() + 6);
            //DWORD Index = ((*pdata & 0xFF000000) >> 24) | ((*pdata & 0x00FF0000) >> 8) | ((*pdata & 0x0000FF00) << 8) | ((*pdata & 0x000000FF) << 24);
            //SPDLOG_LOGGER_WARN(rbudp_nak_logger,  "{},{},{}", Index,RBUdpData->GetPacketIndex(),res);
            if (!res) {
                //SPDLOG_LOGGER_WARN(rbudp_rm_logger, "send packet fail {}", RBUdpData->GetPacketIndex());
                m_add_able = false;
                Last_Store = RBUdpData;
            }
            else {
                m_add_able = true;
                AddTrySendNumber();
                //SPDLOG_LOGGER_WARN(rbudp_rm_logger, "send packet success {}", RBUdpData->GetPacketIndex());
                //this->m_send_queue_map->AddItem(RBUdpData);
            }
            return res;
            //AddTrySendNumber();
            //return true;
        }
        return false;
    }
    //int times = 0;
    bool End2EndReliableTransmission::SendByteData(std::shared_ptr<BYTE> data, DWORD nLength) const {
        bool ret = false;
        DWORD ret_fail_count = 0;
        if (data) {
            while (!ret&&ret_fail_count < 5) {
                ret = SendTIDDataWithSpeedControl(m_dRemoteTermId, data, nLength, false, false, true);
                ret_fail_count++;
            }
            if(!ret && ret_fail_count >= 5) {
                //SPDLOG_LOGGER_WARN(rbudp_logger, "RBUDP send data fail after five try.");
                return false;
            }
            //times++;
            //cout << "send success total="<<times << endl;
            return true;
        }
        return false;
    }


    void End2EndReliableTransmission::OnReceiveData(std::shared_ptr<BYTE> szBytes, int nRecv) {
        //在接收到数据时将其解析并交付给上层使用
        shared_ptr<RBUDPdata> p_reliable_data = make_shared<RBUDPdata>();
        bool result = p_reliable_data->Decode(szBytes, nRecv);
        //cout<<"【End2End::OnReceiveData】 recv:"<<p_reliable_data->GetPacketIndex()<<" result:"<<result<<endl;
        if (result) {
            m_recv_queue_map->OnRecvDataFromLowerLayers(p_reliable_data);
        }
    }

    bool End2EndReliableTransmission::OnReceiveIndex(DWORD nRecv) {
        return this->m_send_queue_map->AddItemtoTimeout(nRecv);
    }


    void End2EndReliableTransmission::OnReceiveMessage(std::shared_ptr<BYTE> szBytes, int nRecv) {
        shared_ptr<NAK_Message> message = make_shared<NAK_Message>();
        bool b_decode = message->Decode(szBytes, nRecv);
        if (b_decode) {
            m_send_queue_map->OnRecvMessageFromRecvTerm(message);
        }
    }

    void End2EndReliableTransmission::OnReceiveMessage(std::shared_ptr<NAK_Message> message) {
        m_send_queue_map->OnRecvMessageFromRecvTerm(message);
    }


    void End2EndReliableTransmission::OnReceiveWithoutReliableData(std::shared_ptr<BYTE> szBytes, int nRecv)
    {
        //直接反馈给上层模块相关数据
        //解码不可靠传输消息
        DWORD new_length = 0;
        shared_ptr<BYTE> new_bytes = RBUDP_Decode_Data_Without_Reliable(std::move(szBytes), new_length);

        printf("_________________________\n");
        printf("+++++++++++++++++++++++++\n");
        printf("^^^^^^^^^^^^^^^^^^^^^^^^^\n");
        printf("RBUDP:recv unreliable data...\n");
        printf("Check Data type:\n");
        //检查数据类型，如果不是数据服务类型数据，则交给BigDataTransfer的回调
        //if(rbudp_dataserver_check(new_bytes, new_length)==false){
            printf("CallBackDataFunc => BigDataTransfer\n");
            CallBackDataFunc(m_dRemoteTermId, new_bytes, new_length);
            printf("...CallBackDataFunc...ok\n");
        //}
        //else { //如果是数据服务类型数据，则交给DATA SERVER的回调
        //    //MRUDP_DATASERVER_CALLBACK
        //    //数据回调服务对应的回调调用 240910-houlc spin_server_callback
        //    printf("spin_server_callback => DATA SERVER\n");
        //    bool res = rbudp_spin_server_callback(m_dRemoteTermId, new_bytes, new_length);
        //    printf("...spin_server_callback...ok(res:%d)\n", res);
        //}
        printf("#######################\n");

    }

    void End2EndReliableTransmission::OnCheckTimeOut()
    {
        if (m_send_queue_map->IsEmpty()) {
            m_send_queue_map->DealTimeOut();
            int t = m_send_queue_map->SendDataInSacks();
            AddRetrySendNUmber(t);
        }
        else
            usleep(1000);

    }


    float End2EndReliableTransmission::ComputeRetryRate()
    {
        //避免除数为0
        if (0 == m_try_send_number)
        {
            m_try_send_number++;
        }
        m_retry_rate = m_retry_send_number * 100.0 / (m_retry_send_number + m_try_send_number * 1.0);
        //SPDLOG_LOGGER_WARN(mrudp_logger, "Remote TID {} loss rate: {}", m_retry_rate);

        m_try_send_number = 0;
        m_retry_send_number = 0;
        return m_retry_rate;
    }
    bool End2EndReliableTransmission::AdjustBandWidth()
    {
        //return m_send_queue_map->SendAllUnACKPackets();
        //TODO
        return false;
    }

    void End2EndReliableTransmission::UploadRecvcQueue() {
        m_recv_queue_map->UploadDataPacket(m_dRemoteTermId);
    }
}
