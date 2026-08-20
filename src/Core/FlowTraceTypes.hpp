#pragma once

#include <cstdint>
#include <string>
#include <functional>
#include "json.hpp"
#include "RuntimeClock.hpp"

namespace rf
{
    enum class FlowTraceEventType : uint8_t
    {
        // Port-level
        PortReceive = 1,   // input port received a message
        PortDrop = 2,   // input port dropped (queue full, mode=Nothing)
        PortNotify = 3,   // output port sent to observers

        // Actor-level
        ProcessStart = 10,
        ProcessEnd = 11,
        ProcessError = 12,
        TaskQueued = 13,  // async task placed in futures queue
        TaskDropped = 14,  // async task dropped (futures queue full)

        // Lifecycle
        ActorActivated = 20,
        ActorDeactivated = 21,

        // Links
        LinkConnected = 30,
        LinkDisconnected = 31,

        // Health
        HealthChanged = 40,
        QueueOverflow = 41,
    };

    inline const char* ToString(FlowTraceEventType t)
    {
        switch (t)
        {
        case FlowTraceEventType::PortReceive:      return "PortReceive";
        case FlowTraceEventType::PortDrop:         return "PortDrop";
        case FlowTraceEventType::PortNotify:       return "PortNotify";
        case FlowTraceEventType::ProcessStart:     return "ProcessStart";
        case FlowTraceEventType::ProcessEnd:       return "ProcessEnd";
        case FlowTraceEventType::ProcessError:     return "ProcessError";
        case FlowTraceEventType::TaskQueued:       return "TaskQueued";
        case FlowTraceEventType::TaskDropped:      return "TaskDropped";
        case FlowTraceEventType::ActorActivated:   return "ActorActivated";
        case FlowTraceEventType::ActorDeactivated: return "ActorDeactivated";
        case FlowTraceEventType::LinkConnected:    return "LinkConnected";
        case FlowTraceEventType::LinkDisconnected: return "LinkDisconnected";
        case FlowTraceEventType::HealthChanged:    return "HealthChanged";
        case FlowTraceEventType::QueueOverflow:    return "QueueOverflow";
        default:                                   return "Unknown";
        }
    }

    /// Compact event stored in ring buffer.
    /// Uses hashes instead of strings on hot path.
    /// ~48 bytes per event.
    struct FlowTraceEvent
    {
        uint64_t           timestamp;        // steady_clock microseconds
        FlowTraceEventType type;
        uint32_t           actorHash;        // hash of actor id
        uint32_t           portHash;         // hash of port id
        uint64_t           messageId;        // IMessage::Id()
        uint16_t           messageType;      // IMessage::Type()
        uint32_t           linkedActorHash;  // hash of remote actor id (sender/receiver)
        uint32_t           linkedPortHash;   // hash of remote port id
        uint32_t           extra;            // duration_us / queue_size / error_code

        nlohmann::json ToJson() const
        {
            nlohmann::json j;
            j["ts"] = timestamp;
            j["type"] = ToString(type);
            j["actorHash"] = actorHash;
            j["portHash"] = portHash;
            j["messageId"] = messageId;
            j["messageType"] = messageType;
            j["linkedActorHash"] = linkedActorHash;
            j["linkedPortHash"] = linkedPortHash;
            j["extra"] = extra;
            return j;
        }
    };

    /// Helper to hash strings consistently
    inline uint32_t FlowTraceHash(const std::string& s)
    {
        return static_cast<uint32_t>(std::hash<std::string>{}(s));
    }
}