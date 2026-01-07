//
// Created by 王炳棋 on 2022/11/18.
//
#include <string>

#ifndef INIHANDLE_INIHANDLEAPI_H
#define INIHANDLE_INIHANDLEAPI_H


/**
 * \brief 这是配置文件模块的初始化函数，用于初始化配置文件
 * \param iniPath 配置文件的绝对路径
 * \return
 */
bool InitIniHandler(const std::string& iniPath);

/**
 * \brief 获取配置文件中某个段，某个key的值，并给其一个默认值。注意该字段必须是整数。
 * \param section
 * \param key
 * \param defaultValue
 * \return
 */

int GetIntegerKeyIni(std::string section, std::string key, int defaultValue);

/**
 * \brief 获取配置文件中某个段，某个key的值，并给其一个默认值。注意该字段是字符串类型的
 * \param section
 * \param key
 * \param defaultValue
 * \return
 */

std::string GetStringValueKeyIni(std::string section, std::string key, std::string defaultValue);

/**
 * \brief 读取配置文件某段中某关键字的值，为float类型
 * \param section
 * \param key
 * \param defaultValue
 * \return
 */

float GetRealValueKeyIni(std::string section, std::string key, float defaultValue);

/**
 * \brief 读取配置文件中某段中某关键字对应的值，为布尔类型
 * \param section
 * \param key
 * \param defaultValue
 * \return
 */

bool GetBoolValueKeyIni(std::string section, std::string key, bool defaultValue);


bool UninitIniHandle();

#endif //INIHANDLE_INIHANDLEAPI_H
