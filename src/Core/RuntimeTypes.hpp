#pragma once

#include <cstdint>
#include <string>

namespace rf
{
    /// Administrative state (set by user)
    enum class AdminState : uint8_t
    {
        Inactive = 0,
        Active = 1
    };

    /// What the actor is actually doing right now
    enum class RuntimeState : uint8_t
    {
        Inactive = 0,
        Idle = 1,
        Processing = 2,
        Backpressured = 3,
        Stopping = 4
    };

    /// Health assessment
    enum class HealthState : uint8_t
    {
        Ok = 0,
        Warning = 1,
        Error = 2
    };

    enum class OperabilityState : uint8_t
    {
        Normal = 0,
        Limited = 1,
        Unavailable = 2
    };

    inline const char* ToString(AdminState s)
    {
        switch (s)
        {
        case AdminState::Inactive: return "inactive";
        case AdminState::Active:   return "active";
        default:                   return "unknown";
        }
    }

    inline const char* ToString(RuntimeState s)
    {
        switch (s)
        {
        case RuntimeState::Inactive:      return "inactive";
        case RuntimeState::Idle:          return "idle";
        case RuntimeState::Processing:    return "processing";
        case RuntimeState::Backpressured: return "backpressured";
        case RuntimeState::Stopping:      return "stopping";
        default:                          return "unknown";
        }
    }

    inline const char* ToString(HealthState s)
    {
        switch (s)
        {
        case HealthState::Ok:      return "ok";
        case HealthState::Warning: return "warning";
        case HealthState::Error:   return "error";
        default:                   return "unknown";
        }
    }

    inline const char* ToString(OperabilityState s)
    {
        switch (s)
        {
        case OperabilityState::Normal:      return "normal";
        case OperabilityState::Limited:     return "limited";
        case OperabilityState::Unavailable: return "unavailable";
        default:                            return "unknown";
        }
    }

    inline OperabilityState OperabilityStateFromString(const std::string& s)
    {
        if (s == "normal")      return OperabilityState::Normal;
        if (s == "limited")     return OperabilityState::Limited;
        if (s == "unavailable") return OperabilityState::Unavailable;
        return OperabilityState::Normal;
    }
}