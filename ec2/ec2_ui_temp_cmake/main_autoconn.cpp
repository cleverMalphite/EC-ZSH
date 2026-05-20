#include "AutoConnWindow.h"
#include "AutoConnController.h"
#include "VideoReceiver.h"
#include "function.h"

VideoReceiver *g_videoReceiver = nullptr;

#include <QApplication>
#include <unistd.h>
#include <limits.h>
#include <vector>
#include <string>
#include <cstdlib>
#include <cerrno>

CLogFile logfile;

void EXIT(int sig);
static std::string resolve_sysconfig_path(const std::string &fileName);

static bool parse_positive_int(const char *arg, int &value)
{
    if (arg == nullptr || *arg == '\0') {
        return false;
    }

    errno = 0;
    char *end = nullptr;
    long parsed = std::strtol(arg, &end, 10);
    if (errno != 0 || end == arg || *end != '\0' || parsed <= 0 || parsed > INT_MAX) {
        return false;
    }

    value = static_cast<int>(parsed);
    return true;
}

static std::vector<std::string> candidate_cfg_names_by_terminal(int terminalNo)
{
    if (terminalNo == 1) {
        return {"SysConfig1.ini", "SysConfig101.ini"};
    }
    if (terminalNo == 2) {
        return {"SysConfig2.ini", "SysConfig102.ini"};
    }
    return {"SysConfig" + std::to_string(terminalNo) + ".ini"};
}

static std::string choose_cfg_path_by_terminal(int terminalNo)
{
    const std::vector<std::string> names = candidate_cfg_names_by_terminal(terminalNo);
    for (const auto &name : names) {
        const std::string path = resolve_sysconfig_path(name);
        if (access(path.c_str(), F_OK) == 0) {
            return path;
        }
    }

    // 如果都不存在，返回首选名字，后续由IniHandle自动生成。
    return resolve_sysconfig_path(names.front());
}

static std::string resolve_sysconfig_path(const std::string &fileName)
{
    std::vector<std::string> candidates;

    char cwdBuf[PATH_MAX] = {0};
    if (getcwd(cwdBuf, sizeof(cwdBuf) - 1) != nullptr) {
        candidates.emplace_back(std::string(cwdBuf) + "/../" + fileName);
        candidates.emplace_back(std::string(cwdBuf) + "/" + fileName);
    }

    char exeBuf[PATH_MAX] = {0};
    const ssize_t exeLen = readlink("/proc/self/exe", exeBuf, sizeof(exeBuf) - 1);
    if (exeLen > 0) {
        exeBuf[exeLen] = '\0';
        std::string exePath(exeBuf);
        const size_t slash = exePath.find_last_of('/');
        if (slash != std::string::npos) {
            const std::string exeDir = exePath.substr(0, slash);
            candidates.emplace_back(exeDir + "/../../" + fileName);
            candidates.emplace_back(exeDir + "/../" + fileName);
            candidates.emplace_back(exeDir + "/" + fileName);
        }
    }

    const char *env = std::getenv("EC2_SYS_CONFIG");
    if (env && env[0] != '\0') {
        candidates.insert(candidates.begin(), std::string(env));
    }

    for (const auto &p : candidates) {
        if (access(p.c_str(), F_OK) == 0) {
            return p;
        }
    }
    return "../" + fileName;
}

/**
 * 用法：
 *   ./ec2_autoconn        → 默认加载终端1配置（优先SysConfig1.ini，其次SysConfig101.ini）
 *   ./ec2_autoconn 2      → 加载终端2配置（优先SysConfig2.ini，其次SysConfig102.ini）
 *   ./ec2_autoconn 3      → 加载 SysConfig3.ini（不存在会自动生成）
 *   ./ec2_autoconn 4/5/6  → 同理，自动走 SysConfig4/5/6.ini
 *
 * AutoConn 配置节（SysConfig*.ini 中的 [AutoConn]）决定角色和地址，
 * 无需手动操作即可自动建立/恢复连接。
 */
int main(int argc, char *argv[])
{
    signal(SIGINT,  EXIT);
    signal(SIGTERM, EXIT);

    // ---- 加载配置 ----
    int terminalNo = 1;
    if (argc >= 2) {
        int parsed = 0;
        if (parse_positive_int(argv[1], parsed)) {
            terminalNo = parsed;
        } else {
            std::cout << "[AutoConn] 参数无效，使用默认终端1配置: " << argv[1] << std::endl;
        }
    }

    std::string cfgFile = choose_cfg_path_by_terminal(terminalNo);

    std::cout << "[AutoConn] 使用配置: " << cfgFile << std::endl;

    // ---- 启动底层系统（不开 QoS，不用 RBUDP） ----
    System_start(cfgFile.c_str(), false, false);

    // UI角色使用实际配置项，避免命令参数和配置角色不一致。
    gUiRole = (GetIntegerKeyIni("AutoConn", "Role", 1) == 2) ? 2 : 1;
    const int currentTid = GetIntegerKeyIni("Main", "DeviceID", terminalNo);

    // ---- 注册回调（任务进度/终端列表） ----
    RegisterFileSendTaskProgressCallBack(SendTaskProgressCallBack);
    RegisterFileSendTaskStatusCallBack(SendTaskStatusCallBack);
    RegisterFileRecvTaskProgressCallBack(RecvTaskProgressCallBack);
    RegisterFileRecvTaskStatusCallBack(RecvTaskStatusCallBack);
    Register_TerminalListCallback(TerminalListCallBack);

    // ---- Qt 应用 ----
    QApplication app(argc, argv);

    // 创建控制器（后台状态机）
    AutoConnController *ctrl = new AutoConnController();

    // 创建窗口（纯 UI，依赖控制器信号）
    AutoConnWindow *window = new AutoConnWindow(ctrl);
    window->setWindowTitle(
        QString("异构网络通信传输系统 (TID %1)")
            .arg(currentTid));
    window->show();

    // 创建视频接收器（后台运行，自动注册 RTP 回调）
    // forceReceive=true 使接收管线在任意角色下均启动，以便能接收对端的实时流
    g_videoReceiver = new VideoReceiver(nullptr, true);

    // 控制器在 show() 之后启动，确保信号能被 UI 收到
    ctrl->start();

    // 退出时清理
    QObject::connect(&app, &QCoreApplication::aboutToQuit,
                     window, &AutoConnWindow::cleanUp);

    return app.exec();
}

void EXIT(int sig)
{
    System_shutdown(false);
    logfile.Write("程序退出, sig=%d\n\n", sig);
    exit(0);
}
