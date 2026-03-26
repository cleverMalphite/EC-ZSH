#include "AudioRtp_callback.h"

pAudioRtpRecvDataCallBack g_audio_rtp_recv_callback = nullptr;

void Register_AudioRtp_RecvDataCallBack(pAudioRtpRecvDataCallBack pFunc) {
    g_audio_rtp_recv_callback = pFunc;
}
