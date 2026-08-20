#include "FlowTraceRecorder.h"
#include "RuntimeClock.hpp"

using rf::FlowTraceRecorder;
using rf::FlowTraceEvent;
using nlohmann::json;

FlowTraceRecorder::FlowTraceRecorder(size_t capacity)
    : _capacity(capacity)
{}

// ============================================================
// Recording
// ============================================================

void FlowTraceRecorder::Record(const FlowTraceEvent& event)
{
    if (!_enabled.load(std::memory_order_relaxed))
        return;

    std::lock_guard<std::mutex> lock(_mutex);
    if (_buffer.size() >= _capacity)
        _buffer.pop_front();
    _buffer.push_back(event);
}

void FlowTraceRecorder::SetEnabled(bool enabled)
{
    _enabled.store(enabled, std::memory_order_relaxed);
}

bool FlowTraceRecorder::IsEnabled() const
{
    return _enabled.load(std::memory_order_relaxed);
}

// ============================================================
// String registry
// ============================================================

void FlowTraceRecorder::RegisterString(const std::string& str)
{
    uint32_t hash = FlowTraceHash(str);
    std::lock_guard<std::mutex> lock(_registryMutex);
    _hashRegistry[hash] = str;
}

std::string FlowTraceRecorder::ResolveHash(uint32_t hash) const
{
    std::lock_guard<std::mutex> lock(_registryMutex);
    auto it = _hashRegistry.find(hash);
    if (it != _hashRegistry.end())
        return it->second;
    return "";
}

// ============================================================
// Core query API
// ============================================================

json FlowTraceRecorder::GetInfo() const
{
    json info;
    info["enabled"] = _enabled.load(std::memory_order_relaxed);

    {
        std::lock_guard<std::mutex> lock(_mutex);
        info["bufferSize"] = _buffer.size();
        info["bufferCapacity"] = _capacity;
        info["oldestTs"] = OldestTs_locked();
        info["newestTs"] = NewestTs_locked();
    }

    info["serverSteadyTs"] = SteadyTimeUs();
    info["serverWallTimeMs"] = WallTimeMs();

    return info;
}

json FlowTraceRecorder::QueryRange(uint64_t fromTs, uint64_t toTs, size_t limit) const
{
    if (limit == 0)
        limit = _capacity;

    QueryResult result;

    {
        std::lock_guard<std::mutex> lock(_mutex);

        for (auto it = _buffer.begin(); it != _buffer.end(); ++it)
        {
            // skip events before range
            if (fromTs > 0 && it->timestamp < fromTs)
                continue;

            // stop after range
            if (toTs > 0 && it->timestamp > toTs)
                break;

            if (result.events.size() >= limit)
            {
                result.hasMore = true;
                break;
            }

            result.events.push_back(*it);
        }
    }

    return BuildResponse(result, fromTs, toTs);
}

json FlowTraceRecorder::QueryByMessage(uint64_t messageId, size_t limit) const
{
    if (limit == 0)
        limit = _capacity;

    QueryResult result;

    {
        std::lock_guard<std::mutex> lock(_mutex);

        for (const auto& event : _buffer)
        {
            if (event.messageId == messageId)
            {
                if (result.events.size() >= limit)
                {
                    result.hasMore = true;
                    break;
                }
                result.events.push_back(event);
            }
        }
    }

    return BuildResponse(result);
}

json FlowTraceRecorder::QueryByActor(const std::string& actorId,
    uint64_t fromTs, uint64_t toTs,
    size_t limit) const
{
    uint32_t actorHash = FlowTraceHash(actorId);

    if (limit == 0)
        limit = _capacity;

    QueryResult result;

    {
        std::lock_guard<std::mutex> lock(_mutex);

        for (auto it = _buffer.begin(); it != _buffer.end(); ++it)
        {
            if (fromTs > 0 && it->timestamp < fromTs)
                continue;

            if (toTs > 0 && it->timestamp > toTs)
                break;

            if (it->actorHash == actorHash || it->linkedActorHash == actorHash)
            {
                if (result.events.size() >= limit)
                {
                    result.hasMore = true;
                    break;
                }
                result.events.push_back(*it);
            }
        }
    }

    return BuildResponse(result, fromTs, toTs);
}

// ============================================================
// Management
// ============================================================

size_t FlowTraceRecorder::Size() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _buffer.size();
}

void FlowTraceRecorder::Clear()
{
    std::lock_guard<std::mutex> lock(_mutex);
    _buffer.clear();
}

void FlowTraceRecorder::SetCapacity(size_t capacity)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _capacity = capacity;
    _buffer.clear();
}

// ============================================================
// Private helpers
// ============================================================

uint64_t FlowTraceRecorder::OldestTs_locked() const
{
    return _buffer.empty() ? 0 : _buffer.front().timestamp;
}

uint64_t FlowTraceRecorder::NewestTs_locked() const
{
    return _buffer.empty() ? 0 : _buffer.back().timestamp;
}

json FlowTraceRecorder::BuildResponse(const QueryResult& result,
    uint64_t queryFromTs,
    uint64_t queryToTs) const
{
    json response;

    // events
    json eventsJson = json::array();
    for (const auto& event : result.events)
    {
        eventsJson.emplace_back(EventToJson(event));
    }
    response["events"] = std::move(eventsJson);

    // query metadata
    response["returnedCount"] = result.events.size();
    response["hasMore"] = result.hasMore;

    if (queryFromTs > 0 || queryToTs > 0)
    {
        response["queryFromTs"] = queryFromTs;
        response["queryToTs"] = queryToTs;
    }

    // buffer metadata
    {
        std::lock_guard<std::mutex> lock(_mutex);
        response["oldestTs"] = OldestTs_locked();
        response["newestTs"] = NewestTs_locked();
    }

    // server time reference
    response["serverSteadyTs"] = SteadyTimeUs();

    return response;
}

json FlowTraceRecorder::EventToJson(const FlowTraceEvent& event) const
{
    json j;
    j["ts"] = event.timestamp;
    j["type"] = ToString(event.type);
    j["messageId"] = event.messageId;
    j["messageType"] = event.messageType;
    j["extra"] = event.extra;

    {
        std::lock_guard<std::mutex> regLock(_registryMutex);

        auto resolve = [&](uint32_t hash) -> std::string {
            if (hash == 0)
                return "";
            auto it = _hashRegistry.find(hash);
            return (it != _hashRegistry.end()) ? it->second : std::to_string(hash);
            };

        j["actorId"] = resolve(event.actorHash);
        j["portId"] = resolve(event.portHash);
        j["linkedActorId"] = resolve(event.linkedActorHash);
        j["linkedPortId"] = resolve(event.linkedPortHash);
    }

    return j;
}