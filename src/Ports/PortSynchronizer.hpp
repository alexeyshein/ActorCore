#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <algorithm>
#include <cstdint>

#include "json.hpp"
#include "PortInput.h"
#include "IMessage.h"

namespace rf
{
    /// Low-level synchronization mechanics for input port queues.
    enum class PortSyncPolicy : int
    {
        AnyData = 1, // Process if ANY queue has data (missing ports yield nullptr in batch)
        ExactId = 2, // Exact ID matching across all N ports (drops lagging items)
        AllData = 3,  // Wait until ALL N queues have at least one message
        TimeWindow = 4 // Match heads by timestamp within maxSkew; drop stale messages exceeding maxAge
    };

    NLOHMANN_JSON_SERIALIZE_ENUM(PortSyncPolicy, {
        {PortSyncPolicy::AnyData, "AnyData"},
        {PortSyncPolicy::ExactId, "ExactId"},
        {PortSyncPolicy::AllData, "AllData"},
        {PortSyncPolicy::TimeWindow, "TimeWindow" }
     })
        
    struct TimeWindowParams
    {
        int64_t maxSkewUs = 5000;   // Maximum allowed timestamp difference between sensors (us)
        int64_t maxAgeUs = 100000;  // Maximum message age before it is discarded (us), 0 = no limit
    };

    class PortSynchronizer
    {
    public:
        using MessagePtr = std::shared_ptr<IMessage>;
        using MessageBatch = std::vector<MessagePtr>; // Size N matching inputPorts.size()
        using Callback = std::function<void(const MessageBatch&)>;

        /// 1. MAIN SYNCHRONIZER (for PortInput*)
        /// Synchronizes messages across N input ports using the specified policy.
        /// Calls `callback` for every processed batch.
        /// Returns total number of processed batches.
        static size_t Synchronize(
            const std::vector<PortInput*>& inputPorts,
            PortSyncPolicy policy,
            Callback callback,
            const TimeWindowParams& twParams = {}) // Handles all policies. Default is empty.
        {
            if (inputPorts.empty() || !callback)
                return 0;

            const size_t numPorts = inputPorts.size();

            // Single port bypass
            if (numPorts == 1)
            {
                size_t count = 0;
                MessagePtr msg;
                while (inputPorts[0]->GetMessageQueueRef().try_pop_front(msg))
                {
                    if (msg)
                    {
                        callback({ msg });
                        ++count;
                    }
                }
                return count;
            }

            // Multi-port synchronization policies
            switch (policy)
            {
            case PortSyncPolicy::AnyData:    return SynchronizeAnyData(inputPorts, callback);
            case PortSyncPolicy::ExactId:    return SynchronizeExactId(inputPorts, callback);
            case PortSyncPolicy::AllData:    return SynchronizeAllData(inputPorts, callback);
            case PortSyncPolicy::TimeWindow: return SynchronizeTimeWindow(inputPorts, callback, twParams);
            default: return 0;
            }
        }

        /// 2. CONVENIENCE OVERLOAD (for std::shared_ptr<IPort>)
        /// Converts shared_ptr<IPort> to PortInput* and calls the main synchronizer.
        static size_t Synchronize(
            const std::vector<std::shared_ptr<IPort>>& ports,
            PortSyncPolicy policy,
            Callback callback,
            const TimeWindowParams& twParams = {}) // Added default parameter here too
        {
            std::vector<PortInput*> inputPorts;
            inputPorts.reserve(ports.size());

            for (const auto& p : ports)
            {
                if (auto* inp = dynamic_cast<PortInput*>(p.get()))
                {
                    inputPorts.push_back(inp);
                }
            }

            if (inputPorts.size() != ports.size())
            {
                // One or more ports in the list were null or not PortInput
                return 0;
            }

            // Forward to the main method with all parameters
            return Synchronize(inputPorts, policy, callback, twParams);
        }

    private:
        // ===================================================================
        // 1. POLICY: AnyData (Process if ANY queue has data, nullptr for empty)
        // ===================================================================
        static size_t SynchronizeAnyData(const std::vector<PortInput*>& inputPorts,
            const Callback& callback)
        {
            const size_t numPorts = inputPorts.size();
            size_t batchCount = 0;

            while (true)
            {
                // Check if AT LEAST ONE queue has data
                bool anyHaveData = false;
                for (size_t i = 0; i < numPorts; ++i)
                {
                    if (!inputPorts[i]->GetMessageQueueRef().empty())
                    {
                        anyHaveData = true;
                        break;
                    }
                }

                if (!anyHaveData)
                    break; // All queues are empty

                // Pop available messages (nullptr for ports that currently have no data)
                MessageBatch batch(numPorts, nullptr);
                bool poppedAny = false;

                for (size_t i = 0; i < numPorts; ++i)
                {
                    MessagePtr msg;
                    if (inputPorts[i]->GetMessageQueueRef().try_pop_front(msg))
                    {
                        batch[i] = std::move(msg);
                        poppedAny = true;
                    }
                }

                if (poppedAny)
                {
                    callback(batch);
                    ++batchCount;
                }
            }

            return batchCount;
        }

        // ===================================================================
        // 2. POLICY: AllData (Process ONLY when ALL N queues have data)
        // ===================================================================
        static size_t SynchronizeAllData(const std::vector<PortInput*>& inputPorts,
            const Callback& callback)
        {
            const size_t numPorts = inputPorts.size();
            size_t batchCount = 0;

            while (true)
            {
                // Check if ALL N queues have data
                bool allHaveData = true;
                for (size_t i = 0; i < numPorts; ++i)
                {
                    if (inputPorts[i]->GetMessageQueueRef().empty())
                    {
                        allHaveData = false;
                        break;
                    }
                }

                if (!allHaveData)
                    break; // Cannot form a complete batch

                MessageBatch batch;
                batch.reserve(numPorts);
                bool success = true;

                for (size_t i = 0; i < numPorts; ++i)
                {
                    MessagePtr msg;
                    if (inputPorts[i]->GetMessageQueueRef().try_pop_front(msg))
                    {
                        batch.push_back(std::move(msg));
                    }
                    else
                    {
                        success = false;
                        break;
                    }
                }

                if (success && batch.size() == numPorts)
                {
                    callback(batch);
                    ++batchCount;
                }
            }

            return batchCount;
        }

        // ===================================================================
        // 3. POLICY: ExactId (Exact ID matching across N ports)
        // ===================================================================
        static size_t SynchronizeExactId(const std::vector<PortInput*>& inputPorts,
            const Callback& callback)
        {
            const size_t numPorts = inputPorts.size();
            size_t batchCount = 0;

            std::vector<MessagePtr> headPeek(numPorts);
            std::vector<uint64_t>   headIds(numPorts);

            while (true)
            {
                // Peek at the heads of all N queues atomically
                bool allHaveHeads = true;
                for (size_t i = 0; i < numPorts; ++i)
                {
                    auto opt = inputPorts[i]->GetMessageQueueRef().try_front();
                    if (!opt.has_value())
                    {
                        allHaveHeads = false;
                        break;
                    }
                    headPeek[i] = *opt;
                    headIds[i] = headPeek[i] ? headPeek[i]->Id() : 0;
                }

                if (!allHaveHeads)
                    break; // One or more queues are empty

                // Discard any null messages found at head
                bool foundNull = false;
                for (size_t i = 0; i < numPorts; ++i)
                {
                    if (!headPeek[i])
                    {
                        auto badMsg = headPeek[i];
                        inputPorts[i]->GetMessageQueueRef().pop_front_if(
                            [&badMsg](const MessagePtr& item) { return item == badMsg; });
                        foundNull = true;
                    }
                }
                if (foundNull)
                    continue;

                // Find max ID among all queue heads
                uint64_t maxId = headIds[0];
                for (size_t i = 1; i < numPorts; ++i)
                {
                    if (headIds[i] > maxId)
                        maxId = headIds[i];
                }

                // Check if ALL N head IDs equal maxId
                bool allMatch = true;
                for (size_t i = 0; i < numPorts; ++i)
                {
                    if (headIds[i] != maxId)
                    {
                        allMatch = false;
                        break;
                    }
                }

                if (allMatch)
                {
                    // Full match! Atomically pop the matched items from all N queues
                    MessageBatch batch;
                    batch.reserve(numPorts);
                    bool popSuccess = true;

                    for (size_t i = 0; i < numPorts; ++i)
                    {
                        auto targetMsg = headPeek[i];
                        MessagePtr popped;
                        bool ok = inputPorts[i]->GetMessageQueueRef().pop_front_if(
                            [&targetMsg](const MessagePtr& item) { return item == targetMsg; },
                            popped);

                        if (ok && popped)
                        {
                            batch.push_back(std::move(popped));
                        }
                        else
                        {
                            popSuccess = false;
                            break;
                        }
                    }

                    if (popSuccess && batch.size() == numPorts)
                    {
                        callback(batch);
                        ++batchCount;
                    }
                }
                else
                {
                    // Mismatch! Safely drop lagging messages (where headId < maxId)
                    for (size_t i = 0; i < numPorts; ++i)
                    {
                        if (headIds[i] < maxId)
                        {
                            auto targetMsg = headPeek[i];
                            inputPorts[i]->GetMessageQueueRef().pop_front_if(
                                [&targetMsg](const MessagePtr& item) { return item == targetMsg; });
                        }
                    }
                }
            }

            return batchCount;
        }
    

        // ===================================================================
        // 4. POLICY: TimeWindow (Timestamp matching within maxSkew, stale purge)
        // ===================================================================
        static size_t SynchronizeTimeWindow(
            const std::vector<PortInput*>& inputPorts,
            const Callback& callback,
            const TimeWindowParams& params)
        {
            const size_t numPorts = inputPorts.size();
            size_t batchCount = 0;

            auto nowUs = []() -> int64_t {
                return std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                };

            while (true)
            {
                // ── Step 0: Purge stale messages exceeding maxAge ──
                if (params.maxAgeUs > 0)
                {
                    int64_t now = nowUs();
                    for (size_t i = 0; i < numPorts; ++i)
                    {
                        auto& q = inputPorts[i]->GetMessageQueueRef();
                        while (true)
                        {
                            auto opt = q.try_front();
                            if (!opt.has_value() || !(*opt))
                                break;
                            int64_t age = now - static_cast<int64_t>((*opt)->Timestamp());
                            if (age > params.maxAgeUs)
                            {
                                MessagePtr dummy;
                                q.pop_front_if(
                                    [&msg = *opt](const MessagePtr& item) { return item == msg; },
                                    dummy);
                            }
                            else
                            {
                                break; // queue is ordered by time
                            }
                        }
                    }
                }

                // ── Step 1: Peek at all queue heads ──
                std::vector<MessagePtr> heads(numPorts);
                std::vector<int64_t> timestamps(numPorts);
                bool allHaveHeads = true;

                for (size_t i = 0; i < numPorts; ++i)
                {
                    auto opt = inputPorts[i]->GetMessageQueueRef().try_front();
                    if (!opt.has_value() || !(*opt))
                    {
                        allHaveHeads = false;
                        break;
                    }
                    heads[i] = *opt;
                    timestamps[i] = static_cast<int64_t>(heads[i]->Timestamp());
                }

                if (!allHaveHeads)
                    break; // waiting for data

                // ── Step 2: Check timestamp spread across all heads ──
                int64_t minTs = timestamps[0];
                int64_t maxTs = timestamps[0];
                size_t minIdx = 0;

                for (size_t i = 1; i < numPorts; ++i)
                {
                    if (timestamps[i] < minTs) { minTs = timestamps[i]; minIdx = i; }
                    if (timestamps[i] > maxTs) { maxTs = timestamps[i]; }
                }

                int64_t skew = maxTs - minTs;

                if (skew <= params.maxSkewUs)
                {
                    // ── Match! Pop all heads and invoke callback ──
                    MessageBatch batch;
                    batch.reserve(numPorts);
                    bool popOk = true;

                    for (size_t i = 0; i < numPorts; ++i)
                    {
                        auto target = heads[i];
                        MessagePtr popped;
                        bool ok = inputPorts[i]->GetMessageQueueRef().pop_front_if(
                            [&target](const MessagePtr& item) { return item == target; },
                            popped);

                        if (ok && popped)
                            batch.push_back(std::move(popped));
                        else
                        {
                            popOk = false;
                            break;
                        }
                    }

                    if (popOk && batch.size() == numPorts)
                    {
                        callback(batch);
                        ++batchCount;
                    }
                }
                else
                {
                    // ── Desync! Drop the oldest (lagging) head ──
                    auto target = heads[minIdx];
                    inputPorts[minIdx]->GetMessageQueueRef().pop_front_if(
                        [&target](const MessagePtr& item) { return item == target; });
                }
            }

            return batchCount;
        }

};
}