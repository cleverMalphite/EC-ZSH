#include "VideoSender.h"

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui_c.h>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPointer>
#include <QMetaObject>
#include <cstring>
#include <cstdlib>

#include "../GlobalMessage.h"

//#define DEBUG_VIDEO_SENDER_
#define TEST_VIDEO_SENDER_

// GStreamer initialization helper
static bool gst_initialized = false;
static void ensure_gst_init() {
    if (!gst_initialized) {
        gst_init(nullptr, nullptr);
        gst_initialized = true;
    }
}

static VideoSender* g_video_sender_instance = nullptr;

static bool VideoSender_SpeedControlRecvDataCallback(DWORD dwTID,
                                                     const std::shared_ptr<BYTE> &pBuffer,
                                                     DWORD dwLength,
                                                     long int &recvtime,
                                                     long int &fb_send_time) {
    if (!g_video_sender_instance) {
        return true;
    }
    return g_video_sender_instance->onSpeedControlRecvData(dwTID, pBuffer, dwLength, recvtime, fb_send_time);
}

static std::string resolve_default_video_file_path() {
    const QStringList candidates = {
        QDir::cleanPath(QDir::currentPath() + "/FileSend/AVtest.mp4"),
        QDir::cleanPath(QDir::currentPath() + "/../FileSend/AVtest.mp4"),
        QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../FileSend/AVtest.mp4"),
        QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../../FileSend/AVtest.mp4"),
        QString("/home/kong/workspace/EC2_12.8/FileSend/AVtest.mp4"),
    };

    for (const auto& path : candidates) {
        if (QFileInfo::exists(path)) {
            return path.toStdString();
        }
    }
    return "FileSend/AVtest.mp4";
}

static std::string to_file_uri(const std::string& path) {
    return QUrl::fromLocalFile(QString::fromStdString(path)).toString().toStdString();
}

static uint64_t to_be_u64(uint64_t v) {
    const uint64_t hi = htonl(static_cast<uint32_t>((v >> 32) & 0xFFFFFFFFULL));
    const uint64_t lo = htonl(static_cast<uint32_t>(v & 0xFFFFFFFFULL));
    return (lo << 32) | hi;
}

VideoSender::VideoSender(QWidget *parent)
    : QWidget(parent)
{
    ensure_gst_init();
    m_audioSender = new AudioSenderCore(this);

    //init param
    fps = 30;

    //init timer for appsink polling
    timer_appsink = new QTimer(this);
    connect(timer_appsink, SIGNAL(timeout()), this, SLOT(on_timer_check_appsink()));

    //init widget
    // Let layout control size, don't force resize
    // this->resize(800, 600);
    this->setWindowTitle(QString::fromUtf8("Video Sender"));
    
    std::cout<<"VideoReceiver-init-5"<<std::endl;
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    this->playerWidget = new PlayerWidget(this);
    this->playerWidget->setLockLastImg(true);

    QLabel *statusLabel = new QLabel("等待信号 (Waiting for Signal)...", this->playerWidget);
    statusLabel->setStyleSheet("QLabel { color: white; font-size: 20px; background-color: rgba(0,0,0,100); padding: 10px; border-radius: 5px; }");
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setObjectName("statusLabel");
    statusLabel->setVisible(true);

    QVBoxLayout *overlayLayout = new QVBoxLayout(this->playerWidget);
    overlayLayout->setContentsMargins(0, 0, 0, 0);
    overlayLayout->addStretch();
    overlayLayout->addWidget(statusLabel, 0, Qt::AlignCenter);
    overlayLayout->addStretch();
    statusLabel->raise();

    mainLayout->addWidget(playerWidget);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *btnPlay = new QPushButton("开始发送");
    QPushButton *btnStop = new QPushButton("停止发送");
    btnLayout->addWidget(btnPlay);
    btnLayout->addWidget(btnStop);
    mainLayout->addLayout(btnLayout);

    connect(btnPlay, &QPushButton::clicked, this, &VideoSender::on_pushButton_play_clicked);
    connect(btnStop, &QPushButton::clicked, this, &VideoSender::on_pushButton_pause_clicked);

    setLayout(mainLayout);
    std::cout<<"VideoSender-init-end"<<std::endl;

    this->init();
    //playerWidget->startPlay(fps); // Managed by GStreamer flow now
    this->is_play=false; // Start stopped

    frameClock.start();
    lastFrameAtMs = -1;
    showWaitingAfterMs = 500;

    if (std::getenv("EC_UI_AUTOSTART_SEND") != nullptr) {
        QTimer::singleShot(0, this, &VideoSender::on_pushButton_play_clicked);
    }
}

void VideoSender::init() {
    if(has_camera_init==false)
    {
        // GStreamer pipeline initialization is dynamic based on startGstPipeline
        has_camera_init = true;
        std::cout<<"[INIT] init video sender base"<<std::endl;
    }

}

void VideoSender::ShowImage(cv::Mat& mat)
{
    this->playerWidget->pushFrame2Show(mat);
}

VideoSender::~VideoSender()
{
    stopGstPipeline();
    if (m_audioSender) {
        m_audioSender->stop();
        delete m_audioSender;
        m_audioSender = nullptr;
    }
    
    on_pushButton_return_clicked();
    delete playerWidget;
    delete timer_appsink;
    if (g_video_sender_instance == this) {
        g_video_sender_instance = nullptr;
    }
}

void VideoSender::on_pushButton_play_clicked()
{
    const int targetTID = getIntFromIni("BigDataTransfer", "DEST_TID", 105);
    if (this->playerWidget) {
        QLabel *statusLabel = this->playerWidget->findChild<QLabel *>("statusLabel");
        if (statusLabel) {
            statusLabel->setText(QString("启动中... (DEST_TID=%1)").arg(targetTID));
            if (statusLabel->isVisible()) {
                statusLabel->raise();
            }
        }
    }
    startDeepIntegrationPipeline(targetTID);
    if (m_audioSender) {
        m_audioSender->start(targetTID);
    }
}

void VideoSender::on_pushButton_pause_clicked()
{
    if (m_audioSender) {
        m_audioSender->stop();
    }
    stopGstPipeline();
}

void VideoSender::on_pushButton_return_clicked() {

    on_pushButton_pause_clicked();
}

void VideoSender::startGstPipeline(const std::string& host, int port) {
    // Legacy method - redirect to deep integration for consistency if needed, 
    // or keep separate. The user asked to implement the "Sender" logic.
    // I will leave this legacy method as is to not break existing demo behavior 
    // if called explicitly, but on_pushButton_play_clicked now calls the new one.
    
    if (pipeline) {
        stopGstPipeline();
    }
    lastFrameAtMs = -1;
    if (this->playerWidget) {
        QLabel *statusLabel = this->playerWidget->findChild<QLabel *>("statusLabel");
        if (statusLabel) statusLabel->setVisible(true);
    }

    const std::string file_path = resolve_default_video_file_path();
    const std::string file_uri = to_file_uri(file_path);

    const std::string preview_branch =
        "t. ! queue ! videoconvert ! video/x-raw,format=BGR ! appsink name=mysink drop=true max-buffers=1 sync=false";
    const std::string rtp_branch =
        "t. ! queue ! videoconvert ! x264enc tune=zerolatency speed-preset=ultrafast bitrate=500 key-int-max=30 ! "
        "rtph264pay pt=96 config-interval=1 ! udpsink host=" + host + " port=" + std::to_string(port);

    std::string pipeline_str =
        "uridecodebin uri=" + file_uri + " ! videoconvert ! videoscale ! videorate ! "
        "video/x-raw,width=1280,height=720,framerate=30/1 ! tee name=t " +
        preview_branch + " " + rtp_branch;

    std::cout << "[VideoSender] Starting file pipeline (loop expected): " << pipeline_str << std::endl;

    GError *error = nullptr;
    pipeline = gst_parse_launch(pipeline_str.c_str(), &error);
    if (error) {
        std::cerr << "[VideoSender] Error creating pipeline: " << error->message << std::endl;
        g_error_free(error);
        return;
    }

    // Get appsink
    appsink_preview = gst_bin_get_by_name(GST_BIN(pipeline), "mysink");
    if (!appsink_preview) {
        std::cerr << "[VideoSender] Could not get appsink from pipeline" << std::endl;
    }

    // Start pipeline
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    
    // Start local preview timer
    timer_appsink->start(33); // ~30fps
    
    this->playerWidget->startPlay(fps);
    this->is_play = true;
}

void VideoSender::startDeepIntegrationPipeline(int destTID) {
    if (pipeline) {
        stopGstPipeline();
    }
    m_isStopping = false;
    m_destTID = destTID;
    m_sendPktsWindow = 0;
    m_sendBytesWindow = 0;
    m_sendFailWindow = 0;
    m_sendStatClock.restart();
    g_video_sender_instance = this;
    static bool s_speedcontrol_cb_registered = false;
    if (!s_speedcontrol_cb_registered) {
        Register_SpeedControl_RecvDataCallBack(VideoSender_SpeedControlRecvDataCallback);
        s_speedcontrol_cb_registered = true;
    }
    
    // Check for camera source override via environment variable
    const char* video_src_env = std::getenv("EC2_VIDEO_SOURCE");
    bool use_camera = (video_src_env && std::string(video_src_env) == "camera");

    std::string pipeline_str;
    if (use_camera) {
        // Camera source pipeline (v4l2src)
        const std::string preview_branch =
            "t. ! queue ! videoconvert ! video/x-raw,format=BGR ! "
            "appsink name=preview_sink drop=true max-buffers=1 sync=true";

        const std::string rtp_branch =
            "t. ! queue ! "
            "x264enc name=enc tune=zerolatency speed-preset=superfast bitrate=2500 key-int-max=30 bframes=0 ! "
            "rtph264pay mtu=1200 pt=96 config-interval=1 ! "
            "appsink name=rtp_sink emit-signals=true sync=true max-buffers=10 drop=true";

        pipeline_str =
            "v4l2src device=/dev/video0 ! videoconvert ! videoscale ! videorate ! "
            "video/x-raw,width=640,height=480,framerate=30/1 ! tee name=t " +
            preview_branch + " " + rtp_branch;
        
        std::cout << "[VideoSender] Starting Camera pipeline: " << pipeline_str << std::endl;
    } else {
        // Existing File source pipeline logic
        const std::string file_path = resolve_default_video_file_path();
        if (!QFileInfo::exists(QString::fromStdString(file_path))) {
            QMessageBox::information(this, "提示信息", QString("找不到视频文件：%1").arg(QString::fromStdString(file_path)));
            return;
        }
        const std::string file_uri = to_file_uri(file_path);

        const std::string preview_branch =
            "t. ! queue ! videoconvert ! video/x-raw,format=BGR ! "
            "appsink name=preview_sink drop=true max-buffers=1 sync=true";

        const std::string rtp_branch =
            "t. ! queue ! "
            "x264enc name=enc tune=zerolatency speed-preset=superfast bitrate=2500 key-int-max=30 bframes=0 ! "
            "rtph264pay mtu=1200 pt=96 config-interval=1 ! "
            "appsink name=rtp_sink emit-signals=true sync=true max-buffers=10 drop=true";

        pipeline_str =
            "uridecodebin uri=" + file_uri + " ! videoconvert ! videoscale ! videorate ! "
            "video/x-raw,width=640,height=480,framerate=30/1 ! tee name=t " +
            preview_branch + " " + rtp_branch;

        std::cout << "[VideoSender] Starting Deep Integration pipeline (mp4 loop): " << pipeline_str << std::endl;
    }

    GError *error = nullptr;
    pipeline = gst_parse_launch(pipeline_str.c_str(), &error);
    if (error) {
        std::cerr << "[VideoSender] Error creating pipeline: " << error->message << std::endl;
        g_error_free(error);
        pipeline = nullptr;
        QMessageBox::information(this, "提示信息", "启动发送失败：GStreamer pipeline 创建失败");
        if (this->playerWidget) {
            QLabel *statusLabel = this->playerWidget->findChild<QLabel *>("statusLabel");
            if (statusLabel) {
                statusLabel->setText("启动失败");
                statusLabel->setVisible(true);
            }
        }
        return;
    }

    encoder = gst_bin_get_by_name(GST_BIN(pipeline), "enc");
    appsink_preview = gst_bin_get_by_name(GST_BIN(pipeline), "preview_sink");
    appsink_rtp = gst_bin_get_by_name(GST_BIN(pipeline), "rtp_sink");
    if (appsink_rtp) {
        g_signal_connect(appsink_rtp, "new-sample", G_CALLBACK(on_new_sample_from_sink), this);
    } else {
        std::cerr << "[VideoSender] Could not get rtp_sink from pipeline" << std::endl;
    }

    GstStateChangeReturn state_ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (state_ret == GST_STATE_CHANGE_FAILURE) {
        QMessageBox::information(this, "提示信息", "启动发送失败：pipeline 无法进入 PLAYING 状态");
        stopGstPipeline();
        return;
    }
    timer_appsink->start(33);
    this->playerWidget->startPlay(fps);
    this->is_play = true;
    if (this->playerWidget) {
        QLabel *statusLabel = this->playerWidget->findChild<QLabel *>("statusLabel");
        if (statusLabel) {
            statusLabel->setText(QString("发送中... (DEST_TID=%1)").arg(m_destTID));
            statusLabel->setVisible(true);
            statusLabel->raise();
        }
    }
}

bool VideoSender::onSpeedControlRecvData(DWORD,
                                         const std::shared_ptr<BYTE> &pBuffer,
                                         DWORD dwLength,
                                         long int &,
                                         long int &) {
    if (!pBuffer || dwLength < 13) {
        return true;
    }
    const BYTE *typed = pBuffer.get();
    if (typed[0] != 0x01) {
        return true;
    }

    uint32_t recv_bps = 0;
    uint32_t loss_x1000 = 0;
    memcpy(&recv_bps, typed + 1, 4);
    memcpy(&loss_x1000, typed + 5, 4);
    recv_bps = ntohl(recv_bps);
    loss_x1000 = ntohl(loss_x1000);

    const uint32_t min_bps = 200000;
    const uint32_t max_bps = 8000000;
    uint32_t target_bps = recv_bps;
    if (loss_x1000 > 0 && loss_x1000 <= 1000) {
        target_bps = static_cast<uint32_t>((static_cast<uint64_t>(recv_bps) * (1000ULL - loss_x1000)) / 1000ULL);
    }
    if (target_bps < min_bps) target_bps = min_bps;
    if (target_bps > max_bps) target_bps = max_bps;

    const uint32_t target_kbps = target_bps / 1000;
    if (m_targetBitrateKbps == 0 ||
        (target_kbps > m_targetBitrateKbps ? target_kbps - m_targetBitrateKbps : m_targetBitrateKbps - target_kbps) >= 50) {
        m_targetBitrateKbps = target_kbps;
        if (encoder) {
            g_object_set(G_OBJECT(encoder), "bitrate", static_cast<guint>(target_kbps), nullptr);
        }
        const uint32_t target_mbps = static_cast<uint32_t>((target_bps + 500000) / 1000000);
        if (m_destTID > 0 && target_mbps > 0) {
            ExChangeTransSpeedOfTID(static_cast<DWORD>(m_destTID), static_cast<DWORD>(target_mbps));
        }
    }
    return true;
}

// Static callback for appsink new-sample signal
GstFlowReturn VideoSender::on_new_sample_from_sink(GstElement *sink, VideoSender *sender) {
    if (!sender || sender->m_isStopping.load()) {
        return GST_FLOW_OK;
    }
    GstSample *sample;
    g_signal_emit_by_name(sink, "pull-sample", &sample);
    
    if (sample) {
        GstBuffer *buffer = gst_sample_get_buffer(sample);
        if (buffer) {
            GstMapInfo map;
            if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
                // Network Injection
                sender->handleRtpData(map.data, map.size);
                gst_buffer_unmap(buffer, &map);
            }
        }
        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }
    return GST_FLOW_ERROR;
}

// Internal helper to send RTP data via NetCombTransfer
void VideoSender::handleRtpData(guint8* data, gsize size) {
    if (size <= 0) return;
    if (m_destTID <= 0) return;
    if (m_isStopping.load()) return;

    const uint64_t captureTsUs = gst_util_get_timestamp() / 1000ULL;
    const uint64_t tsBe = to_be_u64(captureTsUs);
    DWORD totalLen = static_cast<DWORD>(size + 1 + sizeof(uint64_t));
    std::shared_ptr<BYTE> sendBuf(new BYTE[totalLen], std::default_delete<BYTE[]>());
    
    sendBuf.get()[0] = 0x00;
    memcpy(sendBuf.get() + 1, &tsBe, sizeof(uint64_t));
    memcpy(sendBuf.get() + 1 + sizeof(uint64_t), data, size);

    bool ret = SendTIDDataWithSpeedControl(static_cast<DWORD>(m_destTID),
                                           sendBuf,
                                           totalLen,
                                           false,
                                           false,
                                           false); // 改为false，使用MRUDP回调
    
    if (ret) {
        m_sendPktsWindow += 1;
        m_sendBytesWindow += totalLen;
    } else {
        m_sendFailWindow += 1;
    }

    const qint64 elapsedMs = m_sendStatClock.elapsed();
    if (elapsedMs >= 1000) {
        const uint64_t bps = (m_sendBytesWindow * 8ULL * 1000ULL) / static_cast<uint64_t>(elapsedMs);
        const uint64_t pps = (m_sendPktsWindow * 1000ULL) / static_cast<uint64_t>(elapsedMs);
        std::cout << "[VideoSender] TID=" << getTID()
                  << " -> DEST_TID=" << m_destTID
                  << " send_pps=" << pps
                  << " send_kbps=" << (bps / 1000ULL)
                  << " send_fail=" << m_sendFailWindow
                  << std::endl;
        m_sendPktsWindow = 0;
        m_sendBytesWindow = 0;
        m_sendFailWindow = 0;
        m_sendStatClock.restart();
    }
}

void VideoSender::stopGstPipeline() {
    m_isStopping = true;
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        pipeline = nullptr;
    }
    if (encoder) {
        gst_object_unref(encoder);
        encoder = nullptr;
    }
    if (appsink_preview) {
        gst_object_unref(appsink_preview);
        appsink_preview = nullptr;
    }
    if (appsink_rtp) {
        gst_object_unref(appsink_rtp);
        appsink_rtp = nullptr;
    }
    timer_appsink->stop();
    this->playerWidget->stopPlay();
    this->is_play = false;
    lastFrameAtMs = -1;
    if (this->playerWidget) {
        QLabel *statusLabel = this->playerWidget->findChild<QLabel *>("statusLabel");
        if (statusLabel) {
            statusLabel->setText("等待信号 (Waiting for Signal)...");
            statusLabel->setVisible(true);
            statusLabel->raise();
        }
    }
}

void VideoSender::on_timer_check_appsink() {
    if (!pipeline) return;

    GstBus* bus = gst_element_get_bus(pipeline);
    while (true) {
        GstMessage* msg = gst_bus_pop_filtered(bus, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
        if (!msg) break;

        if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_EOS) {
            gst_element_seek_simple(
                pipeline,
                GST_FORMAT_TIME,
                (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT),
                0
            );
            gst_element_set_state(pipeline, GST_STATE_PLAYING);
        } else if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
            GError* err = nullptr;
            gchar* debug = nullptr;
            gst_message_parse_error(msg, &err, &debug);
            if (err) {
                std::cerr << "[VideoSender] GStreamer error: " << err->message << std::endl;
                g_error_free(err);
            }
            if (debug) g_free(debug);
        }
        gst_message_unref(msg);
    }
    gst_object_unref(bus);

    if (!appsink_preview) return;
    
    GstSample *sample = gst_app_sink_try_pull_sample(GST_APP_SINK(appsink_preview), 0);
    if (sample) {
        GstBuffer *buffer = gst_sample_get_buffer(sample);
        GstCaps *caps = gst_sample_get_caps(sample);
        GstStructure *structure = gst_caps_get_structure(caps, 0);
        
        int width, height;
        gst_structure_get_int(structure, "width", &width);
        gst_structure_get_int(structure, "height", &height);
        
        GstMapInfo map;
        gst_buffer_map(buffer, &map, GST_MAP_READ);
        
        // Convert to OpenCV Mat (BGR)
        cv::Mat frame(height, width, CV_8UC3, (char*)map.data, cv::Mat::AUTO_STEP);
        
        // Deep copy because GStreamer buffer will be unref'd
        cv::Mat frame_copy = frame.clone();
        
        gst_buffer_unmap(buffer, &map);
        gst_sample_unref(sample);
        
        // Update UI
        QLabel *statusLabel = this->playerWidget->findChild<QLabel *>("statusLabel");
        if (statusLabel && statusLabel->isVisible()) {
            statusLabel->setVisible(false);
        }
        lastFrameAtMs = frameClock.elapsed();
        ShowImage(frame_copy);
        if (std::getenv("EC_UI_EXIT_AFTER_FIRST_FRAME") != nullptr) {
            QCoreApplication::quit();
        }
    } else {
        if (lastFrameAtMs < 0) {
            QLabel *statusLabel = this->playerWidget->findChild<QLabel *>("statusLabel");
            if (statusLabel && !statusLabel->isVisible()) {
                statusLabel->setVisible(true);
            }
        }
    }
}
