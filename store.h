#pragma once

#include "pch.h"

#include "Action.h"
#include "VoidCoroutine.h"
#include "Utilities.h"

using namespace winrt;

using namespace ndtech;

namespace ndtech {

    template<typename DataType>
    class Store;

    //template <typename StoreType>
    //using StoreGetDataType = decltype(std::declval<StoreType>().GetData());

    //template <typename StoreType>
    //constexpr bool HasInitializeEntity() {
    //    return ndtech::TypeUtilities::is_detected<InitializeEntityType, SystemType>::value;
    //}

    class StoreBase {
    public:
        virtual void Dispatch(Action action) {}
        virtual void AwaitDispatch(size_t dispatchId, Action action){}
        virtual void ProcessActions() {};
        template <typename DataType>
        DataType GetStoreData() {
            if (dynamic_cast<Store<DataType>>(this) == nullptr) {
                return nullptr;
            }
            else {
                return (static_cast<Store<DataType>>(this).GetData());
            }
        }
    };

    class EntityStoreBase : public StoreBase {
    };

    template<typename DataType>
    class Store : public StoreBase
    {
    public:

        Store<DataType>() {
            m_cache_line_size = Utilities::cache_line_size();
        }

        Store<DataType>(const Store<DataType>& other) :
            m_data(other.m_data),
            m_effects(other.m_effects),
            m_anyThreadImmediateActions(other.m_anyThreadImmediateActions),
            m_asyncRequestActions(other.m_asyncRequestActions),
            m_asyncRequestActionsInProcess(other.m_asyncRequestActionsInProcess),
            m_asyncCompletionActions(other.m_asyncCompletionActions),
            m_synchronizeOnMainThreadActions(other.m_synchronizeOnMainThreadActions),
            m_cache_line_size(other.m_cache_line_size)
        {
            m_lastAnyThreadActionId.store(other.m_lastAnyThreadActionId);
        }

        void AddAnyThreadImmediateAction(Action action) {

            { // to scope the lock guard
                // TODO: Do we really need to lock here
                std::lock_guard<std::mutex> guard(m_anyThreadImmediateActionsMutex);

                auto foundRelatedEffect = std::find_if(
                    m_effects.begin(),
                    m_effects.end(),
                    [action](std::tuple<int, int, std::function<DataType(DataType, Action)>> effectTuple) {
                    return std::get<1>(effectTuple) == action.actionTypeId;
                }
                );

                if (foundRelatedEffect != m_effects.end()) {
                    m_anyThreadImmediateActions.push_back(action);
                }
            }
        }

        void ProcessAnyThreadImmediateActions() {
            TraceLogWrite("ndtech::Store::ProcessAnyThreadImmediateActions()\n");

            std::lock_guard lockGuard(m_anyThreadImmediateActionsMutex);
            // TODO: clean up the long lock on processing anyThreadImmediateActions
            // This could be a pretty long lock
            // I should probably take a snapshot of the exising actions,
            // clear them and then execute the ones in the snapshot
            // The only catch to that method would be that we could get duplicates of
            // no duplicate messages because there could be one in the snapshot
            // and we would still add one to the main vector
            std::for_each(
                m_anyThreadImmediateActions.begin(),
                m_anyThreadImmediateActions.end(),
                // run the action
                [this](Action action) {
                ProcessAnyThreadImmediateAction(action);
            }
            );

            m_anyThreadImmediateActions.clear();
        }

        VoidCoroutine ProcessAnyThreadImmediateAction(Action action) {

            apartment_context ui_thread;

            co_await resume_background();

            TraceLogWrite(
                "Store::ProcessAnyThreadImmediateAction:Begin",
                TraceLoggingString(action.typeName.c_str(), "action.typeName"));

            for (size_t i = 0; i < m_effects.size(); i++) {
                if (std::get<1>(m_effects[i]) == action.actionTypeId) {
                    std::lock_guard<std::mutex> guard(m_effectActionMutex);
                    SetData(std::get<2>(m_effects[i])(m_data, action));
                }
            }

            TraceLogWrite(
                "Store::ProcessAnyThreadImmediateAction:end",
                TraceLoggingString(action.typeName.c_str(), "action.typeName"));

            co_await ui_thread;
        }

        void AddAsyncRequestAction(Action action) {

            auto foundRelatedEffect = std::find_if(
                m_effects.begin(),
                m_effects.end(),
                [action](std::tuple<int, int, std::function<DataType(DataType, Action)>> effectTuple) {
                return std::get<1>(effectTuple) == action.actionTypeId;
            }
            );

            if (foundRelatedEffect != m_effects.end()) {
                { // to scope the lock guard
                    std::lock_guard<std::mutex> guard(m_asyncRequestActionsMutex);

                    if (action.actionRepeatCategory == ActionRepeatCategory::Repeat) {
                        m_asyncRequestActions.push_back(action);
                    }
                    else if (action.actionRepeatCategory == ActionRepeatCategory::OnlyApplyLatest) {

                        auto foundAsyncRequestAction = std::find_if(
                            m_asyncRequestActions.begin(),
                            m_asyncRequestActions.end(),
                            [action](Action testAction) {
                            return action.actionTypeId == testAction.actionTypeId;
                        }
                        );

                        if (foundAsyncRequestAction != m_asyncRequestActions.end()) {
                            *foundAsyncRequestAction = action;
                        }
                        else {
                            m_asyncRequestActions.push_back(action);
                        }
                    }
                    else if (action.actionRepeatCategory == ActionRepeatCategory::DoNotProcessDuplicatesByPredicate) {


                        auto foundAsyncRequestAction = std::find_if(
                            m_asyncRequestActions.begin(),
                            m_asyncRequestActions.end(),
                            [action](Action testAction) {
                            return action.predicate(action, testAction);
                        }
                        );

                        if (foundAsyncRequestAction == m_asyncRequestActions.end()) {
                            {
                                std::lock_guard<std::mutex> asyncRequestActionsInProcessGuard(m_asyncRequestActionsInProcessMutex);

                                auto foundAsyncRequestActionInProcess = std::find_if(
                                    m_asyncRequestActionsInProcess.begin(),
                                    m_asyncRequestActionsInProcess.end(),
                                    [action](Action testAction) {
                                    return action.predicate(action, testAction);
                                }
                                );

                                if (foundAsyncRequestActionInProcess == m_asyncRequestActionsInProcess.end()) {
                                    m_asyncRequestActions.push_back(action);
                                }
                            }
                        }
                    }

                }
            }

        }

        void ProcessAsyncRequestActions() {
            TraceLogWrite("ndtech::Store::ProcessAsyncRequestActions()\n");


            std::lock_guard asyncRequestActionsInProcessLockGuard(m_asyncRequestActionsInProcessMutex);
            std::lock_guard asyncRequestActionsLockGuard(m_asyncRequestActionsMutex);

            std::for_each(
                m_asyncRequestActions.begin(),
                m_asyncRequestActions.end(),
                // run the action
                [this](Action action) {
                ProcessAnyThreadImmediateAction(action);
            }
            );

            m_asyncRequestActionsInProcess.insert(
              m_asyncRequestActionsInProcess.end(), 
              m_asyncRequestActions.begin(), 
              m_asyncRequestActions.end()
            );

            m_asyncRequestActions.clear();

        }

        void AddAsyncCompletionAction(Action action) {

            { // to scope the lock guard
                // TODO: Do we really need to lock here
                std::lock_guard<std::mutex> guard(m_asyncCompletionActionsMutex);

                auto foundRelatedEffect = std::find_if(
                    m_effects.begin(),
                    m_effects.end(),
                    [action](std::tuple<int, int, std::function<DataType(DataType, Action)>> effectTuple) {
                    return std::get<1>(effectTuple) == action.actionTypeId;
                }
                );

                if (foundRelatedEffect != m_effects.end()) {
                    m_asyncCompletionActions.push_back(action);
                }
            }
        }

        void ProcessAsyncCompletionActions() {
            TraceLogWrite("ndtech::Store::ProcessAsyncCompletionActions()\n");


            std::lock_guard asyncCompletionActionsLockGuard(m_asyncCompletionActionsMutex);

            std::for_each(
                m_asyncCompletionActions.begin(),
                m_asyncCompletionActions.end(),
                // run the action
                [this](Action action) {
                ProcessAsyncCompletionAction(action);
            }
            );

            m_asyncCompletionActions.clear();
        }

        void ProcessAsyncCompletionAction(Action action) {
            TraceLogWrite("ndtech::Store::ProcessAsyncCompletionAction");

            std::lock_guard lockGuard(m_asyncRequestActionsInProcessMutex);

            m_asyncRequestActionsInProcess.erase(
                std::remove_if(
                    m_asyncRequestActionsInProcess.begin(),
                    m_asyncRequestActionsInProcess.end(),
                    [action](Action asyncRequestActionInProcess) {
                      return asyncRequestActionInProcess.actionId == action.actionId;
                    }
                ),
                m_asyncRequestActionsInProcess.end()
              );
        }

        void AddSynchronizeOnMainThreadAction(Action action) {

            { // to scope the lock guard
                // TODO: Do we really need to lock here
                std::lock_guard<std::mutex> guard(m_synchronizeOnMainThreadActionsMutex);

                auto foundRelatedEffect = std::find_if(
                    m_effects.begin(),
                    m_effects.end(),
                    [action](std::tuple<int, int, std::function<DataType(DataType, Action)>> effectTuple) {
                    return std::get<1>(effectTuple) == action.actionTypeId;
                }
                );

                if (foundRelatedEffect != m_effects.end()) {
                    m_synchronizeOnMainThreadActions.push_back(action);
                }
            }
        }

        void ProcessSynchronizeOnMainThreadActions() {
            TraceLogWrite("ndtech::Store::ProcessSynchronizeOnMainThreadActions()\n");

            std::lock_guard lockGuard(m_synchronizeOnMainThreadActionsMutex);
            // TODO: clean up the long lock on processing synchronizeOnMainThreadActions
            // This could be a pretty long lock
            // I should probably take a snapshot of the exising actions,
            // clear them and then execute the ones in the snapshot
            // The only catch to that method would be that we could get duplicates of
            // no duplicate messages because there could be one in the snapshot
            // and we would still add one to the main vector
            std::for_each(
                m_synchronizeOnMainThreadActions.begin(),
                m_synchronizeOnMainThreadActions.end(),
                // run the action
                [this](Action action) {
                    ProcessSynchronizeOnMainThreadAction(action);
                }
            );

            m_anyThreadImmediateActions.clear();
        }

        void ProcessSynchronizeOnMainThreadAction(Action action) {

            TraceLogWrite("ndtech::Store::ProcessSynchronizeOnMainThreadAction\n");

            for (size_t i = 0; i < m_effects.size(); i++) {
                if (std::get<1>(m_effects[i]) == action.actionTypeId) {
                    std::lock_guard<std::mutex> guard(m_effectActionMutex);
                    SetData(std::get<2>(m_effects[i])(m_data, action));
                }
            }

        }

        void AddAwaitAnyThreadImmediateAction(size_t dispatchId, Action action) {

            { // to scope the lock guard
              // TODO: Do we really need to lock here
                std::lock_guard<std::mutex> guard(m_awaitAnyThreadImmediateActionsMutex);

                auto foundRelatedEffect = std::find_if(
                    m_effects.begin(),
                    m_effects.end(),
                    [action](std::tuple<int, int, std::function<DataType(DataType, Action)>> effectTuple) {
                    return std::get<1>(effectTuple) == action.actionTypeId;
                }
                );

                if (foundRelatedEffect != m_effects.end()) {
                    m_awaitAnyThreadImmediateActions.push_back(std::pair{ dispatchId, action });
                }
            }
        }

        void ProcessAwaitAnyThreadImmediateActions() {
            TraceLogWrite("ndtech::Store::ProcessAwaitAnyThreadImmediateActions()\n");

            std::lock_guard lockGuard(m_awaitAnyThreadImmediateActionsMutex);
            // TODO: clean up the long lock on processing anyThreadImmediateActions
            // This could be a pretty long lock
            // I should probably take a snapshot of the exising actions,
            // clear them and then execute the ones in the snapshot
            // The only catch to that method would be that we could get duplicates of
            // no duplicate messages because there could be one in the snapshot
            // and we would still add one to the main vector
            std::for_each(
                m_awaitAnyThreadImmediateActions.begin(),
                m_awaitAnyThreadImmediateActions.end(),
                // run the action
                [this](std::pair<size_t, Action> actionPair) {
                    ProcessAwaitAnyThreadImmediateAction(actionPair.first, actionPair.second);

                    m_dispatchCompleteCallback(actionPair.first, actionPair.second);
                }
            );

            m_awaitAnyThreadImmediateActions.clear();
        }

        VoidCoroutine ProcessAwaitAnyThreadImmediateAction(size_t dispatchId, Action action) {

            apartment_context ui_thread;

            co_await resume_background();

            TraceLogWrite(
                "Store::ProcessAnyThreadImmediateAction:Begin",
                TraceLoggingString(action.typeName.c_str(), "action.typeName"));

            for (size_t i = 0; i < m_effects.size(); i++) {
                if (std::get<1>(m_effects[i]) == action.actionTypeId) {
                    std::lock_guard<std::mutex> guard(m_effectActionMutex);
                    SetData(std::get<2>(m_effects[i])(m_data, action));
                }
            }

            TraceLogWrite(
                "Store::ProcessAnyThreadImmediateAction:end",
                TraceLoggingString(action.typeName.c_str(), "action.typeName"));

            co_await ui_thread;
        }

        void AddAwaitAsyncRequestAction(size_t dispatchId, Action action) {

            auto foundRelatedEffect = std::find_if(
                m_effects.begin(),
                m_effects.end(),
                [action](std::tuple<int, int, std::function<DataType(DataType, Action)>> effectTuple) {
                return std::get<1>(effectTuple) == action.actionTypeId;
            }
            );

            if (foundRelatedEffect != m_effects.end()) {
                { // to scope the lock guard
                    std::lock_guard<std::mutex> guard(m_awaitAsyncRequestActionsMutex);

                    if (action.actionRepeatCategory == ActionRepeatCategory::Repeat) {
                        m_awaitAsyncRequestActions.push_back(std::pair<size_t, Action>{dispatchId, action});
                    }
                    else if (action.actionRepeatCategory == ActionRepeatCategory::OnlyApplyLatest) {

                        auto foundAsyncRequestActionPair = std::find_if(
                            m_awaitAsyncRequestActions.begin(),
                            m_awaitAsyncRequestActions.end(),
                            [action](std::pair<size_t, Action> testActionPair) {
                                return action.actionTypeId == testActionPair.second.actionTypeId;
                            }
                        );

                        if (foundAsyncRequestActionPair != m_awaitAsyncRequestActions.end()) {
                            (*foundAsyncRequestActionPair).second = action;
                        }
                        else {
                            m_awaitAsyncRequestActions.push_back(std::pair<size_t, Action>{dispatchId, action});
                        }
                    }
                    else if (action.actionRepeatCategory == ActionRepeatCategory::DoNotProcessDuplicatesByPredicate) {


                        auto foundAsyncRequestActionPair = std::find_if(
                            m_awaitAsyncRequestActions.begin(),
                            m_awaitAsyncRequestActions.end(),
                            [action](std::pair<size_t, Action> testActionPair) {
                            return action.predicate(action, testActionPair.second);
                        }
                        );

                        if (foundAsyncRequestActionPair == m_awaitAsyncRequestActions.end()) {
                            {
                                std::lock_guard<std::mutex> awaitAsyncRequestActionsInProcessGuard(m_awaitAsyncRequestActionsInProcessMutex);

                                auto foundAsyncRequestActionInProcessPair = std::find_if(
                                    m_awaitAsyncRequestActionsInProcess.begin(),
                                    m_awaitAsyncRequestActionsInProcess.end(),
                                    [action](std::pair<size_t, Action> testActionPair) {
                                    return action.predicate(action, testActionPair.second);
                                }
                                );

                                if (foundAsyncRequestActionInProcessPair == m_awaitAsyncRequestActionsInProcess.end()) {
                                    m_awaitAsyncRequestActions.push_back(std::pair<size_t, Action>{dispatchId, action});
                                }
                            }
                        }
                    }

                }
            }

        }

        void ProcessAwaitAsyncRequestActions() {
            TraceLogWrite("ndtech::Store::ProcessAwaitAsyncRequestActions()\n");


            std::lock_guard awaitAsyncRequestActionsInProcessLockGuard(m_awaitAsyncRequestActionsInProcessMutex);
            std::lock_guard awaitAsyncRequestActionsLockGuard(m_awaitAsyncRequestActionsMutex);

            std::for_each(
                m_awaitAsyncRequestActions.begin(),
                m_awaitAsyncRequestActions.end(),
                // run the action
                [this](std::pair<size_t, Action> testActionPair) {
                    ProcessAwaitAnyThreadImmediateAction(testActionPair.first, testActionPair.second);
                }
            );

            m_awaitAsyncRequestActionsInProcess.insert(m_awaitAsyncRequestActionsInProcess.end(), m_awaitAsyncRequestActions.begin(), m_awaitAsyncRequestActions.end());
            m_awaitAsyncRequestActions.clear();

        }

        void AddAwaitAsyncCompletionAction(size_t dispatchId, Action action) {

            { // to scope the lock guard
              // TODO: Do we really need to lock here
                std::lock_guard<std::mutex> guard(m_awaitAsyncCompletionActionsMutex);

                auto foundRelatedEffect = std::find_if(
                    m_effects.begin(),
                    m_effects.end(),
                    [action](std::tuple<int, int, std::function<DataType(DataType, Action)>> effectTuple) {
                    return std::get<1>(effectTuple) == action.actionTypeId;
                }
                );

                if (foundRelatedEffect != m_effects.end()) {
                    m_awaitAsyncCompletionActions.push_back(std::pair<size_t, Action>{dispatchId, action});
                }
            }
        }

        void ProcessAwaitAsyncCompletionActions() {
            TraceLogWrite("ndtech::Store::ProcessAwaitAsyncCompletionActions()\n");


            std::lock_guard awaitAsyncCompletionActionsLockGuard(m_awaitAsyncCompletionActionsMutex);

            std::for_each(
                m_awaitAsyncCompletionActions.begin(),
                m_awaitAsyncCompletionActions.end(),
                // run the action
                [this](std::pair<size_t, Action> actionPair) {
                ProcessAwaitAsyncCompletionAction(actionPair.first, actionPair.second);
            }
            );

            m_awaitAsyncCompletionActions.clear();
        }

        void ProcessAwaitAsyncCompletionAction(size_t dispatchId, Action action) {
            TraceLogWrite("ndtech::Store::ProcessAwaitAsyncCompletionAction");

            std::lock_guard lockGuard(m_awaitAsyncRequestActionsInProcessMutex);

            m_awaitAsyncRequestActionsInProcess.erase(
                std::remove_if(
                    m_awaitAsyncRequestActionsInProcess.begin(),
                    m_awaitAsyncRequestActionsInProcess.end(),
                    [action](std::pair<size_t, Action> awaitAsyncRequestActionInProcessPair) {
                        return awaitAsyncRequestActionInProcessPair.second.actionId == action.actionId;
                    }
                ),
                m_awaitAsyncRequestActionsInProcess.end()
            );

            m_dispatchCompleteCallback(dispatchId, action);
        }

        void AddAwaitSynchronizeOnMainThreadAction(size_t dispatchId, Action action) {

            { // to scope the lock guard
              // TODO: Do we really need to lock here
                std::lock_guard<std::mutex> guard(m_awaitSynchronizeOnMainThreadActionsMutex);

                auto foundRelatedEffect = std::find_if(
                    m_effects.begin(),
                    m_effects.end(),
                    [action](std::tuple<int, int, std::function<DataType(DataType, Action)>> effectTuple) {
                        return std::get<1>(effectTuple) == action.actionTypeId;
                    }
                );

                if (foundRelatedEffect != m_effects.end()) {
                    m_awaitSynchronizeOnMainThreadActions.push_back(std::pair<size_t, Action>{dispatchId, action});
                }
            }
        }

        void ProcessAwaitSynchronizeOnMainThreadActions() {
            TraceLogWrite("ndtech::Store::ProcessAwaitSynchronizeOnMainThreadActions()\n");

            std::lock_guard lockGuard(m_awaitSynchronizeOnMainThreadActionsMutex);
            // TODO: clean up the long lock on processing awaitSynchronizeOnMainThreadActions
            // This could be a pretty long lock
            // I should probably take a snapshot of the exising actions,
            // clear them and then execute the ones in the snapshot
            // The only catch to that method would be that we could get duplicates of
            // no duplicate messages because there could be one in the snapshot
            // and we would still add one to the main vector
            std::for_each(
                m_awaitSynchronizeOnMainThreadActions.begin(),
                m_awaitSynchronizeOnMainThreadActions.end(),
                // run the action
                [this](std::pair<size_t, Action> actionPair) {
                    ProcessAwaitSynchronizeOnMainThreadAction(actionPair.first, actionPair.second);
                    m_dispatchCompleteCallback(actionPair.first, actionPair.second);
                }
            );

            m_anyThreadImmediateActions.clear();
        }

        void ProcessAwaitSynchronizeOnMainThreadAction(size_t dispatchId, Action action) {

            TraceLogWrite("ndtech::Store::ProcessAwaitSynchronizeOnMainThreadAction\n");

            for (size_t i = 0; i < m_effects.size(); i++) {
                if (std::get<1>(m_effects[i]) == action.actionTypeId) {
                    std::lock_guard<std::mutex> guard(m_effectActionMutex);
                    SetData(std::get<2>(m_effects[i])(m_data, action));
                }
            }

        }

        void ProcessActions() override {
            ProcessAnyThreadImmediateActions();
            ProcessAwaitAnyThreadImmediateActions();
            ProcessAsyncRequestActions();
            ProcessAwaitAsyncRequestActions();
            ProcessAsyncCompletionActions();
            ProcessAwaitAsyncCompletionActions();
            ProcessSynchronizeOnMainThreadActions();
            ProcessAwaitSynchronizeOnMainThreadActions();
        }

        void Dispatch(Action action) override {

            if (action.actionCategory == ActionCategory::AnyThreadImmediate) {
                m_lastAnyThreadActionId.store(action.actionId);
                //ProcessAnyThreadImmediateAction(action);
                AddAnyThreadImmediateAction(action);
            }
            else if (action.actionCategory == ActionCategory::SynchronizeOnMainThread) {
                //ProcessSynchronizedOnMainThread(action, m_lastAnyThreadActionId);
                AddSynchronizeOnMainThreadAction(action);
            }
            else if (action.actionCategory == ActionCategory::Synchronous) {
                std::lock_guard<std::mutex> guard(m_effectActionMutex);
                TraceLogWrite(
                    "Store::EffectAction:Inside Mutex",
                    TraceLoggingString(action.typeName.c_str(), "action.typeName"));

                for (std::tuple<int, int, std::function<DataType(DataType, Action)>> effectTuple : m_effects) {
                    if (std::get<1>(effectTuple) == action.actionTypeId) {
                        SetData(std::get<2>(effectTuple)(m_data, action));
                    }
                }

                TraceLogWrite(
                    "Store::EffectAction:End",
                    TraceLoggingString(action.typeName.c_str(), "action.typeName"));
            }
            else if (action.actionCategory == ActionCategory::AsyncRequest) {
                AddAsyncRequestAction(action);
            }
            else if (action.actionCategory == ActionCategory::AsyncCompletion) {
                AddAsyncCompletionAction(action);
            }
        }

        void AwaitDispatch(size_t dispatchId, Action action) override {

            if (action.actionCategory == ActionCategory::AnyThreadImmediate) {
                AddAwaitAnyThreadImmediateAction(dispatchId, action);
            }
            else if (action.actionCategory == ActionCategory::SynchronizeOnMainThread) {
                //ProcessSynchronizedOnMainThread(action, m_lastAnyThreadActionId);
                AddAwaitSynchronizeOnMainThreadAction(dispatchId, action);
            }
            else if (action.actionCategory == ActionCategory::Synchronous) {
                std::lock_guard<std::mutex> guard(m_effectActionMutex);
                TraceLogWrite(
                    "Store::EffectAction:Inside Mutex",
                    TraceLoggingString(action.typeName.c_str(), "action.typeName"));

                for (std::tuple<int, int, std::function<DataType(DataType, Action)>> effectTuple : m_effects) {
                    if (std::get<1>(effectTuple) == action.actionTypeId) {
                        SetData(std::get<2>(effectTuple)(m_data, action));
                    }
                }

                TraceLogWrite(
                    "Store::EffectAction:End",
                    TraceLoggingString(action.typeName.c_str(), "action.typeName"));
            }
            else if (action.actionCategory == ActionCategory::AsyncRequest) {
                AddAwaitAsyncRequestAction(dispatchId, action);
            }
            else if (action.actionCategory == ActionCategory::AsyncCompletion) {
                AddAwaitAsyncCompletionAction(dispatchId, action);
            }
        }

        int RegisterEffect(int actionType, std::function<DataType(DataType, Action)> effect) {
            std::lock_guard<std::mutex> guard(m_effectActionMutex);
            int effectId = m_nextEffectId++;
            m_effects.push_back(std::tuple<int, int, std::function<DataType(DataType, Action)>>{effectId, actionType, effect});
            return effectId;
        }

        bool UnregisterEffect(int effectId) {
            std::lock_guard<std::mutex> guard(m_effectActionMutex);

            auto itToEffectToUnregister = std::find_if(
                m_effects.begin(),
                m_effects.end(),
                [effectId](std::tuple<int, int, std::function<DataType(DataType, Action)>> effect) {
                return std::get<0>(effect) == effectId;
            });

            if (itToEffectToUnregister) {
                m_effects.erase(itToEffectToUnregister);
                return true;
            }

            return false;
        }

        const DataType GetData() {
            return m_data;
        }

        void SetDispatchCompleteCallback(std::function<void(size_t, Action)> dispatchCompleteCallback) {
            m_dispatchCompleteCallback = dispatchCompleteCallback;
        }

    private:
        DataType m_data;
        using EffectType = std::tuple<int, int, std::function<DataType(DataType, Action)>>;
        using EffectsType = std::vector<EffectType>;

        std::mutex m_dataMutex;
        std::mutex m_effectActionMutex;
        std::vector<std::tuple<int, int, std::function<DataType(DataType, Action)>>>                            m_effects;
        int                                                                                                     m_nextEffectId = 0;
        std::atomic<size_t>                                                                                     m_lastAnyThreadActionId = 0;

        std::vector<Action>                                                                                     m_anyThreadImmediateActions;
        std::mutex                                                                                              m_anyThreadImmediateActionsMutex;

        std::vector<std::pair<size_t, Action>>                                                                  m_awaitAnyThreadImmediateActions;
        std::mutex                                                                                              m_awaitAnyThreadImmediateActionsMutex;


        std::vector<Action>                                                                                     m_asyncRequestActions;
        std::mutex                                                                                              m_asyncRequestActionsMutex;

        std::vector<Action>                                                                                     m_asyncRequestActionsInProcess;
        std::mutex                                                                                              m_asyncRequestActionsInProcessMutex;

        std::vector<Action>                                                                                     m_asyncCompletionActions;
        std::mutex                                                                                              m_asyncCompletionActionsMutex;


        std::vector<std::pair<size_t, Action>>                                                                  m_awaitAsyncRequestActions;
        std::mutex                                                                                              m_awaitAsyncRequestActionsMutex;

        std::vector<std::pair<size_t, Action>>                                                                  m_awaitAsyncRequestActionsInProcess;
        std::mutex                                                                                              m_awaitAsyncRequestActionsInProcessMutex;

        std::vector<std::pair<size_t, Action>>                                                                  m_awaitAsyncCompletionActions;
        std::mutex                                                                                              m_awaitAsyncCompletionActionsMutex;


        std::vector<Action>                                                                                     m_synchronizeOnMainThreadActions;
        std::mutex                                                                                              m_synchronizeOnMainThreadActionsMutex;


        std::vector<std::pair<size_t, Action>>                                                                  m_awaitSynchronizeOnMainThreadActions;
        std::mutex                                                                                              m_awaitSynchronizeOnMainThreadActionsMutex;

        size_t                                                                                                  m_cache_line_size;
        std::function<void(size_t, Action)>                                                                     m_dispatchCompleteCallback;



        void SetData(DataType data) {
            std::lock_guard<std::mutex> guard(m_dataMutex);
            m_data = data;
        }
    };



    template<typename EntityType>
    class EntityStore : public EntityStoreBase
    {
    public:
        using DataType = std::vector<std::pair<size_t, EntityType>>;
        using StoreDataType = DataType;

        EntityStore<EntityType>() {
            m_cache_line_size = Utilities::cache_line_size();
        }

        EntityStore<EntityType>(const EntityStore<EntityType>& other) :
            m_data(other.m_data),
            m_effects(other.m_effects),
            m_anyThreadImmediateActions(other.m_anyThreadImmediateActions),
            m_asyncRequestActions(other.m_asyncRequestActions),
            m_asyncRequestActionsInProcess(other.m_asyncRequestActionsInProcess),
            m_asyncCompletionActions(other.m_asyncCompletionActions),
            m_synchronizeOnMainThreadActions(other.m_synchronizeOnMainThreadActions),
            m_cache_line_size(other.m_cache_line_size)
        {
            m_lastAnyThreadActionId.store(other.m_lastAnyThreadActionId);
        }

        void ForEntityPairs(std::function<void(DataType)> func) {
            std::lock_guard<std::mutex> guard(m_dataMutex);
            func(m_data);
        }

        template <typename EntityType>
        void Synch(EntityStore<EntityType>* entityStore) {
            
        }

        void AddAnyThreadImmediateAction(Action action) {

            { // to scope the lock guard
              // TODO: Do we really need to lock here
                std::lock_guard<std::mutex> guard(m_anyThreadImmediateActionsMutex);

                auto foundRelatedEffect = std::find_if(
                    m_effects.begin(),
                    m_effects.end(),
                    [action](std::tuple<int, int, std::function<DataType(DataType, Action)>> effectTuple) {
                    return std::get<1>(effectTuple) == action.actionTypeId;
                }
                );

                if (foundRelatedEffect != m_effects.end()) {
                    m_anyThreadImmediateActions.push_back(action);
                }
            }
        }

        void ProcessAnyThreadImmediateActions() {
            TraceLogWrite("ndtech::Store::ProcessAnyThreadImmediateActions()\n");

            std::lock_guard lockGuard(m_anyThreadImmediateActionsMutex);
            // TODO: clean up the long lock on processing anyThreadImmediateActions
            // This could be a pretty long lock
            // I should probably take a snapshot of the exising actions,
            // clear them and then execute the ones in the snapshot
            // The only catch to that method would be that we could get duplicates of
            // no duplicate messages because there could be one in the snapshot
            // and we would still add one to the main vector
            std::for_each(
                m_anyThreadImmediateActions.begin(),
                m_anyThreadImmediateActions.end(),
                // run the action
                [this](Action action) {
                ProcessAnyThreadImmediateAction(action);
            }
            );

            m_anyThreadImmediateActions.clear();
        }

        VoidCoroutine ProcessAnyThreadImmediateAction(Action action) {

            apartment_context ui_thread;

            co_await resume_background();

            TraceLogWrite(
                "Store::ProcessAnyThreadImmediateAction:Begin",
                TraceLoggingString(action.typeName.c_str(), "action.typeName"));

            for (size_t i = 0; i < m_effects.size(); i++) {
                if (std::get<1>(m_effects[i]) == action.actionTypeId) {
                    std::lock_guard<std::mutex> guard(m_effectActionMutex);
                    SetData(std::get<2>(m_effects[i])(m_data, action));
                }
            }

            TraceLogWrite(
                "Store::ProcessAnyThreadImmediateAction:end",
                TraceLoggingString(action.typeName.c_str(), "action.typeName"));

            co_await ui_thread;
        }

        void AddAsyncRequestAction(Action action) {

            auto foundRelatedEffect = std::find_if(
                m_effects.begin(),
                m_effects.end(),
                [action](std::tuple<int, int, std::function<DataType(DataType, Action)>> effectTuple) {
                return std::get<1>(effectTuple) == action.actionTypeId;
            }
            );

            if (foundRelatedEffect != m_effects.end()) {
                { // to scope the lock guard
                    std::lock_guard<std::mutex> guard(m_asyncRequestActionsMutex);

                    if (action.actionRepeatCategory == ActionRepeatCategory::Repeat) {
                        m_asyncRequestActions.push_back(action);
                    }
                    else if (action.actionRepeatCategory == ActionRepeatCategory::OnlyApplyLatest) {

                        auto foundAsyncRequestAction = std::find_if(
                            m_asyncRequestActions.begin(),
                            m_asyncRequestActions.end(),
                            [action](Action testAction) {
                            return action.actionTypeId == testAction.actionTypeId;
                        }
                        );

                        if (foundAsyncRequestAction != m_asyncRequestActions.end()) {
                            *foundAsyncRequestAction = action;
                        }
                        else {
                            m_asyncRequestActions.push_back(action);
                        }
                    }
                    else if (action.actionRepeatCategory == ActionRepeatCategory::DoNotProcessDuplicatesByPredicate) {


                        auto foundAsyncRequestAction = std::find_if(
                            m_asyncRequestActions.begin(),
                            m_asyncRequestActions.end(),
                            [action](Action testAction) {
                            return action.predicate(action, testAction);
                        }
                        );

                        if (foundAsyncRequestAction == m_asyncRequestActions.end()) {
                            {
                                std::lock_guard<std::mutex> asyncRequestActionsInProcessGuard(m_asyncRequestActionsInProcessMutex);

                                auto foundAsyncRequestActionInProcess = std::find_if(
                                    m_asyncRequestActionsInProcess.begin(),
                                    m_asyncRequestActionsInProcess.end(),
                                    [action](Action testAction) {
                                    return action.predicate(action, testAction);
                                }
                                );

                                if (foundAsyncRequestActionInProcess == m_asyncRequestActionsInProcess.end()) {
                                    m_asyncRequestActions.push_back(action);
                                }
                            }
                        }
                    }

                }
            }

        }

        void ProcessAsyncRequestActions() {
            TraceLogWrite("ndtech::Store::ProcessAsyncRequestActions()\n");


            std::lock_guard asyncRequestActionsInProcessLockGuard(m_asyncRequestActionsInProcessMutex);
            std::lock_guard asyncRequestActionsLockGuard(m_asyncRequestActionsMutex);

            std::for_each(
                m_asyncRequestActions.begin(),
                m_asyncRequestActions.end(),
                // run the action
                [this](Action action) {
                ProcessAnyThreadImmediateAction(action);
            }
            );

            m_asyncRequestActionsInProcess.insert(m_asyncRequestActionsInProcess.end(), m_asyncRequestActions.begin(), m_asyncRequestActions.end());
            m_asyncRequestActions.clear();

        }

        void AddAsyncCompletionAction(Action action) {

            { // to scope the lock guard
              // TODO: Do we really need to lock here
                std::lock_guard<std::mutex> guard(m_asyncCompletionActionsMutex);

                auto foundRelatedEffect = std::find_if(
                    m_effects.begin(),
                    m_effects.end(),
                    [action](std::tuple<int, int, std::function<DataType(DataType, Action)>> effectTuple) {
                    return std::get<1>(effectTuple) == action.actionTypeId;
                }
                );

                if (foundRelatedEffect != m_effects.end()) {
                    m_asyncCompletionActions.push_back(action);
                }
            }
        }

        void ProcessAsyncCompletionActions() {
            TraceLogWrite("ndtech::Store::ProcessAsyncCompletionActions()\n");


            std::lock_guard asyncCompletionActionsLockGuard(m_asyncCompletionActionsMutex);

            std::for_each(
                m_asyncCompletionActions.begin(),
                m_asyncCompletionActions.end(),
                // run the action
                [this](Action action) {
                ProcessAsyncCompletionAction(action);
            }
            );

            m_asyncCompletionActions.clear();
        }

        void ProcessAsyncCompletionAction(Action action) {
            TraceLogWrite("ndtech::Store::ProcessAsyncCompletionAction");

            std::lock_guard lockGuard(m_asyncRequestActionsInProcessMutex);

            m_asyncRequestActionsInProcess.erase(
                std::remove_if(
                    m_asyncRequestActionsInProcess.begin(),
                    m_asyncRequestActionsInProcess.end(),
                    [action](Action asyncRequestActionInProcess) {
                return asyncRequestActionInProcess.actionId == action.actionId;
            }
                ),
                m_asyncRequestActionsInProcess.end()
                );
        }

        void AddSynchronizeOnMainThreadAction(Action action) {

            { // to scope the lock guard
              // TODO: Do we really need to lock here
                std::lock_guard<std::mutex> guard(m_synchronizeOnMainThreadActionsMutex);

                auto foundRelatedEffect = std::find_if(
                    m_effects.begin(),
                    m_effects.end(),
                    [action](std::tuple<int, int, std::function<DataType(DataType, Action)>> effectTuple) {
                    return std::get<1>(effectTuple) == action.actionTypeId;
                }
                );

                if (foundRelatedEffect != m_effects.end()) {
                    m_synchronizeOnMainThreadActions.push_back(action);
                }
            }
        }

        void ProcessSynchronizeOnMainThreadActions() {
            TraceLogWrite("ndtech::Store::ProcessSynchronizeOnMainThreadActions()\n");

            std::lock_guard lockGuard(m_synchronizeOnMainThreadActionsMutex);
            // TODO: clean up the long lock on processing synchronizeOnMainThreadActions
            // This could be a pretty long lock
            // I should probably take a snapshot of the exising actions,
            // clear them and then execute the ones in the snapshot
            // The only catch to that method would be that we could get duplicates of
            // no duplicate messages because there could be one in the snapshot
            // and we would still add one to the main vector
            std::for_each(
                m_synchronizeOnMainThreadActions.begin(),
                m_synchronizeOnMainThreadActions.end(),
                // run the action
                [this](Action action) {
                ProcessAnyThreadImmediateAction(action);
            }
            );

            m_anyThreadImmediateActions.clear();
        }

        void ProcessSynchronizeOnMainThreadAction(Action action) {

            TraceLogWrite("ndtech::Store::ProcessSynchronizeOnMainThreadAction\n");

            for (size_t i = 0; i < m_effects.size(); i++) {
                if (std::get<1>(m_effects[i]) == action.actionTypeId) {
                    std::lock_guard<std::mutex> guard(m_effectActionMutex);
                    SetData(std::get<2>(m_effects[i])(m_data, action));
                }
            }

        }

        void ProcessSynchronizedOnMainThread(Action action, int synchronizeAfterActionId) {

            m_lastProcessedActionId
                .get_observable()
                .filter([synchronizeAfterActionId](int actionId) {
                return actionId >= synchronizeAfterActionId;
            })
                .observe_on(m_mainThreadWorker)
                .take(1)
                .subscribe([this, action](int synchronizeAfterActionId) {
                TraceLogWrite(
                    "Store::ProcessAnyThreadSynchronizedAction:Begin",
                    TraceLoggingString(action.typeName.c_str(), "action.typeName"));

                for (size_t i = 0; i < m_effects.size(); i++) {
                    if (std::get<1>(m_effects[i]) == action.actionTypeId) {
                        std::lock_guard<std::mutex> guard(m_effectActionMutex);
                        SetData(std::get<2>(m_effects[i])(m_data, action));
                    }
                }

                TraceLogWrite(
                    "Store::ProcessAnyThreadSynchronizedAction:End",
                    TraceLoggingString(action.typeName.c_str(), "action.typeName"));

                UNREFERENCED_PARAMETER(synchronizeAfterActionId);
            });
        }

        void ProcessActions() override {
            ProcessAnyThreadImmediateActions();
            ProcessAsyncRequestActions();
            ProcessAsyncCompletionActions();
            ProcessSynchronizeOnMainThreadActions();
        }

        void Dispatch(Action action) override {

            if (action.actionCategory == ActionCategory::AnyThreadImmediate) {
                m_lastAnyThreadActionId.store(action.actionId);
                //ProcessAnyThreadImmediateAction(action);
                AddAnyThreadImmediateAction(action);
            }
            else if (action.actionCategory == ActionCategory::SynchronizeOnMainThread) {
                //ProcessSynchronizedOnMainThread(action, m_lastAnyThreadActionId);
                AddSynchronizeOnMainThreadAction(action);
            }
            else if (action.actionCategory == ActionCategory::Synchronous) {
                std::lock_guard<std::mutex> guard(m_effectActionMutex);
                TraceLogWrite(
                    "Store::EffectAction:Inside Mutex",
                    TraceLoggingString(action.typeName.c_str(), "action.typeName"));

                for (std::tuple<int, int, std::function<DataType(DataType, Action)>> effectTuple : m_effects) {
                    if (std::get<1>(effectTuple) == action.actionTypeId) {
                        SetData(std::get<2>(effectTuple)(m_data, action));
                    }
                }

                TraceLogWrite(
                    "Store::EffectAction:End",
                    TraceLoggingString(action.typeName.c_str(), "action.typeName"));
            }
            else if (action.actionCategory == ActionCategory::AsyncRequest) {
                AddAsyncRequestAction(action);
            }
            else if (action.actionCategory == ActionCategory::AsyncCompletion) {
                AddAsyncCompletionAction(action);
            }
        }

        int RegisterEffect(int actionType, std::function<DataType(DataType, Action)> effect) {
            std::lock_guard<std::mutex> guard(m_effectActionMutex);
            int effectId = m_nextEffectId++;
            m_effects.push_back(std::tuple<int, int, std::function<DataType(DataType, Action)>>{effectId, actionType, effect});
            return effectId;
        }

        bool UnregisterEffect(int effectId) {
            std::lock_guard<std::mutex> guard(m_effectActionMutex);

            auto itToEffectToUnregister = std::find_if(
                m_effects.begin(),
                m_effects.end(),
                [effectId](std::tuple<int, int, std::function<DataType(DataType, Action)>> effect) {
                return std::get<0>(effect) == effectId;
            });

            if (itToEffectToUnregister) {
                m_effects.erase(itToEffectToUnregister);
                return true;
            }

            return false;
        }

        const DataType GetData() {
            return m_data;
        }

    private:
        DataType m_data;
        using EffectType = std::tuple<int, int, std::function<DataType(DataType, Action)>>;
        using EffectsType = std::vector<EffectType>;

        std::mutex                                                                                              m_dataMutex;
        std::mutex                                                                                              m_effectActionMutex;
        std::vector<std::tuple<int, int, std::function<DataType(DataType, Action)>>>                            m_effects;
        int                                                                                                     m_nextEffectId = 0;
        std::atomic<int>                                                                                        m_lastAnyThreadActionId = 0;

        std::vector<Action>                                                                                     m_anyThreadImmediateActions;
        std::mutex                                                                                              m_anyThreadImmediateActionsMutex;

        std::vector<Action>                                                                                     m_asyncRequestActions;
        std::mutex                                                                                              m_asyncRequestActionsMutex;

        std::vector<Action>                                                                                     m_asyncRequestActionsInProcess;
        std::mutex                                                                                              m_asyncRequestActionsInProcessMutex;

        std::vector<Action>                                                                                     m_asyncCompletionActions;
        std::mutex                                                                                              m_asyncCompletionActionsMutex;

        std::vector<Action>                                                                                     m_synchronizeOnMainThreadActions;
        std::mutex                                                                                              m_synchronizeOnMainThreadActionsMutex;

        size_t                                                                                                  m_cache_line_size;



        void SetData(DataType data) {
            std::lock_guard<std::mutex> guard(m_dataMutex);
            m_data = data;
        }
    };

}

