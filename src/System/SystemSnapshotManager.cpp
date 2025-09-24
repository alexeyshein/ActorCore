#include "SystemSnapshotManager.h"

using nlohmann::json;
using std::string;
using std::pair;
using rf::SystemSnapshotManager;
using rf::ISnapshotable;

static constexpr size_t DEFAULT_MAX_UNDO_SIZE = 50;

SystemSnapshotManager::SystemSnapshotManager(std::weak_ptr<ISnapshotable> sysWptr):
    systemLocalWptr(sysWptr)
    , maxUndoSize(DEFAULT_MAX_UNDO_SIZE)
{
}

void SystemSnapshotManager::Save(const std::string& caption)
{
    auto system = systemLocalWptr.lock();
    if (!system)
        return;

    std::lock_guard<std::mutex> lock(mutex);

    auto snapshot = system->CreateMemento();

    // Only save if the new snapshot is different from the last saved state
    if (IsSnapshotDifferent(snapshot))
    {
        undoDeque.push_back({ std::move(caption),std::move(snapshot) });
        // Trim old entries if exceeded max size
        while (undoDeque.size() > maxUndoSize) {
            undoDeque.pop_front();  // Remove oldest
        }
        ClearRedoStack();  // New action invalidates redo history
    }
}

void SystemSnapshotManager::Undo()
{
    std::lock_guard<std::mutex> lock(mutex);

    auto system = systemLocalWptr.lock();
    if (!system || !CanUndo())
        return;

    // Move current state to redo stack
    redoDeque.push_back(std::move(undoDeque.back()));
    undoDeque.pop_back();

    // Restore previous state
    system->SetMemento(undoDeque.back().second);
}

void SystemSnapshotManager::Redo()
{
    std::lock_guard<std::mutex> lock(mutex);

    auto system = systemLocalWptr.lock();
    
    if (!system || !CanRedo())
        return;

    // Save current state before redo
    undoDeque.push_back(redoDeque.back());

    // Restore redo state
    system->SetMemento(redoDeque.back().second);
    redoDeque.pop_back();
}

void SystemSnapshotManager::Undo(size_t steps)
{
    std::lock_guard<std::mutex> lock(mutex);

    auto system = systemLocalWptr.lock();
    if (!system)
        return;
    
    // Limit steps to available undo operations
    size_t realSteps = std::min(steps, undoDeque.size() - 1);
    if (realSteps == 0) return;

    // Move states from undo to redo
    for (size_t i = 0; i < realSteps; ++i)
    {
        redoDeque.push_back(std::move(undoDeque.back()));
        undoDeque.pop_back();
    }

    // Restore previous state
    system->SetMemento(undoDeque.back().second);
}

void SystemSnapshotManager::Redo(size_t steps)
{
    std::lock_guard<std::mutex> lock(mutex);

    auto system = systemLocalWptr.lock();
    if (!system)
        return;

    // Limit steps to available redo operations
    size_t realSteps = std::min(steps, redoDeque.size());
    if (realSteps == 0) return;

    // Move states from redo to undo
    for (size_t i = 0; i < realSteps; ++i)
    {
        undoDeque.push_back(redoDeque.back());
        redoDeque.pop_back();
    }
    system->SetMemento(undoDeque.back().second);
}

json SystemSnapshotManager::GetAllCaptionsAsJson() {
    std::lock_guard<std::mutex> lock(mutex);

    json result;
    result["undoCaptions"] = json::array();
    result["redoCaptions"] = json::array();

    // Extract from undo deque (already in correct order)
    for (const auto& [caption, _] : undoDeque) {
        result["undoCaptions"].push_back(caption);
    }

    // Extract from redo deque (already in correct order)
    for (const auto& [caption, _] : redoDeque) {
        result["redoCaptions"].push_back(caption);
    }

    return result;
}

bool SystemSnapshotManager::Clear()
{
    std::lock_guard<std::mutex> lock(mutex);

    undoDeque.clear();
    redoDeque.clear();

    return true;
}

void SystemSnapshotManager::ClearRedoStack() {
    redoDeque.clear();
}

bool SystemSnapshotManager::IsSnapshotDifferent(const json& newSnapshot) const
{
    if (undoDeque.empty())
        return true;

    const size_t newHash = std::hash<json>{}(newSnapshot);
    const size_t lastHash = std::hash<json>{}(undoDeque.back().second);

    return newHash != lastHash;
}

void SystemSnapshotManager::SetMaxUndoSize(size_t size)
{
    std::lock_guard<std::mutex> lock(mutex);
    maxUndoSize = size;

    // Trim if current size exceeds new limit
    while (undoDeque.size() > maxUndoSize) {
        undoDeque.pop_front();
    }
}
