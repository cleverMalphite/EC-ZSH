# EC2 升级计划 — 面向 T-Star 岗位的技术演进路线

## 项目现状概述

EC2 是一个多路径实时应急通信系统，核心架构：

```
Application (Video/Audio/File)
    ↓
SpeedControl (Cycle-based Pacing)
    ↓
NetCombTransfer (Multi-path Routing)
    ↓
epollComm (TCP/UDP/Broadcast)
```

### 已有能力
- ✅ BBR v1 拥塞控制（完整实现）
- ✅ 双路径传输（Ad-hoc Mesh + 5G DTU）
- ✅ 自定义 MRUDP 协议（可靠/不可靠双模式，SACK 机制）
- ✅ 基础 ABR（200ms 反馈 + 简单调参公式）
- ✅ Cycle-based Pacing（100ms 大周期 + 小周期细分）
- ✅ 音视频同步（60ms/150ms 阈值）

### 核心 Gap
- ❌ 无 QUIC / MoQ 协议支持
- ❌ 无 FEC（前向纠错）机制
- ❌ 单流拥塞控制，无多流感知
- ❌ 多路径调度策略简单（无 QoE 感知）
- ❌ 无帧级优先级调度
- ❌ Pacing 非 token bucket 模型

---

## 升级路线图

| Phase | 主题 | 面试价值 | 工作量 | 依赖 |
|-------|------|----------|--------|------|
| [Phase 1](phase1_fec_mechanism.md) | FEC 前向纠错机制 | ⭐⭐⭐⭐⭐ | 2-3 周 | 无 |
| [Phase 2](phase2_multi_cc.md) | 多流拥塞控制（Copa/GCC） | ⭐⭐⭐⭐⭐ | 3-4 周 | 无 |
| [Phase 3](phase3_multipath_scheduling.md) | QoE 感知多路径调度 | ⭐⭐⭐⭐ | 2-3 周 | Phase 2 |
| [Phase 4](phase4_pacing_upgrade.md) | Token Bucket Pacing 升级 | ⭐⭐⭐⭐ | 1-2 周 | Phase 2 |
| [Phase 5](phase5_quic_moq.md) | QUIC + MoQ 协议栈 | ⭐⭐⭐⭐⭐ | 4-6 周 | Phase 1-4 |

### 建议执行顺序

```
Phase 1 (FEC) ──────────┐
                        ├──→ Phase 3 (Multipath) ──→ Phase 5 (QUIC/MoQ)
Phase 2 (Multi-CC) ─────┘         │
                                   ↓
                          Phase 4 (Pacing)
```

Phase 1 和 Phase 2 可以并行开发。Phase 3 依赖两者的接口。Phase 4 是 Phase 2 的工程落地。Phase 5 是最终的协议栈升级，将前几个 Phase 的成果统一到 MoQ 框架中。

---

## 面试话术框架

> "我的项目是一个**多路径实时应急通信系统**，我在上面做了五个核心改进：
> 1. **自适应 FEC**：根据丢包率动态调整冗余度，解决了弱网下的掉帧问题
> 2. **多流拥塞控制**：为音视频和文件传输设计了差异化的 CC 策略
> 3. **QoE 感知调度**：基于端到端 QoE 指标优化多路径数据分配
> 4. **Token Bucket Pacing**：将 cycle-based pacing 升级为精确的令牌桶模型
> 5. **MoQ 语义层**：在自定义协议上实现 MoQ 的 Track/Group/Object 抽象
>
> 这些和 MoQ 标准中的 Track/Object 语义、自适应冗余、多流调度是高度对应的。"
