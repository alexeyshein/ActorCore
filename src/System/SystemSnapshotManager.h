#pragma once

#include <deque>
#include <mutex>
#include <memory>

#include <string>
#include  <utility>
#include <json.hpp>
#include "ISnapshotable.h"

namespace rf
{
    using json = nlohmann::json;

    class SystemSnapshotManager
    {
    public:
        SystemSnapshotManager() = delete;
        ~SystemSnapshotManager() = default;
        explicit  SystemSnapshotManager(std::weak_ptr<ISnapshotable>);
        void Save(const std::string& caption="update");
        void Undo();
        void Redo();
        void Undo(size_t operations);
        void Redo(size_t operations);
        // Проверка доступности операций
        bool CanUndo() const { return undoDeque.size() > 1; }
        bool CanRedo() const { return !redoDeque.empty(); }
        json GetAllCaptionsAsJson();
        bool Clear();
        void SetMaxUndoSize(size_t size);
        size_t GetMaxUndoSize() const { return maxUndoSize; }
    protected:
        void ClearRedoStack();
        bool IsSnapshotDifferent(const json& newSnapshot) const;
    
    protected:
        mutable std::mutex mutex;  // Protects all member access
        std::deque<std::pair<std::string,json>> undoDeque;
        std::deque<std::pair<std::string, json>> redoDeque;
        std::weak_ptr<ISnapshotable> systemLocalWptr;
        size_t maxUndoSize;
    };
}
