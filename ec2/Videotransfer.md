# 视频传输模块详细设计与实现细节

本文档对当前项目中的视频传输模块进行完整、逐层、逐细节说明，覆盖 UI 入口、信令触发、发送端编码与封包、接收端解包与解码、速率反馈与自适应、展示渲染、统计与容错机制，并标注主要实现所在文件与关键函数。

## 1. 模块入口与整体链路

### 1.1 UI 层入口
- 入口位于主界面初始化阶段，创建并挂载发送端与接收端组件：
  - 发送端组件：`VideoSender`，挂载到 `sendVideo_layout`
  - 接收端组件：`VideoReceiver`，挂载到 `recvVideo_layout`
- 入口文件与位置：
  - `ec2/ec2_ui_temp_cmake/mainwindow.cpp` 中 `MainWindow::init()` 里创建并添加到布局
- 设计要点：
  - 发送与接收在同一 UI 结构中并列展示，便于本地测试与联调
  - 组件尺寸通过 `QSizePolicy` 与 `setMaximumHeight` 控制，避免 UI 过大

### 1.2 信令触发链路
- 当用户点击“请求视频”按钮时，会通过 MRUDP 信令请求对端开始视频传输
- 入口触发位置：
  - `mainwindow.cpp` 中按钮 `askVideoTrans_button` 点击后调用 `openTempPage(5)`
  - `temppage.cpp` 会进一步发出 `askVideoTrans` 信号
  - 最终在 `function.cpp` 内调用 `askStartVideoTrans/AskStartVideoTrans`
- 信令处理：
  - `AskStartVideoTrans` 构造 `msg::msg_a`，设置 `server_type="AskStartVideoTrans"`，通过 MRUDP 发送
  - 发送逻辑位于 `ec2/ec2_ui_temp_cmake/function.cpp`
- 接收端信令回调：
  - `mrudp_dataserver_startVideoTrans_callback` 接收该信令后设置 `startVideoTransFlag = true`
  - 回调注册发生在 `createServer/createClient` 中，调用 `register_mrudp_dataserver_callback("AskStartVideoTrans", ...)`

### 1.3 数据面主链路（核心）
```
视频文件 -> GStreamer 解码 -> H264 编码 -> RTP 打包 -> appsink 取样
 -> 自定义 0x00 头 + RTP payload -> SpeedControl/MRUDP 发送
 -> 接收端回调 -> 解析 0x00 头 -> RTP 包推入 appsrc
 -> rtpjitterbuffer -> depay/parse -> 解码 -> BGR -> appsink
 -> UI 渲染
```

## 2. 发送端（VideoSender）实现细节

### 2.1 组件初始化与 UI 结构
- 构造函数中完成：
  - `gst_init` 一次性初始化
  - fps=30
  - 预览定时器 `timer_appsink`，33ms 拉取预览帧
  - 创建 `PlayerWidget` 用于本地预览，设置 `lockLastImg=true`
  - 叠加状态提示 Label（“等待信号/启动中/发送中”）
- 文件位置：
  - `ec2/ec2_ui_temp_cmake/VideoSender.cpp`

### 2.2 视频源选择策略
- 默认读取 MP4 文件作为发送源：
  - 优先尝试 `./FileSend/testp4.mp4`
  - 其次尝试 `../FileSend/testp4.mp4`
  - 再尝试 `applicationDirPath()/../FileSend/testp4.mp4`
  - 最后 fallback 到固定绝对路径 `/home/itzhou/EC1/EC2_12.8/FileSend/testp4.mp4`
- 实现位置：`resolve_default_video_file_path()` in `VideoSender.cpp`

### 2.3 Deep Integration 管线（核心发送管线）
- 由 `startDeepIntegrationPipeline(int destTID)` 负责构建与启动
- Pipeline 结构：
```
uridecodebin uri=... 
 -> videoconvert -> videoscale -> videorate
 -> video/x-raw,width=640,height=480,framerate=30/1
 -> tee name=t
    -> (预览分支) videoconvert -> video/x-raw,format=BGR -> appsink preview_sink
    -> (RTP分支) x264enc -> rtph264pay -> appsink rtp_sink
```
- 关键参数：
  - x264enc：`tune=zerolatency`，`speed-preset=superfast`，`bitrate=2500`，`key-int-max=30`，`bframes=0`
  - rtph264pay：`mtu=1200`，`pt=96`，`config-interval=1`
  - preview_sink：`drop=true`，`max-buffers=1`，`sync=true`
  - rtp_sink：`emit-signals=true`，`sync=true`，`max-buffers=10`，`drop=true`

### 2.4 RTP 数据抽取与封包
- `rtp_sink` 通过 `new-sample` 信号回调 `on_new_sample_from_sink`
- 回调流程：
  1. `pull-sample` 获取 `GstSample`
  2. `gst_buffer_map` 拿到 RTP payload
  3. 将 payload 传入 `handleRtpData`
- `handleRtpData` 的封包格式：
  - 在 RTP payload 前添加 1 字节头 `0x00` 表示视频 RTP 数据
  - 发送调用 `SendTIDDataWithSpeedControl(destTID, buffer, length, false, false, false)`
  - `isRBUDP=false`，走 MRUDP 通道

### 2.5 发送统计与日志
- 每 1 秒输出发送统计：
  - 发送包数 `send_pps`：统计窗口内成功调用发送接口的包数（packets per second）。
  - 发送速率 `send_kbps`：统计窗口内成功发送字节数换算得到的 kbps（kilobits per second）。
  - 发送失败 `send_fail`：统计窗口内发送接口返回失败的次数（用于观察链路拥塞/路由异常等问题）。
- `m_sendStatClock`：发送端侧的窗口计时器（`QElapsedTimer`），用于计算“这一窗口持续了多少 ms”，从而把累计 bytes 换算成速率。
- 输出位置：`VideoSender::handleRtpData`

### 2.6 预览渲染链路
- `timer_appsink` 每 33ms 拉取 `preview_sink`：
  - `gst_app_sink_try_pull_sample` -> BGR Mat -> clone -> `PlayerWidget::pushFrame2Show`
- 若首帧到达，隐藏状态提示 Label
- 若 appsink 无帧且未开始播放，保持“等待信号”

### 2.7 EOS 处理（循环播放）
- 在 `on_timer_check_appsink` 中监听 `GST_MESSAGE_EOS`
- 收到 EOS 后执行 `gst_element_seek_simple` 回到 0 并继续播放
- 保证 MP4 文件循环播放不断流

### 2.8 码率自适应与反馈接收
- 发送端注册 `Register_SpeedControl_RecvDataCallBack`：用于接收来自对端的“反馈包”（业务 type=0x01）。
- 反馈包结构：
  - 第 1 字节：`0x01` 表示反馈（区别于 `0x00` 的视频 RTP 数据包）。
  - 后续 12 字节（均为网络序的 32-bit 整数）：
    - `recv_bps`：对端测得的接收比特率（bit/s）。
    - `loss_x1000`：对端测得的丢包率千分比（×1000）。
    - `last_seq`：对端最近观测到的 RTP 序列号（用于辅助判断链路状态/调试）。
- 收到反馈后（发送端自适应）：
  - 根据 `loss_x1000`（丢包率）对 `recv_bps` 做折减，得到更保守的 `target_bps`，避免在丢包时继续推高码率。
  - 将 `target_bps` 限制在 200kbps ~ 8Mbps 之间，防止过低导致画质不可用或过高导致拥塞。
  - 只有当目标码率变化超过 50kbps 才更新一次，避免频繁抖动。
  - `g_object_set(encoder, "bitrate", target_kbps)`：动态调整 x264enc 的编码码率。
  - 调用 `ExChangeTransSpeedOfTID`：把新的目标速率同步给底层传输层（让发送调度与编码码率一致）。

## 3. 接收端（VideoReceiver）实现细节

### 3.1 组件初始化与自动启动
- 构造函数：
  - `gst_init` 一次性初始化
  - fps=30
  - appsink 轮询定时器 `timer_appsink` 33ms
  - 反馈定时器 `timer_feedback` 200ms
  - 创建 `PlayerWidget`，启用 `lockLastImg=true`
  - 初始化统计窗口与状态提示
- 若 `gUiRole == 2`：
  - 注册 SpeedControl 回调
  - 注册 VideoRtp 回调
  - 自动启动 `startDeepIntegrationPipeline`
  - 启动反馈定时器

### 3.2 Deep Integration 接收管线
- 由 `startDeepIntegrationPipeline()` 构建
- Pipeline 结构：
```
appsrc name=rtp_src is-live=true do-timestamp=true format=time block=false
 caps=application/x-rtp,media=video,encoding-name=H264,payload=96,clock-rate=90000
 -> rtpjitterbuffer latency=200 drop-on-latency=true do-lost=true mode=slave
 -> rtph264depay -> h264parse -> avdec_h264
 -> videoconvert -> video/x-raw,format=BGR
 -> appsink name=mysink drop=true max-buffers=1 sync=false
```
- 关键参数：
  - `rtpjitterbuffer latency=200` 提供抖动缓冲
  - `drop-on-latency=true` 在超时场景丢帧保证实时性
  - `do-lost=true` 允许丢包事件推进播放
  - appsink `drop=true` 限制积压

### 3.3 RTP 数据入口与封包解析
- 两条数据回调入口：
  - `onSpeedControlRecvData`：来自 SpeedControl 的 MRUDP 数据
  - `onNetCombTransferBuffer`：兼容 NetCombTransfer 原始包
- 数据类型判断：
  - 统一要求 typed[0]==0x00 表示 RTP 视频数据
  - RTP 版本检查：`(rtp[0] & 0xC0) == 0x80`
- NetCombTransfer 兼容格式：
  - 若数据长度满足封装格式：解析 `CIMsg::ReadData(raw+12)` 得到 data_len
  - 若 `typed[0]==0x00` 且符合 RTP 版本，则拆出 RTP 包

### 3.4 RTP 序列号与丢包统计
- 从 RTP 头部 `rtp[2..3]` 读取 `seq`：RTP 的 16-bit 序列号（范围 0~65535，按包递增，发生回卷时从 65535 回到 0）。
- `delta` 的含义：用于衡量“当前 seq 相对上一包 seq 的前进幅度”。实现上通常等价于 `delta = (uint16_t)(seq - lastSeq)`（按 65536 取模）。
  - `delta == 1`：连续到达，没有缺包。
  - `delta > 1`：中间缺了 `delta-1` 个 RTP 包，因此累计丢包 `lostPkts += delta-1`。
  - `delta >= 0x8000`：通常认为是乱序/旧包（前进幅度太大等价于负数），会被忽略以避免把乱序误判成大量丢包。
- 丢包统计字段（窗口统计，定期清零）：
  - `m_recvPktsWindow`：当前统计窗口内“成功接收并处理的 RTP 包数量”（即通过类型校验后用于喂给 appsrc 的包数）。
  - `m_recvBytesWindow`：当前窗口内累计的 RTP 字节数（通常为 `rtp_len` 的累加；不包含自定义的 1 字节 type）。
  - `m_lostPktsWindow`：当前窗口内推断出的丢包数量（由 `delta` 推断得到）。
  - `m_rxRtpPktsWindow` / `m_rxRtpBytesWindow`：用于打印 pps/kbps 的窗口计数（语义上与 `m_recvPktsWindow/m_recvBytesWindow` 同类，更多用于日志输出维度区分）。
- 每秒输出统计日志并重置窗口：窗口计数会在打印后清零，以便下一秒重新统计。

### 3.5 推入 appsrc
- 将 RTP payload 包装为 `GstBuffer`：
  - `gst_buffer_new_allocate`
  - `gst_buffer_fill`
  - `gst_app_src_push_buffer`
- 若 push 返回非 OK，则打印错误日志

### 3.6 appsink 拉取与 UI 展示
- `timer_appsink` 每 33ms 拉取 `mysink`：
  - `gst_app_sink_try_pull_sample`
  - 读取 width/height
  - Map 成 BGR Mat -> clone -> `PlayerWidget::pushFrame2Show`
- 若拉不到帧：
  - 当首次无帧时保持“等待信号”提示
  - 若 EOS 则输出日志

### 3.7 反馈发送机制
- 目的：接收端把“实际接收速率 + 丢包率”回传给发送端，用于动态调节编码码率与传输层速率。
- `timer_feedback` 每 200ms 发送一次反馈到对端（窗口由 `m_feedbackClock` 计时，并在每次发送后清零窗口统计）：
  - `recv_bps`：接收端估算的接收比特率（bit/s）。代码实现等价于：`recv_bps = (m_recvBytesWindow * 8 * 1000) / elapsedMs`，其中 `elapsedMs` 是本统计窗口时长。
  - `loss_x1000`：丢包率的千分比（×1000 的比例）。设 `expected_pkts = m_recvPktsWindow + m_lostPktsWindow`，则 `loss_x1000 = (m_lostPktsWindow / expected_pkts) * 1000`。
    - 例：1% 丢包率 → `loss_x1000 = 10`；10% → `100`。
  - `last_seq`：窗口结束时最后一次观测到的 RTP 序列号 `m_lastSeq`（用于发送端侧做参考/调试）。
- 反馈包格式（业务层 type + 三个 32-bit 字段，网络序）：
  - `0x01 + recv_bps(4) + loss_x1000(4) + last_seq(4)`
- 发送接口：
  - `SendTIDDataWithSpeedControl(m_lastRemoteTID, sendBuf, totalLen, true, false, false)`
  - `m_lastRemoteTID`：最近一次收到视频数据的对端 TID（用它来把反馈发回“当前发送视频的那个对端”）。

## 4. UI 渲染组件（PlayerWidget）

### 4.1 帧队列与刷新机制
- 内部使用 `QList<QPixmap>` 作为帧队列
- `len_buf` 默认 2，最大 10
- 通过 `QTimer` 触发 `update()` 进行重绘
- 支持 `lockLastImg`：
  - 为 true 时，最后一帧不会被弹出，避免空帧闪黑

### 4.2 paintEvent 绘制策略
- 若无帧：
  - 若存在 `lastPixmap`，绘制最后一帧
- 若有帧：
  - 绘制队列头并更新 `lastPixmap`
  - 队列长度控制：超过 len_buf 则 pop_front
- 该逻辑用于解决网络短暂抖动导致的黑屏闪烁

### 4.3 图像格式转换
- `MatToQImage2` 支持：
  - 3 通道：BGR->RGB 或直接使用（根据 `is_rgb`）
  - 4 通道：ARGB
  - 单通道：灰度图

## 5. 传输层与回调注册

### 5.1 SpeedControl 作为数据中枢
- SpeedControl 初始化时注册 NetCombTransfer 回调
- SpeedControl 接收到数据后解析内部封装，并调用所有注册的数据回调（包括 VideoSender/VideoReceiver）
- 对上层提供统一的 `SendTIDDataWithSpeedControl`

### 5.2 VideoRtp 回调注册
- `Register_VideoRtp_RecvDataCallBack` 允许视频数据绕过部分路径直接注入
- 当前接收端注册该回调，并指向 `VideoReceiver_SpeedControlRecvDataCallback`

## 6. 兼容与遗留路径

### 6.1 Legacy UDP 管线
- `VideoSender::startGstPipeline(host, port)` 使用 `udpsink` 推流
- `VideoReceiver::startGstPipeline(port)` 使用 `udpsrc` 拉流
- 当前 UI 默认走 Deep Integration 流程，但保留此路径作为兼容方案

### 6.2 MRUDP 信令路径
- “请求视频”信令通过 MRUDP 发送
- 视频数据本身通过 SpeedControl + MRUDP 承载
- MRUDP 回调在 `createServer/createClient` 里注册

## 7. 配置与依赖

### 7.1 配置项
- `BigDataTransfer/DEST_TID`：发送端目标 TID（默认 105）
- `VideoTrans/videoPort_<TID>`：本地视频端口（用于 legacy UDP 模式）

### 7.2 依赖库
- GStreamer（编解码、RTP 管线）
- OpenCV（Mat 与 UI 图像转换）
- Qt（UI、定时器、渲染）
- SpeedControl/NetCombTransfer/MRUDP（底层传输）

## 8. 关键文件清单
- 发送端：`ec2/ec2_ui_temp_cmake/VideoSender.cpp/.h`
- 接收端：`ec2/ec2_ui_temp_cmake/VideoReceiver.cpp/.h`
- UI 入口：`ec2/ec2_ui_temp_cmake/mainwindow.cpp`
- 渲染组件：`ec2/ec2_ui_temp_cmake/playerwidget.cpp/.h`
- 信令与回调：`ec2/ec2_ui_temp_cmake/function.cpp/.h`
- 传输层接口：`ec2/SpeedControl/SpeedControlApi.cpp`、`ec2/NetCombTransfer/NetCombTransferApi.h`
