//
// Created by ubuntu402 on 23-12-2.
//

#ifndef EMERGENCYCOMMUNICATION_ISRBUDP_CALLBACK_H
#define EMERGENCYCOMMUNICATION_ISRBUDP_CALLBACK_H

typedef void(* pMessageCallBack)(bool is_RBUDP);

//定义回调函数
extern pMessageCallBack m_isRBUDP_MessageCallback;

//将传递is_RBUDP的回调函数注册进来
void Register_isRBUDP_CallBack(pMessageCallBack messageCallBack);

#endif //EMERGENCYCOMMUNICATION_ISRBUDP_CALLBACK_H
