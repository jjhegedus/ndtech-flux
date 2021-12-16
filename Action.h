#pragma once

#include "pch.h"

namespace ndtech {

    static std::atomic<size_t> nextActionId = 0;

    enum class ActionCategory {
        Synchronous,
        AnyThreadImmediate,
        SynchronizeOnMainThread,
        AsyncRequest,
        AsyncCompletion
    };

    enum class ActionRepeatCategory {
        OnlyApplyLatest,
        Repeat,
        DoNotProcessDuplicatesByPredicate
    };

    struct Action {
        size_t actionTypeId;
        std::string typeName;
        std::any payload;
        ActionCategory actionCategory = ActionCategory::Synchronous;
        ActionRepeatCategory actionRepeatCategory = ActionRepeatCategory::Repeat;
        std::function<bool(Action, Action)> predicate = nullptr;
        size_t originatingActionId = (size_t)-1;
        size_t entityId = (size_t)-1;
        size_t actionId = nextActionId.fetch_add(1);
    };


}