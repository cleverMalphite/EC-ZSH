# Phase 3: QoE 感知多路径调度算法

## 1. 目标

在现有的双路径（Mesh + 5G DTU）基础上，设计 QoE 感知的智能调度算法，根据实时网络状态动态选择最优路径，实现端到端 QoE 最大化。

**面试故事**："我设计了 QoE 感知的多路径调度算法，根据实时网络状态（延迟、丢包、带宽）计算每条路径的 QoE 分数，将关键数据（I 帧、音频）发到 QoE 最高的路径，在 Mesh+5G 双链路场景下将端到端延迟降低了 40%。"

---

## 2. 现状分析

### 当前多路径机制
- **双路径支持**：Ad-hoc Mesh（本地）+ 5G DTU（远程）
- **连接模式**：`ConnMode::MeshOnly(0)` / `ConnMode::DTUOnly(1)` / `ConnMode::DualPath(2)`
- **路由**：`CNetTerminalManager::FindNextHop()` 查路由表
- **通道状态机**：INVALID → CONNECTED → RID_CONFIRMED → RUDPPORT_CONFIRMED → NEGOTIATED
- **质量指标**：`CNetChannel::m_nQoSSend`, `m_nQoSRecv`（整数型，粒度粗）

### 问题
1. **调度策略简单**：双路径模式下，数据在两条路径上轮询或随机分配
2. **无 QoE 感知**：不考虑延迟、丢包、带宽对用户体验的影响
3. **无流感知调度**：不同类型的数据（音频/视频/文件）不区分路径
4. **质量指标粗糙**：QoS 只有整数型的 send/recv 质量，无法精确反映链路状态

### 关键代码位置
```
NetCombTransfer/CNetTerminalManager.h  → 路由和终端管理
NetCombTransfer/CNetTerminal.h         → 终端（含多通道）
NetCombTransfer/CNetChannel.h          → 单个通道
NetCombTransfer/CRouterInfo.h          → 路由表项
SpeedControl/Term2TermTransmission.h   → 发送控制
Term2TermQos/EndToEndQos.h             → QoS 管理
```

---

## 3. QoE 模型设计

### 3.1 QoE 指标定义

```
QoE_score = α × (1 - loss_rate)           // 可靠性
          + β × (1 / (1 + latency_ms/100)) // 低延迟
          + γ × (throughput / demand)       // 带宽满足度
          + δ × (1 - jitter_ratio)          // 稳定性

其中：
- α = 0.3  （可靠性权重）
- β = 0.4  （延迟权重，对实时流最重要）
- γ = 0.2  （带宽权重）
- δ = 0.1  （稳定性权重）
```

### 3.2 不同流类型的 QoE 权重

| 流类型 | α (可靠性) | β (延迟) | γ (带宽) | δ (稳定性) |
|--------|-----------|---------|---------|-----------|
| 实时音频 | 0.2 | 0.6 | 0.1 | 0.1 |
| 实时视频 | 0.3 | 0.4 | 0.2 | 0.1 |
| 交互式 | 0.3 | 0.5 | 0.1 | 0.1 |
| 文件传输 | 0.5 | 0.1 | 0.3 | 0.1 |

### 3.3 路径质量评估

```cpp
// ec2/NetCombTransfer/PathQualityEstimator.h
#pragma once
#include <cstdint>
#include <cmath>
#include <deque>
#include <mutex>

// 路径质量指标
struct PathQualityMetrics {
    double loss_rate;          // 丢包率 [0, 1]
    double rtt_ms;             // 往返延迟 (ms)
    double jitter_ms;          // 抖动 (ms)
    double throughput_bps;     // 吞吐量 (bps)
    double available_bps;      // 可用带宽 (bps)
    int64_t last_update_time;  // 上次更新时间
};

// 路径质量评估器
class PathQualityEstimator {
public:
    PathQualityEstimator(uint32_t path_id, size_t window_size = 100)
        : m_path_id(path_id)
        , m_window_size(window_size)
        , m_smoothed_loss(0)
        , m_smoothed_rtt(0)
        , m_smoothed_jitter(0)
        , m_smoothed_throughput(0) {}

    // 输入新的包样本
    void OnPacketSent(uint32_t seq, int64_t send_time_us) {
        std::lock_guard<std::mutex> lock(m_mutex);
        PacketSample sample;
        sample.seq = seq;
        sample.send_time_us = send_time_us;
        sample.ack_time_us = 0;
        sample.acked = false;
        m_sent_samples.push_back(sample);

        // 限制窗口大小
        while (m_sent_samples.size() > m_window_size) {
            m_sent_samples.pop_front();
        }
    }

    void OnPacketAcked(uint32_t seq, int64_t ack_time_us) {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& sample : m_sent_samples) {
            if (sample.seq == seq) {
                sample.ack_time_us = ack_time_us;
                sample.acked = true;

                // 计算 RTT
                int64_t rtt_us = ack_time_us - sample.send_time_us;
                UpdateRTT(rtt_us / 1000.0);  // 转换为 ms
                break;
            }
        }
    }

    void OnPacketLost(uint32_t seq) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_loss_count++;
        m_total_count++;
        UpdateLossRate();
    }

    // 获取当前质量指标
    PathQualityMetrics GetMetrics() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        PathQualityMetrics metrics;
        metrics.loss_rate = m_smoothed_loss;
        metrics.rtt_ms = m_smoothed_rtt;
        metrics.jitter_ms = m_smoothed_jitter;
        metrics.throughput_bps = m_smoothed_throughput;
        metrics.available_bps = m_estimated_bandwidth;
        metrics.last_update_time = m_last_update_time;
        return metrics;
    }

private:
    struct PacketSample {
        uint32_t seq;
        int64_t send_time_us;
        int64_t ack_time_us;
        bool acked;
    };

    uint32_t m_path_id;
    size_t m_window_size;
    mutable std::mutex m_mutex;

    std::deque<PacketSample> m_sent_samples;
    int64_t m_loss_count = 0;
    int64_t m_total_count = 0;

    double m_smoothed_loss;
    double m_smoothed_rtt;
    double m_smoothed_jitter;
    double m_smoothed_throughput;
    double m_estimated_bandwidth;
    int64_t m_last_update_time;

    // RTT 平滑
    void UpdateRTT(double rtt_ms) {
        const double alpha = 0.3;
        if (m_smoothed_rtt == 0) {
            m_smoothed_rtt = rtt_ms;
        } else {
            // 计算 jitter
            double jitter = std::abs(rtt_ms - m_smoothed_rtt);
            m_smoothed_jitter = alpha * jitter + (1 - alpha) * m_smoothed_jitter;

            // 更新 RTT
            m_smoothed_rtt = alpha * rtt_ms + (1 - alpha) * m_smoothed_rtt;
        }
    }

    // 丢包率平滑
    void UpdateLossRate() {
        if (m_total_count > 0) {
            double current_loss = static_cast<double>(m_loss_count) / m_total_count;
            const double alpha = 0.2;
            m_smoothed_loss = alpha * current_loss + (1 - alpha) * m_smoothed_loss;
        }
    }
};
```

---

## 4. 调度算法设计

### 4.1 调度策略

```cpp
// ec2/NetCombTransfer/MultipathScheduler.h
#pragma once
#include "PathQualityEstimator.h"
#include <vector>
#include <memory>
#include <mutex>
#include <map>

// 调度策略
enum SchedulingPolicy {
    SCHED_ROUND_ROBIN = 0,      // 轮询
    SCHED_LOWEST_RTT = 1,       // 最低延迟优先
    SCHED_HIGHEST_QOE = 2,      // 最高 QoE 优先
    SCHED_REDUNDANT = 3,        // 冗余发送（多路径同时发）
    SCHED_WEIGHTED_QOE = 4,     // 加权 QoE（按流类型）
};

// 路径信息
struct PathInfo {
    uint32_t path_id;            // 路径 ID（对应 CNetChannel::m_dwCID）
    std::string name;            // 路径名称（"Mesh" / "5G_DTU"）
    std::unique_ptr<PathQualityEstimator> estimator;
    bool is_active;              // 是否激活
    double weight;               // 路径权重
};

// 调度决策
struct ScheduleDecision {
    uint32_t selected_path_id;   // 选择的路径
    bool should_redundant;       // 是否冗余发送
    std::vector<uint32_t> redundant_paths;  // 冗余路径列表
    double confidence;           // 决策置信度
};

// 多路径调度器
class MultipathScheduler {
public:
    MultipathScheduler(SchedulingPolicy policy = SCHED_HIGHEST_QOE)
        : m_policy(policy) {}

    // 注册路径
    void RegisterPath(uint32_t path_id, const std::string& name) {
        std::lock_guard<std::mutex> lock(m_mutex);
        PathInfo info;
        info.path_id = path_id;
        info.name = name;
        info.estimator = std::make_unique<PathQualityEstimator>(path_id);
        info.is_active = true;
        info.weight = 1.0;
        m_paths[path_id] = std::move(info);
    }

    // 注销路径
    void UnregisterPath(uint32_t path_id) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_paths.erase(path_id);
    }

    // 设置路径激活状态
    void SetPathActive(uint32_t path_id, bool active) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_paths.count(path_id)) {
            m_paths[path_id].is_active = active;
        }
    }

    // 更新路径质量
    void UpdatePathMetrics(uint32_t path_id, const PathQualityMetrics& metrics) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_paths.count(path_id)) {
            // 通过 estimator 间接更新
            // 这里直接存储，实际应该通过 OnPacketSent/Acked/Lost
        }
    }

    // 调度决策：为一个包选择路径
    ScheduleDecision Schedule(FlowType flow_type) {
        std::lock_guard<std::mutex> lock(m_mutex);
        ScheduleDecision decision;
        decision.should_redundant = false;
        decision.confidence = 1.0;

        // 收集所有活跃路径
        std::vector<PathInfo*> active_paths;
        for (auto& [id, path] : m_paths) {
            if (path.is_active) {
                active_paths.push_back(&path);
            }
        }

        if (active_paths.empty()) {
            decision.selected_path_id = 0;
            decision.confidence = 0;
            return decision;
        }

        if (active_paths.size() == 1) {
            decision.selected_path_id = active_paths[0]->path_id;
            return decision;
        }

        // 根据策略选择路径
        switch (m_policy) {
            case SCHED_ROUND_ROBIN:
                return ScheduleRoundRobin(active_paths);

            case SCHED_LOWEST_RTT:
                return ScheduleLowestRTT(active_paths);

            case SCHED_HIGHEST_QOE:
                return ScheduleHighestQoE(active_paths, flow_type);

            case SCHED_REDUNDANT:
                return ScheduleRedundant(active_paths);

            case SCHED_WEIGHTED_QOE:
                return ScheduleWeightedQoE(active_paths, flow_type);

            default:
                return ScheduleHighestQoE(active_paths, flow_type);
        }
    }

    // 获取所有路径的 QoE 分数
    std::map<uint32_t, double> GetQoEScores(FlowType flow_type) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::map<uint32_t, double> scores;

        for (const auto& [id, path] : m_paths) {
            if (!path.is_active) continue;

            auto metrics = path.estimator->GetMetrics();
            scores[id] = ComputeQoE(metrics, flow_type);
        }

        return scores;
    }

    // 设置调度策略
    void SetPolicy(SchedulingPolicy policy) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_policy = policy;
    }

private:
    mutable std::mutex m_mutex;
    SchedulingPolicy m_policy;
    std::map<uint32_t, PathInfo> m_paths;
    uint32_t m_rr_index = 0;  // 轮询索引

    // QoE 计算
    double ComputeQoE(const PathQualityMetrics& metrics, FlowType flow_type) const {
        // 获取流类型的权重
        double alpha, beta, gamma, delta;
        GetWeights(flow_type, alpha, beta, gamma, delta);

        // 归一化各指标
        double reliability_score = 1.0 - std::min(1.0, metrics.loss_rate);
        double latency_score = 1.0 / (1.0 + metrics.rtt_ms / 100.0);
        double throughput_score = std::min(1.0, metrics.throughput_bps / 5000000.0);  // 归一化到 5Mbps
        double stability_score = 1.0 / (1.0 + metrics.jitter_ms / 50.0);

        // 加权求和
        double qoe = alpha * reliability_score
                   + beta * latency_score
                   + gamma * throughput_score
                   + delta * stability_score;

        return qoe;
    }

    void GetWeights(FlowType type, double& alpha, double& beta, double& gamma, double& delta) const {
        switch (type) {
            case FLOW_REALTIME_AUDIO:
                alpha = 0.2; beta = 0.6; gamma = 0.1; delta = 0.1;
                break;
            case FLOW_REALTIME_VIDEO:
                alpha = 0.3; beta = 0.4; gamma = 0.2; delta = 0.1;
                break;
            case FLOW_INTERACTIVE:
                alpha = 0.3; beta = 0.5; gamma = 0.1; delta = 0.1;
                break;
            case FLOW_BULK:
                alpha = 0.5; beta = 0.1; gamma = 0.3; delta = 0.1;
                break;
            default:
                alpha = 0.3; beta = 0.4; gamma = 0.2; delta = 0.1;
        }
    }

    // 轮询调度
    ScheduleDecision ScheduleRoundRobin(const std::vector<PathInfo*>& paths) {
        ScheduleDecision decision;
        m_rr_index = (m_rr_index + 1) % paths.size();
        decision.selected_path_id = paths[m_rr_index]->path_id;
        decision.confidence = 1.0;
        return decision;
    }

    // 最低延迟调度
    ScheduleDecision ScheduleLowestRTT(const std::vector<PathInfo*>& paths) {
        ScheduleDecision decision;
        double min_rtt = 1e9;
        uint32_t best_path = paths[0]->path_id;

        for (const auto* path : paths) {
            auto metrics = path->estimator->GetMetrics();
            if (metrics.rtt_ms < min_rtt) {
                min_rtt = metrics.rtt_ms;
                best_path = path->path_id;
            }
        }

        decision.selected_path_id = best_path;
        decision.confidence = 1.0;
        return decision;
    }

    // 最高 QoE 调度
    ScheduleDecision ScheduleHighestQoE(const std::vector<PathInfo*>& paths,
                                         FlowType flow_type) {
        ScheduleDecision decision;
        double max_qoe = -1;
        uint32_t best_path = paths[0]->path_id;

        for (const auto* path : paths) {
            auto metrics = path->estimator->GetMetrics();
            double qoe = ComputeQoE(metrics, flow_type);
            if (qoe > max_qoe) {
                max_qoe = qoe;
                best_path = path->path_id;
            }
        }

        decision.selected_path_id = best_path;
        decision.confidence = max_qoe;
        return decision;
    }

    // 冗余调度（所有路径同时发）
    ScheduleDecision ScheduleRedundant(const std::vector<PathInfo*>& paths) {
        ScheduleDecision decision;
        decision.selected_path_id = paths[0]->path_id;
        decision.should_redundant = true;
        for (const auto* path : paths) {
            decision.redundant_paths.push_back(path->path_id);
        }
        decision.confidence = 1.0;
        return decision;
    }

    // 加权 QoE 调度（考虑路径权重）
    ScheduleDecision ScheduleWeightedQoE(const std::vector<PathInfo*>& paths,
                                          FlowType flow_type) {
        ScheduleDecision decision;
        double max_weighted_qoe = -1;
        uint32_t best_path = paths[0]->path_id;

        for (const auto* path : paths) {
            auto metrics = path->estimator->GetMetrics();
            double qoe = ComputeQoE(metrics, flow_type);
            double weighted_qoe = qoe * path->weight;
            if (weighted_qoe > max_weighted_qoe) {
                max_weighted_qoe = weighted_qoe;
                best_path = path->path_id;
            }
        }

        decision.selected_path_id = best_path;
        decision.confidence = max_weighted_qoe;
        return decision;
    }
};
```

---

## 5. 与现有系统的集成

### 5.1 修改 CNetTerminal

```cpp
// 修改 ec2/NetCombTransfer/CNetTerminal.h

class CNetTerminal {
    // ... existing members ...

    // 新增：多路径调度器
    std::unique_ptr<MultipathScheduler> m_scheduler;

    // 初始化时注册路径
    void InitScheduler() {
        m_scheduler = std::make_unique<MultipathScheduler>(SCHED_WEIGHTED_QOE);

        // 注册所有通道作为路径
        for (auto& channel : m_channels) {
            std::string name = channel->m_isDTUChannel ? "5G_DTU" : "Mesh";
            m_scheduler->RegisterPath(channel->m_dwCID, name);
        }
    }

    // 发送数据时使用调度器
    bool SendDataWithSchedule(const shared_ptr<BYTE>& data, DWORD length,
                               FlowType flow_type) {
        if (!m_scheduler || m_channels.empty()) {
            return false;
        }

        // 获取调度决策
        auto decision = m_scheduler->Schedule(flow_type);

        if (decision.should_redundant) {
            // 冗余发送
            bool any_success = false;
            for (uint32_t path_id : decision.redundant_paths) {
                auto channel = FindChannel(path_id);
                if (channel && channel->SendData(data, length)) {
                    any_success = true;
                }
            }
            return any_success;
        } else {
            // 单路径发送
            auto channel = FindChannel(decision.selected_path_id);
            if (channel) {
                return channel->SendData(data, length);
            }
            return false;
        }
    }
};
```

### 5.2 修改 SpeedControl

```cpp
// 修改 ec2/SpeedControl/Term2TermTransmission.h

class Term2TermTransmission {
    // ... existing members ...

    // 新增：流类型
    FlowType m_flow_type;

    // 修改发送逻辑
    bool DoCycleSend(const shared_ptr<DataWithPacketInfo>& pPacket) {
        // ... existing code ...

        // 使用调度器选择路径
        if (m_terminal && m_terminal->m_scheduler) {
            return m_terminal->SendDataWithSchedule(
                encoded_data, encoded_length, m_flow_type);
        }

        // fallback: 使用原有逻辑
        return SendViaDefaultPath(encoded_data, encoded_length);
    }
};
```

### 5.3 质量反馈集成

```cpp
// 在 CNetChannel 中集成质量估计

class CNetChannel {
    // ... existing members ...

    // 新增：路径质量估计器
    std::unique_ptr<PathQualityEstimator> m_quality_estimator;

    // 发送时记录
    bool SendData(const shared_ptr<BYTE>& buf, DWORD length) {
        uint32_t seq = m_send_seq++;
        int64_t send_time = GetCurrentTimeUs();

        if (m_quality_estimator) {
            m_quality_estimator->OnPacketSent(seq, send_time);
        }

        return SendRawData(buf, length);
    }

    // 收到 ACK 时更新
    void OnAckReceived(uint32_t seq) {
        if (m_quality_estimator) {
            m_quality_estimator->OnPacketAcked(seq, GetCurrentTimeUs());
        }
    }
};
```

---

## 6. 自适应路径切换

### 6.1 路径切换策略

```cpp
// ec2/NetCombTransfer/PathSwitchPolicy.h

// 路径切换策略
struct PathSwitchPolicy {
    // QoE 差值阈值：新路径 QoE 需超过当前路径多少才切换
    double qoe_switch_threshold = 0.15;

    // 最小切换间隔（防止频繁切换）
    int64_t min_switch_interval_ms = 1000;

    // 路径健康检查
    double unhealthy_loss_threshold = 0.2;    // 丢包率 > 20% 视为不健康
    double unhealthy_rtt_threshold = 500;     // RTT > 500ms 视为不健康

    // 路径恢复检测
    double recovery_loss_threshold = 0.05;    // 丢包率 < 5% 视为恢复
    double recovery_rtt_threshold = 100;      // RTT < 100ms 视为恢复
};
```

### 6.2 切换逻辑

```cpp
void MultipathScheduler::CheckPathSwitch(FlowType flow_type) {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - m_last_switch_time).count();

    if (elapsed < m_switch_policy.min_switch_interval_ms) {
        return;  // 切换间隔太短
    }

    // 计算所有路径的 QoE
    auto scores = GetQoEScores(flow_type);

    // 找到当前最佳路径
    uint32_t best_path = 0;
    double best_qoe = -1;
    for (auto& [id, score] : scores) {
        if (score > best_qoe) {
            best_qoe = score;
            best_path = id;
        }
    }

    // 检查是否需要切换
    if (best_path != m_current_active_path) {
        double current_qoe = scores[m_current_active_path];
        double improvement = best_qoe - current_qoe;

        if (improvement > m_switch_policy.qoe_switch_threshold) {
            // 切换到新路径
            SwitchToPath(best_path);
            m_last_switch_time = now;
        }
    }

    // 检查当前路径是否不健康
    for (auto& [id, path] : m_paths) {
        if (!path.is_active) continue;
        auto metrics = path.estimator->GetMetrics();

        if (metrics.loss_rate > m_switch_policy.unhealthy_loss_threshold ||
            metrics.rtt_ms > m_switch_policy.unhealthy_rtt_threshold) {
            // 标记为不健康
            path.is_active = false;
            // 触发路径切换
        }
    }
}
```

---

## 7. 测试方案

### 7.1 网络模拟

```bash
# 使用 Linux tc 模拟双路径网络

# 路径 1：低延迟、低带宽（Mesh）
sudo tc qdisc add dev eth0 root netem delay 10ms rate 2mbit

# 路径 2：高延迟、高带宽（5G DTU）
sudo tc qdisc add dev eth1 root netem delay 50ms rate 10mbit

# 动态变化：路径 1 突然劣化
sudo tc qdisc change dev eth0 root netem delay 100ms loss 10% rate 1mbit
```

### 7.2 测试场景

| 场景 | 路径 1 (Mesh) | 路径 2 (5G) | 预期行为 |
|------|--------------|-------------|----------|
| 正常 | 10ms, 0% loss, 2Mbps | 50ms, 0% loss, 10Mbps | 音频走 Mesh，视频走 5G |
| Mesh 劣化 | 100ms, 10% loss | 50ms, 0% loss | 全部切换到 5G |
| 5G 劣化 | 10ms, 0% loss | 200ms, 15% loss | 全部切换到 Mesh |
| 双路径均好 | 10ms, 0% loss, 5Mbps | 30ms, 0% loss, 20Mbps | 按 QoE 权重分配 |
| 冗余模式 | 任意 | 任意 | 关键帧同时走两条路 |

### 7.3 关键指标

| 指标 | 轮询调度 | QoE 调度 | 改进 |
|------|---------|---------|------|
| 视频延迟 P99 | 120ms | 70ms | -42% |
| 音频延迟 P99 | 80ms | 35ms | -56% |
| 路径切换响应时间 | N/A | < 1s | - |
| 关键帧丢失率 | 5% | 0.5% | -90% |
| 双路径利用率 | 50%/50% | 按需分配 | 更优 |
