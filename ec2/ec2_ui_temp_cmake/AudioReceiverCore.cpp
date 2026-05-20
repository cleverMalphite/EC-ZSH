#include "AudioReceiverCore.h"

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <arpa/inet.h>
#include <cmath>

static bool gst_initialized_audio_receiver = false;
static void ensure_gst_init_audio_receiver() {
    if (!gst_initialized_audio_receiver) {
        gst_init(nullptr, nullptr);
        gst_initialized_audio_receiver = true;
    }
}

static uint64_t from_be_u64_audio(uint64_t v) {
    const uint32_t hi = ntohl(static_cast<uint32_t>(v & 0xFFFFFFFFULL));
    const uint32_t lo = ntohl(static_cast<uint32_t>((v >> 32) & 0xFFFFFFFFULL));
    return (static_cast<uint64_t>(hi) << 32) | static_cast<uint64_t>(lo);
}

AudioReceiverCore::AudioReceiverCore(QObject *parent)
    : QObject(parent)
{
    ensure_gst_init_audio_receiver();
}

AudioReceiverCore::~AudioReceiverCore()
{
    stop();
}

bool AudioReceiverCore::start()
{
    if (m_running.load()) {
        stop();
    }

    m_latencyCurrentMs = 100;

    const QByteArray sinkMode = qgetenv("EC2_AUDIO_SINK");
    m_useAppSink = (sinkMode == "appsink");

    const std::string sink =
        m_useAppSink ? "appsink name=audio_sink emit-signals=true drop=true max-buffers=50 sync=false"
                     : (sinkMode == "fake" ? "fakesink sync=false" : "autoaudiosink sync=false");

    const std::string pipeline_str =
        "appsrc name=rtp_src is-live=true do-timestamp=true format=time block=false "
        "caps=\"application/x-rtp,media=audio,encoding-name=OPUS,payload=97,clock-rate=48000\" ! "
        "rtpjitterbuffer name=audio_jitter latency=100 drop-on-latency=true do-lost=true mode=slave ! "
        "rtpopusdepay ! opusdec ! audioconvert ! audioresample ! " +
        sink;

    GError *error = nullptr;
    pipeline = gst_parse_launch(pipeline_str.c_str(), &error);
    if (error) {
        std::cerr << "[AudioReceiver] pipeline error: " << error->message << std::endl;
        g_error_free(error);
        pipeline = nullptr;
        return false;
    }

    appsrc = gst_bin_get_by_name(GST_BIN(pipeline), "rtp_src");
    if (!appsrc) {
        stop();
        return false;
    }
    jitterbuffer = gst_bin_get_by_name(GST_BIN(pipeline), "audio_jitter");
    if (jitterbuffer) {
        g_object_set(G_OBJECT(jitterbuffer), "latency", m_latencyCurrentMs, nullptr);
    }
    if (m_useAppSink) {
        appsink = gst_bin_get_by_name(GST_BIN(pipeline), "audio_sink");
        if (!appsink) {
            stop();
            return false;
        }
        g_signal_connect(appsink, "new-sample", G_CALLBACK(on_new_sample_from_sink), this);
    }

    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        stop();
        return false;
    }

    m_decodedFrames.store(0);
    m_running.store(true);
    m_hasArrival = false;
    m_interArrivalAvgMs = 0.0;
    m_jitterEmaMs = 0.0;
    m_jitterAdjustClock.restart();
    return true;
}

void AudioReceiverCore::stop()
{
    m_running.store(false);
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
}

bool AudioReceiverCore::onAudioRtpData(DWORD,
                                      const std::shared_ptr<BYTE> &pBuffer,
                                      DWORD dwLength,
                                      long int &,
                                      long int &)
{
    if (!m_running.load() || !appsrc) {
        return true;
    }
    if (!pBuffer || dwLength < 2) {
        return true;
    }
    const BYTE *typed = pBuffer.get();
    if (typed[0] != 0x02) {
        return true;
    }
    DWORD offset = 1;
    if (dwLength >= (1 + sizeof(uint64_t) + 12) && ((typed[1 + sizeof(uint64_t)] & 0xC0) == 0x80)) {
        uint64_t tsBe = 0;
        std::memcpy(&tsBe, typed + 1, sizeof(uint64_t));
        m_latestCaptureTsUs.store(from_be_u64_audio(tsBe));
        offset = 1 + sizeof(uint64_t);
    }
    const DWORD rtp_len = dwLength - offset;
    const BYTE *rtp = typed + offset;
    if (rtp_len < 12 || ((rtp[0] & 0xC0) != 0x80)) {
        return true;
    }

    updateJitterStats();
    maybeAdjustJitterLatency();
    pushRtpToAppSrc(rtp, rtp_len);
    return true;
}

void AudioReceiverCore::updateJitterStats()
{
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

void AudioReceiverCore::maybeAdjustJitterLatency()
{
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

bool AudioReceiverCore::pushRtpToAppSrc(const BYTE *rtp, DWORD rtp_len)
{
    GstBuffer *gstbuf = gst_buffer_new_allocate(nullptr, rtp_len, nullptr);
    if (!gstbuf) {
        return false;
    }
    gst_buffer_fill(gstbuf, 0, rtp, rtp_len);
    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(appsrc), gstbuf);
    return ret == GST_FLOW_OK;
}

GstFlowReturn AudioReceiverCore::on_new_sample_from_sink(GstElement *sink, AudioReceiverCore *self)
{
    if (!self || !self->m_running.load()) {
        return GST_FLOW_OK;
    }
    GstSample *sample = nullptr;
    g_signal_emit_by_name(sink, "pull-sample", &sample);
    if (!sample) {
        return GST_FLOW_OK;
    }
    self->m_decodedFrames.fetch_add(1);
    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

uint64_t AudioReceiverCore::decodedFrameCount() const
{
    return m_decodedFrames.load();
}

uint64_t AudioReceiverCore::latestCaptureTimestampUs() const
{
    return m_latestCaptureTsUs.load();
}
