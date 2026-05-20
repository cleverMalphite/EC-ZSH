//
// Created by 王炳棋 on 2022/12/8.
//
#include <stdio.h>
#include "../NetCombTransfer/NetCombTransferApi.h"
#include "Term2TermTransmission.h"

#include "infoHub/infoHub.h"
#include "infoHub/rateStatistic.h"
#include "infoHub/rateStatisticTable.h"

extern std::shared_ptr<ec2::infoHub> \
           _speedcontrol_infohub_instance;

extern void CallQoSSelf2RemoteInfoFunc(std::shared_ptr<const SelfToRemoteQosInfo> p_self_to_remote_qos_info);

extern void SpeedControl_RBUDPIndexCallBack(DWORD dwTID, DWORD dIndex);

namespace SpeedControl {

    int64_t su_get_sys_time_stamp() {
        timeval tv;
        gettimeofday(&tv, nullptr);
        return (tv.tv_sec * 1000000 + tv.tv_usec) / 1000;
    }

    /*DWORD StartTime = 0;*/

    bool Term2TermTransmission::DoSendData() {
        bool bRealSendResult = false;
        bool bResult = true;
        while (bResult) {
            //printf("DoSendData------Send queue_size is %zu\n", data_send_buffer.size());
            if (m_speed_change) {
                DWORD currentSpeed = 0;
                pthread_mutex_lock(&TempSpeedMutex_);
                currentSpeed = m_d_speed;
                m_speed_change = false;
                pthread_mutex_unlock(&TempSpeedMutex_);
                //printf("change Speed to %d Mbps\n", currentSpeed);
                DoChangeExpectSpeed(currentSpeed);
            }

            bool isForceSleep = false;
            pthread_mutex_lock(&Mutex_);
            if (data_send_buffer.empty()) {
                //printf("[SpeedControlDebug]::通往%d的数据队列中数据包数量为空\n", m_dest_tid);
                pthread_mutex_unlock(&Mutex_);
                bResult = false;
            } else {
                //printf("[SpeedControlDebug]::数据队列中数据包数量:%ld\n", data_send_buffer.size());
                std::shared_ptr<DataSpeedControlSend> data_send = data_send_buffer.front();
                data_send_buffer.pop_front();
                //printf("[SpeedControlDebug]::after_pop数据队列中数据包数量:%ld\n", data_send_buffer.size());
                pthread_mutex_unlock(&Mutex_);
                DWORD dwTID = m_dest_tid;
                std::shared_ptr<BYTE> pBuffer = data_send->pData;
                DWORD dLength = data_send->dataLength;
                bool isRBUDP = data_send->m_isRBUDP;

                DWORD dIndex = 0;
                if (dLength > 1300 && isRBUDP) {
                    DWORD *pdata = (DWORD *) ((BYTE *) pBuffer.get() + 6);
                    dIndex = ((*pdata & 0xFF000000) >> 24) | ((*pdata & 0x00FF0000) >> 8) |
                             ((*pdata & 0x0000FF00) << 8) | ((*pdata & 0x000000FF) << 24);
                    //if (dIndex == 0) {
                    //	std::cout << 1;
                    //}
                }

                //Add Qos Code -- 1
                SelfToRemoteQosInfo self_to_remote;
                self_to_remote.m_d_index = m_d_packet_index;
                m_d_packet_index++;
                if (m_d_packet_index >= 65530) {
                    m_d_packet_index = 0;
                }
                self_to_remote.m_d_remote_tid = dwTID;
                //self_to_remote.m_d_timestamp = GetTickCount();
                std::shared_ptr<DataWithPacketInfo> p_packet = std::make_shared<DataWithPacketInfo>(dwTID,
                                                                                                    self_to_remote,
                                                                                                    pBuffer, dLength,isRBUDP);
                //TEST:测试视频传输
                DWORD len_buffer=0;
                std::shared_ptr<BYTE> bufferData=p_packet->Encode(len_buffer);
//                fwrite(bufferData.get(),len_buffer,1,fp_DataWithPacketInfo_send);

                //
                if (dLength < MIN_PACKET_LENGTH_IGNORE) {
                    //Add Qos Code -- 2
                    DWORD dTemp = 0;
                    std::shared_ptr<BYTE> p_buffer = p_packet->Encode(dTemp);
                    //
                    if (nullptr != p_buffer) {
                        //printf("[QosDebug]::Send Qos Packet\n");
                        //Add Qos Code -- 3
                        //CallQoSSelf2RemoteInfoFunc(p_packet->GetSelfToRemoteQosInfo());
                        //
                        //printf("dLength is %d\n",dLength);
                        //通常小于MIN_PACKET_LENGTH_IGNORE的包是BigDataTransfer的信令
                        if (IsForceTcp) {
                            bResult = SendTIDCommand(dwTID, p_buffer, dTemp);
                        } else {
                            bResult = SendTIDData(dwTID, p_buffer, dTemp, false);
                        }
                        if (bResult) {
                            bRealSendResult = true;
                        }
                    }
                } else {
                    //printf("[QosDebug]::Send Data Packet\n");
                    bResult = DoCycleSend(p_packet);
                    //printf("[QosDebug]::Cycle Send\n");
                    if (bResult) {
                        bRealSendResult = true;
                        if (dLength > 1300 && isRBUDP) {
                            SpeedControl_RBUDPIndexCallBack(dwTID, dIndex);
                        }
                    }
                }
                isForceSleep = data_send->m_isForceSleep;
            }
            if (isForceSleep) {
                usleep(1000);
            }
        }
        return bRealSendResult;
    }

    DWORD Term2TermTransmission::GetPendingQueueDepth() {
        pthread_mutex_lock(&Mutex_);
        const DWORD pending = static_cast<DWORD>(data_send_buffer.size());
        pthread_mutex_unlock(&Mutex_);
        return pending;
    }

    bool
    Term2TermTransmission::PushSendData(const std::shared_ptr<BYTE> &data, DWORD length, bool isSleep, bool isRBUDP) {
        pthread_mutex_lock(&Mutex_);
        if (m_buffer_max_size > data_send_buffer.size()) {
            std::shared_ptr<DataSpeedControlSend> pData = std::make_shared<DataSpeedControlSend>(data, length, isSleep,
                                                                                                 isRBUDP);
            data_send_buffer.push_back(pData);
            //printf("[AutoTestDebug]::成功将数据包加入到通信实例中去,队列数据包个数:%zu\n", data_send_buffer.size());
            pthread_mutex_unlock(&Mutex_);
            return true;
        } else {
            pthread_mutex_unlock(&Mutex_);
            //printf("[AutoTestDebug]::未成功将数据包加入到通信实例中\n");
            return false;
        }
    }

    bool Term2TermTransmission::PushSendDataPri(const std::shared_ptr<BYTE> &data, DWORD length, bool isSleep,
                                                bool isRBUDP) {
        pthread_mutex_lock(&Mutex_);
        if (m_buffer_max_size > data_send_buffer.size()) {
            std::shared_ptr<DataSpeedControlSend> pData = std::make_shared<DataSpeedControlSend>(data, length, isSleep,
                                                                                                 isRBUDP);
            //printf("[SpeedControlDebug]::加入紧急重传数据包，数据队列中数据包数量:%ld\n", data_send_buffer.size());
            data_send_buffer.push_front(pData); //紧急重传包放队首
            pthread_mutex_unlock(&Mutex_);
            return true;
        } else {
            pthread_mutex_unlock(&Mutex_);
            return false;
        }
    }

    bool Term2TermTransmission::DoSendDataWithoutSpeedControl(const std::shared_ptr<BYTE> &data, DWORD length, bool isRBUDP) {
        bool bResult = false;
        SelfToRemoteQosInfo self_to_remote;
        self_to_remote.m_d_index = m_d_packet_index;
        //m_d_packet_index++;
        if (m_d_packet_index >= 65530) {
            m_d_packet_index = 0;
        }
        self_to_remote.m_d_remote_tid = m_dest_tid;
        //self_to_remote.m_d_timestamp = GetTickCount();  时间戳在Encode时更新
        std::shared_ptr<DataWithPacketInfo> p_packet =
                std::make_shared<DataWithPacketInfo>(m_dest_tid, self_to_remote, data, length,isRBUDP);
        DWORD dTemp = 0;
        std::shared_ptr<BYTE> p_buffer = p_packet->Encode(dTemp);
        if (nullptr != data) {
            if (IsForceTcp) {
                bResult = SendTIDCommand(m_dest_tid, p_buffer, dTemp);
            } else {
                bResult = SendTIDData(m_dest_tid, p_buffer, dTemp, false);
            }
        }
        return bResult;
    }

    void Term2TermTransmission::RecvTermSpeedInfo(const std::shared_ptr<TermSpeedInfo> &t2t_speed) {
        pthread_mutex_lock(&TempSpeedMutex_);
        m_d_speed = t2t_speed->m_d_speed;
        m_speed_change = true;
        pthread_mutex_unlock(&TempSpeedMutex_);
    }

    bool Term2TermTransmission::DealTermSpeedInfo(const std::shared_ptr<TermSpeedInfo> &term_speed_info) {
        DoChangeExpectSpeed(term_speed_info->m_d_speed);
        return true;
    }

    bool Term2TermTransmission::SendTIDCommandWithoutSpeedControl(const std::shared_ptr<BYTE> &data, DWORD length, bool isRBUDP) {
        bool bResult = false;
        //pthread_mutex_lock(&Mutex_);
        SelfToRemoteQosInfo self_to_remote;
        self_to_remote.m_d_index = m_d_packet_index;
        //m_d_packet_index++;
        if (m_d_packet_index >= 65530) {
            m_d_packet_index = 0;
        }
        self_to_remote.m_d_remote_tid = m_dest_tid;
        //self_to_remote.m_d_timestamp = GetTickCount();
        std::shared_ptr<DataWithPacketInfo> p_packet =
                std::make_shared<DataWithPacketInfo>(m_dest_tid, self_to_remote, data, length,isRBUDP);
        DWORD dTemp = 0;
        std::shared_ptr<BYTE> p_buffer = p_packet->Encode(dTemp);

        if (nullptr != data) {
            bResult = SendTIDCommand(m_dest_tid, p_buffer, dTemp);
        }
        //pthread_mutex_unlock(&Mutex_);
        return bResult;
    }

    bool Term2TermTransmission::DoCycleSend(const std::shared_ptr<DataWithPacketInfo> &pPacket) {
        static DWORD udp_send_success_packets = 0;
        static DWORD StartTime = su_get_sys_time_stamp();
        bool sleepExtra = false;
        udp_send_success_packets++;
        //printf("StartTime is %d\n", StartTime);
        DWORD ChangePacketCycle = m_d_big_cycle >= CHECK_FREQUENCY ? CHECK_FREQUENCY : m_d_big_cycle;
        if (udp_send_success_packets > ChangePacketCycle) {

            DWORD delay = (su_get_sys_time_stamp() - StartTime);

            float udp_packet_send_real_frequency = udp_send_success_packets * 1000.0 / (delay + 0.01);
            //printf("[SpeedControl Test]::udp_packet_send_real_frequency :%f,delay is %d ms\n", udp_packet_send_real_frequency, delay);
            DWORD udp_packet_send_expect_frequency = m_d_big_cycle * 10;
            //printf("[SpeedControl Test]::udp_packet_send_expect_frequency :%d\n", udp_packet_send_expect_frequency);

            if (udp_packet_send_real_frequency > 1.2 * udp_packet_send_expect_frequency) {
                DWORD old_small_cycle = m_d_big_cycle;
                m_d_small_cycle *= 0.8;
                if (old_small_cycle <= 1 || m_d_big_cycle <= 1) {
                    sleepExtra = true;
                }
            } else if (udp_packet_send_real_frequency < udp_packet_send_expect_frequency * 0.8
                       && m_d_small_cycle * 1.2 < (m_d_big_cycle / m_d_max_small_cycle_of_big_cycle)) {
                m_d_small_cycle *= 1.2;
            }
            udp_send_success_packets = 0;
            StartTime = su_get_sys_time_stamp();
        }

        DWORD d_small_cycle = m_d_small_cycle;
        DWORD d_big_cycle = m_d_big_cycle;

        DWORD d_temp = 0;
        std::shared_ptr<BYTE> p_temp_data = nullptr;
        p_temp_data = pPacket->Encode(d_temp);
        /*std::shared_ptr<DataWithPacketInfo> p;
        p->Decode(p_temp_data,1389);*/
        bool bResult = false;
        if (IsForceTcp) {
            bResult = SendTIDCommand(m_dest_tid, p_temp_data, d_temp);
        } else {
            bResult = SendTIDData(m_dest_tid, p_temp_data, d_temp, false);
            //printf("[SC::AutoTestDebug]::向TID:%d发送数据包,发送结果:%d\n", m_dest_tid, bResult);
        }
        CallQoSSelf2RemoteInfoFunc(pPacket->GetSelfToRemoteQosInfo());
        //printf("[QosDebug]::Return From QosSelf2RemoteCallbackFunc\n");
#ifndef NO_SPEED_CONTROL
        if (m_d_current_big_cycle > d_big_cycle) {
            DWORD d_delay_time = (su_get_sys_time_stamp() -
                                  ((m_d_big_cycle_begin_time.tv_sec * 1000000 + m_d_big_cycle_begin_time.tv_usec) /
                                   1000));
            //printf("[SpeedControl Test]::实际一次大循环时间约为%dms\n", d_delay_time);
            if (d_delay_time < m_d_expect_time_per_big_cycle) {
                DWORD current_sleep_time = m_d_expect_time_per_big_cycle - d_delay_time;
                //const DWORD start_big_sleep = su_get_sys_time_stamp();
                if (sleepExtra) {
                    usleep(500 * 1000);
                }
                usleep(current_sleep_time * 1000);
            }
            m_d_current_big_cycle = 0;
            m_d_current_small_cycle = 0;
            gettimeofday(&m_d_big_cycle_begin_time, nullptr);
        } else {
            m_d_current_big_cycle++;
            m_d_current_small_cycle++;
            if (m_d_current_small_cycle > d_small_cycle) {
                DWORD SetSmallCycleSleepTime = 1;
                if (sleepExtra) {
                    SetSmallCycleSleepTime++;
                }
                usleep(SetSmallCycleSleepTime * 1000);
                m_d_current_small_cycle = 0;
            }
        }
#endif
        return bResult;
    }

    void Term2TermTransmission::DoChangeExpectSpeed(double d_speed) {
        pthread_mutex_lock(&Mutex_);

        m_d_speed = d_speed;

        //------- infohub -------------
        _speedcontrol_infohub_instance->value_set("speedcontrol", "expect_speed_stat", d_speed);

        //printf("[SpeedControl Test]::设置期望速度为%dMbps\n", m_d_speed / 1000000);
        //MARK
        /*printf("m_d_single_packet_length is %d\n", m_d_single_packet_length);*/
        m_d_big_cycle = ((d_speed / 8000) * m_d_expect_time_per_big_cycle) / (m_d_single_packet_length);
        m_d_small_cycle = m_d_big_cycle / 10 + 1;
        pthread_mutex_unlock(&Mutex_);
        //printf("[SpeedControl Test]::向终端%d的通道速率更新,大循环发包数目计算为%d,小循环为%d\n", m_dest_tid, m_d_big_cycle, m_d_small_cycle);
    }

}
