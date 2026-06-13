#include "temppage.h"
#include "ui_temppage.h"
#include "function.h"

tempPage::tempPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::tempPage)
{
    ui->setupUi(this);

    //绑定dataUpdater的信号事件
    {
        connect(dataUpdater,&DataUpdater::updateTerminalSignal,this,&tempPage::updateTerminals);
        printf("绑定dataUpdater的信号事件\n");
    }

    // 初始化开启监听界面
    {
        std::string exec_local_addr= exec_command("hostname -I");

        std::string local_addr= getStringFromIni("FirstOutDoorTest","LocalIP",exec_local_addr);
        ui->lineEdit_addr_launch->setText(QString::fromStdString(local_addr));

        int local_port= getIntFromIni("FirstOutDoorTest","LocalPort",3020);
        ui->lineEdit_port_launch->setText(QString::number(local_port));
        //
        // this->_localAddr= getStringFromIni("FirstOutDoorTest","LocalIP","127.0.0.1");
        // this->_localPort= getIntFromIni("FirstOutDoorTest","LocalPort",3020);
        // ui->lineEdit_addr_launch->setText(QString::fromStdString(this->_localAddr));
        // ui->lineEdit_port_launch->setText(QString::number(this->_localPort));
    }

    //初始化创建连接界面
    {
        // std::string exec_remote_port= exec_command("hostname -I");
        // std::string remote_port= getStringFromIni("FirstOutDoorTest","RemoteIP",exec_remote_port);
        // ui->lineEdit_remoteIp_createConnection->setText(QString::fromStdString(remote_port));
        this->_remote_addr= getStringFromIni("FirstOutDoorTest","RemoteIP","127.0.0.1");
        this->_remote_port= getIntFromIni("FirstOutDoorTest","RemotePort",3020);
        ui->lineEdit_remoteIp_createConnection->setText(QString::fromStdString(this->_remote_addr));
        ui->lineEdit_remotePort_createConnection->setText(QString::number(this->_remote_port));
    }

    //初始化开启自动发送
    {
        connect(this->ui->radioButton_sender,&QRadioButton::clicked,[&](){
            this->is_receiver_autoSend=false;
            this->is_sender_autoSend=true;
        // QMessageBox::information(this,"提示信息","响应！");
        });
        connect(this->ui->radioButton_receiver,&QRadioButton::clicked,[&](){
            this->is_receiver_autoSend=true;
            this->is_sender_autoSend=false;
        // QMessageBox::information(this,"提示信息","响应！");
        });
    }
}

tempPage::~tempPage()
{
    delete ui;
}

 void tempPage::updateTerminals() {
     MutexLockGuard guard(_terminal_Mutex);
     //comboBox数据清空，并放入新的数据【注意：如果绑定了事件，需要在进行这两项操作前后加上comboBox的锁】
     ui->comboBox_sendTaskTerminals->clear();

     for(auto tempClient:_clientTemplate){
         int tempTID=tempClient->_TID;
         ui->comboBox_sendTaskTerminals->addItem(QString::number(tempTID));
         ui->comboBox_askVideoTerminals->addItem(QString::number(tempTID));
         ui->comboBox_autoSendTerminals->addItem(QString::number(tempTID));
     }

    ui->comboBox_sendTaskTerminals->setCurrentIndex(0);
}

void tempPage::on_confirmButton_launch_clicked()
{
    bool isDTU;

    if(ui->comboBox_isDTU_launch->currentIndex() == 0 ){
    //if (ui->comboBox_isDTU_createConnection->currentIndex() == 0) {
        isDTU = false;
        printf("按下DTU确认选项，DTU模式为false\n");
    } else {
        isDTU = true;
        printf("按下DTU确认选项，DTU模式为false\n");
    }

    this->_localAddr=ui->lineEdit_addr_launch->text().toStdString();
    this->_localPort=ui->lineEdit_port_launch->text().toInt();
    //创建成功，关闭本界面，打开mainWindow
    if(createServer(this->_localAddr,this->_localPort,isDTU)){
        QMessageBox msgBox(QMessageBox::Information,"提示信息","创建监听成功！",QMessageBox::Ok,this);
        msgBox.setWindowFlags(this->windowFlags()&~Qt::WindowCloseButtonHint);
        msgBox.exec();
        //开启监听成功后，将信息放入列表中
        {
            //创建通道成功后，将通道信息存入function的数据中
    // printf("放入tempalte成功！\n");
            std::shared_ptr<ChannelTemplate> channel=std::make_shared<ChannelTemplate>();
            channel->address=_localAddr;
            channel->port=_localPort;
            _channelTemplate.push_back(channel);

            //将创建监听的IP地址放入IPList中
            if(std::find(IPList.begin(),IPList.end(), _localAddr)==IPList.end()){
                IPList.push_back(_localAddr);
            }
        }
        //退出界面
        on_cancelButton_launch_clicked();
    }else{
        QMessageBox msgBox(QMessageBox::Information,"提示信息","创建监听失败，请重试！",QMessageBox::Ok,this);
        msgBox.setWindowFlags(this->windowFlags()&~Qt::WindowCloseButtonHint);
        msgBox.exec();
    }
}

void tempPage::on_cancelButton_launch_clicked() {
    this->reset(add_listen);
    this->setVisible(false);
    emit returnToMainWindow();
}

std::string tempPage::getAddr() {
    return this->_localAddr;
}

unsigned int tempPage::getPort() {
    return this->_localPort;
}

void tempPage::turnToFlagPage(int flag){
    switch (flag) {
        case add_listen:{
            this->setWindowTitle(QString("开启监听"));
            ui->stackedWidget->setCurrentWidget(ui->page_launch);
            break;
        }
        case create_connection:{
            this->setWindowTitle(QString("创建连接"));
            ui->stackedWidget->setCurrentWidget(ui->page_createConnection);
            init_createConnection_localIPList();
            break;
        }
        case create_new_send_task:{
            this->setWindowTitle(QString("新建任务"));
            ui->stackedWidget->setCurrentWidget(ui->page_createSendTask);
            break;
        }
        case start_auto_send:{
            this->setWindowTitle(QString("开启自动发送"));
            ui->stackedWidget->setCurrentWidget(ui->page_startAutoSend);
            break;
        }
        case ask_video_trans:{
            this->setWindowTitle(QString("请求视频"));
            ui->stackedWidget->setCurrentWidget(ui->page_askVideo);
            break;
        }
    }
}

void tempPage::init_createConnection_localIPList() {
    {
        disconnect(ui->comboBox_localIPList_createConnection,QOverload<int>::of(&QComboBox::currentIndexChanged),this,&tempPage::on_LocalIPList_ComboBoxIndexChanged);
        ui->comboBox_localIPList_createConnection->clear();
        connect(ui->comboBox_localIPList_createConnection,QOverload<int>::of(&QComboBox::currentIndexChanged),this,&tempPage::on_LocalIPList_ComboBoxIndexChanged);
    }
    //初始化通道列表，并添加点击事件处理
    for(int index=0;index<IPList.size();index++){
        std::string str="地址"+to_string(index+1)+"  "+IPList.at(index);
        ui->comboBox_localIPList_createConnection->addItem(QString::fromStdString(str));
    }
    //默认第一个选项
    this->on_LocalIPList_ComboBoxIndexChanged(0);
    //绑定事件
    connect(ui->comboBox_localIPList_createConnection,QOverload<int>::of(&QComboBox::currentIndexChanged),this,&tempPage::on_LocalIPList_ComboBoxIndexChanged);
    connect(ui->comboBox_localIPList_createConnection,QOverload<int>::of(&QComboBox::currentIndexChanged),this,&tempPage::on_LocalIPList_ComboBoxIndexChanged);
}

void tempPage::on_LocalIPList_ComboBoxIndexChanged(int currentIndex) {
    std::string local_addr=IPList.at(currentIndex);
    this->ui->lineEdit_localIp_createConnection->setText(QString::fromStdString(local_addr));
    this->ui->lineEdit_localPort_createConnection->setText(QString::number(0));
}

void tempPage::on_confirmButton_pageCreateConnection_clicked(){
    bool isDTU;
    if(ui->lineEdit_remoteIp_createConnection->text().isEmpty()||ui->lineEdit_remotePort_createConnection->text().isEmpty()){
        QMessageBox::information(this,"提示信息","请填写完整信息！");
    }else{

        if (ui->comboBox_isDTU_createConnection->currentIndex() == 0) {
            isDTU = false;
            printf("按下DTU确认选项，DTU模式为false\n");
        } else {
            isDTU = true;
            printf("按下DTU确认选项，DTU模式为true\n");
        }

        ui->confirmButton_pageCreateConnection->setText(QString("创建中。。。"));
        ui->confirmButton_pageCreateConnection->setEnabled(false);
        ui->cancelButton_pageCreateConnection->setEnabled(false);
        std::string local_addr=ui->lineEdit_localIp_createConnection->text().toStdString();
        unsigned int local_port=ui->lineEdit_localPort_createConnection->text().toUInt();
        std::string remote_addr=ui->lineEdit_remoteIp_createConnection->text().toStdString();
        unsigned int remote_port=ui->lineEdit_remotePort_createConnection->text().toUInt();
        if(createClient(local_addr,local_port,remote_addr,remote_port,isDTU)){
            QMessageBox msgBox(QMessageBox::Information,"提示信息","连接成功！",QMessageBox::Ok,this);
            msgBox.setWindowFlags(this->windowFlags()&~Qt::WindowCloseButtonHint);
            msgBox.exec();
            //退出界面
            on_cancelButton_pageCreateConnection_clicked();
        }else{
            QMessageBox msgBox(QMessageBox::Information,"提示信息","连接失败！",QMessageBox::Ok,this);
            msgBox.setWindowFlags(this->windowFlags()&~Qt::WindowCloseButtonHint);
            msgBox.exec();
            ui->confirmButton_pageCreateConnection->setEnabled(true);
            ui->cancelButton_pageCreateConnection->setEnabled(true);
            ui->confirmButton_pageCreateConnection->setText(QString("确定"));
        }
    }
}

void tempPage::on_cancelButton_pageCreateConnection_clicked(){
    this->reset(create_connection);
    this->setVisible(false);
    emit returnToMainWindow();
}

void tempPage::reset(int flag) {
    switch (flag) {
        case add_listen:
        {
            ui->lineEdit_addr_launch->setText(QString::fromStdString(this->_localAddr));
            ui->lineEdit_port_launch->setText(QString::number(this->_localPort));
            ui->comboBox_isDTU_launch->setCurrentIndex(0);
        }
        break;
        case create_connection:
        {
            ui->lineEdit_remoteIp_createConnection->setText(QString::fromStdString(this->_remote_addr));
            ui->lineEdit_remotePort_createConnection->setText(QString::number(this->_remote_port));
            ui->confirmButton_pageCreateConnection->setEnabled(true);
            ui->cancelButton_pageCreateConnection->setEnabled(true);
            ui->confirmButton_pageCreateConnection->setText(QString("确定"));
            ui->comboBox_isDTU_createConnection->setCurrentIndex(0);
        }
        break;
        case create_new_send_task:
        {
            ui->sendModelBox_sendTask->setCurrentIndex(0);
            ui->lineEdit_filepath_sendTask->clear();    //这里也有可能从配置文件中读取
            fileNames_sendTask.clear();
            filePaths_sendTask.clear();
        }
        break;
        case start_auto_send:
        {
            ui->radioButton_sender->setChecked(true);
            ui->radioButton_receiver->setChecked(false);
//            is_RBUDP_autoSend=false;
        }
        break;
        case ask_video_trans:
        {
            //TODO:每次重新请求视频时，先进行reset，在切换界面
            //关闭本次视频传输（发送视频结束信令、重置界面展示图片、发送端/接收端结束视频传输）

        }
        break;
    }
}

//选择文件按钮，弹出文件管理器（支持多选）
void tempPage::on_chooseFileButton_sendTask_clicked(){
    this->fileDialog=new QFileDialog(this);
    fileDialog->setWindowTitle(QString("选择文件"));
    fileDialog->setDirectory("./");
    //设置为视图模式
    fileDialog->setViewMode(QFileDialog::Detail);
    QStringList filePaths=fileDialog->getOpenFileNames(this,"选择文件","./");
    if(filePaths.isEmpty()){
        return;
    }
    this->filePaths_sendTask.clear();
    this->fileNames_sendTask.clear();
    for(const QString &path : filePaths){
        this->filePaths_sendTask.push_back(path.toStdString());
        this->fileNames_sendTask.push_back(extractFileName(path.toStdString()));
    }
    if(filePaths.size()==1){
        ui->lineEdit_filepath_sendTask->setText(filePaths.first());
    }else{
        ui->lineEdit_filepath_sendTask->setText(QString("已选择 %1 个文件").arg(filePaths.size()));
    }
}

//创建发送任务执行操作（支持多文件）
void tempPage::on_confirmButton_pageCreateSendTask_clicked(){
    bool is_RBUDP;
    if(ui->comboBox_sendTaskTerminals->currentText().isEmpty()){  //连接终端列表为空，则提示信息
        QMessageBox msgBox(QMessageBox::Information,"提示信息","当前无连接终端，不能新建任务！",QMessageBox::Ok,this);
        msgBox.setWindowFlags(this->windowFlags()&~Qt::WindowCloseButtonHint);
        msgBox.exec();
    }else {
        int TID = ui->comboBox_sendTaskTerminals->currentText().toInt();
        if (ui->sendModelBox_sendTask->currentIndex() == 0) {
            is_RBUDP = false;
        } else {
            is_RBUDP = true;
        }
        if (this->filePaths_sendTask.empty()) {
            QMessageBox msgBox(QMessageBox::Information, "提示信息", "请选择文件！", QMessageBox::Ok, this);
            msgBox.setWindowFlags(this->windowFlags() & ~Qt::WindowCloseButtonHint);
            msgBox.exec();
        } else {
            int successCount = 0;
            int failCount = 0;
            for(size_t i = 0; i < filePaths_sendTask.size(); ++i){
                if(createTransferSendTask(TID, fileNames_sendTask[i], filePaths_sendTask[i], is_RBUDP)){
                    ++successCount;
                }else{
                    ++failCount;
                }
            }
            if(failCount == 0){
                QMessageBox msgBox(QMessageBox::Information, "提示信息",
                                   QString("成功创建 %1 个发送任务，正在发送中！").arg(successCount).toStdString().c_str(),
                                   QMessageBox::Ok, this);
                msgBox.setWindowFlags(this->windowFlags() & ~Qt::WindowCloseButtonHint);
                msgBox.exec();
                on_cancelButton_pageCreateSendTask_clicked();
            }else if(successCount > 0){
                QMessageBox msgBox(QMessageBox::Information, "提示信息",
                                   QString("成功 %1 个，失败 %2 个。").arg(successCount).arg(failCount).toStdString().c_str(),
                                   QMessageBox::Ok, this);
                msgBox.setWindowFlags(this->windowFlags() & ~Qt::WindowCloseButtonHint);
                msgBox.exec();
                on_cancelButton_pageCreateSendTask_clicked();
            }else{
                QMessageBox msgBox(QMessageBox::Information, "提示信息", "发送任务创建失败，请重试！", QMessageBox::Ok,
                                   this);
                msgBox.setWindowFlags(this->windowFlags() & ~Qt::WindowCloseButtonHint);
                msgBox.exec();
            }
        }
    }
}

void tempPage::on_cancelButton_pageCreateSendTask_clicked() {
    this->reset(create_new_send_task);
    this->setVisible(false);
    emit returnToMainWindow();
}

////开启自动发送任务执行操作
//void tempPage::on_confirmButton_startAutoSend_clicked(){
//    if(ui->comboBox_autoSendTerminals->currentText().isEmpty()){  //连接终端列表为空，则提示信息
//        QMessageBox msgBox(QMessageBox::Information,"提示信息","当前无连接终端，不能请求自动发送！",QMessageBox::Ok,this);
//        msgBox.setWindowFlags(this->windowFlags()&~Qt::WindowCloseButtonHint);
//        msgBox.exec();
//    }else {
//        int TID = ui->comboBox_autoSendTerminals->currentText().toInt();
//        //调用自动发送函数，开始执行，弹出执行结果的提示信息
//        if (startAutoSend(TID, is_receiver_autoSend)) {
//            QMessageBox msgBox(QMessageBox::Information, "提示信息", "成功开启自动发送任务！", QMessageBox::Ok, this);
//            msgBox.setWindowFlags(this->windowFlags() & ~Qt::WindowCloseButtonHint);
//            msgBox.exec();
//            //退出界面
//            on_cancelButton_startAutoSend_clicked();
//        } else {
//            if (startAutoSend(TID, is_sender_autoSend)) {
//                QMessageBox msgBox(QMessageBox::Information, "提示信息", "成功开启自动发送任务！", QMessageBox::Ok, this);
//                msgBox.setWindowFlags(this->windowFlags() & ~Qt::WindowCloseButtonHint);
//                msgBox.exec();
//                //退出界面
//                on_cancelButton_startAutoSend_clicked();
//            } else {
//                QMessageBox msgBox(QMessageBox::Information, "提示信息", "开启自动发送任务失败！", QMessageBox::Ok,
//                                   this);
//                msgBox.setWindowFlags(this->windowFlags() & ~Qt::WindowCloseButtonHint);
//                msgBox.exec();
//            }
//        }
//    }
//}

//开启自动发送任务执行操作
void tempPage::on_confirmButton_startAutoSend_clicked(){
    if(ui->comboBox_autoSendTerminals->currentText().isEmpty()){  //连接终端列表为空，则提示信息
        QMessageBox msgBox(QMessageBox::Information,"提示信息","当前无连接终端，不能请求自动发送！",QMessageBox::Ok,this);
        msgBox.setWindowFlags(this->windowFlags()&~Qt::WindowCloseButtonHint);
        msgBox.exec();
    }else {
        int TID = ui->comboBox_autoSendTerminals->currentText().toInt();
        //调用自动发送函数，开始执行，弹出执行结果的提示信息
        if (startAutoSend(TID, is_receiver_autoSend)) {
            QMessageBox msgBox(QMessageBox::Information, "提示信息", "接收端自动接收已完成！", QMessageBox::Ok, this);
            msgBox.setWindowFlags(this->windowFlags() & ~Qt::WindowCloseButtonHint);
            msgBox.exec();
            //退出界面
            on_cancelButton_startAutoSend_clicked();
        } else {
            if (startAutoSend(TID, is_sender_autoSend)) {
                QMessageBox msgBox(QMessageBox::Information, "提示信息", "发送端自动发送已完成！", QMessageBox::Ok, this);
                msgBox.setWindowFlags(this->windowFlags() & ~Qt::WindowCloseButtonHint);
                msgBox.exec();
                //退出界面
                on_cancelButton_startAutoSend_clicked();
            } else {
                QMessageBox msgBox(QMessageBox::Information, "提示信息", "开启自动发送任务失败！", QMessageBox::Ok,
                                   this);
                msgBox.setWindowFlags(this->windowFlags() & ~Qt::WindowCloseButtonHint);
                msgBox.exec();
            }
        }
    }
}


void tempPage::on_cancelButton_startAutoSend_clicked(){
    this->reset(start_auto_send);
    this->setVisible(false);
    emit returnToMainWindow();
}

 //请求视频槽函数
 void tempPage::on_confirmButton_askVideo_clicked(){
     if(ui->comboBox_askVideoTerminals->currentText().isEmpty()){  //连接终端列表为空，则提示信息
         QMessageBox msgBox(QMessageBox::Information,"提示信息","当前无连接终端，不能请求视频！",QMessageBox::Ok,this);
         msgBox.setWindowFlags(this->windowFlags()&~Qt::WindowCloseButtonHint);
         msgBox.exec();
     }else{
         unsigned int TID=ui->comboBox_askVideoTerminals->currentText().toUInt();
         int row=ui->comboBox_askVideoTerminals->currentIndex();
         emit askVideoTrans(TID,row);
         on_cancelButton_askVideo_clicked();
     }

}

void tempPage::on_cancelButton_askVideo_clicked(){
    this->reset(ask_video_trans);
    this->setVisible(false);
    emit returnToMainWindow();
}
