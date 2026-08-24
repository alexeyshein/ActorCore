#pragma once

#include <atomic>
#include <cstdint>
#include "json.hpp"
#include "RuntimeClock.hpp"
#include "RuntimeTypes.hpp"

namespace rf
{
    /// Lightweight runtime statistics for a single port.
    /// Designed to be embedded into PortInput / PortOutput.
    /// All fields are atomic — safe to read from any thread.
    struct PortRuntimeStats
    {
        std::atomic<uint64_t>* pGlobalRevision{ nullptr };

        std::atomic<uint64_t> revision{ 0 };      
        std::atomic<uint64_t> messageCount{ 0 };
        std::atomic<uint64_t> droppedCount{ 0 };
        std::atomic<uint64_t> lastActivityTs{ 0 };

        void BumpRevision()
        {
            uint64_t nextRev = pGlobalRevision
                ? pGlobalRevision->fetch_add(1, std::memory_order_relaxed) + 1
                : revision.fetch_add(1, std::memory_order_relaxed) + 1;
            revision.store(nextRev, std::memory_order_relaxed);
        }

        void RecordActivity()
        {
            messageCount.fetch_add(1, std::memory_order_relaxed);
            lastActivityTs.store(SteadyTimeUs(), std::memory_order_relaxed);
            BumpRevision(); 
        }

        void RecordDrop()
        {
            droppedCount.fetch_add(1, std::memory_order_relaxed);
            BumpRevision(); // 
        }

        void Reset()
        {
            revision.store(0, std::memory_order_relaxed);
            messageCount.store(0, std::memory_order_relaxed);
            droppedCount.store(0, std::memory_order_relaxed);
            lastActivityTs.store(0, std::memory_order_relaxed);
        }

        nlohmann::json ToJson() const
        {
            nlohmann::json j;
            j["revision"] = revision.load(std::memory_order_relaxed);
            j["messageCount"] = messageCount.load(std::memory_order_relaxed);
            j["droppedCount"] = droppedCount.load(std::memory_order_relaxed);
            j["lastActivityTs"] = lastActivityTs.load(std::memory_order_relaxed);
            return j;
        }
    };
}