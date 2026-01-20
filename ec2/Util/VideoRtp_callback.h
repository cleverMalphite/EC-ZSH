#ifndef EC2_VIDEO_RTP_CALLBACK_H
#define EC2_VIDEO_RTP_CALLBACK_H

#include <memory>
#include "../mGlobalDef.h"

typedef bool (*pVideoRtpRecvDataCallBack)(
    DWORD dwTID,
    const std::shared_ptr<BYTE> &pBuffer,
    DWORD dwLength,
    long int &recvtime,
    long int &fb_send_time);

void Register_VideoRtp_RecvDataCallBack(pVideoRtpRecvDataCallBack pFunc);

extern pVideoRtpRecvDataCallBack g_video_rtp_recv_callback;

#endif

