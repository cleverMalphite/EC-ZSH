#ifndef AUDIOSENDERCORE_H
#define AUDIOSENDERCORE_H

#include <QObject>
#include <QElapsedTimer>
#include <QtGlobal>
#include <memory>
#include <atomic>

#include <gst/gst.h>
#include <gst/app/gstappsink.h>

#include "../SpeedControl/SpeedControlApi.h"

class AudioSenderCore : public QObject
{
    Q_OBJECT

public:
    explicit AudioSenderCore(QObject *parent = nullptr);
    ~AudioSenderCore();

    bool start(int destTID);
    void stop();

private:
    static GstFlowReturn on_new_sample_from_sink(GstElement *sink, AudioSenderCore *self);
    void handleRtpData(const guint8 *data, gsize size);

    GstElement *pipeline = nullptr;
    GstElement *appsink_rtp = nullptr;
    GstElement *encoder = nullptr;

    int m_destTID = 0;
    std::atomic<bool> m_running{false};

    QElapsedTimer m_sendStatClock;
    uint64_t m_sendPktsWindow = 0;
    uint64_t m_sendBytesWindow = 0;
    uint64_t m_sendFailWindow = 0;
};

#endif
