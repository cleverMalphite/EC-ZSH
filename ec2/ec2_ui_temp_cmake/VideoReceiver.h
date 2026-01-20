#ifndef VIDEORECEIVER_H
#define VIDEORECEIVER_H

#include <QDateTime>
#include <QElapsedTimer>

#include <QWidget>
#include <QTimer>
#include <future>
#include <thread>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui_c.h>

#include "playerwidget.h"

// GStreamer
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>

#include "../NetCombTransfer/NetCombTransferApi.h"

//udp
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
    VideoReceiver(QWidget *parent = nullptr);
    void ShowImage(cv::Mat& mat);
    
    ~VideoReceiver();

    void init();
    
    // GStreamer pipeline control
    void startGstPipeline(int port);
    void startDeepIntegrationPipeline();
    void stopGstPipeline();

    bool onSpeedControlRecvData(DWORD srcTID,
                                const std::shared_ptr<BYTE> &pBuffer,
                                DWORD dwLength,
                                long int &recvtime,
                                long int &fb_send_time);

    bool onNetCombTransferBuffer(DWORD srcTID,
                                 const std::shared_ptr<BYTE> &pBuffer,
                                 DWORD dwLength,
                                 bool bMsg,
                                 int64_t packet_recv_timeStamp);

    // Legacy or MRUDP-related methods kept but unused for GST path
    void setData(int videoPort);
    void endRecvVideo();
    void startRecvVideo();
    void reset();
    int getVideoPort();

public slots:
    // Legacy slot for MRUDP data
    void onRecvVideoData(QByteArray data,unsigned int dataLength,unsigned int packetIndex);

signals:
    void startSendVideo(QString remoteAddr,QString videoPort,QString commandPort,QString remoteTID);
    void sendEndCommand();
    void endVideoTrans(unsigned int TID_closeVideoTrans);

private slots:
    void on_pushButton_return_clicked();
    
    // Timer slot to pull frames from appsink
    void on_timer_check_appsink();
    void on_timer_send_feedback();

private:
    bool exit_flag;

    // GStreamer members
    GstElement *pipeline = nullptr;
    GstElement *appsink = nullptr;
    GstElement *appsrc = nullptr;
    QTimer *timer_appsink = nullptr;
    QTimer *timer_feedback = nullptr;

    int fps;

    bool is_play = false;

    //houlc playerWidget
    PlayerWidget *playerWidget;

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
    DWORD m_lastRemoteTID = 0;
    bool m_hasSeq = false;
    uint16_t m_lastSeq = 0;
    uint64_t m_recvBytesWindow = 0;
    uint32_t m_recvPktsWindow = 0;
    uint32_t m_lostPktsWindow = 0;
    QElapsedTimer m_feedbackClock;

    QElapsedTimer frameClock;
    qint64 lastFrameAtMs;
    int showWaitingAfterMs;

    QElapsedTimer m_rxStatClock;
    uint64_t m_rxRtpPktsWindow = 0;
    uint64_t m_rxRtpBytesWindow = 0;
    uint64_t m_rxRawPktsWindow = 0;
};
#endif // VIDEORECEIVER_H
#ifndef VIDEORECEIVER_H
#define VIDEORECEIVER_H

#include <QDateTime>
#include <QElapsedTimer>

#include <QWidget>
#include <QTimer>
#include <future>
#include <thread>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui_c.h>

#include "playerwidget.h"

// GStreamer
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>

#include "../NetCombTransfer/NetCombTransferApi.h"

//udp
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
    VideoReceiver(QWidget *parent = nullptr);
    void ShowImage(cv::Mat& mat);
    
    ~VideoReceiver();

    void init();
    
    // GStreamer pipeline control
    void startGstPipeline(int port);
    void startDeepIntegrationPipeline();
    void stopGstPipeline();

    bool onNetCombTransferBuffer(DWORD srcTID,
                                 const std::shared_ptr<BYTE> &pBuffer,
                                 DWORD dwLength,
                                 bool bMsg,
                                 int64_t packet_recv_timeStamp);

    // Legacy or MRUDP-related methods kept but unused for GST path
    void setData(int videoPort);
    void endRecvVideo();
    void startRecvVideo();
    void reset();
    int getVideoPort();

public slots:
    // Legacy slot for MRUDP data
    void onRecvVideoData(QByteArray data,unsigned int dataLength,unsigned int packetIndex);

signals:
    void startSendVideo(QString remoteAddr,QString videoPort,QString commandPort,QString remoteTID);
    void sendEndCommand();
    void endVideoTrans(unsigned int TID_closeVideoTrans);

private slots:
    void on_pushButton_return_clicked();
    
    // Timer slot to pull frames from appsink
    void on_timer_check_appsink();
    void on_timer_send_feedback();

private:
    bool exit_flag;

    // GStreamer members
    GstElement *pipeline = nullptr;
    GstElement *appsink = nullptr;
    GstElement *appsrc = nullptr;
    QTimer *timer_appsink = nullptr;
    QTimer *timer_feedback = nullptr;

    int fps;

    bool is_play = false;

    //houlc playerWidget
    PlayerWidget *playerWidget;

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
    DWORD m_lastRemoteTID = 0;
    bool m_hasSeq = false;
    uint16_t m_lastSeq = 0;
    uint64_t m_recvBytesWindow = 0;
    uint32_t m_recvPktsWindow = 0;
    uint32_t m_lostPktsWindow = 0;
    QElapsedTimer m_feedbackClock;

    QElapsedTimer frameClock;
    qint64 lastFrameAtMs;
    int showWaitingAfterMs;

    QElapsedTimer m_rxStatClock;
    uint64_t m_rxRtpPktsWindow = 0;
    uint64_t m_rxRtpBytesWindow = 0;
    uint64_t m_rxRawPktsWindow = 0;
};
#endif // VIDEORECEIVER_H
