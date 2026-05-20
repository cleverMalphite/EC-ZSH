#include "VideoReceiver.h"
//#include "ui_VideoReceiver.h"

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui_c.h>
#include <iostream>
#include <time.h>
#include <QVBoxLayout>
#include <cstring>

// decode
#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include "opencv2/opencv.hpp"

#include "../GlobalMessage.h"
#include "../SpeedControl/SpeedControlApi.h"
#include "VideoRtp_callback.h"
#include "../Util/AudioRtp_callback.h"

#include <list>
#include <thread>
#include <utility>
#include <mutex>
#include <algorithm>
#include <cmath>

#include <QLabel>
#include <QVBoxLayout>

using namespace cv;

//#define DEBUG_VIDEO_RECEIVER_
//#define TEST_VIDEO_RECEIVER_

// GStreamer initialization helper
static bool gst_initialized = false;
static void ensure_gst_init() {
    if (!gst_initialized) {
        gst_init(nullptr, nullptr);
        gst_initialized = true;
    }
}

static VideoReceiver* g_video_receiver_instance = nullptr;

static bool VideoReceiver_SpeedControlRecvDataCallback(DWORD dwTID,
                                                       const std::shared_ptr<BYTE> &pBuffer,
                                                       DWORD dwLength,
                                                       long int &recvtime,
                                                       long int &fb_send_time) {
    // std::cout << "[DEBUG] Static Callback Triggered. Len=" << dwLength << " Instance=" << (void*)g_video_receiver_instance << std::endl;
    if (!g_video_receiver_instance) {
        std::cout << "[DEBUG] Instance is NULL!" << std::endl;
        return true;
    }
    return g_video_receiver_instance->onSpeedControlRecvData(dwTID, pBuffer, dwLength, recvtime, fb_send_time);
}

static bool VideoReceiver_NetCombTransferMessageCallback(DWORD, bool) {
    return true;
}

static bool VideoReceiver_AudioRtpRecvDataCallback(DWORD dwTID,
                                                   const std::shared_ptr<BYTE> &pBuffer,
                                                   DWORD dwLength,
                                                   long int &recvtime,
                                                   long int &fb_send_time) {
    if (!g_video_receiver_instance) {
        return true;
    }
    return g_video_receiver_instance->onAudioRtpData(dwTID, pBuffer, dwLength, recvtime, fb_send_time);
}

static bool VideoReceiver_NetCombTransferBufferCallback(DWORD dwTID,
                                                        const std::shared_ptr<BYTE> &pBuffer,
                                                        DWORD dwLength,
                                                        bool bMsg,
                                                        int64_t packet_recv_timeStamp) {
    if (!g_video_receiver_instance) {
        return true;
    }
    return g_video_receiver_instance->onNetCombTransferBuffer(dwTID, pBuffer, dwLength, bMsg, packet_recv_timeStamp);
}

static uint64_t from_be_u64_video(uint64_t v) {
    const uint32_t hi = ntohl(static_cast<uint32_t>(v & 0xFFFFFFFFULL));
    const uint32_t lo = ntohl(static_cast<uint32_t>((v >> 32) & 0xFFFFFFFFULL));
    return (static_cast<uint64_t>(hi) << 32) | static_cast<uint64_t>(lo);
}

VideoReceiver::VideoReceiver(QWidget *parent, bool forceReceive)
    : QWidget(parent)
{
    ensure_gst_init();

    /*udp decode etc. init*/
#ifdef DEBUG_VIDEO_RECEIVER_
    std::cout<<"VideoReceiver-init-1"<<std::endl;
#endif
    exit_flag=false;
//    this->setWindowFlags(this->windowFlags()&~Qt::WindowCloseButtonHint);  //隐藏窗口的叉号

    //init param
    fps = 30;

    //init timer for appsink polling
    timer_appsink = new QTimer(this);
    connect(timer_appsink, SIGNAL(timeout()), this, SLOT(on_timer_check_appsink()));

    //init widget
    // Let layout control size
    // this->resize(800, 600);

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
    setLayout(mainLayout);

    frameClock.start();
    lastFrameAtMs = -1;
    showWaitingAfterMs = 500;
    m_rxRawPktsWindow = 0;
    m_rxRtpPktsWindow = 0;
    m_rxRtpBytesWindow = 0;
    m_rxStatClock.start();

    if (gUiRole == 2 || forceReceive) {
        g_video_receiver_instance = this;
        m_audioReceiver = new AudioReceiverCore(this);
        m_audioReceiver->start();
        Register_VideoRtp_RecvDataCallBack(VideoReceiver_SpeedControlRecvDataCallback);
        Register_AudioRtp_RecvDataCallBack(VideoReceiver_AudioRtpRecvDataCallback);
        startDeepIntegrationPipeline();
    }
}

void VideoReceiver::init(){
    // Legacy init - kept empty or minimal as logic moved to constructor
}

void VideoReceiver::reset() {
    // Legacy reset logic
}


void VideoReceiver::ShowImage(cv::Mat& mat)
{
#ifdef DEBUG_VIDEO_RECEIVER_
    printf("show Image...\n");
#endif
    this->playerWidget->pushFrame2Show(mat);
}



VideoReceiver::~VideoReceiver()
{
    stopGstPipeline();
    
    // on_pushButton_return_clicked(); // Removed as UI buttons are removed
    delete playerWidget;
    if (m_audioReceiver) {
        m_audioReceiver->stop();
        delete m_audioReceiver;
        m_audioReceiver = nullptr;
    }
    exit_flag=true;
    delete timer_appsink;
    if (g_video_receiver_instance == this) {
        g_video_receiver_instance = nullptr;
    }
    // Legacy cleanup
    // ffmpegobj.Ffmpeg_Decoder_Close();
}

void VideoReceiver::on_pushButton_return_clicked() {
    // emit this->sendEndCommand(); // Receiver is passive now
    endRecvVideo();
}

void VideoReceiver::startRecvVideo() {
    // Managed by constructor now
}

void VideoReceiver::endRecvVideo() {
    // Managed by destructor now
}

void VideoReceiver::startDeepIntegrationPipeline() {
    if (pipeline) {
        stopGstPipeline();
    }

    m_latencyCurrentMs = 200;

    lastFrameAtMs = -1;
    if (this->playerWidget) {
        QLabel *statusLabel = this->playerWidget->findChild<QLabel *>("statusLabel");
        if (statusLabel) statusLabel->setVisible(true);
    }

    const std::string pipeline_str =
        "appsrc name=rtp_src is-live=true format=time block=false "
        "caps=\"application/x-rtp,media=video,encoding-name=H264,payload=96,clock-rate=90000\" ! "
        "rtpjitterbuffer name=video_jitter latency=200 drop-on-latency=true do-lost=true mode=slave ! "
        "rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! video/x-raw,format=BGR ! "
        "appsink name=mysink drop=true max-buffers=5 sync=false";

    GError *error = nullptr;
    pipeline = gst_parse_launch(pipeline_str.c_str(), &error);
    if (error) {
        std::cerr << "[VideoReceiver] Error creating pipeline: " << error->message << std::endl;
        g_error_free(error);
        return;
    }

    appsrc = gst_bin_get_by_name(GST_BIN(pipeline), "rtp_src");
    appsink = gst_bin_get_by_name(GST_BIN(pipeline), "mysink");
    jitterbuffer = gst_bin_get_by_name(GST_BIN(pipeline), "video_jitter");
    if (jitterbuffer) {
        g_object_set(G_OBJECT(jitterbuffer), "latency", m_latencyCurrentMs, nullptr);
    }

    if (!appsrc || !appsink) {
        std::cerr << "[VideoReceiver] Could not get rtp_src/mysink from pipeline" << std::endl;
    }

    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "[VideoReceiver] Unable to set the pipeline to the PLAYING state." << std::endl;
        gst_object_unref(pipeline);
        pipeline = nullptr;
        if (appsrc) {
            gst_object_unref(appsrc);
            appsrc = nullptr;
        }
        if (appsink) {
            gst_object_unref(appsink);
            appsink = nullptr;
        }
        return;
    }

    timer_appsink->start(33);
    this->playerWidget->startPlay(fps);
    this->is_play = true;

    m_hasArrival = false;
    m_interArrivalAvgMs = 0.0;
    m_jitterEmaMs = 0.0;
    m_jitterAdjustClock.restart();
}

void VideoReceiver::startGstPipeline(int port) {
    if (pipeline) {
        stopGstPipeline();
    }
    lastFrameAtMs = -1;
    if (this->playerWidget) {
        QLabel *statusLabel = this->playerWidget->findChild<QLabel *>("statusLabel");
        if (statusLabel) statusLabel->setVisible(true);
    }

    // Pipeline description:
    // udpsrc port=5000 caps="application/x-rtp,media=video,encoding-name=H264,payload=96" ! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! video/x-raw,format=BGR ! appsink name=mysink
    // Note: 'udpsrc' will block/wait for data. 'avdec_h264' handles stream interruptions/resumes automatically.
    
    std::string pipeline_str = 
        "udpsrc address=127.0.0.1 port=" + std::to_string(port) + " caps=\"application/x-rtp,media=video,encoding-name=H264,payload=96\" ! "
        "rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! video/x-raw,format=BGR ! appsink name=mysink drop=true max-buffers=1 sync=false";

    std::cout << "[VideoReceiver] Starting Zero-Touch pipeline on port " << port << ": " << pipeline_str << std::endl;

    GError *error = nullptr;
    pipeline = gst_parse_launch(pipeline_str.c_str(), &error);
    if (error) {
        std::cerr << "[VideoReceiver] Error creating pipeline: " << error->message << std::endl;
        g_error_free(error);
        return;
    }

    // Get appsink
    appsink = gst_bin_get_by_name(GST_BIN(pipeline), "mysink");
    if (!appsink) {
        std::cerr << "[VideoReceiver] Could not get appsink from pipeline" << std::endl;
    }

    // Start pipeline immediately
    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "[VideoReceiver] Unable to set the pipeline to the PLAYING state." << std::endl;
        gst_object_unref(pipeline);
        pipeline = nullptr;
        return;
    }

    // Start local preview timer to pull frames
    timer_appsink->start(33); // ~30fps
    
    this->playerWidget->startPlay(fps);
    this->is_play = true;
}

void VideoReceiver::stopGstPipeline() {
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        pipeline = nullptr;
    }
    if (appsrc) {
        gst_object_unref(appsrc);
        appsrc = nullptr;
    }
    if (appsink) {
        gst_object_unref(appsink);
        appsink = nullptr;
    }
    if (jitterbuffer) {
        gst_object_unref(jitterbuffer);
        jitterbuffer = nullptr;
    }
    timer_appsink->stop();
    this->playerWidget->stopPlay();
    this->is_play = false;
    lastFrameAtMs = -1;
    if (this->playerWidget) {
        QLabel *statusLabel = this->playerWidget->findChild<QLabel *>("statusLabel");
        if (statusLabel) statusLabel->setVisible(true);
    }
}

void VideoReceiver::on_timer_check_appsink() {
    if (!appsink) return;
    
    // Check bus messages first
    if (pipeline) {
        GstBus *bus = gst_element_get_bus(pipeline);
        GstMessage *msg = gst_bus_pop(bus);
        if (msg) {
            switch (GST_MESSAGE_TYPE(msg)) {
                case GST_MESSAGE_ERROR: {
                    GError *err;
                    gchar *debug;
                    gst_message_parse_error(msg, &err, &debug);
                    std::cerr << "[VideoReceiver] Pipeline Error: " << err->message << std::endl;
                    g_error_free(err);
                    g_free(debug);
                    break;
                }
                case GST_MESSAGE_WARNING: {
                    GError *err;
                    gchar *debug;
                    gst_message_parse_warning(msg, &err, &debug);
                    std::cerr << "[VideoReceiver] Pipeline Warning: " << err->message << std::endl;
                    g_error_free(err);
                    g_free(debug);
                    break;
                }
                case GST_MESSAGE_STATE_CHANGED:
                    // Ignore state changes for now to avoid spam
                    break;
                default:
                    // std::cout << "[VideoReceiver] Bus message: " << GST_MESSAGE_TYPE_NAME(msg) << std::endl;
                    break;
            }
            gst_message_unref(msg);
        }
        gst_object_unref(bus);
    }

    GstSample *sample = gst_app_sink_try_pull_sample(GST_APP_SINK(appsink), 10 * GST_MSECOND);
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
        // Hide status label if visible
        QLabel *statusLabel = this->playerWidget->findChild<QLabel *>("statusLabel");
        if (statusLabel && statusLabel->isVisible()) {
            statusLabel->setVisible(false);
        }
        lastFrameAtMs = frameClock.elapsed();
        ShowImage(frame_copy);
    } else {
        if (gst_app_sink_is_eos(GST_APP_SINK(appsink))) {
             std::cout << "[VideoReceiver] AppSink is EOS" << std::endl;
        }
        if (lastFrameAtMs < 0) {
            QLabel *statusLabel = this->playerWidget->findChild<QLabel *>("statusLabel");
            if (statusLabel && !statusLabel->isVisible()) {
                statusLabel->setVisible(true);
            }
        }
        // std::cout << "[VideoReceiver] No sample available" << std::endl;
    }
}

bool VideoReceiver::onNetCombTransferBuffer(DWORD srcTID,
                                            const std::shared_ptr<BYTE> &pBuffer,
                                            DWORD dwLength,
                                            bool bMsg,
                                            int64_t) {
    if (bMsg) {
        return true;
    }
    if (!appsrc || !pBuffer || dwLength < 2) {
        return true;
    }
    m_rxRawPktsWindow += 1;
    if (m_expectedRemoteTID != 0 && srcTID != m_expectedRemoteTID) {
        return true;
    }

    auto process_typed_rtp = [&](DWORD remoteTID, const BYTE *typed, DWORD typed_len) -> bool {
        if (!typed || typed_len < 2) {
            return true;
        }
        if (typed[0] != 0x00) {
            return true;
        }
        DWORD offset = 1;
        uint64_t videoCaptureTsUs = 0;
        if (typed_len >= (1 + sizeof(uint64_t) + 12) && ((typed[1 + sizeof(uint64_t)] & 0xC0) == 0x80)) {
            uint64_t tsBe = 0;
            std::memcpy(&tsBe, typed + 1, sizeof(uint64_t));
            videoCaptureTsUs = from_be_u64_video(tsBe);
            offset = 1 + sizeof(uint64_t);
        }
        const DWORD rtp_len = typed_len - offset;
        const BYTE *rtp = typed + offset;
        if (rtp_len >= 12) {
            updateJitterStats();
            maybeAdjustJitterLatency();
        }

        m_rxRtpPktsWindow += 1;
        m_rxRtpBytesWindow += rtp_len;

        if (videoCaptureTsUs > 0 && m_audioReceiver) {
            const uint64_t audioCaptureTsUs = m_audioReceiver->latestCaptureTimestampUs();
            if (audioCaptureTsUs > 0) {
                const int64_t avDiffMs = static_cast<int64_t>(videoCaptureTsUs / 1000ULL) - static_cast<int64_t>(audioCaptureTsUs / 1000ULL);
                if (avDiffMs > 60) {
                    const int64_t waitMs = std::min<int64_t>(avDiffMs - 40, 30);
                    if (waitMs > 0) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(waitMs));
                    }
                } else if (avDiffMs < -150) {
                    return true;
                }
            }
        }

        GstBuffer *gstbuf = gst_buffer_new_allocate(nullptr, rtp_len, nullptr);
        if (!gstbuf) {
            return true;
        }
        gst_buffer_fill(gstbuf, 0, rtp, rtp_len);
        if (videoCaptureTsUs > 0) {
            GST_BUFFER_PTS(gstbuf) = videoCaptureTsUs * 1000;
        }
        gst_app_src_push_buffer(GST_APP_SRC(appsrc), gstbuf);

        const qint64 elapsedMs = m_rxStatClock.elapsed();
        if (elapsedMs >= 1000) {
            const uint64_t bps = (m_rxRtpBytesWindow * 8ULL * 1000ULL) / static_cast<uint64_t>(elapsedMs);
            const uint64_t pps = (m_rxRtpPktsWindow * 1000ULL) / static_cast<uint64_t>(elapsedMs);
            std::cout << "[VideoReceiver] TID=" << getTID()
                      << " from=" << remoteTID
                      << " raw_pkts=" << m_rxRawPktsWindow
                      << " rtp_pps=" << pps
                      << " rtp_kbps=" << (bps / 1000ULL)
                      << std::endl;
            m_rxRawPktsWindow = 0;
            m_rxRtpPktsWindow = 0;
            m_rxRtpBytesWindow = 0;
            m_rxStatClock.restart();
        }
        return true;
    };

    const BYTE *raw = pBuffer.get();
        if (dwLength >= 21) {
        const DWORD data_len = CIMsg::ReadData(const_cast<BYTE *>(raw + 12));
        if (data_len >= 2 && data_len + 20 == dwLength) {
            const BYTE *typed = raw + 20;
                if (typed[0] == 0x00 && (((typed[1] & 0xC0) == 0x80) || (data_len >= 1 + sizeof(uint64_t) + 12 && ((typed[1 + sizeof(uint64_t)] & 0xC0) == 0x80)))) {
                return process_typed_rtp(srcTID, typed, data_len);
            }
        }
    }

    if (dwLength >= 2 && raw[0] == 0x00 &&
        (((raw[1] & 0xC0) == 0x80) || (dwLength >= 1 + sizeof(uint64_t) + 12 && ((raw[1 + sizeof(uint64_t)] & 0xC0) == 0x80)))) {
        return process_typed_rtp(srcTID, raw, dwLength);
    }
    return true;
}

bool VideoReceiver::onAudioRtpData(DWORD srcTID,
                                   const std::shared_ptr<BYTE> &pBuffer,
                                   DWORD dwLength,
                                   long int &recvtime,
                                   long int &fb_send_time) {
    if (!m_audioReceiver) {
        return true;
    }
    return m_audioReceiver->onAudioRtpData(srcTID, pBuffer, dwLength, recvtime, fb_send_time);
}

bool VideoReceiver::onSpeedControlRecvData(DWORD srcTID,
                                           const std::shared_ptr<BYTE> &pBuffer,
                                           DWORD dwLength,
                                           long int &,
                                           long int &) {
    if (!appsrc) {
        std::cout << "[DEBUG] onSpeedControlRecvData: appsrc is NULL!" << std::endl;
        return true;
    }
    if (!pBuffer || dwLength < 2) {
        std::cout << "[DEBUG] onSpeedControlRecvData: Invalid buffer. Len=" << dwLength << std::endl;
        return true;
    }
    m_rxRawPktsWindow += 1;
    if (m_expectedRemoteTID != 0 && srcTID != m_expectedRemoteTID) {
        std::cout << "[DEBUG] TID mismatch. Exp=" << m_expectedRemoteTID << " Act=" << srcTID << std::endl;
        return true;
    }

        // std::cout << "[DEBUG] Not Video Data. Type=" << (int)typed[0] << std::endl;
    const BYTE *typed = pBuffer.get();
    if (typed[0] != 0x00) {
        return true;
    }
    DWORD offset = 1;
    uint64_t videoCaptureTsUs = 0;
    if (dwLength >= (1 + sizeof(uint64_t) + 12) && ((typed[1 + sizeof(uint64_t)] & 0xC0) == 0x80)) {
        uint64_t tsBe = 0;
        std::memcpy(&tsBe, typed + 1, sizeof(uint64_t));
        videoCaptureTsUs = from_be_u64_video(tsBe);
        offset = 1 + sizeof(uint64_t);
    }
    const DWORD rtp_len = dwLength - offset;
    const BYTE *rtp = typed + offset;
    if (rtp_len < 12 || ((rtp[0] & 0xC0) != 0x80)) {
        return true;
    }

    updateJitterStats();
    maybeAdjustJitterLatency();

    m_rxRtpPktsWindow += 1;
    m_rxRtpBytesWindow += rtp_len;

    if (videoCaptureTsUs > 0 && m_audioReceiver) {
        const uint64_t audioCaptureTsUs = m_audioReceiver->latestCaptureTimestampUs();
        if (audioCaptureTsUs > 0) {
            const int64_t avDiffMs = static_cast<int64_t>(videoCaptureTsUs / 1000ULL) - static_cast<int64_t>(audioCaptureTsUs / 1000ULL);
            if (avDiffMs > 60) {
                const int64_t waitMs = std::min<int64_t>(avDiffMs - 40, 30);
                if (waitMs > 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(waitMs));
                }
            } else if (avDiffMs < -150) {
                return true;
            }
        }
    }

    GstBuffer *gstbuf = gst_buffer_new_allocate(nullptr, rtp_len, nullptr);
    if (!gstbuf) {
        return true;
    }
    gst_buffer_fill(gstbuf, 0, rtp, rtp_len);
    if (videoCaptureTsUs > 0) {
        GST_BUFFER_PTS(gstbuf) = videoCaptureTsUs * 1000;
    }
    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc), gstbuf);
    if (ret != GST_FLOW_OK) {
        std::cerr << "[VideoReceiver] appsrc push failed: " << ret << std::endl;
    }

    const qint64 elapsedMs = m_rxStatClock.elapsed();
    if (elapsedMs >= 1000) {
        const uint64_t bps = (m_rxRtpBytesWindow * 8ULL * 1000ULL) / static_cast<uint64_t>(elapsedMs);
        const uint64_t pps = (m_rxRtpPktsWindow * 1000ULL) / static_cast<uint64_t>(elapsedMs);
        std::cout << "[VideoReceiver] TID=" << getTID()
                  << " from=" << srcTID
                  << " raw_pkts=" << m_rxRawPktsWindow
                  << " rtp_pps=" << pps
                  << " rtp_kbps=" << (bps / 1000ULL)
                  << " appsrc=" << (void*)appsrc
                  << std::endl;
        m_rxRawPktsWindow = 0;
        m_rxRtpPktsWindow = 0;
        m_rxRtpBytesWindow = 0;
        m_rxStatClock.restart();
    }
    return true;
}

void VideoReceiver::updateJitterStats() {
    const auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(m_jitterMutex);
    if (!m_hasArrival) {
        m_hasArrival = true;
        m_lastArrival = now;
        return;
    }
    const double delta_ms = std::chrono::duration<double, std::milli>(now - m_lastArrival).count();
    m_lastArrival = now;

    const double alpha = 0.10;
    if (m_interArrivalAvgMs <= 0.0) {
        m_interArrivalAvgMs = delta_ms;
    } else {
        m_interArrivalAvgMs += alpha * (delta_ms - m_interArrivalAvgMs);
    }

    const double dev = std::abs(delta_ms - m_interArrivalAvgMs);
    const double beta = 0.20;
    if (m_jitterEmaMs <= 0.0) {
        m_jitterEmaMs = dev;
    } else {
        m_jitterEmaMs += beta * (dev - m_jitterEmaMs);
    }
}

void VideoReceiver::maybeAdjustJitterLatency() {
    if (!jitterbuffer) {
        return;
    }
    if (!m_jitterAdjustClock.isValid()) {
        m_jitterAdjustClock.start();
    }
    if (m_jitterAdjustClock.elapsed() < 1000) {
        return;
    }

    double jitter_ms = 0.0;
    {
        std::lock_guard<std::mutex> lock(m_jitterMutex);
        jitter_ms = m_jitterEmaMs;
    }

    int target_ms = static_cast<int>(std::lround(50.0 + jitter_ms * 3.0));
    if (target_ms < 50) target_ms = 50;
    if (target_ms > 300) target_ms = 300;

    if (std::abs(target_ms - m_latencyCurrentMs) >= 10) {
        g_object_set(G_OBJECT(jitterbuffer), "latency", target_ms, nullptr);
        m_latencyCurrentMs = target_ms;
    }
    m_jitterAdjustClock.restart();
}

// Legacy methods kept to satisfy interface but emptied or redirected

void VideoReceiver::setData(int videoPort) {
    // If port changes dynamically, we can restart pipeline here
    if (this->videoListenPort != videoPort) {
        this->videoListenPort = videoPort;
        startGstPipeline(videoPort);
    }
}

int VideoReceiver::getVideoPort() {
    return videoListenPort;
}

void VideoReceiver::onRecvVideoData(QByteArray data,unsigned int dataLength,unsigned int packetIndex) {
    // Disabled for GST path as we use udpsrc
}
