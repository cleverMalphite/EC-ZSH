# Phase 5: QUIC + MoQ 协议栈升级

## 1. 目标

将现有自定义 MRUDP 协议升级为兼容 IETF Media over QUIC (MoQ) 标准的传输栈，实现 Track/Group/Object 三级抽象，支持标准化的 Publish/Subscribe 模型。

**面试故事**："我参考 IETF MoQ 标准，在现有 MRUDP 协议基础上设计了 MoQ 语义层，实现了 Track/Group/Object 三级抽象和 Publish/Subscribe 模型，为后续接入标准 QUIC 传输层奠定了基础。"

---

## 2. MoQ 标准概述

### 2.1 MoQ 核心概念

```
MoQ (Media over QUIC) 是 IETF 工作组正在制定的标准，
目标是为实时媒体传输定义一个基于 QUIC 的应用层协议。

核心抽象：
┌─────────────────────────────────────────────────────────┐
│  Track（轨道）                                          │
│  ┌──────────────────────────────────────────────┐      │
│  │  Group（组，通常对应一个 GOP）                │      │
│  │  ┌─────────────────────────────────────┐    │      │
│  │  │  Object（对象，通常对应一个帧）      │    │      │
│  │  │  ┌─────────────────────────────┐   │    │      │
│  │  │  │  Payload（数据负载）        │   │    │      │
│  │  │  └─────────────────────────────┘   │    │      │
│  │  └─────────────────────────────────────┘    │      │
│  └──────────────────────────────────────────────┘      │
└─────────────────────────────────────────────────────────┘

层级关系：
- Track: 一个独立的媒体流（如视频轨道、音频轨道、字幕轨道）
- Group: 一组有序的 Object（通常对应一个 GOP，可以独立解码）
- Object: 最小的传输单元（通常对应一个视频帧或音频帧）
```

### 2.2 MoQ 传输模型

```
Publisher                          Subscriber
    │                                  │
    │──── Announce (Track) ───────────→│
    │                                  │
    │←── Subscribe (Track) ────────────│
    │                                  │
    │==== Stream (Group/Object) ══════→│
    │                                  │
    │──── Object (Frame Data) ────────→│
    │                                  │
```

### 2.3 MoQ vs 当前 MRUDP

| 特性 | MRUDP | MoQ |
|------|-------|-----|
| 抽象级别 | 字节流/数据报 | Track/Group/Object |
| 可靠性 | 可靠/不可靠双模式 | 可配置：每个 Track 独立 |
| 多路复用 | 通过 TID 区分 | 通过 Stream ID 区分 |
| 流控 | 自定义窗口 | QUIC Stream Flow Control |
| 拥塞控制 | BBR | QUIC CC (可插拔) |
| 标准化 | 自定义 | IETF 标准 |

---

## 3. MoQ 语义层设计

### 3.1 核心数据结构

```cpp
// ec2/MoQ/MoQTypes.h
#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <map>
#include <deque>
#include <mutex>

namespace moq {

// === 基本类型 ===

// Track ID（全局唯一标识一个媒体轨道）
using TrackId = uint64_t;

// Group ID（组序号，单调递增）
using GroupId = uint64_t;

// Object ID（组内对象序号）
using ObjectId = uint64_t;

// Stream ID（底层传输流 ID）
using StreamId = uint64_t;

// === Object 优先级 ===
enum class ObjectPriority : uint8_t {
    URGENT = 0,        // 最高优先级（如音频关键帧）
    HIGH = 1,          // 高优先级（如视频 I 帧）
    NORMAL = 2,        // 普通优先级（如视频 P 帧）
    LOW = 3,           // 低优先级（如视频 B 帧）
    BACKGROUND = 4,    // 后台优先级（如元数据）
};

// === Object 发送状态 ===
enum class ObjectStatus : uint8_t {
    IN_PROGRESS = 0,   // 正在传输
    END_OF_STREAM = 1, // 流结束
    ERROR = 2,         // 传输错误
};

// === Track 可靠性模式 ===
enum class TrackReliability : uint8_t {
    NONE = 0,          // 完全不可靠（实时音视频）
    PARTIAL = 1,       // 部分可靠（最新 Group 可靠）
    FULL = 2,          // 完全可靠（文件传输）
};

// === Object（最小传输单元）===
struct Object {
    TrackId track_id;
    GroupId group_id;
    ObjectId object_id;
    ObjectPriority priority;
    ObjectStatus status;

    std::shared_ptr<uint8_t> payload;
    uint32_t payload_length;

    // 发送时间戳（发送端设置）
    int64_t send_timestamp_us;

    // 扩展头（可选）
    std::vector<uint8_t> extensions;

    // 辅助方法
    bool IsEndOfGroup() const {
        return status == ObjectStatus::END_OF_STREAM;
    }

    uint32_t GetWireSize() const {
        return sizeof(TrackId) + sizeof(GroupId) + sizeof(ObjectId) +
               1 + 1 + 4 + payload_length + extensions.size();
    }
};

// === Group（一组有序的 Object）===
struct Group {
    GroupId group_id;
    TrackId track_id;

    std::deque<Object> objects;
    bool is_complete;

    // Group 级别的属性
    int64_t start_timestamp_us;
    int64_t end_timestamp_us;

    // 获取 Group 总大小
    uint32_t GetTotalSize() const {
        uint32_t total = 0;
        for (const auto& obj : objects) {
            total += obj.payload_length;
        }
        return total;
    }
};

// === Track 元数据 ===
struct TrackMetadata {
    TrackId track_id;
    std::string namespace_name;    // 命名空间（如 "live/stream1"）
    std::string track_name;        // 轨道名称（如 "video/h264"）
    TrackReliability reliability;
    ObjectPriority default_priority;

    // 编解码信息
    std::string codec;             // 编解码器（如 "h264", "opus"）
    uint32_t clock_rate;           // 时钟频率（如 90000 for video, 48000 for audio）
    uint32_t max_object_size;      // 最大 Object 大小

    // 扩展属性
    std::map<std::string, std::string> attributes;
};

// === Subscribe 请求 ===
struct SubscribeRequest {
    uint64_t subscribe_id;
    TrackId track_id;
    GroupId start_group;           // 从哪个 Group 开始订阅
    ObjectPriority priority;       // 请求的优先级
};

// === Subscribe 响应 ===
struct SubscribeResponse {
    uint64_t subscribe_id;
    bool accepted;
    std::string error_reason;
};

// === Announce 声明 ===
struct Announce {
    std::string namespace_name;
    std::vector<TrackMetadata> tracks;
};

} // namespace moq
```

### 3.2 MoQ Session（会话管理）

```cpp
// ec2/MoQ/MoQSession.h
#pragma once
#include "MoQTypes.h"
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

namespace moq {

// 回调类型
using OnObjectReceived = std::function<void(const Object&)>;
using OnTrackAnnounced = std::function<void(const TrackMetadata&)>;
using OnSubscribeRequest = std::function<SubscribeResponse(const SubscribeRequest&)>;
using OnSessionClosed = std::function<void(uint32_t error_code)>;

// MoQ 会话
class MoQSession {
public:
    MoQSession(uint32_t self_tid, uint32_t remote_tid);
    ~MoQSession();

    // === Publisher 接口 ===

    // 声明一个 Track（Publisher 调用）
    bool Announce(const TrackMetadata& metadata);

    // 发送一个 Object（Publisher 调用）
    bool SendObject(const Object& object);

    // 发送 Group 结束标记
    bool SendEndOfGroup(TrackId track_id, GroupId group_id);

    // 发送 Track 结束标记
    bool SendEndOfTrack(TrackId track_id);

    // === Subscriber 接口 ===

    // 订阅一个 Track（Subscriber 调用）
    bool Subscribe(const SubscribeRequest& request);

    // 取消订阅
    bool Unsubscribe(uint64_t subscribe_id);

    // === 回调注册 ===

    void SetOnObjectReceived(OnObjectReceived callback) {
        m_on_object_received = callback;
    }

    void SetOnTrackAnnounced(OnTrackAnnounced callback) {
        m_on_track_announced = callback;
    }

    void SetOnSubscribeRequest(OnSubscribeRequest callback) {
        m_on_subscribe_request = callback;
    }

    void SetOnSessionClosed(OnSessionClosed callback) {
        m_on_session_closed = callback;
    }

    // === 底层传输接口 ===

    // 连接到远端（使用现有 MRUDP 传输）
    bool Connect(const std::string& address, uint16_t port);

    // 断开连接
    bool Disconnect();

    // 处理接收到的数据（由底层传输层调用）
    void OnDataReceived(const uint8_t* data, uint32_t length);

    // 获取会话状态
    bool IsConnected() const { return m_connected; }

    // 获取统计信息
    struct SessionStats {
        uint64_t objects_sent;
        uint64_t objects_received;
        uint64_t bytes_sent;
        uint64_t bytes_received;
        uint64_t groups_sent;
        uint64_t groups_received;
    };

    SessionStats GetStats() const;

private:
    uint32_t m_self_tid;
    uint32_t m_remote_tid;
    bool m_connected;

    // Track 管理
    std::mutex m_tracks_mutex;
    std::map<TrackId, TrackMetadata> m_local_tracks;    // 本地声明的 Track
    std::map<TrackId, TrackMetadata> m_remote_tracks;   // 远端声明的 Track
    std::map<uint64_t, SubscribeRequest> m_subscriptions; // 订阅请求

    // 接收缓冲
    std::mutex m_recv_mutex;
    std::map<TrackId, std::map<GroupId, Group>> m_recv_buffer;

    // 回调
    OnObjectReceived m_on_object_received;
    OnTrackAnnounced m_on_track_announced;
    OnSubscribeRequest m_on_subscribe_request;
    OnSessionClosed m_on_session_closed;

    // 统计
    SessionStats m_stats;

    // 内部方法
    bool SendControlMessage(const uint8_t* data, uint32_t length);
    bool SendDataStream(TrackId track_id, const Object& object);
    void ProcessControlMessage(const uint8_t* data, uint32_t length);
    void ProcessDataStream(const uint8_t* data, uint32_t length);

    // 消息序列化
    std::vector<uint8_t> SerializeAnnounce(const Announce& announce);
    std::vector<uint8_t> SerializeSubscribe(const SubscribeRequest& request);
    std::vector<uint8_t> SerializeObject(const Object& object);

    // 消息反序列化
    Announce DeserializeAnnounce(const uint8_t* data, uint32_t length);
    Object DeserializeObject(const uint8_t* data, uint32_t length);
};

} // namespace moq
```

### 3.3 MoQ Session 实现

```cpp
// ec2/MoQ/MoQSession.cpp
#include "MoQSession.h"
#include "../MRUDP/MRUDPApi.h"
#include <cstring>
#include <algorithm>

namespace moq {

// === 消息类型定义 ===
enum MoQMessageType : uint8_t {
    MSG_ANNOUNCE = 0x01,
    MSG_ANNOUNCE_OK = 0x02,
    MSG_ANNOUNCE_ERROR = 0x03,
    MSG_SUBSCRIBE = 0x10,
    MSG_SUBSCRIBE_OK = 0x11,
    MSG_SUBSCRIBE_ERROR = 0x12,
    MSG_UNSUBSCRIBE = 0x13,
    MSG_OBJECT_STREAM = 0x20,      // Object 作为独立 Stream
    MSG_OBJECT_DATAGRAM = 0x21,    // Object 作为 Datagram
    MSG_END_OF_GROUP = 0x30,
    MSG_END_OF_TRACK = 0x31,
};

MoQSession::MoQSession(uint32_t self_tid, uint32_t remote_tid)
    : m_self_tid(self_tid)
    , m_remote_tid(remote_tid)
    , m_connected(false) {
    std::memset(&m_stats, 0, sizeof(m_stats));
}

MoQSession::~MoQSession() {
    Disconnect();
}

// === Publisher 接口 ===

bool MoQSession::Announce(const TrackMetadata& metadata) {
    std::lock_guard<std::mutex> lock(m_tracks_mutex);

    // 存储本地 Track
    m_local_tracks[metadata.track_id] = metadata;

    // 构造 Announce 消息
    Announce announce;
    announce.namespace_name = metadata.namespace_name;
    announce.tracks.push_back(metadata);

    auto data = SerializeAnnounce(announce);
    return SendControlMessage(data.data(), data.size());
}

bool MoQSession::SendObject(const Object& object) {
    if (!m_connected) return false;

    // 更新统计
    m_stats.objects_sent++;
    m_stats.bytes_sent += object.payload_length;

    // 根据可靠性模式选择传输方式
    auto it = m_local_tracks.find(object.track_id);
    if (it == m_local_tracks.end()) {
        return false;  // Track 未声明
    }

    const auto& track = it->second;

    if (track.reliability == TrackReliability::FULL) {
        // 可靠传输：使用 MRUDP 可靠模式
        auto data = SerializeObject(object);
        return SendDataStream(object.track_id, object);
    } else {
        // 不可靠传输：使用 MRUDP 不可靠模式
        auto data = SerializeObject(object);
        return SendDataStream(object.track_id, object);
    }
}

bool MoQSession::SendEndOfGroup(TrackId track_id, GroupId group_id) {
    Object eog;
    eog.track_id = track_id;
    eog.group_id = group_id;
    eog.object_id = 0;
    eog.priority = ObjectPriority::BACKGROUND;
    eog.status = ObjectStatus::END_OF_STREAM;
    eog.payload = nullptr;
    eog.payload_length = 0;

    return SendObject(eog);
}

bool MoQSession::SendEndOfTrack(TrackId track_id) {
    Object eot;
    eot.track_id = track_id;
    eot.group_id = 0;
    eot.object_id = 0;
    eot.priority = ObjectPriority::BACKGROUND;
    eot.status = ObjectStatus::END_OF_STREAM;
    eot.payload = nullptr;
    eot.payload_length = 0;

    return SendObject(eot);
}

// === Subscriber 接口 ===

bool MoQSession::Subscribe(const SubscribeRequest& request) {
    std::lock_guard<std::mutex> lock(m_tracks_mutex);

    // 存储订阅
    m_subscriptions[request.subscribe_id] = request;

    // 发送 Subscribe 消息
    auto data = SerializeSubscribe(request);
    return SendControlMessage(data.data(), data.size());
}

bool MoQSession::Unsubscribe(uint64_t subscribe_id) {
    std::lock_guard<std::mutex> lock(m_tracks_mutex);

    m_subscriptions.erase(subscribe_id);

    // 发送 Unsubscribe 消息
    uint8_t msg[9];
    msg[0] = MSG_UNSUBSCRIBE;
    std::memcpy(msg + 1, &subscribe_id, 8);

    return SendControlMessage(msg, sizeof(msg));
}

// === 底层传输接口 ===

bool MoQSession::Connect(const std::string& address, uint16_t port) {
    // 使用现有 MRUDP API 建立连接
    // 这里假设 MRUDP 已经初始化
    m_connected = true;
    return true;
}

bool MoQSession::Disconnect() {
    if (m_connected) {
        // 发送关闭消息
        m_connected = false;
    }
    return true;
}

void MoQSession::OnDataReceived(const uint8_t* data, uint32_t length) {
    if (length == 0) return;

    uint8_t msg_type = data[0];

    switch (msg_type) {
        case MSG_ANNOUNCE:
        case MSG_ANNOUNCE_OK:
        case MSG_ANNOUNCE_ERROR:
        case MSG_SUBSCRIBE:
        case MSG_SUBSCRIBE_OK:
        case MSG_SUBSCRIBE_ERROR:
        case MSG_UNSUBSCRIBE:
            ProcessControlMessage(data, length);
            break;

        case MSG_OBJECT_STREAM:
        case MSG_OBJECT_DATAGRAM:
            ProcessDataStream(data, length);
            break;

        default:
            // 未知消息类型
            break;
    }
}

void MoQSession::ProcessControlMessage(const uint8_t* data, uint32_t length) {
    uint8_t msg_type = data[0];

    switch (msg_type) {
        case MSG_ANNOUNCE: {
            auto announce = DeserializeAnnounce(data + 1, length - 1);
            for (const auto& track : announce.tracks) {
                m_remote_tracks[track.track_id] = track;
                if (m_on_track_announced) {
                    m_on_track_announced(track);
                }
            }
            break;
        }

        case MSG_SUBSCRIBE: {
            // 反序列化 SubscribeRequest
            SubscribeRequest request;
            if (length >= 13) {
                std::memcpy(&request.subscribe_id, data + 1, 8);
                std::memcpy(&request.track_id, data + 9, 4);
            }

            if (m_on_subscribe_request) {
                auto response = m_on_subscribe_request(request);
                // 发送响应
                uint8_t resp[14];
                resp[0] = response.accepted ? MSG_SUBSCRIBE_OK : MSG_SUBSCRIBE_ERROR;
                std::memcpy(resp + 1, &response.subscribe_id, 8);
                SendControlMessage(resp, sizeof(resp));
            }
            break;
        }

        default:
            break;
    }
}

void MoQSession::ProcessDataStream(const uint8_t* data, uint32_t length) {
    // 反序列化 Object
    auto object = DeserializeObject(data + 1, length - 1);

    // 更新统计
    m_stats.objects_received++;
    m_stats.bytes_received += object.payload_length;

    // 存入接收缓冲
    {
        std::lock_guard<std::mutex> lock(m_recv_mutex);
        auto& group = m_recv_buffer[object.track_id][object.group_id];
        group.group_id = object.group_id;
        group.track_id = object.track_id;
        group.objects.push_back(object);

        if (object.status == ObjectStatus::END_OF_STREAM) {
            group.is_complete = true;
            m_stats.groups_received++;
        }
    }

    // 通知上层
    if (m_on_object_received) {
        m_on_object_received(object);
    }
}

// === 消息序列化 ===

std::vector<uint8_t> MoQSession::SerializeAnnounce(const Announce& announce) {
    std::vector<uint8_t> data;

    // 消息类型
    data.push_back(MSG_ANNOUNCE);

    // 命名空间长度 + 内容
    uint16_t ns_len = static_cast<uint16_t>(announce.namespace_name.size());
    data.push_back((ns_len >> 8) & 0xFF);
    data.push_back(ns_len & 0xFF);
    data.insert(data.end(), announce.namespace_name.begin(), announce.namespace_name.end());

    // Track 数量
    uint8_t track_count = static_cast<uint8_t>(announce.tracks.size());
    data.push_back(track_count);

    // 每个 Track 的元数据
    for (const auto& track : announce.tracks) {
        // Track ID
        data.push_back((track.track_id >> 24) & 0xFF);
        data.push_back((track.track_id >> 16) & 0xFF);
        data.push_back((track.track_id >> 8) & 0xFF);
        data.push_back(track.track_id & 0xFF);

        // Track 名称长度 + 内容
        uint16_t name_len = static_cast<uint16_t>(track.track_name.size());
        data.push_back((name_len >> 8) & 0xFF);
        data.push_back(name_len & 0xFF);
        data.insert(data.end(), track.track_name.begin(), track.track_name.end());

        // 可靠性模式
        data.push_back(static_cast<uint8_t>(track.reliability));

        // 默认优先级
        data.push_back(static_cast<uint8_t>(track.default_priority));

        // 编解码器长度 + 内容
        uint16_t codec_len = static_cast<uint16_t>(track.codec.size());
        data.push_back((codec_len >> 8) & 0xFF);
        data.push_back(codec_len & 0xFF);
        data.insert(data.end(), track.codec.begin(), track.codec.end());

        // 时钟频率
        data.push_back((track.clock_rate >> 24) & 0xFF);
        data.push_back((track.clock_rate >> 16) & 0xFF);
        data.push_back((track.clock_rate >> 8) & 0xFF);
        data.push_back(track.clock_rate & 0xFF);
    }

    return data;
}

std::vector<uint8_t> MoQSession::SerializeObject(const Object& object) {
    std::vector<uint8_t> data;

    // 消息类型
    data.push_back(MSG_OBJECT_DATAGRAM);

    // Track ID (4 bytes)
    data.push_back((object.track_id >> 24) & 0xFF);
    data.push_back((object.track_id >> 16) & 0xFF);
    data.push_back((object.track_id >> 8) & 0xFF);
    data.push_back(object.track_id & 0xFF);

    // Group ID (8 bytes)
    for (int i = 7; i >= 0; i--) {
        data.push_back((object.group_id >> (i * 8)) & 0xFF);
    }

    // Object ID (4 bytes)
    data.push_back((object.object_id >> 24) & 0xFF);
    data.push_back((object.object_id >> 16) & 0xFF);
    data.push_back((object.object_id >> 8) & 0xFF);
    data.push_back(object.object_id & 0xFF);

    // 优先级 + 状态
    data.push_back(static_cast<uint8_t>(object.priority));
    data.push_back(static_cast<uint8_t>(object.status));

    // Payload 长度 (4 bytes)
    data.push_back((object.payload_length >> 24) & 0xFF);
    data.push_back((object.payload_length >> 16) & 0xFF);
    data.push_back((object.payload_length >> 8) & 0xFF);
    data.push_back(object.payload_length & 0xFF);

    // Payload
    if (object.payload && object.payload_length > 0) {
        data.insert(data.end(), object.payload.get(),
                   object.payload.get() + object.payload_length);
    }

    // 扩展头长度
    uint16_t ext_len = static_cast<uint16_t>(object.extensions.size());
    data.push_back((ext_len >> 8) & 0xFF);
    data.push_back(ext_len & 0xFF);
    if (ext_len > 0) {
        data.insert(data.end(), object.extensions.begin(), object.extensions.end());
    }

    return data;
}

Object MoQSession::DeserializeObject(const uint8_t* data, uint32_t length) {
    Object object;
    uint32_t offset = 0;

    if (length < 22) return object;  // 最小长度检查

    // Track ID
    object.track_id = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
    offset = 4;

    // Group ID
    object.group_id = 0;
    for (int i = 0; i < 8; i++) {
        object.group_id = (object.group_id << 8) | data[offset + i];
    }
    offset += 8;

    // Object ID
    object.object_id = (data[offset] << 24) | (data[offset+1] << 16) |
                       (data[offset+2] << 8) | data[offset+3];
    offset += 4;

    // 优先级 + 状态
    object.priority = static_cast<ObjectPriority>(data[offset++]);
    object.status = static_cast<ObjectStatus>(data[offset++]);

    // Payload 长度
    object.payload_length = (data[offset] << 24) | (data[offset+1] << 16) |
                            (data[offset+2] << 8) | data[offset+3];
    offset += 4;

    // Payload
    if (object.payload_length > 0 && offset + object.payload_length <= length) {
        object.payload = std::shared_ptr<uint8_t>(
            new uint8_t[object.payload_length], std::default_delete<uint8_t[]>());
        std::memcpy(object.payload.get(), data + offset, object.payload_length);
        offset += object.payload_length;
    }

    // 扩展头
    if (offset + 2 <= length) {
        uint16_t ext_len = (data[offset] << 8) | data[offset + 1];
        offset += 2;
        if (ext_len > 0 && offset + ext_len <= length) {
            object.extensions.assign(data + offset, data + offset + ext_len);
        }
    }

    return object;
}

Announce MoQSession::DeserializeAnnounce(const uint8_t* data, uint32_t length) {
    Announce announce;
    uint32_t offset = 0;

    if (length < 3) return announce;

    // 命名空间长度
    uint16_t ns_len = (data[0] << 8) | data[1];
    offset = 2;

    if (offset + ns_len > length) return announce;
    announce.namespace_name.assign(data + offset, data + offset + ns_len);
    offset += ns_len;

    // Track 数量
    if (offset >= length) return announce;
    uint8_t track_count = data[offset++];

    // 每个 Track
    for (uint8_t i = 0; i < track_count && offset + 15 <= length; i++) {
        TrackMetadata track;

        // Track ID
        track.track_id = (data[offset] << 24) | (data[offset+1] << 16) |
                         (data[offset+2] << 8) | data[offset+3];
        offset += 4;

        // Track 名称
        uint16_t name_len = (data[offset] << 8) | data[offset + 1];
        offset += 2;
        if (offset + name_len > length) break;
        track.track_name.assign(data + offset, data + offset + name_len);
        offset += name_len;

        // 可靠性 + 优先级
        track.reliability = static_cast<TrackReliability>(data[offset++]);
        track.default_priority = static_cast<ObjectPriority>(data[offset++]);

        // 编解码器
        uint16_t codec_len = (data[offset] << 8) | data[offset + 1];
        offset += 2;
        if (offset + codec_len > length) break;
        track.codec.assign(data + offset, data + offset + codec_len);
        offset += codec_len;

        // 时钟频率
        track.clock_rate = (data[offset] << 24) | (data[offset+1] << 16) |
                           (data[offset+2] << 8) | data[offset+3];
        offset += 4;

        announce.tracks.push_back(track);
    }

    return announce;
}

// === 内部方法 ===

bool MoQSession::SendControlMessage(const uint8_t* data, uint32_t length) {
    // 使用 MRUDP 可靠模式发送控制消息
    // 调用 MRUDP API
    return true;  // placeholder
}

bool MoQSession::SendDataStream(TrackId track_id, const Object& object) {
    // 根据 Track 的可靠性模式选择传输方式
    auto it = m_local_tracks.find(track_id);
    if (it == m_local_tracks.end()) return false;

    const auto& track = it->second;
    auto data = SerializeObject(object);

    if (track.reliability == TrackReliability::FULL) {
        // 可靠传输
        // return SendDataBytesToTermByMRUDP(m_remote_tid, data.data(), data.size());
    } else {
        // 不可靠传输
        // return SendDataBytesToTermByMRUDPWithoutReliable(m_remote_tid, data.data(), data.size());
    }

    return true;
}

MoQSession::SessionStats MoQSession::GetStats() const {
    return m_stats;
}

} // namespace moq
```

---

## 4. 与现有 MRUDP 的桥接

### 4.1 MRUDP 作为 MoQ 传输层

```cpp
// ec2/MoQ/MoQOverMRUDP.h
#pragma once
#include "MoQSession.h"
#include "../MRUDP/MRUDPApi.h"

namespace moq {

// MoQ over MRUDP 适配器
class MoQOverMRUDP {
public:
    MoQOverMRUDP(uint32_t self_tid, uint32_t remote_tid)
        : m_session(std::make_unique<MoQSession>(self_tid, remote_tid))
        , m_self_tid(self_tid)
        , m_remote_tid(remote_tid) {}

    // 初始化（注册 MRUDP 回调）
    bool Init() {
        // 注册数据回调
        RegisterMRUDPCallback(m_remote_tid,
            [this](const uint8_t* data, uint32_t length) {
                m_session->OnDataReceived(data, length);
            });
        return true;
    }

    // 获取 MoQ Session
    MoQSession* GetSession() { return m_session.get(); }

    // 发送 Object（通过 MRUDP）
    bool SendObject(const Object& object) {
        auto track_it = m_session->m_local_tracks.find(object.track_id);
        if (track_it == m_session->m_local_tracks.end()) return false;

        const auto& track = track_it->second;
        auto data = m_session->SerializeObject(object);

        if (track.reliability == TrackReliability::FULL) {
            return SendDataBytesToTermByMRUDP(
                m_remote_tid, data.data(), data.size());
        } else {
            return SendDataBytesToTermByMRUDPWithoutReliable(
                m_remote_tid, data.data(), data.size());
        }
    }

private:
    std::unique_ptr<MoQSession> m_session;
    uint32_t m_self_tid;
    uint32_t m_remote_tid;
};

} // namespace moq
```

### 4.2 与 VideoSender 集成

```cpp
// 修改 ec2/ec2_ui_temp_cmake/VideoSender.cpp

#include "../MoQ/MoQOverMRUDP.h"

class VideoSender {
    // ... existing members ...

    // MoQ Session
    std::unique_ptr<moq::MoQOverMRUDP> m_moq;
    moq::TrackId m_video_track_id;
    moq::GroupId m_current_group_id;
    moq::ObjectId m_current_object_id;

    // 初始化 MoQ
    void InitMoQ() {
        m_moq = std::make_unique<moq::MoQOverMRUDP>(m_self_tid, m_remote_tid);
        m_moq->Init();

        // 声明视频 Track
        moq::TrackMetadata video_track;
        video_track.track_id = 1;
        video_track.namespace_name = "live/uav";
        video_track.track_name = "video/h264";
        video_track.reliability = moq::TrackReliability::NONE;
        video_track.default_priority = moq::ObjectPriority::HIGH;
        video_track.codec = "h264";
        video_track.clock_rate = 90000;

        m_moq->GetSession()->Announce(video_track);
        m_video_track_id = 1;
        m_current_group_id = 0;
        m_current_object_id = 0;
    }

    // 发送视频帧（改造为 MoQ Object）
    void SendVideoFrame(const uint8_t* data, uint32_t length,
                        uint32_t rtp_timestamp, bool is_keyframe) {
        moq::Object object;
        object.track_id = m_video_track_id;
        object.group_id = rtp_timestamp / 90000;  // 按秒分组
        object.object_id = m_current_object_id++;
        object.priority = is_keyframe ?
            moq::ObjectPriority::HIGH : moq::ObjectPriority::NORMAL;
        object.status = moq::ObjectStatus::IN_PROGRESS;
        object.payload = std::shared_ptr<uint8_t>(
            new uint8_t[length], std::default_delete<uint8_t[]>());
        std::memcpy(object.payload.get(), data, length);
        object.payload_length = length;
        object.send_timestamp_us = GetCurrentTimeUs();

        m_moq->SendObject(object);

        // 如果是关键帧，发送 EndOfGroup
        if (is_keyframe && m_current_group_id > 0) {
            m_moq->GetSession()->SendEndOfGroup(
                m_video_track_id, m_current_group_id);
            m_current_group_id = object.group_id;
        }
    }
};
```

---

## 5. 协议线格式

### 5.1 Object 线格式

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|    Type=0x20  |                Track ID (32 bits)             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                        Group ID (64 bits)                     |
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                       Object ID (32 bits)                     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
| Priority(4b)  | Status(4b)   |       Payload Length (32b)     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                          Payload ...                          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|    Ext Len    |            Extensions (variable)              |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

### 5.2 Announce 线格式

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|    Type=0x01  |        Namespace Length (16 bits)              |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                     Namespace (variable)                      |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
| Track Count   |                                               |
+-+-+-+-+-+-+-+-+                                               |
|                    Track Metadata (variable)                  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

---

## 6. 测试方案

### 6.1 单元测试

```cpp
// test_moq.cpp

void test_object_serialization() {
    moq::Object obj;
    obj.track_id = 1;
    obj.group_id = 1000;
    obj.object_id = 42;
    obj.priority = moq::ObjectPriority::HIGH;
    obj.status = moq::ObjectStatus::IN_PROGRESS;

    uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04};
    obj.payload = std::shared_ptr<uint8_t>(
        new uint8_t[4], std::default_delete<uint8_t[]>());
    std::memcpy(obj.payload.get(), test_data, 4);
    obj.payload_length = 4;

    // 序列化
    moq::MoQSession session(101, 102);
    auto serialized = session.SerializeObject(obj);

    // 反序列化
    auto deserialized = session.DeserializeObject(
        serialized.data() + 1, serialized.size() - 1);

    assert(deserialized.track_id == 1);
    assert(deserialized.group_id == 1000);
    assert(deserialized.object_id == 42);
    assert(deserialized.payload_length == 4);
    assert(std::memcmp(deserialized.payload.get(), test_data, 4) == 0);
}

void test_pub_sub_workflow() {
    moq::MoQSession publisher(101, 102);
    moq::MoQSession subscriber(102, 101);

    // 设置回调
    bool track_announced = false;
    subscriber.SetOnTrackAnnounced([&](const moq::TrackMetadata& track) {
        track_announced = true;
        assert(track.track_name == "video/h264");
    });

    bool object_received = false;
    subscriber.SetOnObjectReceived([&](const moq::Object& obj) {
        object_received = true;
        assert(obj.payload_length > 0);
    });

    // Publisher 声明 Track
    moq::TrackMetadata track;
    track.track_id = 1;
    track.track_name = "video/h264";
    track.reliability = moq::TrackReliability::NONE;
    publisher.Announce(track);

    // 模拟网络传输...
    // subscriber.OnDataReceived(...)

    assert(track_announced);

    // Publisher 发送 Object
    moq::Object obj;
    obj.track_id = 1;
    obj.group_id = 1;
    obj.object_id = 1;
    obj.payload_length = 100;
    publisher.SendObject(obj);

    // 模拟网络传输...
    // subscriber.OnDataReceived(...)

    assert(object_received);
}
```

### 6.2 集成测试

```bash
# 测试 MoQ over MRUDP

# 启动 Publisher
./ec2_autoconn --mode sender --protocol moq --track video/h264

# 启动 Subscriber
./ec2_autoconn --mode receiver --protocol moq --subscribe video/h264

# 监控 MoQ 统计
# - objects_sent / objects_received
# - bytes_sent / bytes_received
# - groups_sent / groups_received
```

---

## 7. 迁移路径

### 7.1 渐进式迁移

```
阶段 1：MoQ 语义层（当前 Phase）
  - 实现 Track/Group/Object 抽象
  - 在 MRUDP 之上实现 MoQ 语义
  - 验证功能正确性

阶段 2：QUIC 传输层（未来）
  - 引入 msquic 或 lsquic
  - 将 MRUDP 替换为 QUIC
  - 利用 QUIC 的流控和多路复用

阶段 3：标准 MoQ 协议（远期）
  - 完全对齐 IETF MoQ 标准
  - 支持标准化的互操作
```

### 7.2 向后兼容

- 现有 MRUDP 接口保持不变
- MoQ 作为新的应用层协议，通过 MRUDP 传输
- 可以逐步迁移音视频流到 MoQ，文件传输保持 MRUDP
