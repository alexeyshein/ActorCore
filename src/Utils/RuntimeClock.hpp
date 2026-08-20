#pragma once

#include <cstdint>
#include <chrono>

namespace rf
{
    /// Monotonic microseconds — for duration measurement and timestamps
    inline uint64_t SteadyTimeUs()
    {
        return static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now().time_since_epoch()
            ).count()
            );
    }

    /// Monotonic milliseconds
    inline uint64_t SteadyTimeMs()
    {
        return static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()
            ).count()
            );
    }

    /// Wall-clock milliseconds since epoch — for human-readable timestamps
    inline uint64_t WallTimeMs()
    {
        return static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()
            );
    }
}