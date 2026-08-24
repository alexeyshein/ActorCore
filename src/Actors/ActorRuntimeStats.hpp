#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include "json.hpp"
#include "RuntimeClock.hpp"
#include "RuntimeTypes.hpp"

namespace rf
{
    /// Lightweight runtime statistics for a single actor.
    /// Designed to be embedded into ActorLocal / ActorEventBased / ActorBlocking.
    /// All fields are atomic — safe to read from any thread.
    struct ActorRuntimeStats
    {
        std::atomic<uint64_t>* pGlobalRevision{ nullptr };
        // --- revision ---
        std::atomic<uint64_t> revision{ 0 };     // 

        // --- counters ---
        std::atomic<uint64_t> receivedTotal{ 0 };
        std::atomic<uint64_t> processedTotal{ 0 };
        std::atomic<uint64_t> sentTotal{ 0 };
        std::atomic<uint64_t> errorTotal{ 0 };
        std::atomic<uint64_t> droppedTotal{ 0 };

        // --- timestamps ---
        std::atomic<uint64_t> lastInputTs{ 0 };
        std::atomic<uint64_t> lastProcessStartTs{ 0 };
        std::atomic<uint64_t> lastProcessEndTs{ 0 };
        std::atomic<uint64_t> lastOutputTs{ 0 };
        std::atomic<uint64_t> lastErrorTs{ 0 };

        // --- processing ---
        std::atomic<bool>     isProcessingNow{ false };
        std::atomic<uint64_t> totalProcessTimeUs{ 0 };
        std::atomic<uint64_t> maxProcessTimeUs{ 0 };

        // --- helpers ---

        /// Call this after any stats change to bump revision
        void Touch()
        {
            uint64_t nextRev = pGlobalRevision
                ? pGlobalRevision->fetch_add(1, std::memory_order_relaxed) + 1
                : revision.fetch_add(1, std::memory_order_relaxed) + 1;
            revision.store(nextRev, std::memory_order_relaxed);
        }

        uint64_t AvgProcessTimeUs() const
        {
            uint64_t n = processedTotal.load(std::memory_order_relaxed);
            return n > 0
                ? totalProcessTimeUs.load(std::memory_order_relaxed) / n
                : 0;
        }

        void Reset()
        {
            revision.store(0, std::memory_order_relaxed);
            receivedTotal.store(0, std::memory_order_relaxed);
            processedTotal.store(0, std::memory_order_relaxed);
            sentTotal.store(0, std::memory_order_relaxed);
            errorTotal.store(0, std::memory_order_relaxed);
            droppedTotal.store(0, std::memory_order_relaxed);
            lastInputTs.store(0, std::memory_order_relaxed);
            lastProcessStartTs.store(0, std::memory_order_relaxed);
            lastProcessEndTs.store(0, std::memory_order_relaxed);
            lastOutputTs.store(0, std::memory_order_relaxed);
            lastErrorTs.store(0, std::memory_order_relaxed);
            isProcessingNow.store(false, std::memory_order_relaxed);
            totalProcessTimeUs.store(0, std::memory_order_relaxed);
            maxProcessTimeUs.store(0, std::memory_order_relaxed);
        }

        nlohmann::json ToJson() const
        {
            nlohmann::json j;
            j["revision"] = revision.load(std::memory_order_relaxed);
            j["receivedTotal"] = receivedTotal.load(std::memory_order_relaxed);
            j["processedTotal"] = processedTotal.load(std::memory_order_relaxed);
            j["sentTotal"] = sentTotal.load(std::memory_order_relaxed);
            j["errorTotal"] = errorTotal.load(std::memory_order_relaxed);
            j["droppedTotal"] = droppedTotal.load(std::memory_order_relaxed);
            j["lastInputTs"] = lastInputTs.load(std::memory_order_relaxed);
            j["lastProcessStartTs"] = lastProcessStartTs.load(std::memory_order_relaxed);
            j["lastProcessEndTs"] = lastProcessEndTs.load(std::memory_order_relaxed);
            j["lastOutputTs"] = lastOutputTs.load(std::memory_order_relaxed);
            j["lastErrorTs"] = lastErrorTs.load(std::memory_order_relaxed);
            j["isProcessingNow"] = isProcessingNow.load(std::memory_order_relaxed);
            j["avgProcessTimeUs"] = AvgProcessTimeUs();
            j["maxProcessTimeUs"] = maxProcessTimeUs.load(std::memory_order_relaxed);
            return j;
        }
    };
}