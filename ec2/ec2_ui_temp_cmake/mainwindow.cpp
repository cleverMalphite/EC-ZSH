#include <QHeaderView>
#include <QAbstractItemView>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "infoHub/infoHub.h"
#include "infoHub/infoHubRpcClient.h"
#include <QDebug>
#include <QWidget>
#include <QLabel>
#include <QComboBox>
#include <QDir>
#include <QCoreApplication>


//infoHubRpcClient sender_infohub_rpcclient;
//infoHubRpcClient receiver_infohub_rpcclient;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    temppage=new tempPage();  //这里如果使用this作为parent，则temppage会嵌入在主界面中
    temppage->setVisible(false);
    this->init();
}


MainWindow::~MainWindow()
{
    this->cleanUp();
}

void MainWindow::init(){
    // MainWindow初始化
    {
        TID=getTID();
        this->setWindowTitle(QString::fromStdString("应急系统："+ to_string(TID)+"终端"));
    }
    //按钮初始化
    {
        //添加操作按钮
        QPushButton* addListen_button=new QPushButton("添加监听");
        QPushButton* createConnection_button=new QPushButton("创建连接");
        QPushButton* createNewSendTask_button=new QPushButton("新建任务");
        QPushButton* prePicture_send=new  QPushButton("展示图片");
        QPushButton* startAutoSend_button=new QPushButton("请求自动发送");
        QPushButton* askVideoTrans_button=new QPushButton("请求视频");
        QPushButton* prePicture_recv=new QPushButton("展示图片");

        //将按钮放到布局中
        //发送端
        ui->sendbutton_layout->addWidget(addListen_button);
        ui->sendbutton_layout->addWidget(createConnection_button);
        ui->sendbutton_layout->addWidget(createNewSendTask_button);
        ui->sendbutton_layout->addWidget(prePicture_send);
        //接收端
        ui->recvbutton_layout->addWidget(startAutoSend_button);
        ui->recvbutton_layout->addWidget(askVideoTrans_button);
        ui->recvbutton_layout->addWidget(prePicture_recv);

        //智能选网算法下拉框
        QLabel* algorithmLabel = new QLabel("选网算法:");
        algorithmLabel->setFont(QFont("黑体", 10));
        ui->sendbutton_layout->addWidget(algorithmLabel);

        QComboBox* algorithmComboBox = new QComboBox(this);
        algorithmComboBox->setObjectName("algorithmComboBox");
        algorithmComboBox->addItem("基于长短期记忆网络模型和深度强化学习的智能选网算法");
        algorithmComboBox->addItem("基于不排队传输模型的无线网络带宽探测算法");
        algorithmComboBox->addItem("基于BBR的无线网络拥塞控制算法");
        algorithmComboBox->addItem("数据实时可靠传输控制算法");
        algorithmComboBox->addItem("基于可变粒度的速率控制算法");
        algorithmComboBox->addItem("基于深度学习的无线网络负载");
        algorithmComboBox->addItem("基于子流耦合感知的多路拥塞控制算法");
        algorithmComboBox->setCurrentIndex(0);
        algorithmComboBox->setMinimumWidth(200);
        ui->sendbutton_layout->addWidget(algorithmComboBox);

         //NOTE:测试用应急底层传输信令-》测试成功
         QPushButton* videoTransCommand=new QPushButton("测试视频信令");
         ui->recvbutton_layout->addWidget(videoTransCommand);
         localVideoPort= getIntFromIni("VideoTrans","videoPort_"+to_string(TID),100);
         connect(videoTransCommand,&QPushButton::clicked,this,[=]{
             askStartVideoTrans(GetIntegerKeyIni("BigDataTransfer","DEST_TID",105),localVideoPort);
         });
        //
        //
        //绑定按钮点击事件
        connect(addListen_button,&QPushButton::clicked,this,[=](){
            this->openTempPage(1);
        });
        connect(createConnection_button,&QPushButton::clicked,this,[=](){
            this->openTempPage(2);
        });
        connect(createNewSendTask_button,&QPushButton::clicked,this,[=](){
            this->openTempPage(3);
        });
        connect(startAutoSend_button,&QPushButton::clicked,this,[=](){
            this->openTempPage(4);
        });
        connect(askVideoTrans_button,&QPushButton::clicked,this,[=](){
            this->openTempPage(5);
        });
        connect(prePicture_send,&QPushButton::clicked,this,[=](){
            this->handlePrePicture(picture_send);
        });
        connect(prePicture_recv,&QPushButton::clicked,this,[=](){
            this->handlePrePicture(picture_recv);
        });
    }

    //返回主界面响应
    connect(this->temppage,&tempPage::returnToMainWindow,this,[this](){
        this->setEnabled(true);
    });

    //表格初始化
    {
        sendNowModel_=new QStandardItemModel(this);
        sendHistoryModel_=new QStandardItemModel(this);
        recvNowModel_=new QStandardItemModel(this);
        recvHistoryModel_=new QStandardItemModel(this);
        ui->tableView_sendNow->setModel(sendNowModel_);
        ui->tableView_sendHistory->setModel(sendHistoryModel_);
        ui->tableView_recvNow->setModel(recvNowModel_);
        ui->tableView_recvHistory->setModel(recvHistoryModel_);
        initTable(ui->tableView_sendNow,sendNowModel_,sendNow);
        initTable(ui->tableView_sendHistory,sendHistoryModel_,sendHistory);
        initTable(ui->tableView_recvNow,recvNowModel_,recvNow);
        initTable(ui->tableView_recvHistory,recvHistoryModel_,recvHistory);

        connect(dataUpdater,&DataUpdater::updateSendProgressSignal,this,&MainWindow::updateSendProgress,Qt::QueuedConnection);
        qDebug() << "Signal connected to updateSendProgress";
        connect(dataUpdater,&DataUpdater::updateRecvProgressSignal,this,&MainWindow::updateRecvProgress,Qt::QueuedConnection);
        qDebug() << "Signal connected to updateRecvProgress";
        connect(dataUpdater,&DataUpdater::updateSendStatusSignal,this,&MainWindow::updateSendTable,Qt::QueuedConnection);
        qDebug() << "Signal connected to updateSendTable";
        connect(dataUpdater,&DataUpdater::updateRecvStatusSignal,this,&MainWindow::updateRecvTable,Qt::QueuedConnection);
        qDebug() << "Signal connected to updateSendTable";
    }
    //视频区域替换为图片展示
    {
        // 尝试从多个路径加载图片
        auto loadImage = [](QLabel *label, const QStringList &paths) -> bool {
            for (const QString &p : paths) {
                QPixmap pix(p);
                if (!pix.isNull()) {
                    label->setPixmap(pix.scaled(label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
                    label->setAlignment(Qt::AlignCenter);
                    label->setScaledContents(false);
                    return true;
                }
            }
            return false;
        };

        // 发送端图片
        QLabel *sendImageLabel = new QLabel(this);
        sendImageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sendImageLabel->setMinimumHeight(200);
        sendImageLabel->setStyleSheet("QLabel{background-color: #1a1a2e; border-radius: 6px;}");
        ui->sendVideo_layout->addWidget(sendImageLabel);
        loadImage(sendImageLabel, {
            QStringLiteral("../pic/OIP.webp"),
            QStringLiteral("pic/OIP.webp"),
            QStringLiteral("../../pic/OIP.webp"),
            QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("../pic/OIP.webp")
        });

        // 接收端图片
        QLabel *recvImageLabel = new QLabel(this);
        recvImageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        recvImageLabel->setMinimumHeight(200);
        recvImageLabel->setStyleSheet("QLabel{background-color: #1a1a2e; border-radius: 6px;}");
        ui->recvVideo_layout->addWidget(recvImageLabel);
        loadImage(recvImageLabel, {
            QStringLiteral("../pic/OIP.webp"),
            QStringLiteral("pic/OIP.webp"),
            QStringLiteral("../../pic/OIP.webp"),
            QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("../pic/OIP.webp")
        });

        // 保留 videoSender/videoReceiver 对象（底层传输仍需要），但不添加到 UI
        videoSender = new VideoSender(this);
        videoReceiver = new VideoReceiver(this);
    }

     //传输速率曲线初始化
     {
         sendSpeed=0;
         recvSpeed=0;
         timeLength=60;


         for(int i=0;i<2;i++){
             QValueAxis* axisX=new QValueAxis();
             QValueAxis* axisY=new QValueAxis();
             axisX->setRange(0,timeLength);
             axisY->setRange(0,5);

             QChart* chart=new QChart();


             if(i==0){
                 sendSpeed_series=new QSplineSeries();
                 chart->addSeries(sendSpeed_series);
                 chart->addAxis(axisX,Qt::AlignBottom);
                 chart->addAxis(axisY,Qt::AlignLeft);

                 sendSpeed_series->attachAxis(axisX);
                 sendSpeed_series->attachAxis(axisY);
                 sendSpeed_chartView=new QChartView(chart);
                 ui->sendGraph_layout->addWidget(sendSpeed_chartView);
             }else if(i==1){
                 recvSpeed_series=new QSplineSeries();
                 chart->addSeries(recvSpeed_series);
                 chart->addAxis(axisX,Qt::AlignBottom);
                 chart->addAxis(axisY,Qt::AlignLeft);

                 recvSpeed_series->attachAxis(axisX);
                 recvSpeed_series->attachAxis(axisY);
                 recvSpeed_chartView=new QChartView(chart);
                 ui->recvGraph_layout->addWidget(recvSpeed_chartView);
             }
         }
         sendSpeed_chartView->chart()->setTitle(QString("发送端速率变化图"));
         sendSpeed_chartView->chart()->axisY()->setTitleText(QString("发送速率（Mbps）"));
         recvSpeed_chartView->chart()->setTitle(QString("接收端速率变化图"));
         recvSpeed_chartView->chart()->axisY()->setTitleText(QString("接收速率（Mbps）"));

         timer_transSpeed=new QTimer();
         connect(timer_transSpeed, SIGNAL(timeout()),this,SLOT(updateTransRate_chart()));
         timer_transSpeed->start(1000);  //每秒统计一次速率
     }

    // //图片初始化
    {
        sendPic_label=new ResizableLabel();
        sendPic_label->setStyleSheet("QLabel{background-color:rgb(0,0,0);}");
        sendPic_label->setAlignment(Qt::AlignCenter);
        sendPic_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sendPic_label->setMaximumHeight(260);
        ui->sendPicture_layout->addWidget(sendPic_label);
        // Default hidden to allow video area expansion
        sendPic_label->setVisible(false);

        recvPic_label=new ResizableLabel();
        recvPic_label->setAlignment(Qt::AlignCenter);
        recvPic_label->setStyleSheet("QLabel{background-color:rgb(0,0,0);}");
        recvPic_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        recvPic_label->setMaximumHeight(260);
        ui->recvPicture_layout->addWidget(recvPic_label);
        // Default hidden to allow video area expansion
        recvPic_label->setVisible(false);
    }

}


void MainWindow::initTable(QTableView *table,QStandardItemModel* model,int flag){
    table->setSelectionBehavior(QAbstractItemView::SelectRows);  //设置整行选中
    table->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);  //只能单选
    table->setEditTriggers(QTableView::EditTrigger::NoEditTriggers);  //设置全部单元格不可编辑
    table->setAlternatingRowColors(true);  //设置表格隔行变色

    //设置列宽根据单元格内容自动调整
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    //设置表头文字和高度
    table->horizontalHeader()->setFont(QFont("黑体",12));
    table->horizontalHeader()->setFixedHeight(30);
    table->setFont(QFont("黑体",11));

    //添加垂直滚动条
    table->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    //添加水平滚动条
    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    //初始化model
    this->initTableModel(model,flag);
}

void MainWindow::initTableModel(QStandardItemModel *model, int flag) {
    switch (flag) {
        case sendNow:
        {
            model->setColumnCount(5);
            model->setHeaderData(0,Qt::Horizontal,tr("任务号"));
            model->setHeaderData(1,Qt::Horizontal,tr("接收终端"));
            model->setHeaderData(2,Qt::Horizontal,tr("发送文件"));
            model->setHeaderData(3,Qt::Horizontal,tr("实时发送速度"));
            model->setHeaderData(4,Qt::Horizontal,tr("发送进度"));
        }
            break;
        case sendHistory:
        {
            model->setColumnCount(6);
            model->setHeaderData(0,Qt::Horizontal,tr("任务号"));
            model->setHeaderData(1,Qt::Horizontal,tr("接收终端"));
            model->setHeaderData(2,Qt::Horizontal,tr("发送文件"));
            model->setHeaderData(3,Qt::Horizontal,tr("状态"));
            model->setHeaderData(4,Qt::Horizontal,tr("完成时间"));
            model->setHeaderData(5,Qt::Horizontal,tr("平均发送速率"));
//            model->setHeaderData(6,Qt::Horizontal,tr("历史发送速率"));
        }
            break;
        case recvNow:
        {
            model->setColumnCount(5);
            model->setHeaderData(0,Qt::Horizontal,tr("任务号"));
            model->setHeaderData(1,Qt::Horizontal,tr("发送终端"));
            model->setHeaderData(2,Qt::Horizontal,tr("接收文件"));
            model->setHeaderData(3,Qt::Horizontal,tr("接收进度"));
            model->setHeaderData(4,Qt::Horizontal,tr("接收速度"));
        }
            break;
        case recvHistory:
        {
            model->setColumnCount(6);
            model->setHeaderData(0,Qt::Horizontal,tr("任务号"));
            model->setHeaderData(1,Qt::Horizontal,tr("发送终端"));
            model->setHeaderData(2,Qt::Horizontal,tr("接收文件"));
            model->setHeaderData(3,Qt::Horizontal,tr("状态"));
            model->setHeaderData(4,Qt::Horizontal,tr("完成时间"));
            model->setHeaderData(5,Qt::Horizontal,tr("平均接收速率"));
//        model->setHeaderData(6,Qt::Horizontal,tr("丢包率"));
//            model->setHeaderData(6,Qt::Horizontal,tr("历史接收速率"));
        }
            break;
    }
}


void MainWindow::openTempPage(int flag){
    //开启tempPage页面
    temppage->setVisible(true);
    //根据标志位，确定转到哪个页面
    temppage->turnToFlagPage(flag);
}

void MainWindow::cleanUp() {
    delete ui;
    delete temppage;
    // delete videoSender;
    // delete videoReceiver;
    delete sendSpeed_chartView;
    delete recvSpeed_chartView;
    timer_transSpeed->stop();
    delete timer_transSpeed;
    System_shutdown(true);
}



//添加正在执行的发送任务到列表中
void MainWindow::addNewSendRow(unsigned int taskID,unsigned int TID,std::string fileName,unsigned int process,float speed,int row){
    //添加一行数据到里面
    QList<QStandardItem*> newRow;
    qDebug() << "addNewSendRow called with taskID:" << taskID << "TID:" << TID << "fileName:" << QString::fromStdString(fileName);

    newRow.append(new QStandardItem(QString::number(taskID)));
    newRow.append(new QStandardItem(QString::number(TID)));
    newRow.append(new QStandardItem(QString::fromStdString(fileName)));
    if(speed>=1000000){
        newRow.append(new QStandardItem(QString("%1 Gbps").arg(speed/1000000,0,'f',2)));
    }else{
        newRow.append(new QStandardItem(QString("%1 Mbps").arg(speed/1000,0,'f',2)));
    }
    //添加行数据到表格
    this->sendNowModel_->insertRow(row,newRow);

    // newRow.append(new QStandardItem(QString::number(process)));
    //添加进度条
    QProgressBar *progressBar=new QProgressBar;
    progressBar->setMinimum(0);
    progressBar->setMaximum(100);
    progressBar->setValue(process);
    ui->tableView_sendNow->setIndexWidget(this->sendNowModel_->index(row,4),progressBar);


//    sender_infohub_rpcclient.init("127.0.0.1", 8000);
//
//    printf("__>> connetion state: %d\n", sender_infohub_rpcclient.get_connection_state());

//    printf("--------------- epollcomm infohub --------------\n");
//    double epollcomm_tx_rate_stat = sender_infohub_rpcclient.value_get<double>("epollcomm", "tx_rate_stat");
//    double epollcomm_rx_rate_stat = sender_infohub_rpcclient.value_get<double>("epollcomm", "rx_rate_stat");
//    printf("<infohub_rpcclient> epollcomm tx_rate_value = %lf\n", epollcomm_tx_rate_stat);
//    printf("<infohub_rpcclient> epollcomm rx_rate_value = %lf\n", epollcomm_rx_rate_stat);
//

}

//添加已完成的发送任务到列表中
void MainWindow::addNewSendRow(unsigned int taskID, unsigned int TID, std::string fileName, unsigned int state,
                                        unsigned int finishTime, float averagaeSpeed, int row) {
    QList<QStandardItem*> newRow;
    newRow.append(new QStandardItem(QString::number(taskID)));
    newRow.append(new QStandardItem(QString::number(TID)));
    newRow.append(new QStandardItem(QString::fromStdString(fileName)));
    switch (state) {
        case 1:
            newRow.append(new QStandardItem(QString::fromStdString("发送成功")));
            break;
        case 2:
            newRow.append(new QStandardItem(QString::fromStdString("发送失败")));
            break;
        case 3:
            newRow.append(new QStandardItem(QString::fromStdString("发送任务取消")));
            break;
        default:
            newRow.append(new QStandardItem(QString::fromStdString("未知状态")));
            break;
    }
    //时间>=1s，单位用s；<1s，用ms
    if(finishTime>=1000){
        newRow.append(new QStandardItem(QString::fromStdString(to_string(int(finishTime/1000))+"s")));
    }else{
        newRow.append(new QStandardItem(QString::fromStdString(to_string(finishTime)+"ms")));
    }

    //保留两位小数
    if(averagaeSpeed>=1000000){
        newRow.append(new QStandardItem(QString("%1 Gbps").arg(averagaeSpeed/1000000,0,'f',2)));
    }else{
        newRow.append(new QStandardItem(QString("%1 Mbps").arg(averagaeSpeed/1000,0,'f',2)));
    }
    this->sendHistoryModel_->insertRow(row,newRow);

}

//添加正在接收的传输任务到列表中
void MainWindow::addNewRecvRow(unsigned int taskID,unsigned int TID,std::string fileName,unsigned int process,float speed,float loseRate,int row){
    //添加一行数据到里面
    QList<QStandardItem*> newRow;
    newRow.append(new QStandardItem(QString::number(taskID)));
    newRow.append(new QStandardItem(QString::number(TID)));
    newRow.append(new QStandardItem(QString::fromStdString(fileName)));
    newRow.append(new QStandardItem(QString::number(process)));
    if(speed>=1000000){
        newRow.append(new QStandardItem(QString("%1 Gbps").arg(speed/1000000,0,'f',2)));
    }else{
        newRow.append(new QStandardItem(QString("%1 Mbps").arg(speed/1000,0,'f',2)));
    }
//    newRow.append(new QStandardItem(QString::number(loseRate)));
    //添加行数据到表格
    this->recvNowModel_->insertRow(row,newRow);

    //添加进度条
    QProgressBar *progressBar=new QProgressBar;
    progressBar->setMinimum(0);
    progressBar->setMaximum(100);
    progressBar->setValue(process);
    ui->tableView_recvNow->setIndexWidget(this->recvNowModel_->index(row,3),progressBar);
}

//添加接收完成的任务到列表中
void MainWindow::addNewRecvRow(unsigned int taskID, unsigned int TID, std::string fileName, unsigned int state,
                                        unsigned int finishTime, float averageSpeed, float loseRate, int row) {
    QList<QStandardItem*> newRow;
    newRow.append(new QStandardItem(QString::number(taskID)));
    newRow.append(new QStandardItem(QString::number(TID)));
    newRow.append(new QStandardItem(QString::fromStdString(fileName)));
    switch (state) {
        case 5:
            newRow.append(new QStandardItem(QString::fromStdString("接收成功")));
            break;
        case 6:
            newRow.append(new QStandardItem(QString::fromStdString("接收失败")));
            break;
        case 7:
            newRow.append(new QStandardItem(QString::fromStdString("接收任务取消")));
            break;
        default:
            newRow.append(new QStandardItem(QString::fromStdString("未知状态")));
            break;
    }
    //时间>=1s，单位用s；<1s，用ms
    if(finishTime>=1000){
        newRow.append(new QStandardItem(QString::fromStdString(to_string(int(finishTime/1000))+"s")));
    }else{
        newRow.append(new QStandardItem(QString::fromStdString(to_string(finishTime)+"ms")));
    }

    if(averageSpeed>=1000000){
        newRow.append(new QStandardItem(QString("%1 Gbps").arg(averageSpeed/1000000,0,'f',2)));
    }else{
        newRow.append(new QStandardItem(QString("%1 Mbps").arg(averageSpeed/1000,0,'f',2)));
    }
//    newRow.append(new QStandardItem(QString::number(loseRate)));
    this->recvHistoryModel_->insertRow(row,newRow);

}



void MainWindow::updateSendTable(){
    MutexLockGuard guard(_sendTask_Mutex);
    this->sendNowModel_->clear();
    this->sendHistoryModel_->clear();
    this->initTableModel(sendNowModel_,sendNow);
    this->initTableModel(sendHistoryModel_,sendHistory);
    int sendNowCount=0;
    int sendHistoryCount=0;

    for(auto temp:_sendTaskState){
        qDebug() << "Checking task ID:" << temp->_taskID; // 查看所有任务 ID
        if(temp->_taskState==0){
            addNewSendRow(temp->_taskID,temp->_TID,temp->_taskName,temp->_process,temp->_transferSpeed,sendNowCount);
            sendNowCount++;
        }
        else if(temp->_taskState!=0){
            addNewSendRow(temp->_taskID,temp->_TID,temp->_taskName,temp->_taskState,temp->_transferTime,temp->_transferSpeed,sendHistoryCount);
            sendHistoryCount++;
        }

    }
}

void MainWindow::updateSendProgress(DWORD task_id) {
    qDebug() << "updateSendProgress called with task_id:" << task_id;
    MutexLockGuard guard(_sendTask_Mutex);
    //查找到对应改变的数据，将信息修改
    int rowCount=this->sendNowModel_->rowCount();
    int targetColumn=0;
    int targetRow;
    //找到table中需要修改的行
    for(int row=0;row<rowCount;row++){
        QModelIndex index=this->sendNowModel_->index(row,targetColumn);
        DWORD task_id_row=this->sendNowModel_->data(index).toUInt();
        if(task_id_row==task_id){
            targetRow=row;
            break;
        }
    }
    // qDebug() << "Found target row:" << targetRow;
    //找到template中更新的数据
    float speed;
    unsigned int process;
    for(auto temp:_sendTaskState){
        if(temp->_taskID==task_id){
            speed=temp->_transferSpeed;
            process=temp->_process;
            break;
        }
    }
    this->sendNowModel_->setData(this->sendNowModel_->index(targetRow,3),QVariant(QString("%1 Mbps").arg(speed/1000,0,'f',2)));
    //添加进度条
    QProgressBar *progressBar=new QProgressBar;
    progressBar->setMinimum(0);
    progressBar->setMaximum(100);
    progressBar->setValue(process);
    ui->tableView_sendNow->setIndexWidget(this->sendNowModel_->index(targetRow,4),progressBar);
}

void MainWindow::updateRecvTable(){
    MutexLockGuard guard(_recvTask_Mutex);
    this->recvNowModel_->clear();
    this->recvHistoryModel_->clear();
    this->initTableModel(recvNowModel_,recvNow);
    this->initTableModel(recvHistoryModel_,recvHistory);
    int recvNowCount=0;
    int recvHistoryCount=0;
    printf("updateRecvTable!");
    for(auto temp:_recvTaskState){
        if(temp->_taskState==4){
            addNewRecvRow(temp->_taskID,temp->_TID,temp->_taskName,temp->_process,temp->_transferSpeed,0,recvNowCount);
            recvNowCount++;
        }else if(temp->_taskState!=4){
            addNewRecvRow(temp->_taskID,temp->_TID,temp->_taskName,temp->_taskState,temp->_transferTime,temp->_transferSpeed,temp->_loseRate,recvHistoryCount);
            recvHistoryCount++;
        }
    }
}

void MainWindow::updateRecvProgress(DWORD task_id) {
    qDebug() << "updateRecvProgress called with task_id:" << task_id;
    MutexLockGuard guard(_recvTask_Mutex);
    int rowCount=this->recvNowModel_->rowCount();
    int targetColumn=0;
    int targetRow;
    //找到需要修改的tabel的行
    for(int row=0;row<rowCount;row++){
        QModelIndex index=this->recvNowModel_->index(row,targetColumn);
        DWORD task_id_row=this->recvNowModel_->data(index).toUInt();
        if(task_id_row==task_id){
            targetRow=row;
            break;
        }
    }
    //找到更新的数据
    float speed;
    unsigned int process;
    for(auto temp:_recvTaskState){
        if(temp->_taskID==task_id){
            speed=temp->_transferSpeed;
            process=temp->_process;
            break;
        }
    }
    this->recvNowModel_->setData(this->recvNowModel_->index(targetRow,4),QVariant(QString("%1 Mbps").arg(speed/1000,0,'f',2)));
    //添加进度条
    QProgressBar *progressBar=new QProgressBar;
    progressBar->setMinimum(0);
    progressBar->setMaximum(100);
    progressBar->setValue(process);
    ui->tableView_recvNow->setIndexWidget(this->recvNowModel_->index(targetRow,3),progressBar);
}


void MainWindow::updateTransRate_chart() {
    //获取当前传输速率
    calculateTransSpeed();
    sendSpeedList.append(sendSpeed);
    recvSpeedList.append(recvSpeed);
    if(sendSpeedList.length()>timeLength+1) {
        sendSpeedList.removeFirst();
        recvSpeedList.removeFirst();
    }
    QList<QPointF> points_sendSpeed;
    QList<QPointF> points_recvSpeed;
    points_sendSpeed.clear();
    points_recvSpeed.clear();
    float sendSpeedMax=sendSpeedList[0];
    float recvSpeedMax=recvSpeedList[0];
    for(int i=0;i<sendSpeedList.length();i++){
        points_sendSpeed.append(QPointF(i,sendSpeedList[i]));
        points_recvSpeed.append(QPointF(i,recvSpeedList[i]));
        if(sendSpeedMax<sendSpeedList[i]){
            sendSpeedMax=sendSpeedList[i];
        }
        if(recvSpeedMax<recvSpeedList[i]){
            recvSpeedMax=recvSpeedList[i];
        }
    }

    if(sendSpeedMax==0){
        sendSpeedMax=5;
    }
    if(recvSpeedMax==0){
        recvSpeedMax=5;
    }
    sendSpeed_chartView->chart()->axisY()->setMax(sendSpeedMax);
    recvSpeed_chartView->chart()->axisY()->setMax(recvSpeedMax);
    sendSpeed_series->replace(points_sendSpeed);
    recvSpeed_series->replace(points_recvSpeed);

    //更新纵坐标最大值，如果=0则设最大值为5，否则更新最大值为max
    /*sendSpeed_chartView->chart()->axisY()->setMax(sendSpeedMax);
    recvSpeed_chartView->chart()->axisY()->setMax(recvSpeedMax);*/
}


void MainWindow::calculateTransSpeed() {
    sendSpeed=0;
    recvSpeed=0;
    MutexLockGuard guard(_recvTask_Mutex);
    MutexLockGuard guard1(_sendTask_Mutex);
    for(auto sendTask_temp:_sendTaskState){
        if(sendTask_temp->_taskState==0){
            sendSpeed+=sendTask_temp->_transferSpeed/1000;
        }
    }
    for(auto recvTask_temp:_recvTaskState){
        if(recvTask_temp->_taskState==4){
            recvSpeed+=recvTask_temp->_transferSpeed/1000;
        }
    }
}





void MainWindow::handlePrePicture(int picFlag) {
    //先获取当前选中的行号，然后找到对应的文件路径，判断文件能否打开为图片，不能则提示信息，可以则直接更换图片
    switch (picFlag) {
        case picture_send:{
            MutexLockGuard guard(_sendTask_Mutex);
            QModelIndex currentIndex=ui->tableView_sendHistory->currentIndex();
            if(currentIndex.isValid()){
                int row=currentIndex.row();
                QModelIndex index=sendHistoryModel_->index(row,0);
                int taskID=sendHistoryModel_->itemFromIndex(index)->text().toUInt();
                bool isFind=false;
                string filePath="";
                for(auto sendTaskTemp:_sendTaskState){
                    if(sendTaskTemp->_taskID==taskID){
                        filePath=sendTaskTemp->_taskFilePath;
                        isFind=true;
                        break;
                    }
                }
                if(isFind){
                    QImage image;
                    if(!image.load(QString::fromStdString(filePath))){
                            cout<<"文件打开失败！"<<endl;
                        return;
                    }
                    sendPic_label->setImage(image);
                    int tmp_width = sendPic_label->width();
                    int tmp_height = sendPic_label->height();
                    sendPic_label->resize(tmp_width-1,tmp_height-1);
                    sendPic_label->resize(tmp_width+1,tmp_height+1);
                    
                    // Show label if it was hidden
                    if(!sendPic_label->isVisible()) sendPic_label->setVisible(true);

                }else{
                    cout<<"该图片不存在！"<<endl;
                    return;
                }
            }else{
                cout<<"请先选中要展示的图片！"<<endl;
                return;
            }
        }
        break;
        case picture_recv:{
            MutexLockGuard guard(_recvTask_Mutex);
            QModelIndex currentIndex=ui->tableView_recvHistory->currentIndex();
            if(currentIndex.isValid()){
                int row=currentIndex.row();
                QModelIndex index=recvHistoryModel_->index(row,0);
                int taskID=recvHistoryModel_->itemFromIndex(index)->text().toUInt();
                bool isFind=false;
                string filePath= getStringFromIni("BigDataTransfer","recv_dir","../../FileRecv_c/");
                for(auto recvTaskTemp:_recvTaskState){
                    if(recvTaskTemp->_taskID==taskID){
                        filePath+=recvTaskTemp->_taskName;
                        isFind=true;
                        break;
                    }
                }
                cout<<"RecvFilePath:"<<filePath<<endl;
                if(isFind){
                    QImage image;
                    if(!image.load(QString::fromStdString(filePath))){
                            cout<<"打开文件失败！"<<endl;
                            return;
                    }
                    recvPic_label->setImage(image);
                    int tmp_width = recvPic_label->width();
                    int tmp_height = recvPic_label->height();
                    recvPic_label->resize(tmp_width-1,tmp_height-1);
                    recvPic_label->resize(tmp_width+1,tmp_height+1);
                    
                    // Show label if it was hidden
                    if(!recvPic_label->isVisible()) recvPic_label->setVisible(true);

                }else{
                    cout<<"该图片不存在！"<<endl;
                    return;
                }
            }else{
                cout<<"请先选中要展示的图片！"<<endl;
                return;
            }
        }
        break;
    }
}
string MainWindow::getSelectData(QTableView *tableView, int column) {
    //获取当前选中行，以此获取TID
    int row;  //当前选中行
    QList<QStandardItem *>rowData;  //选中行数据
    QModelIndex currentIndex=tableView->currentIndex();
    if(currentIndex.isValid()){
        row=currentIndex.row();
        QStandardItemModel *model= qobject_cast<QStandardItemModel*>(tableView->model());

        if(model){
            for(int column=0;column<model->columnCount();column++){
                QModelIndex index=model->index(row,column);
                QStandardItem *item=model->itemFromIndex(index);
                rowData.append(item);
            }
        }
    }
    string result;
    return result;
}
