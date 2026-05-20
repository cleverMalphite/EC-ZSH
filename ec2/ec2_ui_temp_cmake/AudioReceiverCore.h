#ifndef AUDIORECEIVERCORE_H
#define AUDIORECEIVERCORE_H

#include <QObject>
#include <QElapsedTimer>
#include <QtGlobal>
#include <atomic>
#include <memory>
#include <mutex>
#include <chrono>

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>

#include "../mGlobalDef.h"

class AudioReceiverCore : public QObject
{
    Q_OBJECT

public:
    explicit AudioReceiverCore(QObject *parent = nullptr);
    ~AudioReceiverCore();

    bool start();
    void stop();

    bool onAudioRtpData(DWORD srcTID,
                        const std::shared_ptr<BYTE> &pBuffer,
                        DWORD dwLength,
                        long int &recvtime,
                        long int &fb_send_time);

    uint64_t decodedFrameCount() const;
    uint64_t latestCaptureTimestampUs() const;

private:
    static GstFlowReturn on_new_sample_from_sink(GstElement *sink, AudioReceiverCore *self);
    bool pushRtpToAppSrc(const BYTE *rtp, DWORD rtp_len);
    void updateJitterStats();
    void maybeAdjustJitterLatency();

    GstElement *pipeline = nullptr;
    GstElement *appsrc = nullptr;
    GstElement *appsink = nullptr;
    GstElement *jitterbuffer = nullptr;

    std::atomic<bool> m_running{false};
    std::atomic<uint64_t> m_decodedFrames{0};
    std::atomic<uint64_t> m_latestCaptureTsUs{0};

    bool m_useAppSink = false;

    std::mutex m_jitterMutex;
    bool m_hasArrival = false;
    std::chrono::steady_clock::time_point m_lastArrival;
    double m_interArrivalAvgMs = 0.0;
    double m_jitterEmaMs = 0.0;
    int m_latencyCurrentMs = 100;
    QElapsedTimer m_jitterAdjustClock;
};

#endif
