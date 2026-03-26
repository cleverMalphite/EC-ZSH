#include <QCoreApplication>
#include <QEventLoop>
#include <QTimer>
#include <QElapsedTimer>
#include <QtGlobal>

#include <gst/gst.h>
#include <gst/app/gstappsink.h>

#include <iostream>
#include <memory>
#include <cstring>

#include "AudioReceiverCore.h"

static GstFlowReturn on_rtp_sample(GstElement *sink, gpointer user_data)
{
    AudioReceiverCore *receiver = static_cast<AudioReceiverCore *>(user_data);
    if (!receiver) {
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

    const DWORD totalLen = static_cast<DWORD>(map.size + 1);
    std::shared_ptr<BYTE> typed(new BYTE[totalLen], std::default_delete<BYTE[]>());
    typed.get()[0] = 0x02;
    std::memcpy(typed.get() + 1, map.data, map.size);
    long int rt = 0;
    long int ft = 0;
    receiver->onAudioRtpData(100, typed, totalLen, rt, ft);

    gst_buffer_unmap(buffer, &map);
    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    qputenv("EC2_AUDIO_SINK", QByteArray("appsink"));

    AudioReceiverCore receiver;
    if (!receiver.start()) {
        std::cerr << "[selftest] receiver start failed" << std::endl;
        return 2;
    }

    const std::string pipeline_str =
        "audiotestsrc is-live=true wave=sine freq=440 ! audioconvert ! audioresample ! "
        "opusenc bitrate=64000 inband-fec=true packet-loss-percentage=10 ! "
        "rtpopuspay pt=97 mtu=1200 ! "
        "appsink name=rtp_sink emit-signals=true sync=false max-buffers=200 drop=true";

    GError *error = nullptr;
    GstElement *pipeline = gst_parse_launch(pipeline_str.c_str(), &error);
    if (error) {
        std::cerr << "[selftest] sender pipeline error: " << error->message << std::endl;
        g_error_free(error);
        return 3;
    }

    GstElement *appsink = gst_bin_get_by_name(GST_BIN(pipeline), "rtp_sink");
    if (!appsink) {
        gst_object_unref(pipeline);
        return 4;
    }
    g_signal_connect(appsink, "new-sample", G_CALLBACK(on_rtp_sample), &receiver);

    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        gst_object_unref(appsink);
        gst_object_unref(pipeline);
        return 5;
    }

    QEventLoop loop;
    QTimer::singleShot(2000, &loop, &QEventLoop::quit);
    loop.exec();

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(appsink);
    gst_object_unref(pipeline);

    receiver.stop();

    const uint64_t frames = receiver.decodedFrameCount();
    std::cout << "[selftest] decoded_frames=" << frames << std::endl;
    if (frames == 0) {
        return 1;
    }
    return 0;
}
