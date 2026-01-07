//
// Created by 王炳棋 on 2022/12/11.
//
#include "DataWithPacketInfo.h"
#include "../NetCombTransfer/NetCombTransferApi.h"
#include "SpeedControlManager.h"

namespace SpeedControl {
    extern std::shared_ptr<SpeedControlManager> gSpeedControlManager;
    extern bool g_speed_control_stop;
    /*extern HANDLE g_speed_control_event;*/

    /**
	 * 本模块注册到别的模块的回调函数
	 */
    //本模块注册到NetCombTransfer模块的终端上下线信息回调函数
    bool SpeedControl_NetCombTransferMessageCallback(DWORD dwCID, bool bCreate) {
        if (nullptr != gSpeedControlManager) {

            /*if (NULL != g_speed_control_event) {
                SetEvent(g_speed_control_event);
            }*/

            gSpeedControlManager->DoRecvTermOnOrOff(dwCID, bCreate);
            return true;
        }
        return false;
    }

    /*DWORD Recv_Count = 0;  //总收包次数
    DWORD OutTime_count = 0;  //超时次数
    DWORD Last_time = 0;
    DWORD This_time = 0;

    DWORD RecvData_Count = 0;
    DWORD StartData_time = 0;
    DWORD LastData_time = 0;

    DWORD recv_data_count = 0;
    DWORD number_of_thousand = 0;*/

    bool SpeedControl_NetCombTransferBufferCallback(DWORD dwTID, const std::shared_ptr<BYTE>& pBuffer, DWORD dwLength, bool bMsg,
                                                    int64_t packet_recv_Timestamp) {
        if (nullptr != gSpeedControlManager) {
            /*if (NULL != g_speed_control_event) {
                SetEvent(g_speed_control_event);
            }*/
            gSpeedControlManager->DoRecvData(dwTID, pBuffer, dwLength, packet_recv_Timestamp);
            return true;
        }
        return false;
    }
}

//本模块注册到QoS模块的数据发送速率控制信息回调函数 TODO 别忘了回调函数里都要加信号量
bool SpeedControl_QoSSpeedInfoCallBack(TermSpeedInfo term_speed_info)
{
    if (nullptr != SpeedControl::gSpeedControlManager)
    {
        /*if (NULL != SpeedControl::g_speed_control_event)
        {
            SetEvent(SpeedControl::g_speed_control_event);
        }*/
        SpeedControl::gSpeedControlManager->DoRecvSpeedInfo(term_speed_info);
        return true;
    }
    return false;
}

//注册到NetCombTransfer模块的路由回调
void ON_ROUTE_CALL_BACK_FROM_SPEEDCONTROL(DWORD dwSourceTID, DWORD dwDestinationTID, UINT m_nNop, UINT m_nQoSSend, UINT m_nQosRecv, bool bSetted)
{
    //获取本终端ID
    if (nullptr != SpeedControl::gSpeedControlManager)
    {
        /*if (NULL != SpeedControl::g_speed_control_event)
        {
            SetEvent(SpeedControl::g_speed_control_event);
        }*/

        SpeedControl::gSpeedControlManager->DoRecvTermOnOrOff(dwDestinationTID, bSetted);
    }
}

