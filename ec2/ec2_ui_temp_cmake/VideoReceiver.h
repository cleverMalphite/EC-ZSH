#ifndef VIDEORECEIVER_H
#define VIDEORECEIVER_H

#include <QDateTime>
#include <QElapsedTimer>
#include <QWidget>
#include <QTimer>
#include <future>
#include <thread>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui_c.h>
#include "playerwidget.h"
#include "AudioReceiverCore.h"
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include "../NetCombTransfer/NetCombTransferApi.h"
#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include "function.h"
#include <iostream>
#include <string>
#include <list>
#include <vector>
#include <thread>
#include <utility>
#include <mutex>

class VideoReceiver : public QWidget
{
    Q_OBJECT

public:
    VideoReceiver(QWidget *parent = nullptr, bool forceReceive = false);
    void ShowImage(cv::Mat& mat);
    ~VideoReceiver();
    void init();
    void startGstPipeline(int port);
    void startDeepIntegrationPipeline();
    void stopGstPipeline();
    bool onSpeedControlRecvData(DWORD srcTID,
                                const std::shared_ptr<BYTE> &pBuffer,
                                DWORD dwLength,
                                long int &recvtime,
                                long int &fb_send_time);
    bool onAudioRtpData(DWORD srcTID,
                        const std::shared_ptr<BYTE> &pBuffer,
                        DWORD dwLength,
                        long int &recvtime,
                        long int &fb_send_time);
    bool onNetCombTransferBuffer(DWORD srcTID,
                                 const std::shared_ptr<BYTE> &pBuffer,
                                 DWORD dwLength,
                                 bool bMsg,
                                 int64_t packet_recv_timeStamp);
    void setData(int videoPort);
    void endRecvVideo();
    void startRecvVideo();
    void reset();
    int getVideoPort();

    PlayerWidget *playerWidget = nullptr;

public slots:
    void onRecvVideoData(QByteArray data,unsigned int dataLength,unsigned int packetIndex);

signals:
    void startSendVideo(QString remoteAddr,QString videoPort,QString commandPort,QString remoteTID);
    void sendEndCommand();
    void endVideoTrans(unsigned int TID_closeVideoTrans);

private slots:
    void on_pushButton_return_clicked();
    void on_timer_check_appsink();

private:
    void updateJitterStats();
    void maybeAdjustJitterLatency();
    bool exit_flag;

    // GStreamer members
    GstElement *pipeline = nullptr;
    GstElement *appsink = nullptr;
    GstElement *appsrc = nullptr;
    GstElement *jitterbuffer = nullptr;
    QTimer *timer_appsink = nullptr;

    int fps;

    bool is_play = false;

    //houlc data rate statistic
    QTimer *timer_test;
    bool sta_first_get;
    QElapsedTimer sta_timer;
    qint64 sta_time_pass;
    qint64 sta_data_sum;
    qint64 sta_time_start = 0;
    double sta_data_rate;
    double sta_pic_sum;

    int videoListenPort;
    DWORD m_expectedRemoteTID = 0;

    std::mutex m_jitterMutex;
    bool m_hasArrival = false;
    std::chrono::steady_clock::time_point m_lastArrival;
    double m_interArrivalAvgMs = 0.0;
    double m_jitterEmaMs = 0.0;
    int m_latencyCurrentMs = 200;
    QElapsedTimer m_jitterAdjustClock;

    QElapsedTimer frameClock;
    qint64 lastFrameAtMs;
    int showWaitingAfterMs;

    QElapsedTimer m_rxStatClock;
    uint64_t m_rxRtpPktsWindow = 0;
    uint64_t m_rxRtpBytesWindow = 0;
    uint64_t m_rxRawPktsWindow = 0;
    AudioReceiverCore *m_audioReceiver = nullptr;
};

#endif // VIDEORECEIVER_H
