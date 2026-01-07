//
// Created by ubuntu402 on 23-12-2.
//

#include "isRBUDP_callback.h"


//传递is_RBUDP的回调函数
pMessageCallBack m_isRBUDP_MessageCallback;

void Register_isRBUDP_CallBack(pMessageCallBack messageCallBack){
    m_isRBUDP_MessageCallback=messageCallBack;
}