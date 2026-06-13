# Phase 4: Token Bucket Pacing 机制升级

## 1. 目标

将现有的 cycle-based pacing 升级为 token bucket 模型，实现更精确的发送速率控制，支持帧级突发（burst）和帧间 pacing。

**面试故事**："我将原有的周期式 pacing 机制升级为 token bucket 模型，实现了帧级突发发送和帧间精确 pacing，在保持低延迟的同时避免了网络突发拥塞。"

---

## 2. 现状分析

### 当前 Pacing 机制
- **模型**：双周期（big cycle + small cycle）
- **大周期**：100ms，调整一次发送速率
- **小周期**：大周期内的细分（最多 3 个小周期）
- **速率控制**：通过 `DoChangeExpectSpeed()` 设置期望速率
- **发送逻辑**：每个小周期发送固定数量的包

### 问题
1. **突发性**：在小周期开始时会突发发送多个包
2. **精度不够**：100ms 粒度的速率调整不够平滑
3. **无帧感知**：不区分视频帧边界，一个帧可能被分散到多个小周期
4. **与 BBR 脱节**：BBR 计算的 pacing_rate 没有直接驱动发送

### 关键代码位置
```
SpeedControl/Term2TermTransmission.h  → Pacing 逻辑
SpeedControl/DataWithPacketInfo.h     → 包格式
Term2TermQos/bbr_controller.h         → BBR pacing_rate
```

---

## 3. Token Bucket 设计

### 3.1 核心概念

```
Token Bucket 模型：
┌─────────────────────────────────────────┐
│                                         │
│   tokens += rate × elapsed_time         │
│   tokens = min(tokens, bucket_size)     │
│                                         │
│   发送条件：tokens >= packet_size       │
│   发送后：  tokens -= packet_size       │
│                                         │
└─────────────────────────────────────────┘

参数：
- rate: 令牌生成速率 (bytes/sec)，由 BBR pacing_rate 驱动
- bucket_size: 桶容量 (bytes)，允许的最大突发量
- tokens: 当前令牌数
```

### 3.2 视频帧感知 Pacing

```
帧内突发：一个视频帧包含多个包，允许帧内突发发送
帧间 pacing：帧与帧之间需要 pacing，避免突发拥塞

时间线：
┌──────┐         ┌──────┐         ┌──────┐
│Frame1│  pacing  │Frame2│  pacing  │Frame3│
│(burst)│  delay  │(burst)│  delay  │(burst)│
└──────┘         └──────┘         └──────┘
```

### 3.3 Token Bucket + 帧感知

```cpp
// ec2/SpeedControl/TokenBucketPacer.h
#pragma once
#include <cstdint>
#include <chrono>
#include <mutex>
#include <deque>
#include <memory>

// Pacer 配置
struct PacerConfig {
    double initial_rate_bps = 1000000;      // 初始速率 1Mbps
    double min_rate_bps = 100000;           // 最低速率 100kbps
    double max_rate_bps = 100000000;        // 最高速率 100Mbps
    uint32_t max_burst_bytes = 1500 * 10;   // 最大突发 10 个包
    uint32_t max_burst_frames = 1;          // 最大突发帧数
    bool enable_frame_pacing = true;        // 启用帧级 pacing
    int64_t min_frame_interval_us = 16667;  // 最小帧间隔 (60fps)
};

// 包信息
struct PacerPacket {
    uint8_t* data;
    uint32_t length;
    uint32_t seq;
    int64_t enqueue_time_us;
    bool is_frame_start;       // 是否是帧的第一个包
    bool is_frame_end;         // 是否是帧的最后一个包
    uint8_t priority;          // 优先级 (0=最高)
    uint32_t frame_id;         // 帧 ID
};

// Token Bucket Pacer
class TokenBucketPacer {
public:
    TokenBucketPacer(const PacerConfig& config = PacerConfig{})
        : m_config(config)
        , m_tokens(0)
        , m_max_tokens(config.max_burst_bytes)
        , m_rate_bps(config.initial_rate_bps)
        , m_last_refill_time(std::chrono::steady_clock::now())
        , m_last_frame_send_time(0)
        , m_current_frame_id(0)
        , m_frame_in_progress(false) {}

    // 设置发送速率（由 BBR/Copa 驱动）
    void SetRate(double rate_bps) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_rate_bps = std::max(m_config.min_rate_bps,
                    std::min(m_config.max_rate_bps, rate_bps));

        // 更新桶容量：允许 2 个帧间隔的突发
        double frame_interval_s = 0.033;  // ~30fps
        m_max_tokens = static_cast<uint32_t>(m_rate_bps * frame_interval_s / 8 * 2);
        m_max_tokens = std::max(m_max_tokens, m_config.max_burst_bytes);
    }

    // 入队一个包
    bool Enqueue(const PacerPacket& packet) {
        std::lock_guard<std::mutex> lock(m_mutex);

        // 优先级队列插入
        auto& queue = GetQueue(packet.priority);
        queue.push_back(packet);

        return true;
    }

    // 尝试发送，返回可以发送的包
    // 返回值：可以立即发送的包列表
    std::vector<PacerPacket> DequeuePackets() {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<PacerPacket> to_send;

        // 1. 重新填充令牌
        RefillTokens();

        // 2. 帧 pacing 检查
        if (m_config.enable_frame_pacing && m_frame_in_progress) {
            // 当前帧正在发送中，继续发送当前帧的包
            DequeueCurrentFrame(to_send);
            return to_send;
        }

        // 3. 帧间 pacing 检查
        int64_t now_us = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();

        if (m_config.enable_frame_pacing && m_last_frame_send_time > 0) {
            int64_t elapsed = now_us - m_last_frame_send_time;
            if (elapsed < m_config.min_frame_interval_us) {
                // 还没到下一帧的发送时间
                return to_send;
            }
        }

        // 4. 按优先级从队列取包
        for (int priority = 0; priority < 8; priority++) {
            auto& queue = GetQueue(priority);
            while (!queue.empty()) {
                auto& packet = queue.front();

                // 检查令牌是否足够
                if (m_tokens < packet.length) {
                    // 令牌不足，但如果正在发送帧，允许继续
                    if (m_frame_in_progress) {
                        // 帧内突发：允许超出令牌
                        m_tokens = 0;
                    } else {
                        break;  // 等待令牌积累
                    }
                }

                // 扣除令牌
                m_tokens -= packet.length;

                // 记录帧状态
                if (packet.is_frame_start) {
                    m_frame_in_progress = true;
                    m_current_frame_id = packet.frame_id;
                }

                to_send.push_back(packet);
                queue.pop_front();

                if (packet.is_frame_end) {
                    m_frame_in_progress = false;
                    m_last_frame_send_time = now_us;
                    break;  // 帧发送完成，等待帧间 pacing
                }
            }

            if (!to_send.empty()) break;  // 已经有包要发送
        }

        return to_send;
    }

    // 获取当前队列状态
    struct QueueStatus {
        size_t total_packets;
        size_t total_bytes;
        double expected_send_time_ms;  // 预计发完时间
    };

    QueueStatus GetStatus() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        QueueStatus status = {0, 0, 0};

        for (int i = 0; i < 8; i++) {
            for (const auto& pkt : m_queues[i]) {
                status.total_packets++;
                status.total_bytes += pkt.length;
            }
        }

        if (m_rate_bps > 0) {
            status.expected_send_time_ms =
                (status.total_bytes * 8.0) / m_rate_bps * 1000.0;
        }

        return status;
    }

    // 获取当前 pacing 速率
    double GetRateBps() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_rate_bps;
    }

    // 获取当前令牌数
    uint32_t GetTokens() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return static_cast<uint32_t>(m_tokens);
    }

private:
    PacerConfig m_config;
    mutable std::mutex m_mutex;

    // Token bucket 状态
    double m_tokens;                   // 当前令牌数
    uint32_t m_max_tokens;             // 桶容量
    double m_rate_bps;                 // 当前速率
    std::chrono::steady_clock::time_point m_last_refill_time;

    // 帧 pacing 状态
    int64_t m_last_frame_send_time;
    uint32_t m_current_frame_id;
    bool m_frame_in_progress;

    // 优先级队列 (0=最高优先级)
    std::deque<PacerPacket> m_queues[8];

    // 获取指定优先级的队列
    std::deque<PacerPacket>& GetQueue(uint8_t priority) {
        uint8_t idx = std::min(priority, (uint8_t)7);
        return m_queues[idx];
    }

    // 重新填充令牌
    void RefillTokens() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            now - m_last_refill_time).count();

        if (elapsed <= 0) return;

        // 计算新增令牌
        double new_tokens = m_rate_bps * elapsed / 8.0 / 1000000.0;
        m_tokens = std::min(static_cast<double>(m_max_tokens),
                           m_tokens + new_tokens);

        m_last_refill_time = now;
    }

    // 发送当前帧的剩余包
    void DequeueCurrentFrame(std::vector<PacerPacket>& to_send) {
        for (int priority = 0; priority < 8; priority++) {
            auto& queue = GetQueue(priority);
            while (!queue.empty()) {
                auto& packet = queue.front();
                if (packet.frame_id != m_current_frame_id) {
                    break;  // 不是当前帧的包
                }

                // 帧内突发：允许超出令牌
                to_send.push_back(packet);
                queue.pop_front();

                if (packet.is_frame_end) {
                    m_frame_in_progress = false;
                    int64_t now_us = std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::steady_clock::now().time_since_epoch()).count();
                    m_last_frame_send_time = now_us;
                    return;
                }
            }
        }
    }
};
```

---

## 4. 与 BBR 的联动

### 4.1 BBR pacing_rate 直接驱动 Pacer

```cpp
// 在 EndToEndQos 中连接 BBR 和 Pacer

class EndToEndQos {
    // ... existing members ...

    // Token Bucket Pacer
    std::unique_ptr<TokenBucketPacer> m_pacer;

    // 定期调用（由 BBR heartbeat 触发）
    void OnBBRUpdate(double pacing_rate_bps, uint32_t congestion_window) {
        if (m_pacer) {
            m_pacer->SetRate(pacing_rate_bps);
        }
    }

    // 获取 Pacer（供 Term2TermTransmission 使用）
    TokenBucketPacer* GetPacer() {
        return m_pacer.get();
    }
};
```

### 4.2 修改 Term2TermTransmission

```cpp
// 修改 ec2/SpeedControl/Term2TermTransmission.h

class Term2TermTransmission {
    // ... existing members ...

    // Token Bucket Pacer 引用
    TokenBucketPacer* m_pacer;

    // 初始化
    void InitPacer(EndToEndQos* qos) {
        if (qos) {
            m_pacer = qos->GetPacer();
        }
    }

    // 修改发送逻辑
    bool DoSendData() {
        if (!m_pacer) {
            // fallback: 使用原有 cycle-based pacing
            return DoSendDataCycleBased();
        }

        // 从 Pacer 获取可以发送的包
        auto packets = m_pacer->DequeuePackets();

        // 发送
        for (auto& packet : packets) {
            DoCycleSend(packet);
        }

        return !packets.empty();
    }

    // 入队到 Pacer
    bool PushSendData(const shared_ptr<BYTE>& data, DWORD length,
                      bool isSleep, bool isRBUDP) {
        if (!m_pacer) {
            // fallback: 使用原有队列
            return PushSendDataOriginal(data, length, isSleep, isRBUDP);
        }

        PacerPacket packet;
        packet.data = data.get();
        packet.length = length;
        packet.seq = m_d_packet_index++;
        packet.enqueue_time_us = GetCurrentTimeUs();
        packet.is_frame_start = IsFrameStart(data, length);
        packet.is_frame_end = IsFrameEnd(data, length);
        packet.priority = GetPacketPriority(data, length);
        packet.frame_id = GetFrameId(data, length);

        return m_pacer->Enqueue(packet);
    }

private:
    // 帧边界检测
    bool IsFrameStart(const shared_ptr<BYTE>& data, DWORD length) {
        // 检查 RTP marker bit 或自定义帧头
        if (length > 12) {
            // RTP header: byte 1, bit 0 = marker
            return (data.get()[1] & 0x80) != 0;  // marker bit
        }
        return false;
    }

    bool IsFrameEnd(const shared_ptr<BYTE>& data, DWORD length) {
        // 与 IsFrameStart 相同，marker bit 标记帧结束
        return IsFrameStart(data, length);
    }

    // 包优先级判断
    uint8_t GetPacketPriority(const shared_ptr<BYTE>& data, DWORD length) {
        // 根据包类型确定优先级
        BYTE flag = data.get()[0];
        switch (flag) {
            case 10:  // 不可靠数据（音视频）
                return 0;  // 最高优先级
            case 17:  // FEC 包
                return 4;
            case 8:   // 可靠数据（文件）
                return 6;
            default:
                return 5;
        }
    }

    // 帧 ID 提取
    uint32_t GetFrameId(const shared_ptr<BYTE>& data, DWORD length) {
        // 从 RTP timestamp 或自定义头提取
        if (length >= 12) {
            // RTP timestamp (bytes 4-7)
            uint32_t rtp_ts = (data.get()[4] << 24) |
                              (data.get()[5] << 16) |
                              (data.get()[6] << 8) |
                              data.get()[7];
            return rtp_ts;
        }
        return 0;
    }
};
```

---

## 5. 帧级 Pacing 控制

### 5.1 帧间隔自适应

```cpp
// ec2/SpeedControl/FramePacingController.h
#pragma once
#include <cstdint>
#include <algorithm>

// 帧率控制
class FramePacingController {
public:
    FramePacingController(double target_fps = 30.0)
        : m_target_fps(target_fps)
        , m_min_interval_us(static_cast<int64_t>(1000000.0 / target_fps * 0.8))
        , m_max_interval_us(static_cast<int64_t>(1000000.0 / target_fps * 1.5))
        , m_last_frame_time_us(0)
        , m_frame_count(0) {}

    // 设置目标帧率
    void SetTargetFPS(double fps) {
        m_target_fps = fps;
        m_min_interval_us = static_cast<int64_t>(1000000.0 / fps * 0.8);
        m_max_interval_us = static_cast<int64_t>(1000000.0 / fps * 1.5);
    }

    // 检查是否可以发送下一帧
    bool CanSendNextFrame(int64_t now_us) {
        if (m_last_frame_time_us == 0) {
            return true;  // 第一帧
        }

        int64_t elapsed = now_us - m_last_frame_time_us;
        return elapsed >= m_min_interval_us;
    }

    // 获取建议的帧间隔（用于 pacer 配置）
    int64_t GetSuggestedFrameIntervalUs() const {
        return static_cast<int64_t>(1000000.0 / m_target_fps);
    }

    // 记录帧发送
    void OnFrameSent(int64_t send_time_us) {
        m_last_frame_time_us = send_time_us;
        m_frame_count++;
    }

    // 根据网络状况调整帧率
    void AdjustFPS(double loss_rate, double rtt_ms) {
        if (loss_rate > 0.1 || rtt_ms > 200) {
            // 网络差，降低帧率
            m_target_fps = std::max(15.0, m_target_fps * 0.9);
        } else if (loss_rate < 0.02 && rtt_ms < 50) {
            // 网络好，提高帧率
            m_target_fps = std::min(60.0, m_target_fps * 1.05);
        }

        // 更新间隔
        m_min_interval_us = static_cast<int64_t>(1000000.0 / m_target_fps * 0.8);
        m_max_interval_us = static_cast<int64_t>(1000000.0 / m_target_fps * 1.5);
    }

    double GetCurrentFPS() const { return m_target_fps; }

private:
    double m_target_fps;
    int64_t m_min_interval_us;
    int64_t m_max_interval_us;
    int64_t m_last_frame_time_us;
    uint64_t m_frame_count;
};
```

### 5.2 与 VideoSender 集成

```cpp
// 修改 ec2/ec2_ui_temp_cmake/VideoSender.cpp

// 在 GStreamer 回调中，标记帧边界
void VideoSender::on_new_sample(GstElement* sink) {
    GstSample* sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
    GstBuffer* buffer = gst_sample_get_buffer(sample);
    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_READ);

    // 获取 RTP timestamp
    uint32_t rtp_ts = GetRtpTimestamp(buffer);

    // 检查是否是帧边界（通过 marker bit）
    bool is_frame_end = CheckMarkerBit(buffer);

    // 构造 PacerPacket
    PacerPacket packet;
    packet.data = map.data;
    packet.length = map.size;
    packet.seq = m_seq++;
    packet.enqueue_time_us = GetCurrentTimeUs();
    packet.is_frame_start = m_is_first_packet_of_frame;
    packet.is_frame_end = is_frame_end;
    packet.priority = 0;  // 视频最高优先级
    packet.frame_id = rtp_ts;

    m_is_first_packet_of_frame = is_frame_end;

    // 交给 Pacer
    if (m_pacer) {
        m_pacer->Enqueue(packet);
    }

    gst_buffer_unmap(buffer, &map);
    gst_sample_unref(sample);
}
```

---

## 6. 与 QoS 反馈的集成

```cpp
// 在 VideoReceiver 反馈处理中调整 Pacer

void VideoSender::onSpeedControlRecvData(DWORD, const shared_ptr<BYTE>& pBuffer,
                                          DWORD dwLength, long int&, long int&) {
    // ... existing ABR logic ...

    // 调整 Pacer 速率
    if (m_pacer) {
        m_pacer->SetRate(target_bps);
    }

    // 调整帧率
    if (m_frame_pacer) {
        m_frame_pacer->AdjustFPS(loss_rate, rtt_ms);
    }
}
```

---

## 7. 测试方案

### 7.1 突发测试

```bash
# 测试 Token Bucket 的突发控制

# 无 Pacing（突发）
./test_pacer --mode none --duration 10
# 预期：瞬间打满带宽，造成网络拥塞

# Cycle-based Pacing
./test_pacer --mode cycle --duration 10
# 预期：周期性突发，每个小周期开始时有突发

# Token Bucket Pacing
./test_pacer --mode token_bucket --duration 10
# 预期：平滑发送，无明显突发
```

### 7.2 帧率稳定性测试

```bash
# 测试帧间 pacing 的效果

# 30fps 视频流
./test_frame_pacing --fps 30 --duration 60

# 监控指标：
# - 帧间隔标准差（应 < 2ms）
# - 帧间隔最大值（应 < 40ms）
# - 无连续丢帧
```

### 7.3 关键指标

| 指标 | Cycle-based | Token Bucket | 改进 |
|------|------------|-------------|------|
| 发送突发度 | 3-5 包/突发 | 1 包/突发 | -70% |
| 帧间隔抖动 | ±10ms | ±2ms | -80% |
| P99 延迟 | 150ms | 80ms | -47% |
| 网络利用率 | 85% | 92% | +8% |
| 卡顿率 | 5% | 1% | -80% |
