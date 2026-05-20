#include "VideoSender.h"

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui_c.h>
#include <QCoreApplication>
#include <QDateTime>
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
#include <algorithm>
#include <cmath>
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
        QDir::cleanPath(QDir::currentPath() + "/FileSend/videotest.mp4"),
        QDir::cleanPath(QDir::currentPath() + "/../FileSend/videotest.mp4"),
        QDir::cleanPath(QDir::currentPath() + "/../../FileSend/videotest.mp4"),
        QDir::cleanPath(QDir::currentPath() + "/../../../FileSend/videotest.mp4"),
        QDir::cleanPath(QDir::currentPath() + "/../../../../FileSend/videotest.mp4"),
        QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../FileSend/videotest.mp4"),
        QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../../FileSend/videotest.mp4"),
        QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../../../FileSend/videotest.mp4"),
        QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../../../../FileSend/videotest.mp4"),
        QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../../../../../FileSend/videotest.mp4"),
    };

    for (const auto& path : candidates) {
        if (QFileInfo::exists(path)) {
            return path.toStdString();
        }
    }
    return "FileSend/videotest.mp4";
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

VideoSender::StreamProfile VideoSender::profileForLevel(AdaptiveProfileLevel level) const {
    switch (level) {
    case AdaptiveProfileLevel::Normal:
        return StreamProfile{640, 480, 30, 1200, 900, 1600};
    case AdaptiveProfileLevel::LowBandwidth:
        return StreamProfile{480, 360, 20, 700, 450, 900};
    case AdaptiveProfileLevel::Extreme:
    default:
        return StreamProfile{320, 240, 15, 350, 200, 500};
    }
}

void VideoSender::updateStatusLabel(const QString &text, bool forceVisible) {
    if (!this->playerWidget) {
        return;
    }
    QLabel *statusLabel = this->playerWidget->findChild<QLabel *>("statusLabel");
    if (!statusLabel) {
        return;
    }
    statusLabel->setText(text);
    if (forceVisible) {
        statusLabel->setVisible(true);
        statusLabel->raise();
    }
}

bool VideoSender::startAdaptivePipelineInternal() {
    const StreamProfile profile = profileForLevel(m_currentProfile);
    fps = profile.fps;
    lastFrameAtMs = -1;
    updateStatusLabel(QString("启动中... %1x%2 @ %3fps")
                          .arg(profile.width)
                          .arg(profile.height)
                          .arg(profile.fps),
                      true);

    std::string pipeline_str;
    const std::string preview_branch =
        "t. ! queue leaky=downstream max-size-buffers=5 ! videoconvert ! video/x-raw,format=BGR ! "
        "appsink name=preview_sink drop=true max-buffers=5 sync=false";

    const uint32_t vbvBufSizeKbits = std::max<uint32_t>(profile.maxBitrateKbps, profile.defaultBitrateKbps * 2);
    const std::string rtp_branch =
        "t. ! queue leaky=downstream max-size-buffers=4 ! "
        "x264enc name=enc tune=zerolatency speed-preset=ultrafast bitrate=" + std::to_string(profile.defaultBitrateKbps) +
        " bframes=0 threads=1 option-string=vbv-bufsize=" + std::to_string(vbvBufSizeKbits) + ":scenecut=0:sliced-threads=1 ! "
        "rtph264pay mtu=1200 pt=96 config-interval=1 ! "
        "appsink name=rtp_sink emit-signals=true sync=false max-buffers=10 drop=true";

    if (m_useCameraSource) {
        pipeline_str =
            "v4l2src device=/dev/video0 ! videoconvert ! videoscale ! videorate ! "
            "video/x-raw,width=" + std::to_string(profile.width) +
            ",height=" + std::to_string(profile.height) +
            ",framerate=" + std::to_string(profile.fps) + "/1 ! tee name=t " +
            preview_branch + " " + rtp_branch;
        std::cout << "[VideoSender] Starting Camera pipeline: " << pipeline_str << std::endl;
    } else {
        if (m_videoFilePath.empty()) {
            m_videoFilePath = resolve_default_video_file_path();
        }
        if (!QFileInfo::exists(QString::fromStdString(m_videoFilePath))) {
            QMessageBox::information(this, "提示信息", QString("找不到视频文件：%1").arg(QString::fromStdString(m_videoFilePath)));
            updateStatusLabel("启动失败：视频文件不存在", true);
            return false;
        }
        const std::string file_uri = to_file_uri(m_videoFilePath);
        pipeline_str =
            "uridecodebin uri=" + file_uri + " ! videoconvert ! videoscale ! videorate ! "
            "video/x-raw,width=" + std::to_string(profile.width) +
            ",height=" + std::to_string(profile.height) +
            ",framerate=" + std::to_string(profile.fps) + "/1 ! tee name=t " +
            preview_branch + " " + rtp_branch;
        std::cout << "[VideoSender] Starting adaptive file pipeline: " << pipeline_str << std::endl;
    }

    GError *error = nullptr;
    pipeline = gst_parse_launch(pipeline_str.c_str(), &error);
    if (error) {
        std::cerr << "[VideoSender] Error creating pipeline: " << error->message << std::endl;
        g_error_free(error);
        pipeline = nullptr;
        QMessageBox::information(this, "提示信息", "启动发送失败：GStreamer pipeline 创建失败");
        updateStatusLabel("启动失败", true);
        return false;
    }

    encoder = gst_bin_get_by_name(GST_BIN(pipeline), "enc");
    appsink_preview = gst_bin_get_by_name(GST_BIN(pipeline), "preview_sink");
    appsink_rtp = gst_bin_get_by_name(GST_BIN(pipeline), "rtp_sink");
    if (!encoder || !appsink_preview || !appsink_rtp) {
        std::cerr << "[VideoSender] Could not get encoder/preview_sink/rtp_sink from pipeline" << std::endl;
        stopGstPipeline();
        return false;
    }

    if (GParamSpec *prop = g_object_class_find_property(G_OBJECT_GET_CLASS(encoder), "byte-stream")) {
        (void)prop;
        g_object_set(G_OBJECT(encoder), "byte-stream", TRUE, nullptr);
    }
    if (GParamSpec *prop = g_object_class_find_property(G_OBJECT_GET_CLASS(encoder), "intra-refresh")) {
        (void)prop;
        g_object_set(G_OBJECT(encoder), "intra-refresh", TRUE, nullptr);
    }

    g_signal_connect(appsink_rtp, "new-sample", G_CALLBACK(on_new_sample_from_sink), this);

    GstStateChangeReturn state_ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (state_ret == GST_STATE_CHANGE_FAILURE) {
        QMessageBox::information(this, "提示信息", "启动发送失败：pipeline 无法进入 PLAYING 状态");
        stopGstPipeline();
        return false;
    }

    timer_appsink->start(33);
    if (timer_adaptation && !timer_adaptation->isActive()) {
        timer_adaptation->start(m_adaptationPollIntervalMs);
    }
    this->playerWidget->startPlay(fps);
    this->is_play = true;
    m_appliedProfile = m_currentProfile;
    applyEncoderBitrate(profile.defaultBitrateKbps);

    QString profileName;
    switch (m_currentProfile) {
    case AdaptiveProfileLevel::Normal:
        profileName = "Normal";
        break;
    case AdaptiveProfileLevel::LowBandwidth:
        profileName = "Low";
        break;
    case AdaptiveProfileLevel::Extreme:
    default:
        profileName = "Extreme";
        break;
    }
    updateStatusLabel(QString("发送中... %1 %2x%3 @ %4fps")
                          .arg(profileName)
                          .arg(profile.width)
                          .arg(profile.height)
                          .arg(profile.fps),
                      true);
    return true;
}

void VideoSender::restartAdaptivePipeline() {
    if (m_isStopping.load()) {
        return;
    }
    stopGstPipeline();
    m_targetBitrateKbps = 0;
    startAdaptivePipelineInternal();
}

void VideoSender::startGstPipeline(const std::string&, int) {
    const int targetTID = getIntFromIni("BigDataTransfer", "DEST_TID", 105);
    startDeepIntegrationPipeline(targetTID);
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
    m_targetBitrateKbps = 0;
    m_lastAppliedSpeedMbps = 0;
    m_smoothedBandwidthKbps = 0.0;
    m_smoothedRttMs = 0.0;
    m_lastRawRttMs = 0.0;
    m_lastQueueDepth = 0;
    m_lastQueueCapacity = 0;
    m_lastAdaptiveLogMs = 0;
    m_lastProfileSwitchMs = QDateTime::currentMSecsSinceEpoch();
    m_currentProfile = AdaptiveProfileLevel::Normal;
    m_appliedProfile = AdaptiveProfileLevel::Normal;
    g_video_sender_instance = this;

    m_normalMinKbps = static_cast<uint32_t>(std::max(600, getIntFromIni("VideoAdaptive", "NormalMinKbps", 1000)));
    m_lowMinKbps = static_cast<uint32_t>(std::max(250, getIntFromIni("VideoAdaptive", "LowMinKbps", 500)));
    m_queueLowWatermark = static_cast<DWORD>(std::max(10, getIntFromIni("VideoAdaptive", "QueueLowPackets", 40)));
    m_queueHighWatermark = static_cast<DWORD>(std::max(20, getIntFromIni("VideoAdaptive", "QueueHighPackets", 120)));
    m_queueCriticalWatermark = static_cast<DWORD>(std::max(static_cast<int>(m_queueHighWatermark + 20), getIntFromIni("VideoAdaptive", "QueueCriticalPackets", 240)));
    m_profileSwitchCooldownMs = std::max(500, getIntFromIni("VideoAdaptive", "ProfileSwitchCooldownMs", 1500));
    m_adaptationPollIntervalMs = std::max(100, getIntFromIni("VideoAdaptive", "PollIntervalMs", 300));

    const char *video_src_env = std::getenv("EC2_VIDEO_SOURCE");
    m_useCameraSource = (video_src_env && std::string(video_src_env) == "camera");
    m_videoFilePath = resolve_default_video_file_path();

    if (timer_adaptation) {
        timer_adaptation->setInterval(m_adaptationPollIntervalMs);
    }

    if (!startAdaptivePipelineInternal()) {
        return;
    }
}

void VideoSender::on_adaptation_tick() {
    updateAdaptiveController();
}

void VideoSender::updateAdaptiveController() {
    if (m_destTID <= 0 || m_isStopping.load()) {
        return;
    }

    const TermToTermRoundTripTime qos = MRUDP_Get_QosRtt(static_cast<DWORD>(m_destTID));
    const double rawBandwidthKbps = qos.bandwidth > 0 ? static_cast<double>(qos.bandwidth) : 0.0;
    const double rawRttMs = qos.roundtriptime > 0 ? static_cast<double>(qos.roundtriptime) : 5.0;

    if (rawBandwidthKbps > 0.0) {
        if (m_smoothedBandwidthKbps <= 0.0) {
            m_smoothedBandwidthKbps = rawBandwidthKbps;
        } else {
            m_smoothedBandwidthKbps = m_smoothedBandwidthKbps * 0.75 + rawBandwidthKbps * 0.25;
        }
    }
    if (m_smoothedRttMs <= 0.0) {
        m_smoothedRttMs = rawRttMs;
    } else {
        m_smoothedRttMs = m_smoothedRttMs * 0.80 + rawRttMs * 0.20;
    }
    const double rttTrendMs = (m_lastRawRttMs > 0.0) ? (rawRttMs - m_lastRawRttMs) : 0.0;
    m_lastRawRttMs = rawRttMs;

    DWORD queueDepth = 0;
    DWORD queueCapacity = 0;
    if (GetSpeedControlQueueStats(static_cast<DWORD>(m_destTID), queueDepth, queueCapacity)) {
        m_lastQueueDepth = queueDepth;
        m_lastQueueCapacity = queueCapacity;
    } else {
        queueDepth = m_lastQueueDepth;
        queueCapacity = m_lastQueueCapacity;
    }

    double effectiveBandwidthKbps = (m_smoothedBandwidthKbps > 0.0)
        ? m_smoothedBandwidthKbps
        : static_cast<double>(profileForLevel(m_currentProfile).defaultBitrateKbps);
    if (rttTrendMs > 20.0) {
        effectiveBandwidthKbps *= 0.92;
    }
    if (rttTrendMs > 40.0) {
        effectiveBandwidthKbps *= 0.85;
    }
    if (queueDepth >= m_queueHighWatermark) {
        effectiveBandwidthKbps *= 0.85;
    }
    if (queueDepth >= m_queueCriticalWatermark) {
        effectiveBandwidthKbps *= 0.65;
    }

    const AdaptiveProfileLevel targetProfile = decideProfile(effectiveBandwidthKbps, m_smoothedRttMs, rttTrendMs, queueDepth);
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    if (targetProfile != m_currentProfile && (nowMs - m_lastProfileSwitchMs) >= m_profileSwitchCooldownMs) {
        m_currentProfile = targetProfile;
        m_lastProfileSwitchMs = nowMs;
        restartAdaptivePipeline();
    }

    const StreamProfile profile = profileForLevel(m_currentProfile);
    double desiredBitrateKbps = effectiveBandwidthKbps * 0.85;
    if (desiredBitrateKbps <= 0.0) {
        desiredBitrateKbps = static_cast<double>(profile.defaultBitrateKbps);
    }
    if (queueDepth >= m_queueHighWatermark) {
        desiredBitrateKbps *= 0.90;
    }
    if (queueDepth >= m_queueCriticalWatermark) {
        desiredBitrateKbps = std::min<double>(desiredBitrateKbps, profile.minBitrateKbps);
    }
    desiredBitrateKbps = std::max<double>(desiredBitrateKbps, profile.minBitrateKbps);
    desiredBitrateKbps = std::min<double>(desiredBitrateKbps, profile.maxBitrateKbps);
    applyEncoderBitrate(static_cast<uint32_t>(std::lround(desiredBitrateKbps)));

    if (nowMs - m_lastAdaptiveLogMs >= 1500) {
        const char *profileName = (m_currentProfile == AdaptiveProfileLevel::Normal)
            ? "Normal"
            : (m_currentProfile == AdaptiveProfileLevel::LowBandwidth ? "Low" : "Extreme");
        std::cout << "[VideoSender][Adaptive] dest=" << m_destTID
                  << " bw_kbps=" << static_cast<int64_t>(m_smoothedBandwidthKbps)
                  << " rtt_ms=" << static_cast<int64_t>(m_smoothedRttMs)
                  << " q=" << queueDepth;
        if (queueCapacity > 0) {
            std::cout << "/" << queueCapacity;
        }
        std::cout << " profile=" << profileName
                  << " bitrate_kbps=" << m_targetBitrateKbps
                  << std::endl;
        m_lastAdaptiveLogMs = nowMs;
    }
}

VideoSender::AdaptiveProfileLevel VideoSender::decideProfile(double effectiveBandwidthKbps,
                                                             double smoothedRttMs,
                                                             double rttTrendMs,
                                                             DWORD queueDepth) const {
    const bool queueLow = queueDepth <= m_queueLowWatermark;
    const bool queueHigh = queueDepth >= m_queueHighWatermark;
    const bool queueCritical = queueDepth >= m_queueCriticalWatermark;

    switch (m_currentProfile) {
    case AdaptiveProfileLevel::Normal:
        if (queueCritical || effectiveBandwidthKbps < static_cast<double>(m_lowMinKbps) * 0.85 ||
            (smoothedRttMs > 260.0 && rttTrendMs > 25.0)) {
            return AdaptiveProfileLevel::Extreme;
        }
        if (queueHigh || effectiveBandwidthKbps < static_cast<double>(m_normalMinKbps) ||
            (smoothedRttMs > 180.0 && rttTrendMs > 20.0)) {
            return AdaptiveProfileLevel::LowBandwidth;
        }
        return AdaptiveProfileLevel::Normal;
    case AdaptiveProfileLevel::LowBandwidth:
        if (queueCritical || effectiveBandwidthKbps < static_cast<double>(m_lowMinKbps) * 0.80 ||
            (smoothedRttMs > 260.0 && rttTrendMs > 25.0)) {
            return AdaptiveProfileLevel::Extreme;
        }
        if (queueLow && effectiveBandwidthKbps > static_cast<double>(m_normalMinKbps) + 150.0 && smoothedRttMs < 140.0) {
            return AdaptiveProfileLevel::Normal;
        }
        return AdaptiveProfileLevel::LowBandwidth;
    case AdaptiveProfileLevel::Extreme:
    default:
        if (queueLow && effectiveBandwidthKbps > static_cast<double>(m_lowMinKbps) + 120.0 && smoothedRttMs < 180.0) {
            return AdaptiveProfileLevel::LowBandwidth;
        }
        return AdaptiveProfileLevel::Extreme;
    }
}

void VideoSender::applyEncoderBitrate(uint32_t targetKbps) {
    if (targetKbps == 0) {
        return;
    }

    const uint32_t delta = (m_targetBitrateKbps > targetKbps)
        ? (m_targetBitrateKbps - targetKbps)
        : (targetKbps - m_targetBitrateKbps);
    if (m_targetBitrateKbps != 0 && delta < 40) {
        return;
    }

    m_targetBitrateKbps = targetKbps;
    if (encoder) {
        g_object_set(G_OBJECT(encoder), "bitrate", static_cast<guint>(targetKbps), nullptr);
    }

    if (m_destTID > 0) {
        const uint32_t targetMbps = std::max<uint32_t>(1, (targetKbps + 999U) / 1000U);
        if (targetMbps != m_lastAppliedSpeedMbps) {
            ExChangeTransSpeedOfTID(static_cast<DWORD>(m_destTID), static_cast<DWORD>(targetMbps));
            m_lastAppliedSpeedMbps = targetMbps;
        }
    }
}

bool VideoSender::onSpeedControlRecvData(DWORD,
                                         const std::shared_ptr<BYTE> &,
                                         DWORD,
                                         long int &,
                                         long int &) {
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
    if (timer_adaptation) {
        timer_adaptation->stop();
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
