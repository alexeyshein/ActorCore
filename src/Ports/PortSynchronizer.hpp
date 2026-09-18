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
        AllData = 3  // Wait until ALL N queues have at least one message
    };

    NLOHMANN_JSON_SERIALIZE_ENUM(PortSyncPolicy, {
        {PortSyncPolicy::AnyData, "AnyData"},
        {PortSyncPolicy::ExactId, "ExactId"},
        {PortSyncPolicy::AllData, "AllData"}
        })

        class PortSynchronizer
    {
    public:
        using MessagePtr = std::shared_ptr<IMessage>;
        using MessageBatch = std::vector<MessagePtr>; // Size N matching inputPorts.size()
        using Callback = std::function<void(const MessageBatch&)>;

        /// Synchronizes messages across N input ports using the specified policy.
        /// Calls `callback` for every processed batch.
        /// Returns total number of processed batches.
        static size_t Synchronize(const std::vector<PortInput*>& inputPorts,
            PortSyncPolicy policy,
            Callback callback)
        {
            if (inputPorts.empty() || !callback)
                return 0;

            const size_t numPorts = inputPorts.size();

            // 1. Single port bypass
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

            // 2. Multi-port synchronization policies
            switch (policy)
            {
            case PortSyncPolicy::AnyData:
                return SynchronizeAnyData(inputPorts, callback);

            case PortSyncPolicy::ExactId:
                return SynchronizeExactId(inputPorts, callback);

            case PortSyncPolicy::AllData:
                return SynchronizeAllData(inputPorts, callback);

            default:
                return 0;
            }
        }

        /// Convenience overload accepting std::vector<std::shared_ptr<IPort>>
        static size_t Synchronize(const std::vector<std::shared_ptr<IPort>>& ports,
            PortSyncPolicy policy,
            Callback callback)
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

            return Synchronize(inputPorts, policy, callback);
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
    };
}