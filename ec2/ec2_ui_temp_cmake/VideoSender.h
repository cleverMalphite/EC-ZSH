#ifndef VIDEOSENDER_H
#define VIDEOSENDER_H

#include <iostream>
#include <string>
#include <QWidget>
#include <QTimer>
#include <QElapsedTimer>
#include <atomic>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui_c.h>

#include "playerwidget.h"

// GStreamer
#include <gst/gst.h>
#include <gst/app/gstappsink.h>

#include <QResizeEvent>
#include <stdio.h>
#include <list>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include "function.h"
#include "../NetCombTransfer/NetCombTransferApi.h" // Add NetCombTransfer API
#include "../SpeedControl/SpeedControlApi.h"
#include "AudioSenderCore.h"

using namespace cv;

class VideoSender : public QWidget
{
    Q_OBJECT

public:
    VideoSender(QWidget *parent = nullptr);
    void ShowImage(cv::Mat& mat);
    ~VideoSender();

    void init();

    // GStreamer pipeline control
    void startGstPipeline(const std::string& host, int port); // host param ignored in new logic
    void startDeepIntegrationPipeline(int destTID); // New start method
    void stopGstPipeline();

    PlayerWidget *playerWidget = nullptr;

    // Internal helper to send RTP data via SpeedControl
    void handleRtpData(guint8* data, gsize size);
    bool onSpeedControlRecvData(DWORD srcTID,
                                const std::shared_ptr<BYTE> &pBuffer,
                                DWORD dwLength,
                                long int &recvtime,
                                long int &fb_send_time);

private slots:
    void on_pushButton_play_clicked();

    void on_pushButton_pause_clicked();

    void on_pushButton_return_clicked();

    // Timer slot to pull frames from appsink (local preview) - Kept for compatibility or legacy
    void on_timer_check_appsink();
    void on_adaptation_tick();

private:
    enum class AdaptiveProfileLevel {
        Normal,
        LowBandwidth,
        Extreme
    };

    struct StreamProfile {
        int width = 640;
        int height = 480;
        int fps = 30;
        uint32_t defaultBitrateKbps = 1200;
        uint32_t minBitrateKbps = 900;
        uint32_t maxBitrateKbps = 1600;
    };

    // Static callback for appsink new-sample signal
    static GstFlowReturn on_new_sample_from_sink(GstElement *sink, VideoSender *sender);

    StreamProfile profileForLevel(AdaptiveProfileLevel level) const;
    bool startAdaptivePipelineInternal();
    void restartAdaptivePipeline();
    void updateAdaptiveController();
    AdaptiveProfileLevel decideProfile(double effectiveBandwidthKbps,
                                       double smoothedRttMs,
                                       double rttTrendMs,
                                       DWORD queueDepth) const;
    void applyEncoderBitrate(uint32_t targetKbps);
    void updateStatusLabel(const QString &text, bool forceVisible);

    // GStreamer members
    GstElement *pipeline = nullptr;
    GstElement *appsink_preview = nullptr;
    GstElement *appsink_rtp = nullptr;
    GstElement *encoder = nullptr;
    QTimer *timer_appsink = nullptr;
    QTimer *timer_adaptation = nullptr;

    int fps;
    bool is_play;

    cv::Mat _imageTemp;

    bool has_camera_init=false;

    QElapsedTimer frameClock;
    qint64 lastFrameAtMs;
    int showWaitingAfterMs;

    // Destination TID for NetCombTransfer
    int m_destTID = 0;
    uint32_t m_targetBitrateKbps = 0;
    uint32_t m_lastAppliedSpeedMbps = 0;
    AudioSenderCore *m_audioSender = nullptr;

    QElapsedTimer m_sendStatClock;
    uint64_t m_sendPktsWindow = 0;
    uint64_t m_sendBytesWindow = 0;
    uint64_t m_sendFailWindow = 0;

    AdaptiveProfileLevel m_currentProfile = AdaptiveProfileLevel::Normal;
    AdaptiveProfileLevel m_appliedProfile = AdaptiveProfileLevel::Normal;
    double m_smoothedBandwidthKbps = 0.0;
    double m_smoothedRttMs = 0.0;
    double m_lastRawRttMs = 0.0;
    DWORD m_lastQueueDepth = 0;
    DWORD m_lastQueueCapacity = 0;
    qint64 m_lastProfileSwitchMs = 0;
    qint64 m_lastAdaptiveLogMs = 0;

    uint32_t m_normalMinKbps = 1000;
    uint32_t m_lowMinKbps = 500;
    DWORD m_queueLowWatermark = 40;
    DWORD m_queueHighWatermark = 120;
    DWORD m_queueCriticalWatermark = 240;
    int m_profileSwitchCooldownMs = 1500;
    int m_adaptationPollIntervalMs = 300;

    bool m_useCameraSource = false;
    std::string m_videoFilePath;

    std::atomic<bool> m_isStopping{false};
};
#endif // VideoSender_H
