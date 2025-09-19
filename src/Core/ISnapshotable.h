#pragma once

#include "json.hpp"

namespace rf
{
    using json = nlohmann::json;

    class ISnapshotable {
    public:
        virtual ~ISnapshotable() = default;
        virtual json CreateMemento() const = 0;
        virtual void SetMemento(const json&) = 0;
    };
}

