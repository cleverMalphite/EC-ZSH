#ifndef TEMPPAGE_H
#define TEMPPAGE_H

#include <QWidget>
#include "function.h"
#include <QMessageBox>
#include <QPushButton>
#include <QFileDialog>

namespace Ui {
class tempPage;
}

enum tempPageFlag{
    add_listen=1,
    create_connection=2,
    create_new_send_task=3,
    start_auto_send=4,
    ask_video_trans=5
};

class tempPage : public QWidget
{
    Q_OBJECT

public:
    explicit tempPage(QWidget *parent = nullptr);
    ~tempPage();
    std::string getAddr();
    unsigned int getPort();
    void turnToFlagPage(int flag);  //转到flag对应的界面

signals:
    // void createServerSuccess();  //开启监听成功信号
    void returnToMainWindow();   //返回主界面信号
    void askVideoTrans(unsigned int TID,int row);   //temp界面请求mainWindow处理请求视频

private slots:
    void on_confirmButton_launch_clicked(); //开启监听
    void on_cancelButton_launch_clicked();
    void on_confirmButton_pageCreateConnection_clicked();  //创建连接
    void on_cancelButton_pageCreateConnection_clicked();
    void updateTerminals();   //更新终端列表，由回调函数发送信号
    void on_LocalIPList_ComboBoxIndexChanged(int currentIndex);  //创建连接界面的本机地址列表初始化

    //创建发送任务槽函数
    void on_chooseFileButton_sendTask_clicked();  //选择文件按钮，弹出文件管理器
    void on_confirmButton_pageCreateSendTask_clicked();   //创建发送任务执行操作
    void on_cancelButton_pageCreateSendTask_clicked();

    //开启自动发送槽函数
    void on_confirmButton_startAutoSend_clicked();  //开启自动发送任务执行操作
    void on_cancelButton_startAutoSend_clicked();

    //请求视频槽函数
    void on_confirmButton_askVideo_clicked();   //请求视频执行操作
    void on_cancelButton_askVideo_clicked();

private:
    Ui::tempPage *ui;
    std::string _localAddr;
    unsigned int _localPort;
    std::string _remote_addr;  //对端IP
    unsigned int _remote_port; //对端port

    //创建发送任务
    QFileDialog *fileDialog;
    std::vector<std::string> fileNames_sendTask;
    std::vector<std::string> filePaths_sendTask;
    //开启自动发送
    bool is_receiver_autoSend;
    bool is_sender_autoSend;

    void init_createConnection_localIPList();  //初始化创建连接界面的本机地址列表
    void reset(int flag);  //根据flag重置不同的界面
};

#endif // TEMPPAGE_H
























