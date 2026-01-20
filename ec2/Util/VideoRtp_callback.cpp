#include "VideoRtp_callback.h"

pVideoRtpRecvDataCallBack g_video_rtp_recv_callback = nullptr;

void Register_VideoRtp_RecvDataCallBack(pVideoRtpRecvDataCallBack pFunc) {
    g_video_rtp_recv_callback = pFunc;
}

