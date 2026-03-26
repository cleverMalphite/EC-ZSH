#ifndef EC2_AUDIO_RTP_CALLBACK_H
#define EC2_AUDIO_RTP_CALLBACK_H

#include <memory>
#include "../mGlobalDef.h"

typedef bool (*pAudioRtpRecvDataCallBack)(
    DWORD dwTID,
    const std::shared_ptr<BYTE> &pBuffer,
    DWORD dwLength,
    long int &recvtime,
    long int &fb_send_time);

void Register_AudioRtp_RecvDataCallBack(pAudioRtpRecvDataCallBack pFunc);

extern pAudioRtpRecvDataCallBack g_audio_rtp_recv_callback;

#endif
