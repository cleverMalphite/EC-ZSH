# Phase 2: 多流拥塞控制（Copa / GCC / Multi-Flow CC）

## 1. 目标

打破传统单流 BBR 的局限，为不同模态的数据流（语音/视频/文件）设计差异化的拥塞控制策略，实现多流公平竞争和带宽分配。

**面试故事**："我设计了多流感知的拥塞控制框架，为不同模态的数据流分配不同的 CC 策略和权重——实时音视频使用延迟敏感的 Copa 算法，文件传输使用带宽最优的 BBR，在多流竞争场景下实现了公平的带宽分配和更低的交互延迟。"

---

## 2. 现状分析

### 当前 BBR 实现
- 完整的 BBR v1 状态机：STARTUP → DRAIN → PROBE_BW → PROBE_RTT
- 带宽采样器：`bbr_bandwidth_sampler_t`，基于 skiplist 追踪包级记录
- RTT 估计：`bbr_rtt_stat_t`，含 min_rtt / smoothed_rtt / mean_deviation
- 拥塞窗口：`congestion_window`，范围 [min_congestion_window, max_congestion_window]
- Pacing rate：基于 gain cycling 的 8 阶段增益

### 问题
1. **单流视角**：BBR 只看到自己的流，不知道其他流的存在
2. **延迟不敏感**：BBR 主要基于带宽估计，对实时延迟变化响应慢
3. **多流竞争**：音视频流和文件流共享同一条路径时，BBR 会让文件流占满带宽
4. **无流分类**：所有数据走同一个 CC，没有区分实时流和弹性流

### 关键代码位置
```
Term2TermQos/bbr_controller.h        → BBR 状态机和核心算法
Term2TermQos/EndToEndQos.h           → 每终端 QoS 实体
Term2TermQos/Remote2SelfQosUnit.h    → 接收端 QoS 处理
Term2TermQos/SelfToRemoteQosUnit.h   → 发送端 QoS 处理
SpeedControl/Term2TermTransmission.h → Pacing 控制
SpeedControl/QosStructInterfaceInfo.h → QoS 数据结构
```

---

## 3. 多流 CC 架构设计

### 3.1 整体架构

```
┌─────────────────────────────────────────────────────────┐
│                    Application Layer                     │
│   Audio Stream    Video Stream    File Transfer    ...   │
│   (实时流)        (实时流)        (弹性流)               │
└───────┬───────────────┬───────────────┬─────────────────┘
        │               │               │
        ▼               ▼               ▼
┌─────────────────────────────────────────────────────────┐
│              Flow Classifier & Scheduler                 │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐              │
│  │ Flow 0   │  │ Flow 1   │  │ Flow 2   │              │
│  │ Audio    │  │ Video    │  │ File     │              │
│  │ Weight=5 │  │ Weight=3 │  │ Weight=1 │              │
│  │ CC=Copa  │  │ CC=Copa  │  │ CC=BBR   │              │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘              │
│       │              │              │                    │
│       ▼              ▼              ▼                    │
│  ┌─────────────────────────────────────────┐            │
│  │       Bandwidth Allocator                │            │
│  │  按权重分配各流的发送速率上限            │            │
│  └─────────────────────────────────────────┘            │
└─────────────────────────────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────────────────────────────┐
│              SpeedControl / Pacing Layer                 │
│  ┌─────────────────────────────────────────┐            │
│  │  Token Bucket Pacer (per flow)           │            │
│  └─────────────────────────────────────────┘            │
└─────────────────────────────────────────────────────────┘
        │
        ▼
┌─────────────────────────────────────────────────────────┐
│              NetCombTransfer / Transport                 │
└─────────────────────────────────────────────────────────┘
```

### 3.2 流分类策略

```cpp
enum FlowType {
    FLOW_TYPE_REALTIME_AUDIO = 0,   // 实时音频：延迟最敏感
    FLOW_TYPE_REALTIME_VIDEO = 1,   // 实时视频：延迟敏感 + 带宽需求
    FLOW_TYPE_INTERACTIVE   = 2,   // 交互式数据（如控制消息）
    FLOW_TYPE_BULK          = 3,   // 批量传输（文件）：带宽优先
};

struct FlowInfo {
    DWORD flow_id;
    DWORD dest_tid;
    FlowType type;
    int weight;                    // 调度权重
    CCAlgorithm cc_algo;           // 使用的 CC 算法
    double min_rate_bps;           // 最低保障速率
    double max_rate_bps;           // 最高速率限制
};
```

### 3.3 CC 算法选择

| 流类型 | CC 算法 | 原因 |
|--------|---------|------|
| 实时音频 | Copa | 基于延迟，对排队延迟敏感 |
| 实时视频 | Copa / GCC | 需要低延迟 + 一定带宽 |
| 交互式 | Copa | 低延迟优先 |
| 批量传输 | BBR | 带宽利用率优先 |

---

## 4. Copa 拥塞控制算法

### 4.1 算法原理

Copa 是一种基于延迟的拥塞控制算法，核心思想：
- **目标**：最小化排队延迟
- **速率公式**：`rate = 1 / (c * standing_rtt)`
  - `standing_rtt`：当前 RTT 相对于 min_rtt 的比值
  - `c`：竞争力参数（默认 0.04）
- **竞争力模式**：当与 loss-based CC 共存时，增大 c 以抢占更多带宽

### 4.2 Copa 状态机

```
┌──────────┐    RTT 稳定    ┌──────────┐
│  START   │───────────────→│  STEADY  │
└──────────┘                └──────────┘
     │                           │
     │ RTT 上升                  │ RTT 上升
     ▼                           ▼
┌──────────┐                ┌──────────┐
│  PROBE   │←───────────────│  REACT   │
└──────────┘    RTT 下降     └──────────┘
```

### 4.3 Copa 控制器实现

```cpp
// ec2/Term2TermQos/copa_controller.h
#pragma once
#include <cstdint>
#include <algorithm>

// Copa 拥塞控制状态
enum CopaState {
    COPA_START = 0,
    COPA_STEADY = 1,
    COPA_REACT = 2,
    COPA_PROBE = 3,
};

// Copa 配置
struct CopaConfig {
    double delta = 0.04;              // 竞争力参数
    double delta_range[2] = {0.01, 0.1};  // delta 范围
    int64_t min_rtt_filter_len_ms = 10000;  // min RTT 滤波器长度
    int64_t standing_rtt_filter_len_ms = 2000;  // standing RTT 滤波器长度
    double rate_scale = 1.0;          // 速率缩放因子
    bool use_deterministic_loss = false;
    uint32_t initial_cwnd = 10;       // 初始窗口
    uint32_t min_cwnd = 4;            // 最小窗口
    uint32_t max_cwnd = 10000;        // 最大窗口
};

// Copa 控制器
struct CopaController {
    CopaState state;
    CopaConfig config;

    // RTT 测量
    int64_t min_rtt_us;               // 最小 RTT (微秒)
    int64_t standing_rtt_us;          // 当前 standing RTT
    int64_t last_rtt_sample_us;       // 最新 RTT 样本

    // 速率和窗口
    double pacing_rate_bps;           // 当前 pacing 速率
    uint32_t congestion_window;       // 拥塞窗口
    double current_delta;             // 当前使用的 delta

    // 统计
    int64_t last_update_time_us;      // 上次更新时间
    uint64_t bytes_sent;              // 已发送字节数
    uint64_t bytes_acked;             // 已确认字节数
    uint32_t consecutive_rtt_increases;  // 连续 RTT 上升次数

    // 初始化
    void Init(const CopaConfig& cfg) {
        config = cfg;
        state = COPA_START;
        min_rtt_us = INT64_MAX;
        standing_rtt_us = 0;
        last_rtt_sample_us = 0;
        pacing_rate_bps = 0;
        congestion_window = cfg.initial_cwnd;
        current_delta = cfg.delta;
        last_update_time_us = 0;
        bytes_sent = 0;
        bytes_acked = 0;
        consecutive_rtt_increases = 0;
    }

    // 处理 ACK 反馈
    void OnAck(uint32_t bytes, int64_t rtt_sample_us, int64_t now_us) {
        bytes_acked += bytes;
        last_rtt_sample_us = rtt_sample_us;

        // 更新 min RTT（指数衰减滤波器）
        if (rtt_sample_us < min_rtt_us) {
            min_rtt_us = rtt_sample_us;
        }

        // 更新 standing RTT（指数移动平均）
        double alpha = 0.1;
        if (standing_rtt_us == 0) {
            standing_rtt_us = rtt_sample_us;
        } else {
            standing_rtt_us = static_cast<int64_t>(
                alpha * rtt_sample_us + (1 - alpha) * standing_rtt_us);
        }

        // 计算目标速率
        UpdateRate();

        last_update_time_us = now_us;
    }

    // 处理丢包
    void OnLoss(uint32_t lost_bytes, int64_t now_us) {
        // Copa 不直接响应丢包，而是通过 RTT 变化间接响应
        // 但如果检测到持续丢包，可以增大 delta 以更激进
    }

    // 更新速率和窗口
    void UpdateRate() {
        if (min_rtt_us <= 0 || standing_rtt_us <= 0) return;

        // standing_rtt_ratio = standing_rtt / min_rtt
        double standing_rtt_ratio = static_cast<double>(standing_rtt_us) / min_rtt_us;

        // Copa 核心公式：rate = 1 / (delta * standing_rtt_ratio)
        double target_rate_factor = 1.0 / (current_delta * standing_rtt_ratio);

        // 检测 RTT 趋势
        if (standing_rtt_ratio > 1.5) {
            // RTT 显著上升，进入 REACT 状态
            state = COPA_REACT;
            consecutive_rtt_increases++;
        } else if (standing_rtt_ratio < 1.1) {
            // RTT 接近最小值，稳态
            state = COPA_STEADY;
            consecutive_rtt_increases = 0;
        }

        // 在 REACT 状态下减小窗口
        if (state == COPA_REACT) {
            congestion_window = std::max(
                config.min_cwnd,
                static_cast<uint32_t>(congestion_window * 0.7));
        } else {
            // 正常增长
            congestion_window = std::min(
                config.max_cwnd,
                static_cast<uint32_t>(congestion_window * target_rate_factor));
        }

        // 计算 pacing rate
        // rate = cwnd * 1000000 / rtt (bytes per second)
        if (standing_rtt_us > 0) {
            pacing_rate_bps = static_cast<double>(congestion_window) * 8.0 * 1000000.0 / standing_rtt_us;
        }
    }

    // 获取当前 pacing 速率
    double GetPacingRateBps() const {
        return pacing_rate_bps;
    }

    // 获取拥塞窗口
    uint32_t GetCongestionWindow() const {
        return congestion_window;
    }

    // 设置竞争力参数（与 BBR 共存时增大）
    void SetCompetitiveness(double delta) {
        current_delta = std::max(config.delta_range[0],
                       std::min(config.delta_range[1], delta));
    }
};
```

---

## 5. GCC 拥塞控制算法（Google Congestion Control）

### 5.1 算法原理

GCC 是 WebRTC 使用的延迟梯度拥塞控制：
- **核心思想**：通过 inter-arrival delay 的变化趋势判断拥塞
- **优点**：对延迟变化非常敏感，适合实时媒体
- **两层控制**：
  - 接收端：延迟梯度检测器（Overuse Detector）
  - 发送端：速率控制器（AIMD 或基于延迟的调节）

### 5.2 延迟梯度检测器

```cpp
// ec2/Term2TermQos/gcc_delay_detector.h
#pragma once
#include <cstdint>
#include <cmath>

// GCC 过载状态
enum GccOveruseState {
    GCC_OVERUSE_NORMAL = 0,    // 正常
    GCC_OVERUSE_UNDERUSE = 1,  // 欠载（带宽有余）
    GCC_OVERUSE_OVERUSE = 2,   // 过载（拥塞）
};

// GCC 延迟梯度检测器
struct GccDelayDetector {
    // 参数
    double eta_up = 0.008;         // 上升阈值
    double eta_down = -0.008;      // 下降阈值
    double gamma = 0.05;           // 梯度平滑因子
    double theta = 12.5;           // 过载检测阈值（ms）

    // 状态
    double smoothed_gradient;       // 平滑后的延迟梯度
    int64_t last_arrival_time_us;   // 上一个包的到达时间
    int64_t last_send_time_us;      // 上一个包的发送时间
    GccOveruseState overuse_state;  // 当前过载状态

    // 初始化
    void Init() {
        smoothed_gradient = 0;
        last_arrival_time_us = 0;
        last_send_time_us = 0;
        overuse_state = GCC_OVERUSE_NORMAL;
    }

    // 输入一个包的收发时间，更新过载状态
    // send_time_us: 包的发送时间戳
    // arrival_time_us: 包的接收时间戳
    GccOveruseState Update(int64_t send_time_us, int64_t arrival_time_us) {
        if (last_arrival_time_us == 0) {
            // 第一个包，初始化
            last_arrival_time_us = arrival_time_us;
            last_send_time_us = send_time_us;
            return overuse_state;
        }

        // 计算 inter-arrival delay
        int64_t arrival_delta = arrival_time_us - last_arrival_time_us;
        int64_t send_delta = send_time_us - last_send_time_us;

        // 延迟梯度 = arrival_delta - send_delta
        // 正值表示排队延迟增加，负值表示减少
        int64_t delay_gradient = arrival_delta - send_delta;

        // 指数平滑
        smoothed_gradient = gamma * delay_gradient + (1 - gamma) * smoothed_gradient;

        // 状态判断
        GccOveruseState new_state;
        if (smoothed_gradient > theta) {
            new_state = GCC_OVERUSE_OVERUSE;
        } else if (smoothed_gradient < -theta) {
            new_state = GCC_OVERUSE_UNDERUSE;
        } else {
            new_state = GCC_OVERUSE_NORMAL;
        }

        // 更新状态（需要连续检测才切换）
        if (new_state == GCC_OVERUSE_OVERUSE && overuse_state != GCC_OVERUSE_OVERUSE) {
            overuse_state = GCC_OVERUSE_OVERUSE;
        } else if (new_state == GCC_OVERUSE_UNDERUSE && overuse_state != GCC_OVERUSE_UNDERUSE) {
            overuse_state = GCC_OVERUSE_UNDERUSE;
        } else if (new_state == GCC_OVERUSE_NORMAL) {
            overuse_state = GCC_OVERUSE_NORMAL;
        }

        last_arrival_time_us = arrival_time_us;
        last_send_time_us = send_time_us;

        return overuse_state;
    }
};
```

### 5.3 GCC 速率控制器（AIMD）

```cpp
// ec2/Term2TermQos/gcc_rate_controller.h
#pragma once
#include "gcc_delay_detector.h"
#include <algorithm>
#include <cmath>

// GCC 速率控制器
struct GccRateController {
    // 参数
    double increase_factor = 1.05;     // 正常增长因子 (5%)
    double decrease_factor = 0.85;     // 过载减少因子 (15%)
    double min_rate_bps = 100000;      // 最低速率 100kbps
    double max_rate_bps = 8000000;     // 最高速率 8Mbps

    // 状态
    double current_rate_bps;           // 当前速率
    GccDelayDetector delay_detector;   // 延迟检测器

    // 初始化
    void Init(double initial_rate_bps = 3000000) {
        current_rate_bps = initial_rate_bps;
        delay_detector.Init();
    }

    // 输入包信息，更新速率
    double Update(int64_t send_time_us, int64_t arrival_time_us) {
        GccOveruseState state = delay_detector.Update(send_time_us, arrival_time_us);

        switch (state) {
            case GCC_OVERUSE_NORMAL:
                // 正常状态，缓慢增加速率
                current_rate_bps *= increase_factor;
                break;

            case GCC_OVERUSE_UNDERUSE:
                // 欠载状态，增加速率（更激进）
                current_rate_bps *= (increase_factor * 1.5);
                break;

            case GCC_OVERUSE_OVERUSE:
                // 过载状态，减少速率
                current_rate_bps *= decrease_factor;
                break;
        }

        // 限幅
        current_rate_bps = std::max(min_rate_bps,
                          std::min(max_rate_bps, current_rate_bps));

        return current_rate_bps;
    }

    // 获取当前速率
    double GetRateBps() const {
        return current_rate_bps;
    }
};
```

---

## 6. Multi-Flow 带宽分配器

### 6.1 核心设计

```cpp
// ec2/Term2TermQos/MultiFlowAllocator.h
#pragma once
#include <vector>
#include <memory>
#include <mutex>
#include <cstdint>
#include <algorithm>

// 流类型
enum FlowType {
    FLOW_REALTIME_AUDIO = 0,
    FLOW_REALTIME_VIDEO = 1,
    FLOW_INTERACTIVE = 2,
    FLOW_BULK = 3,
};

// CC 算法类型
enum CCAlgorithm {
    CC_BBR = 0,
    CC_COPA = 1,
    CC_GCC = 2,
};

// 流描述符
struct FlowDescriptor {
    uint32_t flow_id;
    uint32_t dest_tid;
    FlowType type;
    CCAlgorithm cc_algo;
    int weight;                    // 调度权重
    double min_rate_bps;           // 最低保障速率
    double max_rate_bps;           // 最高速率
    double current_rate_bps;       // 当前分配速率
    double measured_rtt_ms;        // 测量的 RTT
    double measured_loss_rate;     // 测量的丢包率
};

// 带宽分配结果
struct BandwidthAllocation {
    uint32_t flow_id;
    double allocated_rate_bps;     // 分配的速率
    double pacing_rate_bps;        // pacing 速率
    uint32_t congestion_window;    // 拥塞窗口
};

// 多流带宽分配器
class MultiFlowAllocator {
public:
    MultiFlowAllocator(double total_bandwidth_bps = 100000000)  // 默认 100Mbps
        : m_total_bandwidth(total_bandwidth_bps)
        , m_available_bandwidth(total_bandwidth_bps) {}

    // 注册一个流
    uint32_t RegisterFlow(FlowType type, uint32_t dest_tid,
                          double min_rate_bps, double max_rate_bps) {
        std::lock_guard<std::mutex> lock(m_mutex);

        FlowDescriptor flow;
        flow.flow_id = m_next_flow_id++;
        flow.dest_tid = dest_tid;
        flow.type = type;
        flow.weight = GetDefaultWeight(type);
        flow.cc_algo = GetDefaultCC(type);
        flow.min_rate_bps = min_rate_bps;
        flow.max_rate_bps = max_rate_bps;
        flow.current_rate_bps = min_rate_bps;
        flow.measured_rtt_ms = 0;
        flow.measured_loss_rate = 0;

        m_flows.push_back(flow);
        return flow.flow_id;
    }

    // 注销一个流
    void UnregisterFlow(uint32_t flow_id) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_flows.erase(
            std::remove_if(m_flows.begin(), m_flows.end(),
                [flow_id](const FlowDescriptor& f) { return f.flow_id == flow_id; }),
            m_flows.end());
    }

    // 更新流的测量信息
    void UpdateFlowMetrics(uint32_t flow_id, double rtt_ms, double loss_rate,
                           double cc_suggested_rate_bps) {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& flow : m_flows) {
            if (flow.flow_id == flow_id) {
                flow.measured_rtt_ms = rtt_ms;
                flow.measured_loss_rate = loss_rate;
                // CC 算法建议的速率作为参考
                flow.current_rate_bps = cc_suggested_rate_bps;
                break;
            }
        }
    }

    // 执行带宽分配
    std::vector<BandwidthAllocation> Allocate() {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<BandwidthAllocation> allocations;

        if (m_flows.empty()) return allocations;

        // 1. 计算各流的需求和权重
        double total_weight = 0;
        double total_demand = 0;
        for (const auto& flow : m_flows) {
            total_weight += flow.weight;
            total_demand += flow.current_rate_bps;
        }

        // 2. 按权重分配可用带宽
        double available = m_total_bandwidth;

        // 3. 先保障最低速率
        for (auto& flow : m_flows) {
            available -= flow.min_rate_bps;
        }
        available = std::max(0.0, available);

        // 4. 按权重分配剩余带宽
        for (auto& flow : m_flows) {
            BandwidthAllocation alloc;
            alloc.flow_id = flow.flow_id;

            double weight_share = (total_weight > 0) ?
                (flow.weight / total_weight) * available : 0;

            // 最终分配 = 最低保障 + 权重份额
            alloc.allocated_rate_bps = flow.min_rate_bps + weight_share;

            // 限幅
            alloc.allocated_rate_bps = std::max(flow.min_rate_bps,
                std::min(flow.max_rate_bps, alloc.allocated_rate_bps));

            // 设置 pacing 和 cwnd
            alloc.pacing_rate_bps = alloc.allocated_rate_bps;
            if (flow.measured_rtt_ms > 0) {
                alloc.congestion_window = static_cast<uint32_t>(
                    alloc.allocated_rate_bps * flow.measured_rtt_ms / 8000.0);
            } else {
                alloc.congestion_window = 100;
            }

            allocations.push_back(alloc);
        }

        return allocations;
    }

    // 设置总带宽（动态调整）
    void SetTotalBandwidth(double bandwidth_bps) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_total_bandwidth = bandwidth_bps;
    }

    // 获取流数量
    size_t GetFlowCount() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_flows.size();
    }

private:
    mutable std::mutex m_mutex;
    double m_total_bandwidth;
    double m_available_bandwidth;
    uint32_t m_next_flow_id = 1;
    std::vector<FlowDescriptor> m_flows;

    // 默认权重
    int GetDefaultWeight(FlowType type) const {
        switch (type) {
            case FLOW_REALTIME_AUDIO: return 5;
            case FLOW_REALTIME_VIDEO: return 3;
            case FLOW_INTERACTIVE:    return 4;
            case FLOW_BULK:           return 1;
            default: return 1;
        }
    }

    // 默认 CC 算法
    CCAlgorithm GetDefaultCC(FlowType type) const {
        switch (type) {
            case FLOW_REALTIME_AUDIO: return CC_COPA;
            case FLOW_REALTIME_VIDEO: return CC_COPA;
            case FLOW_INTERACTIVE:    return CC_COPA;
            case FLOW_BULK:           return CC_BBR;
            default: return CC_BBR;
        }
    }
};
```

---

## 7. 与现有系统的集成

### 7.1 修改 EndToEndQos

```cpp
// 修改 ec2/Term2TermQos/EndToEndQos.h

// 新增成员
class EndToEndQos {
    // ... existing members ...

    // 多流 CC 支持
    CopaController m_copa_controller;          // Copa 控制器
    GccRateController m_gcc_controller;        // GCC 控制器
    CCAlgorithm m_active_cc_algo;              // 当前使用的 CC 算法
    FlowType m_flow_type;                      // 流类型

    // 切换 CC 算法
    void SetCCAlgorithm(CCAlgorithm algo, FlowType type) {
        m_active_cc_algo = algo;
        m_flow_type = type;
        switch (algo) {
            case CC_COPA:
                m_copa_controller.Init(CopaConfig{});
                break;
            case CC_GCC:
                m_gcc_controller.Init();
                break;
            case CC_BBR:
                // 使用已有的 BBR 控制器
                break;
        }
    }

    // 获取当前建议速率（根据使用的 CC 算法）
    double GetSuggestedRateBps() const {
        switch (m_active_cc_algo) {
            case CC_COPA:
                return m_copa_controller.GetPacingRateBps();
            case CC_GCC:
                return m_gcc_controller.GetRateBps();
            case CC_BBR:
                return bbr_controller->pacing_rate;  // 已有 BBR
            default:
                return 0;
        }
    }
};
```

### 7.2 修改 Term2TermTransmission

```cpp
// 修改 ec2/SpeedControl/Term2TermTransmission.h

class Term2TermTransmission {
    // ... existing members ...

    // 新增：多流分配器的引用
    std::shared_ptr<MultiFlowAllocator> m_allocator;
    uint32_t m_flow_id;

    // 接收带宽分配结果
    void OnBandwidthAllocation(const BandwidthAllocation& alloc) {
        // 更新 pacing 速率
        DoChangeExpectSpeed(alloc.pacing_rate_bps / SPEED_MBPS);

        // 更新拥塞窗口（如果使用 Copa/GCC）
        m_congestion_window = alloc.congestion_window;
    }
};
```

### 7.3 修改 QoS 反馈流程

```cpp
// 在 Remote2SelfQosUnit::DoCycleTime() 中：

void Remote2SelfQosUnit::DoCycleTime() {
    // 1. 计算丢包率和 RTT（已有逻辑）
    double loss_rate = ...;
    double rtt_ms = ...;

    // 2. 根据 CC 算法计算建议速率
    double suggested_rate;
    switch (m_active_cc_algo) {
        case CC_COPA:
            m_copa_controller.OnAck(bytes_acked, rtt_us, now_us);
            suggested_rate = m_copa_controller.GetPacingRateBps();
            break;
        case CC_GCC:
            suggested_rate = m_gcc_controller.Update(send_time_us, arrival_time_us);
            break;
        case CC_BBR:
            // 已有 BBR 逻辑
            suggested_rate = bbr_controller->pacing_rate;
            break;
    }

    // 3. 上报给多流分配器
    if (m_allocator && m_flow_id > 0) {
        m_allocator->UpdateFlowMetrics(m_flow_id, rtt_ms, loss_rate, suggested_rate);
    }
}
```

---

## 8. 测试方案

### 8.1 多流竞争测试

```bash
# 场景：1 个视频流 + 1 个文件流共享 10Mbps 带宽

# 终端 101：发送视频流（Copa）
./ec2_autoconn --mode sender --flow-type video --cc copa

# 终端 101：同时发送文件流（BBR）
./ec_singletransfer --cc bbr

# 终端 102：接收并监控
# 预期：视频流获得 ~7.5Mbps（权重 3），文件流获得 ~2.5Mbps（权重 1）
```

### 8.2 关键指标

| 场景 | 指标 | 单流 BBR | 多流 CC |
|------|------|----------|---------|
| 视频+文件 | 视频延迟 P99 | ~200ms | ~80ms |
| 视频+文件 | 视频卡顿率 | ~10% | ~2% |
| 视频+文件 | 文件吞吐 | ~8Mbps | ~2.5Mbps |
| 音频+视频 | 音频延迟 P99 | ~150ms | ~50ms |
| 三流并发 | 各流公平性 | 不公平 | 权重公平 |

### 8.3 Copa vs BBR 对比测试

```bash
# 使用 mahimahi 模拟不同网络条件

# 50ms RTT, 0% 丢包
mm-delay 25 mm-loss 0 ./test_copa_vs_bbr

# 100ms RTT, 5% 丢包
mm-delay 50 mm-loss 0.05 ./test_copa_vs_bbr

# 200ms RTT, 10% 丢包
mm-delay 100 mm-loss 0.10 ./test_copa_vs_bbr
```

---

## 9. 预期效果

| 场景 | 改进前 | 改进后 |
|------|--------|--------|
| 视频+文件共享链路 | 视频卡顿 10%，文件独占带宽 | 视频卡顿 2%，按权重分配 |
| 音频+视频 | 音频延迟 150ms | 音术延迟 50ms |
| 弱网（10% 丢包） | BBR 过度探测导致延迟飙升 | Copa 快速退避，延迟稳定 |
| 多流公平性 | 文件流饿死视频流 | 按权重公平分配 |
