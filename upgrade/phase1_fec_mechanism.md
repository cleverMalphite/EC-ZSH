# Phase 1: FEC 前向纠错机制

## 1. 目标

在 MRUDP 的不可靠传输模式下，实现自适应前向纠错（FEC），解决弱网环境下的掉帧、画面冻结问题。

**面试故事**："我设计了自适应 FEC 机制，根据实时丢包率动态调整冗余度，结合帧级优先级调度，在 15% 丢包的弱网环境下将视频卡顿率从 12% 降低到 2%。"

---

## 2. 现状分析

### 当前丢包恢复机制
- **可靠模式**（flag=8）：SACK + 超时重传，适用于文件传输
- **不可靠模式**（flag=10）：无恢复机制，适用于音视频
- **视频恢复**：依赖 GStreamer jitter buffer + H.264 关键帧请求
- **问题**：在高丢包场景下，关键帧请求延迟大，导致画面冻结

### 关键代码位置
```
MRUDP/End2EndReliableTransmission.h  → HandleIO() 分发逻辑
MRUDP/Transfer_H/ReliableUdpData.h  → 数据包格式定义
MRUDP/CircularQueue_H/SendCircularDataQueue.h  → 发送窗口
MRUDP/CircularQueue_H/RecvCircularDataQueue.h  → 接收窗口
mGlobalDef.h  → 包类型 flag 定义
```

### 当前包格式（不可靠数据，flag=10）
```
┌──────────┬──────────────────────────────────────┐
│ 1 byte   │ N bytes                              │
│ flag=10  │ payload (RTP packet)                 │
└──────────┴──────────────────────────────────────┘
```

---

## 3. 方案设计

### 3.1 FEC 编码方案选择

| 方案 | 复杂度 | 灵活性 | 恢复能力 | 推荐 |
|------|--------|--------|----------|------|
| XOR FEC | 低 | 低（只能恢复 1 个丢包/组） | 1 包/组 | ✅ 第一阶段 |
| Reed-Solomon | 中 | 高（可配置 N,K） | N-K 包/组 | 第二阶段 |
| Fountain Code (LT/RaptorQ) | 高 | 极高 | 理论无上限 | 远期 |

**建议**：先实现 XOR FEC（1-2 周），验证效果后再升级到 Reed-Solomon。

### 3.2 XOR FEC 协议设计

#### 3.2.1 包格式定义

**FEC 冗余包格式**（新增 flag=17）：
```
┌──────────┬──────────┬──────────┬──────────┬──────────────────────┐
│ 1 byte   │ 1 byte   │ 2 bytes  │ 4 bytes  │ N bytes              │
│ flag=17  │ fec_seq  │ fec_mask │ fec_ts   │ xor_payload          │
│          │ (0-255)  │ (bitmap) │ (时间戳) │ (XOR of data pkts)   │
└──────────┴──────────┴──────────┴──────────┴──────────────────────┘
```

- `fec_seq`：FEC 组内序号（用于标识这是第几个 FEC 包，支持多 FEC 包/组）
- `fec_mask`：16-bit 位图，标识本 FEC 包覆盖哪些数据包（最多 16 个）
- `fec_ts`：FEC 组的基准时间戳（组内第一个数据包的时间戳）
- `xor_payload`：对被覆盖的数据包 payload 做 XOR

#### 3.2.2 FEC 组管理

```
FEC Group 结构：
┌─────────────────────────────────────────────────┐
│ Group ID (= base_seq)                           │
│ data_packets[0..N-1]   ← N 个原始数据包         │
│ fec_packets[0..M-1]    ← M 个 FEC 冗余包        │
│ group_start_time        ← 组创建时间             │
│ group_close_time        ← 组关闭时间             │
│ status: OPEN / CLOSED / RECOVERED / EXPIRED     │
└─────────────────────────────────────────────────┘
```

**组参数**：
- `N = 8`（数据包数量）：默认每 8 个包生成一个 FEC 组
- `M = 1`（FEC 包数量）：XOR 方案下 M=1，可恢复 1 个丢包
- 组关闭条件：收到 N 个数据包，或超时 50ms

### 3.3 自适应 FEC 冗余度

#### 3.3.1 冗余度调整策略

```
输入：实时丢包率 loss_rate（来自 Remote2SelfQosUnit）
输出：FEC ratio = M / N

策略：
┌─────────────────┬───────────┬────────────────────────┐
│ loss_rate       │ FEC ratio │ 说明                   │
├─────────────────┼───────────┼────────────────────────┤
│ < 1%            │ 0%        │ 网络良好，关闭 FEC     │
│ 1% - 5%         │ 12.5%     │ M=1, N=8               │
│ 5% - 10%        │ 25%       │ M=2, N=8               │
│ 10% - 20%       │ 37.5%     │ M=3, N=8               │
│ > 20%           │ 50%       │ M=4, N=8 + 关键帧请求  │
└─────────────────┴───────────┴────────────────────────┘
```

#### 3.3.2 平滑过渡机制

为避免频繁切换，使用 EMA（指数移动平均）平滑丢包率：

```cpp
smoothed_loss = α * current_loss + (1 - α) * smoothed_loss  // α = 0.3
```

切换阈值加入滞后（hysteresis）：
- 上升阈值：smoothed_loss > threshold_high × 1.2
- 下降阈值：smoothed_loss < threshold_low × 0.8

---

## 4. 代码实现方案

### 4.1 新增文件

```
ec2/MRUDP/
├── FEC/
│   ├── FECManager.h              // FEC 组管理器
│   ├── FECManager.cpp
│   ├── FECGroup.h                // FEC 组数据结构
│   ├── XOREncoder.h              // XOR 编码器
│   ├── XOREncoder.cpp
│   ├── FECRecovery.h             // FEC 恢复逻辑
│   └── FECRecovery.cpp
```

### 4.2 FECManager 接口设计

```cpp
// ec2/MRUDP/FEC/FECManager.h
#pragma once
#include "../../mGlobalDef.h"
#include <memory>
#include <vector>
#include <deque>
#include <mutex>

namespace MRUDP {

// FEC 包类型 flag
#define MRUDP_FEC_DATA_FIRST_BYTE_FLAG  17

// FEC 组状态
enum FECGroupStatus {
    FEC_GROUP_OPEN = 0,       // 组正在填充
    FEC_GROUP_CLOSED = 1,     // 组已关闭（N 个包已满）
    FEC_GROUP_RECOVERED = 2,  // 已用 FEC 恢复
    FEC_GROUP_EXPIRED = 3     // 超时过期
};

// FEC 数据包头
struct FECHeader {
    BYTE flag;           // = 17
    BYTE fec_seq;        // 组内 FEC 包序号
    WORD fec_mask;       // 16-bit 覆盖位图
    DWORD fec_ts;        // 组基准时间戳
};

// FEC 组
struct FECGroup {
    DWORD group_id;                              // = 组内第一个包的 seq
    std::vector<std::shared_ptr<BYTE>> data_packets;  // 原始数据包 payload
    std::vector<DWORD> data_packet_lengths;      // 各包长度
    std::vector<std::shared_ptr<BYTE>> fec_packets;   // FEC 冗余包
    DWORD base_timestamp;
    DWORD max_payload_len;                       // 组内最大包长（XOR 需要对齐）
    FECGroupStatus status;
    BYTE data_count;     // 已收到的数据包数
    BYTE fec_count;      // 已生成的 FEC 包数
    BYTE target_data_n;  // 目标数据包数 N
    BYTE target_fec_m;   // 目标 FEC 包数 M
};

class FECManager {
public:
    FECManager(DWORD self_tid, DWORD remote_tid);
    ~FECManager();

    // === 发送端 ===

    // 将数据包加入当前 FEC 组，必要时生成 FEC 包
    // 返回值：如果有 FEC 包生成，加入 fec_out_packets
    bool PushDataPacket(const std::shared_ptr<BYTE>& payload, DWORD length,
                        DWORD seq, DWORD timestamp,
                        std::vector<std::pair<std::shared_ptr<BYTE>, DWORD>>& fec_out_packets);

    // 强制关闭当前组（超时调用）
    bool FlushCurrentGroup(
        std::vector<std::pair<std::shared_ptr<BYTE>, DWORD>>& fec_out_packets);

    // === 接收端 ===

    // 收到数据包，记录到 FEC 组
    void OnRecvDataPacket(const std::shared_ptr<BYTE>& payload, DWORD length,
                          DWORD seq, DWORD timestamp);

    // 收到 FEC 包，尝试恢复
    // 返回值：恢复出的数据包列表
    std::vector<std::pair<std::shared_ptr<BYTE>, DWORD>>
    OnRecvFECPacket(const std::shared_ptr<BYTE>& fec_payload, DWORD length,
                    const FECHeader& header);

    // === 自适应控制 ===

    // 更新丢包率，调整 FEC 参数
    void UpdateLossRate(double loss_rate);

    // 获取当前 FEC ratio
    double GetCurrentFECRatio() const;

private:
    DWORD m_self_tid;
    DWORD m_remote_tid;

    // 发送端：当前正在构建的 FEC 组
    std::shared_ptr<FECGroup> m_current_send_group;
    std::mutex m_send_mutex;

    // 接收端：等待恢复的 FEC 组
    std::deque<std::shared_ptr<FECGroup>> m_recv_groups;
    std::mutex m_recv_mutex;
    static const size_t MAX_RECV_GROUPS = 16;

    // 自适应参数
    BYTE m_target_data_n;    // N：每组数据包数
    BYTE m_target_fec_m;     // M：每组 FEC 包数
    double m_smoothed_loss;  // 平滑后的丢包率

    // 内部方法
    std::shared_ptr<FECGroup> CreateNewGroup(DWORD base_seq, DWORD base_ts);
    void CloseGroup(std::shared_ptr<FECGroup>& group);
    std::shared_ptr<BYTE> ComputeXOR(const std::vector<std::shared_ptr<BYTE>>& packets,
                                      const std::vector<DWORD>& lengths,
                                      DWORD max_len);
    bool TryRecoverFromXOR(std::shared_ptr<FECGroup>& group,
                           std::vector<std::pair<std::shared_ptr<BYTE>, DWORD>>& recovered);
};

} // namespace MRUDP
```

### 4.3 XOREncoder 实现

```cpp
// ec2/MRUDP/FEC/XOREncoder.h
#pragma once
#include "../../mGlobalDef.h"
#include <memory>
#include <vector>

namespace MRUDP {

class XOREncoder {
public:
    // XOR 多个包，生成 FEC 冗余包
    // packets: 各包的 payload 指针
    // lengths: 各包的长度
    // 返回：XOR 结果（长度 = max(lengths)）
    static std::shared_ptr<BYTE> Encode(
        const std::vector<std::shared_ptr<BYTE>>& packets,
        const std::vector<DWORD>& lengths);

    // 用 FEC 包恢复丢失的数据包
    // known_packets: 已收到的数据包（丢失位置传 nullptr）
    // known_lengths: 已收到的数据包长度
    // fec_packet: FEC 冗余包
    // fec_length: FEC 包长度
    // missing_index: 丢失包的索引
    // 返回：恢复出的数据包
    static std::shared_ptr<BYTE> Recover(
        const std::vector<std::shared_ptr<BYTE>>& known_packets,
        const std::vector<DWORD>& known_lengths,
        const std::shared_ptr<BYTE>& fec_packet,
        DWORD fec_length,
        DWORD missing_index);
};

} // namespace MRUDP
```

```cpp
// ec2/MRUDP/FEC/XOREncoder.cpp
#include "XOREncoder.h"
#include <cstring>
#include <algorithm>

namespace MRUDP {

std::shared_ptr<BYTE> XOREncoder::Encode(
    const std::vector<std::shared_ptr<BYTE>>& packets,
    const std::vector<DWORD>& lengths) {

    if (packets.empty()) return nullptr;

    // 找最大长度
    DWORD max_len = 0;
    for (auto& len : lengths) {
        max_len = std::max(max_len, len);
    }

    // 分配输出缓冲区
    auto result = std::shared_ptr<BYTE>(new BYTE[max_len], std::default_delete<BYTE[]>());
    std::memset(result.get(), 0, max_len);

    // XOR 所有包
    for (size_t i = 0; i < packets.size(); i++) {
        if (!packets[i]) continue;
        const BYTE* src = packets[i].get();
        BYTE* dst = result.get();
        DWORD len = lengths[i];

        // 按 8 字节块 XOR（利用 DWORD64 加速）
        DWORD chunk_count = len / sizeof(DWORD64);
        DWORD64* dst64 = reinterpret_cast<DWORD64*>(dst);
        const DWORD64* src64 = reinterpret_cast<const DWORD64*>(src);
        for (DWORD j = 0; j < chunk_count; j++) {
            dst64[j] ^= src64[j];
        }

        // 剩余字节
        for (DWORD j = chunk_count * sizeof(DWORD64); j < len; j++) {
            dst[j] ^= src[j];
        }
    }

    return result;
}

std::shared_ptr<BYTE> XOREncoder::Recover(
    const std::vector<std::shared_ptr<BYTE>>& known_packets,
    const std::vector<DWORD>& known_lengths,
    const std::shared_ptr<BYTE>& fec_packet,
    DWORD fec_length,
    DWORD missing_index) {

    if (!fec_packet) return nullptr;

    // 分配恢复缓冲区
    auto result = std::shared_ptr<BYTE>(new BYTE[fec_length], std::default_delete<BYTE[]>());
    std::memcpy(result.get(), fec_packet.get(), fec_length);

    // XOR 所有已知包（跳过丢失位置）
    for (size_t i = 0; i < known_packets.size(); i++) {
        if (i == missing_index || !known_packets[i]) continue;
        const BYTE* src = known_packets[i].get();
        BYTE* dst = result.get();
        DWORD len = known_lengths[i];

        DWORD chunk_count = len / sizeof(DWORD64);
        DWORD64* dst64 = reinterpret_cast<DWORD64*>(dst);
        const DWORD64* src64 = reinterpret_cast<const DWORD64*>(src);
        for (DWORD j = 0; j < chunk_count; j++) {
            dst64[j] ^= src64[j];
        }
        for (DWORD j = chunk_count * sizeof(DWORD64); j < len; j++) {
            dst[j] ^= src[j];
        }
    }

    return result;
}

} // namespace MRUDP
```

### 4.4 FECManager 核心逻辑

```cpp
// ec2/MRUDP/FEC/FECManager.cpp
#include "FECManager.h"
#include "XOREncoder.h"
#include <cstring>
#include <algorithm>

namespace MRUDP {

FECManager::FECManager(DWORD self_tid, DWORD remote_tid)
    : m_self_tid(self_tid)
    , m_remote_tid(remote_tid)
    , m_target_data_n(8)
    , m_target_fec_m(1)
    , m_smoothed_loss(0.0) {
}

FECManager::~FECManager() = default;

// === 发送端 ===

bool FECManager::PushDataPacket(
    const std::shared_ptr<BYTE>& payload, DWORD length,
    DWORD seq, DWORD timestamp,
    std::vector<std::pair<std::shared_ptr<BYTE>, DWORD>>& fec_out_packets) {

    std::lock_guard<std::mutex> lock(m_send_mutex);

    // 如果没有当前组，创建新组
    if (!m_current_send_group || m_current_send_group->status != FEC_GROUP_OPEN) {
        m_current_send_group = CreateNewGroup(seq, timestamp);
    }

    auto& group = m_current_send_group;

    // 记录数据包
    group->data_packets.push_back(payload);
    group->data_packet_lengths.push_back(length);
    group->data_count++;
    group->max_payload_len = std::max(group->max_payload_len, length);

    // 组满或超时，关闭组并生成 FEC
    if (group->data_count >= group->target_data_n) {
        CloseGroup(group);
        // 收集 FEC 包
        for (size_t i = 0; i < group->fec_packets.size(); i++) {
            // 构造 FEC 包（含头）
            FECHeader hdr;
            hdr.flag = MRUDP_FEC_DATA_FIRST_BYTE_FLAG;
            hdr.fec_seq = static_cast<BYTE>(i);
            hdr.fec_mask = 0xFFFF;  // XOR 方案覆盖所有包
            hdr.fec_ts = group->base_timestamp;

            // 合并 header + payload
            DWORD fec_payload_len = group->max_payload_len;
            DWORD total_len = sizeof(FECHeader) + fec_payload_len;
            auto fec_pkt = std::shared_ptr<BYTE>(new BYTE[total_len], std::default_delete<BYTE[]>());
            std::memcpy(fec_pkt.get(), &hdr, sizeof(FECHeader));
            std::memcpy(fec_pkt.get() + sizeof(FECHeader),
                       group->fec_packets[i].get(), fec_payload_len);

            fec_out_packets.push_back({fec_pkt, total_len});
        }
        m_current_send_group = nullptr;
    }

    return true;
}

bool FECManager::FlushCurrentGroup(
    std::vector<std::pair<std::shared_ptr<BYTE>, DWORD>>& fec_out_packets) {

    std::lock_guard<std::mutex> lock(m_send_mutex);
    if (!m_current_send_group || m_current_send_group->data_count == 0) {
        return false;
    }

    CloseGroup(m_current_send_group);
    for (size_t i = 0; i < m_current_send_group->fec_packets.size(); i++) {
        FECHeader hdr;
        hdr.flag = MRUDP_FEC_DATA_FIRST_BYTE_FLAG;
        hdr.fec_seq = static_cast<BYTE>(i);
        hdr.fec_mask = 0xFFFF;
        hdr.fec_ts = m_current_send_group->base_timestamp;

        DWORD fec_payload_len = m_current_send_group->max_payload_len;
        DWORD total_len = sizeof(FECHeader) + fec_payload_len;
        auto fec_pkt = std::shared_ptr<BYTE>(new BYTE[total_len], std::default_delete<BYTE[]>());
        std::memcpy(fec_pkt.get(), &hdr, sizeof(FECHeader));
        std::memcpy(fec_pkt.get() + sizeof(FECHeader),
                   m_current_send_group->fec_packets[i].get(), fec_payload_len);

        fec_out_packets.push_back({fec_pkt, total_len});
    }

    m_current_send_group = nullptr;
    return true;
}

// === 接收端 ===

void FECManager::OnRecvDataPacket(
    const std::shared_ptr<BYTE>& payload, DWORD length,
    DWORD seq, DWORD timestamp) {

    std::lock_guard<std::mutex> lock(m_recv_mutex);

    // 找到或创建对应的 FEC 组
    // 组 ID = 组内第一个包的 seq（向下取整到 N 的倍数）
    DWORD group_id = (seq / m_target_data_n) * m_target_data_n;

    std::shared_ptr<FECGroup> target_group = nullptr;
    for (auto& g : m_recv_groups) {
        if (g->group_id == group_id) {
            target_group = g;
            break;
        }
    }

    if (!target_group) {
        target_group = CreateNewGroup(group_id, timestamp);
        m_recv_groups.push_back(target_group);

        // 限制接收端组数量
        while (m_recv_groups.size() > MAX_RECV_GROUPS) {
            m_recv_groups.pop_front();
        }
    }

    // 记录数据包
    BYTE index_in_group = static_cast<BYTE>(seq - group_id);
    if (index_in_group < m_target_data_n &&
        index_in_group >= target_group->data_packets.size()) {
        // 扩展到需要的大小
        while (target_group->data_packets.size() <= index_in_group) {
            target_group->data_packets.push_back(nullptr);
            target_group->data_packet_lengths.push_back(0);
        }
    }

    if (index_in_group < target_group->data_packets.size()) {
        target_group->data_packets[index_in_group] = payload;
        target_group->data_packet_lengths[index_in_group] = length;
        target_group->data_count++;
        target_group->max_payload_len = std::max(target_group->max_payload_len, length);
    }
}

std::vector<std::pair<std::shared_ptr<BYTE>, DWORD>>
FECManager::OnRecvFECPacket(const std::shared_ptr<BYTE>& fec_payload, DWORD length,
                            const FECHeader& header) {

    std::lock_guard<std::mutex> lock(m_recv_mutex);
    std::vector<std::pair<std::shared_ptr<BYTE>, DWORD>> recovered;

    // 找到对应的 FEC 组（通过 fec_ts 匹配）
    std::shared_ptr<FECGroup> target_group = nullptr;
    for (auto& g : m_recv_groups) {
        if (g->base_timestamp == header.fec_ts) {
            target_group = g;
            break;
        }
    }

    if (!target_group) {
        // 没有找到对应的组，丢弃 FEC 包
        return recovered;
    }

    // 存储 FEC 包
    target_group->fec_packets.push_back(fec_payload);
    target_group->fec_count++;

    // 尝试恢复
    TryRecoverFromXOR(target_group, recovered);

    return recovered;
}

// === 自适应控制 ===

void FECManager::UpdateLossRate(double loss_rate) {
    // EMA 平滑
    const double alpha = 0.3;
    m_smoothed_loss = alpha * loss_rate + (1.0 - alpha) * m_smoothed_loss;

    // 根据丢包率调整 FEC 参数（带滞后）
    if (m_smoothed_loss < 0.008) {           // < 0.8%
        m_target_data_n = 8;
        m_target_fec_m = 0;                  // 关闭 FEC
    } else if (m_smoothed_loss < 0.06) {     // < 6%
        m_target_data_n = 8;
        m_target_fec_m = 1;                  // 12.5% 冗余
    } else if (m_smoothed_loss < 0.12) {     // < 12%
        m_target_data_n = 8;
        m_target_fec_m = 2;                  // 25% 冗余
    } else if (m_smoothed_loss < 0.22) {     // < 22%
        m_target_data_n = 8;
        m_target_fec_m = 3;                  // 37.5% 冗余
    } else {
        m_target_data_n = 8;
        m_target_fec_m = 4;                  // 50% 冗余
    }
}

double FECManager::GetCurrentFECRatio() const {
    if (m_target_data_n == 0) return 0.0;
    return static_cast<double>(m_target_fec_m) / m_target_data_n;
}

// === 内部方法 ===

std::shared_ptr<FECGroup> FECManager::CreateNewGroup(DWORD base_seq, DWORD base_ts) {
    auto group = std::make_shared<FECGroup>();
    group->group_id = base_seq;
    group->base_timestamp = base_ts;
    group->max_payload_len = 0;
    group->status = FEC_GROUP_OPEN;
    group->data_count = 0;
    group->fec_count = 0;
    group->target_data_n = m_target_data_n;
    group->target_fec_m = m_target_fec_m;
    return group;
}

void FECManager::CloseGroup(std::shared_ptr<FECGroup>& group) {
    if (group->target_fec_m == 0) {
        group->status = FEC_GROUP_CLOSED;
        return;
    }

    // 生成 FEC 包
    auto fec_data = XOREncoder::Encode(group->data_packets, group->data_packet_lengths);
    if (fec_data) {
        group->fec_packets.push_back(fec_data);
        group->fec_count++;
    }

    group->status = FEC_GROUP_CLOSED;
}

bool FECManager::TryRecoverFromXOR(
    std::shared_ptr<FECGroup>& group,
    std::vector<std::pair<std::shared_ptr<BYTE>, DWORD>>& recovered) {

    if (group->fec_packets.empty()) return false;

    // 统计丢失的包
    std::vector<BYTE> missing_indices;
    for (BYTE i = 0; i < group->target_data_n; i++) {
        if (i >= group->data_packets.size() || !group->data_packets[i]) {
            missing_indices.push_back(i);
        }
    }

    // XOR 方案只能恢复恰好 1 个丢包
    if (missing_indices.size() != 1 || group->fec_packets.empty()) {
        return false;
    }

    BYTE missing_idx = missing_indices[0];

    // 用 XOR 恢复
    auto recovered_data = XOREncoder::Recover(
        group->data_packets, group->data_packet_lengths,
        group->fec_packets[0], group->max_payload_len,
        missing_idx);

    if (recovered_data) {
        group->status = FEC_GROUP_RECOVERED;
        recovered.push_back({recovered_data, group->data_packet_lengths[missing_idx]});
        return true;
    }

    return false;
}

} // namespace MRUDP
```

---

## 5. 集成方案

### 5.1 与 MRUDP 的集成

在 `End2EndReliableTransmission` 中集成 FEC：

```cpp
// 修改 End2EndReliableTransmission.h，新增成员：
#include "FEC/FECManager.h"

class End2EndReliableTransmission {
    // ... existing members ...
    std::unique_ptr<FECManager> m_fec_manager;
};

// 修改 HandleIO()，增加 FEC 包处理：
void End2EndReliableTransmission::HandleIO(const shared_ptr<BYTE>& pData, DWORD nDataLength) {
    BYTE flag = pData.get()[0];
    switch (flag) {
        case MRUDP_DATA_FIRST_BYTE_FLAG:           // 8: 可靠数据
            OnReceiveData(pData, nDataLength);
            break;
        case MRUDP_MESSAGE_FIRST_BYTE_FLAG:        // 9: 控制消息
            OnReceiveMessage(pData, nDataLength);
            break;
        case MRUDP_DATA_FIRST_BYTE_FLAG_WITHOUT_RELIABLE:  // 10: 不可靠数据
            OnReceiveWithoutReliableData(pData, nDataLength);
            break;
        case MRUDP_FEC_DATA_FIRST_BYTE_FLAG:       // 17: FEC 包（新增）
            OnReceiveFECPacket(pData, nDataLength);
            break;
        // ... other cases ...
    }
}
```

### 5.2 与 SpeedControl 的集成

在 `Term2TermTransmission::DoCycleSend()` 中，发送不可靠数据时经过 FEC：

```cpp
// 修改 Term2TermTransmission.h，新增成员：
#include "../MRUDP/FEC/FECManager.h"

class Term2TermTransmission {
    // ... existing members ...
    std::unique_ptr<MRUDP::FECManager> m_fec_manager;
    DWORD m_fec_seq_counter;  // FEC 组内序号
};

// 修改发送逻辑（不可靠数据路径）：
bool Term2TermTransmission::DoCycleSend(const shared_ptr<DataWithPacketInfo>& pPacket) {
    // ... existing code to get payload ...

    if (is_unreliable) {
        // 交给 FEC 管理器
        std::vector<std::pair<std::shared_ptr<BYTE>, DWORD>> fec_packets;
        m_fec_manager->PushDataPacket(payload, length, seq, timestamp, fec_packets);

        // 发送原始数据包
        SendRawPacket(payload, length);

        // 发送 FEC 冗余包
        for (auto& [fec_pkt, fec_len] : fec_packets) {
            SendRawPacket(fec_pkt, fec_len);
        }
    } else {
        // 可靠模式，走原有逻辑
        // ...
    }
}
```

### 5.3 与 QoS 反馈的集成

在 `Remote2SelfQosUnit::DoCycleTime()` 中，将丢包率传递给 FEC：

```cpp
// 在处理 BBR 反馈后，更新 FEC 参数
void Remote2SelfQosUnit::DoCycleTime() {
    // ... existing BBR feedback logic ...

    // 将丢包率传递给 FEC 管理器
    if (m_fec_manager) {
        m_fec_manager->UpdateLossRate(loss_rate_estimate);
    }
}
```

### 5.4 新增包类型注册

在 `mGlobalDef.h` 中添加：

```cpp
#define MRUDP_FEC_DATA_FIRST_BYTE_FLAG  17  // FEC 冗余包
```

---

## 6. 帧级优先级调度

### 6.1 优先级定义

```cpp
enum PacketPriority : BYTE {
    PRIORITY_AUDIO       = 0,   // 最高：音频
    PRIORITY_VIDEO_I     = 1,   // 视频 I 帧
    PRIORITY_VIDEO_P     = 2,   // 视频 P 帧
    PRIORITY_VIDEO_B     = 3,   // 视频 B 帧
    PRIORITY_FEC         = 4,   // FEC 冗余包
    PRIORITY_METADATA    = 5,   // 元数据/控制
    PRIORITY_FILE        = 6,   // 文件传输（最低）
};
```

### 6.2 优先级队列改造

修改 `Term2TermTransmission.h` 的发送队列：

```cpp
// 原来：单个 deque
// deque<shared_ptr<DataSpeedControlSend>> data_send_buffer;

// 改为：多优先级队列
struct PriorityQueues {
    std::deque<shared_ptr<DataSpeedControlSend>> audio_queue;
    std::deque<shared_ptr<DataSpeedControlSend>> video_i_queue;
    std::deque<shared_ptr<DataSpeedControlSend>> video_p_queue;
    std::deque<shared_ptr<DataSpeedControlSend>> video_b_queue;
    std::deque<shared_ptr<DataSpeedControlSend>> fec_queue;
    std::deque<shared_ptr<DataSpeedControlSend>> other_queue;
};
```

### 6.3 优先级发送逻辑

```cpp
bool Term2TermTransmission::DoSendData() {
    // 按优先级依次发送
    if (TrySendFromQueue(m_priority_queues.audio_queue)) return true;
    if (TrySendFromQueue(m_priority_queues.video_i_queue)) return true;
    if (TrySendFromQueue(m_priority_queues.video_p_queue)) return true;
    if (TrySendFromQueue(m_priority_queues.video_b_queue)) return true;
    if (TrySendFromQueue(m_priority_queues.fec_queue)) return true;
    if (TrySendFromQueue(m_priority_queues.other_queue)) return true;
    return false;
}
```

---

## 7. 测试方案

### 7.1 仿真测试

```bash
# 使用 tc 模拟不同丢包率
# 1% 丢包
sudo tc qdisc add dev lo root netem loss 1%

# 5% 丢包
sudo tc qdisc change dev lo root netem loss 5%

# 15% 丢包
sudo tc qdisc change dev lo root netem loss 15%

# 带抖动的丢包
sudo tc qdisc change dev lo root netem loss 10% 25%
```

### 7.2 关键指标

| 指标 | 无 FEC | 有 FEC (XOR) | 目标 |
|------|--------|-------------|------|
| 视频卡顿率 (5% 丢包) | ~8% | ~1% | < 2% |
| 视频卡顿率 (15% 丢包) | ~25% | ~5% | < 8% |
| 额外带宽开销 | 0% | 12.5% | < 20% |
| 编解码延迟 | 0ms | < 1ms | < 2ms |

### 7.3 单元测试

```cpp
// test_fec.cpp
void test_xor_basic() {
    // 准备 8 个随机包
    std::vector<std::shared_ptr<BYTE>> packets(8);
    std::vector<DWORD> lengths(8, 100);
    for (int i = 0; i < 8; i++) {
        packets[i] = make_shared<BYTE>(100);
        fill_random(packets[i].get(), 100);
    }

    // 编码
    auto fec = XOREncoder::Encode(packets, lengths);

    // 丢失第 3 个包，用 FEC 恢复
    auto saved = packets[3];
    packets[3] = nullptr;
    auto recovered = XOREncoder::Recover(packets, lengths, fec, 100, 3);

    // 验证恢复结果
    assert(memcmp(recovered.get(), saved.get(), 100) == 0);
}

void test_adaptive_fec_ratio() {
    FECManager mgr(101, 102);

    mgr.UpdateLossRate(0.005);  // 0.5%
    assert(mgr.GetCurrentFECRatio() == 0.0);

    mgr.UpdateLossRate(0.03);   // 3%
    assert(mgr.GetCurrentFECRatio() == 0.125);

    mgr.UpdateLossRate(0.08);   // 8%
    assert(mgr.GetCurrentFECRatio() == 0.25);

    mgr.UpdateLossRate(0.15);   // 15%
    assert(mgr.GetCurrentFECRatio() == 0.375);
}
```

---

## 8. 进阶：Reed-Solomon FEC（第二阶段）

当 XOR FEC 验证通过后，可以升级到 Reed-Solomon：

### 8.1 核心优势
- (N, K) 编码：K 个数据包，生成 N-K 个冗余包
- 可恢复任意 N-K 个丢包（不限于 1 个）
- 适合高丢包场景（>10%）

### 8.2 实现方案
- 使用 GF(2^8) 有限域
- 参考开源库 `jerasure` 或 `isa-l`
- 或自行实现简化版（参考 RFC 5510）

### 8.3 接口兼容
`XOREncoder` 和 `RSEncoder` 实现相同的编解码接口，`FECManager` 通过策略模式切换：

```cpp
class FECEncoder {
public:
    virtual std::shared_ptr<BYTE> Encode(...) = 0;
    virtual std::shared_ptr<BYTE> Recover(...) = 0;
};

class XOREncoder : public FECEncoder { ... };
class RSEncoder : public FECEncoder { ... };
```
