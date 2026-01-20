## 现象与结论
- 102 端终端日志只看到 NetCombTransfer 收到数据（例如 `DecodeData的首字节是:0`、`Recv Data!!!`），但看不到 `VideoReceiver` 里任何 `[VideoReceiver] rtp_pps=...` 或 `[DEBUG] ...`。
- 主要原因不是 GStreamer 解码，而是“数据没有走到 VideoReceiver 的回调”。

## 根因定位
- SpeedControl 的接收回调并不是“广播给所有注册者”，而是按 `isRBUDP` 固定路由到 `g_p_data_func[0]` 或 `g_p_data_func[1]`：
  - [SpeedControlApi.cpp](file:///home/itzhou/EC1/EC2_12.8/ec2/SpeedControl/SpeedControlApi.cpp#L275-L298)
- MRUDP/RBUDP 模块会在初始化时注册自己的 `Register_SpeedControl_RecvDataCallBack(...)`（占用 0/1 槽位）：
  - [MRUDPManager.cpp](file:///home/itzhou/EC1/EC2_12.8/ec2/MRUDP/MRUDPManager.cpp#L105-L121)
  - [RBUDPManager.cpp](file:///home/itzhou/EC1/EC2_12.8/ec2/RBUDP/RBUDPManager.cpp#L71-L94)
- 目前 `VideoReceiver` 虽然也调用了 `Register_SpeedControl_RecvDataCallBack(...)`，但它只会被追加到数组后面，实际永远不会被 `CallAllDataFunc()` 调用；并且 `Register_VideoRtp_RecvDataCallBack(...)` 在接收端被注释掉了：
  - [VideoReceiver.cpp](file:///home/itzhou/EC1/EC2_12.8/ec2/ec2_ui_temp_cmake/VideoReceiver.cpp#L126-L138)

## 修复思路（让视频数据“必达” VideoReceiver）
- 保持 MRUDP/RBUDP/SpeedControl 现有分流不动，新增一条“视频 RTP 专用旁路回调”。
- 收包入口（MRUDP/RBUDP 的 ON_RECV_DATA_FROM_NETCOMBTRANSFER）检测到是视频 RTP 包（你现有协议里 0x00 + RTP header），则优先转发到该旁路回调，由 `VideoReceiver` 负责喂给 GStreamer `appsrc`。
- 这样无论 `isRBUDP` 走槽位 0 还是 1，视频包都能被转发到 `VideoReceiver`，不再受 SpeedControl 槽位覆盖影响。

## 具体改动（代码层面）
1. **接收端注册旁路回调**
   - 在 `VideoReceiver` 构造里恢复注册：`Register_VideoRtp_RecvDataCallBack(VideoReceiver_SpeedControlRecvDataCallback);`
   - 位置：[VideoReceiver.cpp](file:///home/itzhou/EC1/EC2_12.8/ec2/ec2_ui_temp_cmake/VideoReceiver.cpp#L126-L138)

2. **MRUDP/RBUDP 收包入口做旁路转发**
   - 在 [MRUDPManager.cpp](file:///home/itzhou/EC1/EC2_12.8/ec2/MRUDP/MRUDPManager.cpp) 与 [RBUDPManager.cpp](file:///home/itzhou/EC1/EC2_12.8/ec2/RBUDP/RBUDPManager.cpp) 的 `ON_RECV_DATA_FROM_NETCOMBTRANSFER(...)`：
     - 若 `pBuffer[0]==0x00` 且 `pBuffer[1]` 符合 RTP V2（`(pBuffer[1] & 0xC0)==0x80`），并且 `g_video_rtp_recv_callback != nullptr`，则调用该回调并直接 return。

3. **补齐最小可观测性**
   - 在旁路转发点增加一条轻量日志（每秒一次统计即可），确认“视频包已转发”。
   - 在 `VideoReceiver::onSpeedControlRecvData()` 保留 `appsrc push` 返回值检查（已有），若 push 失败直接打印。

4. **构建与运行校验**
   - 确认你运行的二进制是重新编译后的（`ec_xgz_ui` 或 `ec2_ui_test`）。
   - 启动接收端必须使用参数 `2`，确保 `gUiRole==2` 走接收逻辑：[main.cpp](file:///home/itzhou/EC1/EC2_12.8/ec2/ec2_ui_temp_cmake/main.cpp#L24-L36)

## 验证标准
- 102 端终端出现：
  - `VideoReceiver` 每秒统计 `rtp_pps/rtp_kbps`（说明数据已喂进 `VideoReceiver`）。
  - `on_timer_check_appsink()` 能 pull 到 sample（界面开始出画面，Waiting 覆盖消失）。
- 若仍无画面但有 `rtp_pps`：
  - 重点看 `VideoReceiver` bus error/warning 输出（已在 `on_timer_check_appsink()` 中处理），再针对 caps/parse/decoder 做二次调整。

确认这个方案后，我会按上述步骤把旁路转发链路补齐并给出你在 102 端应该看到的关键日志与排查分支。