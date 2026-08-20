#pragma once

#include <mutex>
#include <deque>
#include <vector>
#include <string>
#include <unordered_map>
#include <atomic>

#include "FlowTraceTypes.hpp"
#include "json.hpp"

namespace rf
{
    class FlowTraceRecorder
    {
    public:
        explicit FlowTraceRecorder(size_t capacity = 100000);
        ~FlowTraceRecorder() = default;

        // --- recording ---
        void Record(const FlowTraceEvent& event);
        void SetEnabled(bool enabled);
        bool IsEnabled() const;

        // --- string registry ---
        void RegisterString(const std::string& str);
        std::string ResolveHash(uint32_t hash) const;

        // --- core query API ---
        nlohmann::json GetInfo() const;
        nlohmann::json QueryRange(uint64_t fromTs, uint64_t toTs, size_t limit) const;
        nlohmann::json QueryByMessage(uint64_t messageId, size_t limit) const;
        nlohmann::json QueryByActor(const std::string& actorId,
            uint64_t fromTs, uint64_t toTs,
            size_t limit) const;

        // --- management ---
        size_t Size() const;
        void Clear();
        size_t Capacity() const { return _capacity; }
        void SetCapacity(size_t capacity);

    private:
        struct QueryResult
        {
            std::vector<FlowTraceEvent> events;
            bool hasMore = false;
        };

        nlohmann::json BuildResponse(const QueryResult& result,
            uint64_t queryFromTs = 0,
            uint64_t queryToTs = 0) const;

        nlohmann::json EventToJson(const FlowTraceEvent& event) const;

        // buffer metadata (must be called under _mutex)
        uint64_t OldestTs_locked() const;
        uint64_t NewestTs_locked() const;

    private:
        size_t _capacity;
        std::atomic<bool> _enabled{ false };
        mutable std::mutex _mutex;
        std::deque<FlowTraceEvent> _buffer;

        mutable std::mutex _registryMutex;
        std::unordered_map<uint32_t, std::string> _hashRegistry;
    };
}