## 目标（第 3 步）

* 在现有“1 字节类型头”基础上补齐 RTCP/反馈闭环：

  * 接收端：解析收到的 RTP 序号，周期性发送 0x01（RTCP/反馈）包回发送端。

  * 发送端：接收 0x01 反馈包，根据接收端反馈动态调整编码器码率，并把观测量同步给 SpeedControl（先做码率闭环，后续再把 pacing 深度接入）。

## 设计约束与实现选择

* 不引入新文件，只修改现有 VideoSender/VideoReceiver。

* RTCP 不用 GStreamer rtpbin（保持你当前“rtph264pay + appsink 注入 NCT”的架构），而实现一个“轻量自定义 RTCP 反馈包”（类型头=0x01）。

* 反馈包字段使用网络字节序，避免端序问题。

## 需要修改的文件与内容

### 1) 修改 VideoReceiver（生成并发送 0x01 反馈）

**文件**：`ec2_ui_temp_cmake/VideoReceiver.h/.cpp`

* 新增统计成员：

  * `DWORD m_lastRemoteTID`、`bool m_hasSeq`、`uint16_t m_lastSeq`、`uint64_t m_recvBytesWindow`、`uint32_t m_lostPktsWindow`、`QElapsedTimer m_feedbackClock`。

  * `QTimer* timer_feedback`（例如 200ms 周期）。

* 在 `onNetCombTransferBuffer` 中：

  * 对 0x00 RTP：在推送到 appsrc 前，从 RTP 头解析 `sequence number`（payload 偏移=1，seq 在 RTP header 的第 3\~4 字节），用简单算法累积丢包（seq 跳变）。

  * 记录窗口内接收字节数。

* 新增 `sendFeedback()` 槽函数：

  * 根据窗口统计计算 `recv_bps`、`loss_per_thousand`（或 ppm），打包为：

    * `0x01 | recv_bps(u32) | loss_x1000(u32) | last_seq(u32)`

  * 调用 `SendTIDData(m_lastRemoteTID, buf, len, false)` 发送反馈包（仍走 NetCombTransfer/UDP）。

  * 发送后清空窗口统计。

### 2) 修改 VideoSender（接收 0x01 反馈并调节编码器 + 同步 SpeedControl）

**文件**：`ec2_ui_temp_cmake/VideoSender.h/.cpp`

* 在发送端也注册 NetCombTransfer buffer callback（与 SpeedControl 并存，NCT 支持多回调）：

  * 增加一个 `VideoSender::onNetCombTransferBuffer(...)` 用于处理 0x01。

  * 通过静态实例指针/转发函数把 C 风格回调转到对象。

* 在 `startDeepIntegrationPipeline` 中：

  * 给编码器命名，便于运行时取到元素：`nvv4l2h264enc name=enc ...`

  * 保存 `GstElement* encoder` 成员。

* 在 `onNetCombTransferBuffer` 中：

  * 若首字节为 0x01：解析 `recv_bps / loss`。

  * 根据反馈计算目标码率（最简策略：`target_bps = clamp(recv_bps * (1 - loss_margin), min_bps, max_bps)`），并通过 `g_object_set(encoder, "bitrate", target_kbps, NULL)` 更新硬件编码器码率。

  * 同步给 SpeedControl：把 `target_bps` 换算为 Mbps，调用 `ExChangeTransSpeedOfTID(destTID, target_mbps)`（仅做闭环输入，后续第 4 步再把发送 pacing 彻底切到 SpeedControl）。

## 验证（用户确认后执行）

* 编译验证：确保新增的 `gstappsrc`、SpeedControl API 引用与符号可用。

* 运行验证：

  * 两端启动后，接收端能持续显示视频。

  * 接收端定时回发 0x01 反馈，发送端能动态调整 encoder bitrate（可在日志/调试输出观察）。

## 输出结果

* 完整实现“RTP(0x00) 数据 + RTCP/反馈(0x01) 闭环”，并把反馈结果接入 SpeedControl 的速率设置接口，为后续 pacing 深度融合打基础。

