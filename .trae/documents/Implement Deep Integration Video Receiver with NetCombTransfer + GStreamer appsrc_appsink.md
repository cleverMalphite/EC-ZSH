## 目标
- 实现接收端深度集成：NetCombTransfer 回调拿到 RTP 包（前置 1 字节类型头）→ 注入 GStreamer（appsrc）→ depay/解码 → appsink 拉取 BGR 帧显示。

## 设计要点（结合现有代码与约束）
- 发送端已约定：每个 RTP 包前置 1 字节头，RTP=0x00。
- NetCombTransfer 支持多路 buffer callback 注册（不会覆盖已有注册），因此接收端可以安全追加注册一个数据回调。
- GStreamer 解码链路从 udpsrc 改为 appsrc：避免使用 udpsrc。

## 代码修改范围
### 1) 修改 VideoReceiver.h
- 增加依赖：
  - `#include <gst/app/gstappsrc.h>`
  - `#include "../NetCombTransfer/NetCombTransferApi.h"`
- 增加成员：
  - `GstElement* appsrc`（命名 rtp_src）
  - 接收过滤用的 `m_expectedRemoteTID`（可选：0 表示不限制来源）
- 增加成员函数：
  - `void startDeepIntegrationPipeline();`
  - `bool onNetCombTransferBuffer(DWORD srcTID, const std::shared_ptr<BYTE>& buf, DWORD len, bool bMsg, int64_t ts);`

### 2) 修改 VideoReceiver.cpp
- 新增全局/静态桥接回调（C 风格函数指针转发到对象实例）：
  - `VideoReceiver_NetCombTransferMessageCallback`（可空实现）
  - `VideoReceiver_NetCombTransferBufferCallback`（转发到 `onNetCombTransferBuffer`）
  - 保存一个静态 `VideoReceiver* g_videoReceiverInstance` 供转发。
- 在构造函数中：
  - 调用 `Register_NetCombTransferCallback(VideoReceiver_NetCombTransferMessageCallback, VideoReceiver_NetCombTransferBufferCallback)` 注册回调。
  - 将原先 `startGstPipeline(5004)`（udpsrc 路径）替换为 `startDeepIntegrationPipeline()`（appsrc 路径）。保留旧函数但不再默认使用。
- 实现 `startDeepIntegrationPipeline()`：
  - 创建 pipeline：
    - `appsrc name=rtp_src is-live=true do-timestamp=true format=time caps="application/x-rtp,media=video,encoding-name=H264,payload=96,clock-rate=90000" ! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! video/x-raw,format=BGR ! appsink name=mysink drop=true max-buffers=1 sync=false`
  - 取出 `rtp_src`（appsrc）与 `mysink`（appsink）元素指针。
  - 保持现有的 `QTimer` 轮询 appsink 拉帧显示逻辑不变（复用 `on_timer_check_appsink()`），只改数据来源。
- 实现 `onNetCombTransferBuffer(...)`：
  - 过滤：`bMsg==true` 直接忽略。
  - 校验长度 >= 2；读取首字节类型：
    - `0x00` → RTP：将剩余 `len-1` 拷贝到 `GstBuffer`，用 `gst_app_src_push_buffer(GST_APP_SRC(appsrc), gstbuf)` 注入。
    - 其他类型（如未来 `0x01` RTCP）先丢弃或预留分支。
  - 在 pipeline 未 ready 或 appsrc 为空时直接丢弃（避免阻塞网络线程）。

## 验证方式（用户确认后执行）
- 编译检查：确保新增 include、符号与链接正常。
- 运行验证：启动两端，观察接收端 appsink 能持续拉到 BGR 帧并显示；同时确认不再使用 udpsrc/udpsink。

## 交付物
- 修改文件：`VideoReceiver.h/.cpp`。
- 输出效果：接收端通过 NetCombTransfer 接收 RTP 包并在 UI 显示视频帧。