#pragma once

#include "pch.h"

#include "Action.h"
#include "VoidCoroutine.h"
#include "Utilities.h"

namespace ndtech {
    
    template<typename DataType>
    struct ActionEffectScheduler {

        ActionEffectScheduler() {
            TraceLogWrite("ndtech::ActionEffectScheduler::ActionEffectScheduler()\n");
        }

        void Run() {
            while (true) {

                std::unique_lock<std::mutex> lockGuard(m_waitMutex);

                while (m_effects.size() == 0) {
                    m_conditionVariable.wait(lockGuard);
                }

                ProcessActions();

            }

        }

        void ProcessActions() {
            TraceLogWrite("ndtech::ActionEffectScheduler::ProcessActions()\n");

            //std::lock_guard lockGuard(m_tasksMutex);
            //std::for_each(
            //    m_effects.begin(),
            //    m_effects.end(),
            //    // run the task
            //    [](std::tuple<int, int, std::function<DataType(DataType, Action)>> effect) {
            //        task.first();
            //    }
            //);

            //m_tasks.erase(
            //    m_tasks.begin(),
            //    std::find_if(
            //        m_tasks.begin(),
            //        m_tasks.end(),
            //        // check if the scheduled time for the task is now or past
            //        [this, beginProcessingTime](std::pair<std::function<void(void)>, time_point<system_clock>> task) {
            //    return task.second > beginProcessingTime;
            //})
            //);

            //if (m_tasks.size() > 0) {
            //    m_wakeTime = m_tasks[0].second;
            //}
            //else {
            //    m_wakeTime = system_clock::now() + 100ms;
            //}
        }

        void ProcessAction(Action action) {

            if (action.actionRepeatCategory == ActionRepeatCategory::OnlyApplyLatest) {

            }
            else if (action.actionRepeatCategory == ActionRepeatCategory::DoNotProcessDuplicatesByPredicate) {

            }
            else if (action.actionRepeatCategory == ActionRepeatCategory::Repeat) {

            }

        }

        using EffectType = std::tuple<int, int, std::function<DataType(DataType, Action)>>;
        using EffectsType = std::vector<EffectType>;

        std::mutex m_dataMutex;
        std::mutex m_effectActionMutex;
        std::vector<std::tuple<int, int, std::function<DataType(DataType, Action)>>>                            m_effects;
        int                                                                                                     m_nextEffectId = 0;
        std::atomic<int>                                                                                        m_lastAnyThreadActionId = 0;

        std::mutex                                                                                              m_waitMutex;
        std::condition_variable                                                                                 m_conditionVariable;

        size_t                                                                                                  m_cache_line_size;

    };

}