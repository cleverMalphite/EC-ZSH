//
// Created by 王炳棋 on 2022/11/18.
//

#include "IniHandleApi.h"
#include "INIReader.h"

#include <cctype>
#include <climits>
#include <cstdlib>
#include <fstream>
#include <string>

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

namespace {

bool FileExists(const std::string &path) {
    std::ifstream ifs(path.c_str());
    return ifs.good();
}

int ParsePositiveInt(const char *value, int fallback) {
    if (value == nullptr || *value == '\0') {
        return fallback;
    }

    char *endPtr = nullptr;
    long parsed = std::strtol(value, &endPtr, 10);
    if (endPtr == value || *endPtr != '\0' || parsed <= 0 || parsed > INT_MAX) {
        return fallback;
    }

    return static_cast<int>(parsed);
}

int ReadPositiveIntEnv(const char *name, int fallback) {
    return ParsePositiveInt(std::getenv(name), fallback);
}

std::string ReadStringEnv(const char *name, const std::string &fallback) {
    const char *value = std::getenv(name);
    if (value == nullptr || *value == '\0') {
        return fallback;
    }

    return value;
}

int GuessDeviceIdFromIniPath(const std::string &iniPath) {
    int deviceId = ParsePositiveInt(std::getenv("EC2_DEVICE_ID"), -1);
    if (deviceId > 0) {
        return deviceId;
    }

    std::string fileName = iniPath;
    size_t pos = fileName.find_last_of("/\\");
    if (pos != std::string::npos) {
        fileName = fileName.substr(pos + 1);
    }

    std::string digits;
    for (char ch: fileName) {
        if (std::isdigit(static_cast<unsigned char>(ch))) {
            digits.push_back(ch);
        }
    }

    deviceId = ParsePositiveInt(digits.c_str(), -1);
    if (deviceId > 0) {
        return deviceId;
    }

    return 101;
}

bool CreateDefaultIniIfMissing(const std::string &iniPath) {
    if (FileExists(iniPath)) {
        return true;
    }

    const int deviceId = GuessDeviceIdFromIniPath(iniPath);
    const int defaultRemoteTid = (deviceId == 101) ? 102 : 101;
    const int remoteTid = ReadPositiveIntEnv("EC2_REMOTE_TID", defaultRemoteTid);

    int role = ReadPositiveIntEnv("EC2_AUTOCONN_ROLE", (deviceId == 101) ? 1 : 2);
    if (role != 1 && role != 2) {
        role = (deviceId == 101) ? 1 : 2;
    }

    const int defaultLocalPort = (role == 1) ? 3020 : 3021;
    const int defaultRemotePort = (role == 1) ? 3021 : 3020;
    const int localPort = ReadPositiveIntEnv("EC2_LOCAL_PORT", defaultLocalPort);
    const int remotePort = ReadPositiveIntEnv("EC2_REMOTE_PORT", defaultRemotePort);

    const std::string localIp = ReadStringEnv("EC2_LOCAL_IP", "127.0.0.1");
    const std::string remoteIp = ReadStringEnv("EC2_REMOTE_IP", "127.0.0.1");

    std::ofstream ofs(iniPath.c_str(), std::ios::out | std::ios::trunc);
    if (!ofs.is_open()) {
        return false;
    }

    ofs
            << "[Main]\n"
            << "DeviceID=" << deviceId << "\n"
            << "NETCOMM_MAX_UNDEAL_NUMBER=8000\n"
            << "SPEEDCONTROL_MAX_UNDEAL_NUMBER=20000\n"
            << "CheckFrequency=1000\n"
            << "TestCycle=1000\n"
            << "UdpSocketBufferSize=16777216\n"
            << "IsForceTcp=false\n"
            << "IsNat=false\n"
            << "bIsPublicServer=false\n"
            << "bIsClientConnectServer=false\n\n"
            << "[SpeedControl]\n"
            << "ExpectSpeed=300\n\n"
            << "[BigDataTransfer]\n"
            << "recv_dir=../../FileRecv/\n"
            << "send_dir=../../FileSend/\n"
            << "DEST_TID=" << remoteTid << "\n\n"
            << "[NETCOMM]\n"
            << "UdpSocketBufferSize=67108864\n"
            << "Tcp_PostRecv_Number=5\n"
            << "Udp_PostRecv_Number=10\n"
            << "Bcast_PostRecv_Number=5\n\n"
            << "[QOS]\n"
            << "IS_QOS_OPEN=false\n"
            << "Hop_count=1\n\n"
            << "[MRUDP]\n"
            << "ScanNextQueueHeadForRetransfer=20\n"
            << "TIMESTAMP_DIFF_THRESHOLD=90\n"
            << "TimestampDiffThreshold=90\n"
            << "RUDP_CYCLEFILE_CAPACITY=40000\n"
            << "RUDP_FILE_PACKET_ENCODE_LENGTH=1400\n"
            << "RUDP_SACK_BLOCKS_MAX_NUM=8\n"
            << "WINDOWS_EXPECT_NUM_MULTI_WHEN_LOSS_DETECTED=1\n"
            << "RUDP_UNDEAL_DATA_NUMBER=30000\n\n"
            << "[RBUDP]\n"
            << "RBUDP_SEND_TEMP_FILEPATH=/tmp/ec2_rbudp_send.tmp\n"
            << "RBUDP_RECV_TEMP_FILEPATH=/tmp/ec2_rbudp_recv.tmp\n"
            << "RBUDP_FILESIZE=1369\n\n"
            << "[DTU]\n"
            << "LocalPortBegin=8000\n"
            << "LocalPortEnd=8999\n"
            << "NatPortBegin=7000\n"
            << "NatPortEnd=7999\n\n"
            << "[Display]\n"
            << "IsSatellite=false\n"
            << "IsMobile=false\n"
            << "IsBroadband=false\n\n"
            << "[FirstOutDoorTest]\n"
            << "IsServer=false\n"
            << "LocalIP=" << localIp << "\n"
            << "LocalPort=" << localPort << "\n"
            << "RemoteIP=" << remoteIp << "\n"
            << "RemotePort=" << remotePort << "\n"
            << "useRBUDP=false\n"
            << "Runtime=1800\n\n"
            << "[AutoConn]\n"
            << "Role=" << role << "\n"
            << "LocalIP=" << localIp << "\n"
            << "LocalPort=" << localPort << "\n"
            << "RemoteIP=" << remoteIp << "\n"
            << "RemotePort=" << remotePort << "\n"
            << "RemoteTID=" << remoteTid << "\n"
            << "RetryIntervalMs=3000\n"
            << "HeartbeatTimeoutMs=5000\n";

    return true;
}

} // namespace

/**
 * \brief 这是配置文件模块的初始化函数，用于初始化配置文件
 * \param iniPath 配置文件的绝对路径
 * \return
 */
bool InitIniHandler(const std::string& iniPath) {
    if (nullptr == iniReader) {
        if (!CreateDefaultIniIfMissing(iniPath)) {
            return false;
        }
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
        iniReader = nullptr;
    }

    return true;
}