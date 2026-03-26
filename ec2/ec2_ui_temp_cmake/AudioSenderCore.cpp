#include "AudioSenderCore.h"

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <arpa/inet.h>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>
#include <QUrl>

static bool gst_initialized_audio_sender = false;
static void ensure_gst_init_audio_sender() {
    if (!gst_initialized_audio_sender) {
        gst_init(nullptr, nullptr);
        gst_initialized_audio_sender = true;
    }
}

static std::string resolve_default_audio_file_path() {
    const QStringList candidates = {
        QDir::cleanPath(QDir::currentPath() + "/FileSend/AVtest.mp4"),
        QDir::cleanPath(QDir::currentPath() + "/../FileSend/AVtest.mp4"),
        QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../FileSend/AVtest.mp4"),
        QString("/home/itzhou/EC1/EC2_12.8/FileSend/AVtest.mp4"),
    };

    for (const auto& path : candidates) {
        if (QFileInfo::exists(path)) {
            return path.toStdString();
        }
    }
    return "FileSend/AVtest.mp4";
}

static std::string to_file_uri_audio(const std::string& path) {
    return QUrl::fromLocalFile(QString::fromStdString(path)).toString().toStdString();
}

static uint64_t to_be_u64_audio(uint64_t v) {
    const uint64_t hi = htonl(static_cast<uint32_t>((v >> 32) & 0xFFFFFFFFULL));
    const uint64_t lo = htonl(static_cast<uint32_t>(v & 0xFFFFFFFFULL));
    return (lo << 32) | hi;
}

AudioSenderCore::AudioSenderCore(QObject *parent)
    : QObject(parent)
{
    ensure_gst_init_audio_sender();
}

AudioSenderCore::~AudioSenderCore()
{
    stop();
}

bool AudioSenderCore::start(int destTID)
{
    if (m_running.load()) {
        stop();
    }
    m_destTID = destTID;
    if (m_destTID <= 0) {
        return false;
    }

    const QByteArray srcMode = qgetenv("EC2_AUDIO_SOURCE");
    std::string src;
    if (srcMode == "mic" || srcMode == "microphone") {
        src = "autoaudiosrc";
    } else if (srcMode == "test") {
        src = "audiotestsrc is-live=true wave=sine freq=440";
    } else {
        // Default to extract audio from the same test MP4 file used by VideoSender
        const std::string file_path = resolve_default_audio_file_path();
        src = "uridecodebin uri=" + to_file_uri_audio(file_path);
    }

    // Notice sync=true here. When reading from a file, sync=true forces GStreamer
    // to output audio frames at the correct real-time pace.
    const std::string sink =
        "appsink name=rtp_sink emit-signals=true sync=true max-buffers=200 drop=true";

    const std::string pipeline_str =
        src + " ! audioconvert ! audioresample ! "
              "opusenc name=enc bitrate=64000 inband-fec=true packet-loss-percentage=10 ! "
              "rtpopuspay pt=97 mtu=1200 ! " +
        sink;

    std::cout << "[AudioSender] Starting pipeline: " << pipeline_str << std::endl;

    GError *error = nullptr;
    pipeline = gst_parse_launch(pipeline_str.c_str(), &error);
    if (error) {
        std::cerr << "[AudioSender] pipeline error: " << error->message << std::endl;
        g_error_free(error);
        pipeline = nullptr;
        return false;
    }

    encoder = gst_bin_get_by_name(GST_BIN(pipeline), "enc");
    appsink_rtp = gst_bin_get_by_name(GST_BIN(pipeline), "rtp_sink");
    if (!appsink_rtp) {
        stop();
        return false;
    }

    g_signal_connect(appsink_rtp, "new-sample", G_CALLBACK(on_new_sample_from_sink), this);
    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        stop();
        return false;
    }

    m_sendPktsWindow = 0;
    m_sendBytesWindow = 0;
    m_sendFailWindow = 0;
    m_sendStatClock.restart();
    m_running.store(true);
    return true;
}

void AudioSenderCore::stop()
{
    m_running.store(false);
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        pipeline = nullptr;
    }
    if (appsink_rtp) {
        gst_object_unref(appsink_rtp);
        appsink_rtp = nullptr;
    }
    if (encoder) {
        gst_object_unref(encoder);
        encoder = nullptr;
    }
}

GstFlowReturn AudioSenderCore::on_new_sample_from_sink(GstElement *sink, AudioSenderCore *self)
{
    if (!self || !self->m_running.load()) {
        return GST_FLOW_OK;
    }

    GstSample *sample = nullptr;
    g_signal_emit_by_name(sink, "pull-sample", &sample);
    if (!sample) {
        return GST_FLOW_OK;
    }
    GstBuffer *buffer = gst_sample_get_buffer(sample);
    if (!buffer) {
        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }

    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }

    self->handleRtpData(map.data, map.size);
    gst_buffer_unmap(buffer, &map);
    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

void AudioSenderCore::handleRtpData(const guint8 *data, gsize size)
{
    if (!data || size < 12) {
        return;
    }

    const uint64_t captureTsUs = gst_util_get_timestamp() / 1000ULL;
    const uint64_t tsBe = to_be_u64_audio(captureTsUs);
    const DWORD totalLen = static_cast<DWORD>(size + 1 + sizeof(uint64_t));
    std::shared_ptr<BYTE> sendBuf(new BYTE[totalLen], std::default_delete<BYTE[]>());
    BYTE *p = sendBuf.get();
    p[0] = 0x02;
    std::memcpy(p + 1, &tsBe, sizeof(uint64_t));
    std::memcpy(p + 1 + sizeof(uint64_t), data, size);
    const bool ok = SendTIDDataWithSpeedControl(static_cast<DWORD>(m_destTID), sendBuf, totalLen, false, false, false);
    if (ok) {
        m_sendPktsWindow += 1;
        m_sendBytesWindow += static_cast<uint64_t>(totalLen);
    } else {
        m_sendFailWindow += 1;
    }

    const qint64 elapsedMs = m_sendStatClock.elapsed();
    if (elapsedMs >= 1000) {
        const uint64_t bps = (m_sendBytesWindow * 8ULL * 1000ULL) / static_cast<uint64_t>(elapsedMs);
        const uint64_t pps = (m_sendPktsWindow * 1000ULL) / static_cast<uint64_t>(elapsedMs);
        std::cout << "[AudioSender] to=" << m_destTID
                  << " pps=" << pps
                  << " kbps=" << (bps / 1000ULL)
                  << " fail=" << m_sendFailWindow
                  << std::endl;
        m_sendPktsWindow = 0;
        m_sendBytesWindow = 0;
        m_sendFailWindow = 0;
        m_sendStatClock.restart();
    }
}
