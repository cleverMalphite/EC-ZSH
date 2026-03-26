#ifndef AUDIORECEIVERCORE_H
#define AUDIORECEIVERCORE_H

#include <QObject>
#include <QElapsedTimer>
#include <QtGlobal>
#include <atomic>
#include <memory>

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

    GstElement *pipeline = nullptr;
    GstElement *appsrc = nullptr;
    GstElement *appsink = nullptr;

    std::atomic<bool> m_running{false};
    std::atomic<uint64_t> m_decodedFrames{0};
    std::atomic<uint64_t> m_latestCaptureTsUs{0};

    bool m_useAppSink = false;
};

#endif
