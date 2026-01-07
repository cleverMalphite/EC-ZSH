#include "mainwindow.h"
#include <QApplication>
#include "function.h"
#include <thread>

CLogFile logfile;

//退出函数
void EXIT(int sig);


    int main(int argc, char *argv[])
{
//    //关闭全部的信号和输入输出
//    CloseIOAndSignal();

    //处理程序退出的信号
    signal(SIGINT,EXIT); signal(SIGTERM,EXIT);

	char* buffer;
	buffer=getcwd(NULL,0);
	cout<<"[current filePath:] "<<buffer<<endl;

	if(*argv[1]=='2'){
		System_start("../SysConfig102.ini",false);//recv
	}
	if(*argv[1]=='1'){
		System_start("../SysConfig101.ini",false);//send
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