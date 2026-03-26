#include "mainwindow.h"
#include <QApplication>
#include "function.h"
#include <thread>
#include <unistd.h>
#include <limits.h>
#include <vector>

CLogFile logfile;

//退出函数
void EXIT(int sig);

static std::string resolve_sysconfig_path(const std::string& fileName) {
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

    const char* env = std::getenv("EC2_SYS_CONFIG");
    if (env && env[0] != '\0') {
        candidates.insert(candidates.begin(), std::string(env));
    }

    for (const auto& p : candidates) {
        if (access(p.c_str(), F_OK) == 0) {
            return p;
        }
    }
    return "../" + fileName;
}


    int main(int argc, char *argv[])
{
//    //关闭全部的信号和输入输出
//    CloseIOAndSignal();

    //处理程序退出的信号
    signal(SIGINT,EXIT); signal(SIGTERM,EXIT);

	char* buffer;
	buffer=getcwd(NULL,0);
	cout<<"[current filePath:] "<<buffer<<endl;

    if (argc < 2 || argv[1] == nullptr) {
        gUiRole = 1;
        const std::string cfg = resolve_sysconfig_path("SysConfig101.ini");
        std::cout << "[SysConfig] using: " << cfg << std::endl;
        System_start(cfg.c_str(), false);
    } else if (*argv[1] == '2') {
        gUiRole = 2;
        const std::string cfg = resolve_sysconfig_path("SysConfig102.ini");
        std::cout << "[SysConfig] using: " << cfg << std::endl;
        System_start(cfg.c_str(), false);//recv
	} else if (*argv[1] == '1') {
        gUiRole = 1;
        const std::string cfg = resolve_sysconfig_path("SysConfig101.ini");
        std::cout << "[SysConfig] using: " << cfg << std::endl;
        System_start(cfg.c_str(), false);//send
	} else {
        gUiRole = 1;
        const std::string cfg = resolve_sysconfig_path("SysConfig101.ini");
        std::cout << "[SysConfig] using: " << cfg << std::endl;
        System_start(cfg.c_str(), false);
    }

	cout<<"注册信息回调！"<<endl;
	//注册信息回调
	RegisterFileSendTaskProgressCallBack(SendTaskProgressCallBack);
	RegisterFileSendTaskStatusCallBack(SendTaskStatusCallBack);
	RegisterFileRecvTaskProgressCallBack(RecvTaskProgressCallBack);
	RegisterFileRecvTaskStatusCallBack(RecvTaskStatusCallBack);
	Register_TerminalListCallback(TerminalListCallBack);
//    register_mrudp_dataserver_callback("AutoSendMessage", mrudp_dataserver_a_callback);
	cout<<"注册信息回调ok！"<<endl;

    QApplication a(argc, argv);
	MainWindow *mainWindow=new MainWindow();
	mainWindow->show();
	MainWindow::connect(&a,&QCoreApplication::aboutToQuit,mainWindow,&MainWindow::cleanUp);   //保证系统退出时会释放资源、关闭线程

    return a.exec();
}

void EXIT(int sig)
{
    System_shutdown();
    logfile.Write("程序退出, sig=%d\n\n", sig);
    exit(0);
}
