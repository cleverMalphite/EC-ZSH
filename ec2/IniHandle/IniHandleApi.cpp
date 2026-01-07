//
// Created by 王炳棋 on 2022/11/18.
//

#include "IniHandleApi.h"
#include "INIReader.h"

/**
 * 这个模块主要负责读取配置文件中配置项的值
 * 目前引入的是 https://github.com/jtilly/inih 这个第三方开源库
 * 本模块目前是对该模块的封装
 */

//定义一些常用的默认配置项值
#define DEFAULT_INT_VALUE -1
#define DEFAULT_STRING_VALUE "UNKNOWN"
#define DEFAULT_REAL_VALUE -1.0
#define DEFAULT_BOOL_VALUE false

//统一的读取配置文件的对象，注意各个模块调用InitIniHandler函数时务必保证配置文件中的iniPath一致，因为这个配置对象只会被初始化一次
INIReader *iniReader = nullptr;

/**
 * \brief 这是配置文件模块的初始化函数，用于初始化配置文件
 * \param iniPath 配置文件的绝对路径
 * \return
 */
bool InitIniHandler(const std::string& iniPath) {
    if (nullptr == iniReader) {
        //iniReader只会被初始化一次
        iniReader = new INIReader(iniPath);
        return 0 == (iniReader->ParseError());
    }

    return true;
}


/**
 * \brief 获取配置文件中某个段，某个key的值，并给其一个默认值。注意该字段必须是整数。
 * \param section
 * \param key
 * \param defaultValue
 * \return
 */
int GetIntegerKeyIni(std::string section, std::string key, int defaultValue) {
    if (iniReader) {
        return iniReader->GetInteger(section, key, defaultValue);
    }

    //TODO 其实应该给用户警示信息的
    return DEFAULT_INT_VALUE;
}


/**
 * \brief 获取配置文件中某个段，某个key的值，并给其一个默认值。注意该字段是字符串类型的
 * \param section
 * \param key
 * \param defaultValue
 * \return
 */

std::string GetStringValueKeyIni(std::string section, std::string key, std::string defaultValue) {
    if (nullptr != iniReader) {
        return iniReader->Get(section, key, defaultValue);
    }

    return DEFAULT_STRING_VALUE;
}


/**
 * \brief 读取配置文件某段中某关键字的值，为float类型
 * \param section
 * \param key
 * \param defaultValue
 * \return
 */

float GetRealValueKeyIni(std::string section, std::string key, float defaultValue) {
    if (nullptr != iniReader) {
        return iniReader->GetReal(section, key, defaultValue);
    }

    return DEFAULT_REAL_VALUE;
}

/**
 * \brief 读取配置文件中某段中某关键字对应的值，为布尔类型
 * \param section
 * \param key
 * \param defaultValue
 * \return
 */

bool GetBoolValueKeyIni(std::string section, std::string key, bool defaultValue) {
    if (nullptr != iniReader) {
        return iniReader->GetBoolean(section, key, defaultValue);
    }

    return DEFAULT_BOOL_VALUE;
}


bool UninitIniHandle() {
    if (nullptr != iniReader) {
        delete iniReader;
    }

    return true;
}