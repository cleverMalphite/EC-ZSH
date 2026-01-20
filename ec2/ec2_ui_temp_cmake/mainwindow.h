#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include "temppage.h"
#include "VideoSender.h"
#include "VideoReceiver.h"
#include "ResizableLabel.h"
// #include "LocalPlayer.h"
#include <QMainWindow>
#include <QPushButton>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QSplineSeries>
#include <QtCharts/QValueAxis>
#include <QTimer>
#include <QPixmap>
#include <QStandardItemModel>
#include <QAbstractItemView>
#include <QTableView>
#include <QProgressBar>
#include <iostream>
#include <string>


using namespace std;

QT_CHARTS_USE_NAMESPACE

namespace Ui {
class MainWindow;
}


enum transTaskTableFlag{
    sendNow=1,
    sendHistory=2,
    recvNow=3,
    recvHistory=4
};

enum prePictureFlag{
    picture_send=1,
    picture_recv=2
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    // DataUpdater* dataUpdater; // 声明为成员变量


private slots:
    void openTempPage(int flag);
    void updateSendTable(); //更新实时发送和发送历史两个表
    void updateRecvTable(); //更新实时接收和接收历史两个表
    void updateSendProgress(DWORD task_id);  //更新实时发送进度
    void updateRecvProgress(DWORD task_id);  //更新实时接收进度

    void updateTransRate_chart();  //更新图表上的传输速率
    void handlePrePicture(int picFlag);  //图片展示按钮响应，根据picFlag来判断具体是send还是recv

public slots:
        void cleanUp();  //在退出界面之前进行资源释放

private:
    Ui::MainWindow *ui;
    tempPage* temppage;  //包含所有的操作按钮弹出窗口，例如：创建连接、创建发送任务等

    unsigned int TID;   //本机终端号
    std::string local_addr;   //本机IP
    unsigned int local_port;  //本机port


    //表格相关参数
    //TODO:将send和recv的函数分别合并，防止多次遍历
    void addNewSendRow(unsigned int taskID,unsigned int TID,std::string fileName,unsigned int process,float speed,int row);
    void addNewSendRow(unsigned int taskID,unsigned int TID,std::string fileName,unsigned int state,unsigned int finishTime,float averagaeSpeed,int row);
    void addNewRecvRow(unsigned int taskID,unsigned int TID,std::string fileName,unsigned int process,float speed,float loseRate,int row);
    void addNewRecvRow(unsigned int taskID,unsigned int TID,std::string fileName,unsigned int state,unsigned int finishTime,float averageSpeed,float loseRate,int row);
    QStandardItemModel *sendNowModel_;
    QStandardItemModel *sendHistoryModel_;
    QStandardItemModel *recvNowModel_;
    QStandardItemModel *recvHistoryModel_;

     //视频相关操作
    VideoSender* videoSender;
    VideoReceiver* videoReceiver;
     unsigned int localVideoPort;  //本地接收视频的port

    //图片相关数据
    ResizableLabel* sendPic_label;
    ResizableLabel* recvPic_label;

    // //传输速率相关数据
    float sendSpeed;
    float recvSpeed;
    pthread_mutex_t sendSpeed_mutex;
    pthread_mutex_t recvSpeed_mutex;
    QChartView* sendSpeed_chartView;
    QChartView* recvSpeed_chartView;
    QSplineSeries* sendSpeed_series;
    QSplineSeries* recvSpeed_series;
    QList<float> sendSpeedList;
    QList<float> recvSpeedList;
    int timeLength;
    QTimer* timer_transSpeed;  //传输速率计时器，每隔1s统计一下速率
    void calculateTransSpeed();  //计算传输速率



    void init();  //初始化函数
    void initTable(QTableView *table,QStandardItemModel* model,int flag);  //初始化表格
    void initTableModel(QStandardItemModel* model,int flag);  //初始化表格model
    string getSelectData(QTableView* tableView,int column);   //获取指定table选中行的指定列的数据，先都转成string，统一输出
};



#endif // MAINWINDOW_H

