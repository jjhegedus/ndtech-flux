#include "pch.h"
#include "Dispatcher.h"
#include "Entity.h"

namespace ndtech {

    std::vector<StoreBase*>                                                         Dispatcher::m_stores;
    EntityStoreBase*                                                                Dispatcher::m_mainEntityStore = nullptr;
    std::vector<EntityStoreBase*>                                                   Dispatcher::m_entityStores;
    std::atomic<size_t>                                                             Dispatcher::m_nextActionTypeId = 0;
    EntityManagerBase*                                                              Dispatcher::m_entityManager = nullptr;
    std::atomic<size_t>                                                             Dispatcher::m_dispatchId = 0;
    std::vector<std::tuple<size_t, Action, std::function<void(size_t, Action)>>>    Dispatcher::m_dispatchesInProcess;
    std::mutex                                                                      Dispatcher::m_dispatchesInProcessMutex;

    void Dispatcher::RegisterStore(StoreBase *store) {
        m_stores.push_back(store);
    }

    void Dispatcher::Dispatch(Action action) {

        for (StoreBase *store : m_stores) {
            store->Dispatch(action);
        }
    }

    void Dispatcher::AwaitDispatch(Action action, std::function<void(size_t, Action)> callback) {
        for (StoreBase *store : m_stores) {
            std::lock_guard lockGuard(m_dispatchesInProcessMutex);
            size_t dispatchId = m_dispatchId.fetch_add(1);
            m_dispatchesInProcess.emplace_back(std::tuple<size_t, Action, std::function<void(size_t, Action)>>{dispatchId, action, callback});
            store->Dispatch(action);
        }
    }

    void Dispatcher::DispatchComplete(size_t dispatchId, Action action) {
        std::lock_guard lockGuard(m_dispatchesInProcessMutex);
        std::function<void(size_t, Action)> callback;

        m_dispatchesInProcess.erase(
            std::remove_if(
                m_dispatchesInProcess.begin(),
                m_dispatchesInProcess.end(),
                [dispatchId, callback](std::tuple<size_t, Action, std::function<void(size_t, Action)>> dispatchTuple) mutable{
                    callback = std::get<2>(dispatchTuple);
                    return std::get<0>(dispatchTuple) == dispatchId;
                }
            ),
            m_dispatchesInProcess.end()
        );


        auto foundDispatchInProcess = std::find_if(
            m_dispatchesInProcess.begin(),
            m_dispatchesInProcess.end(),
            [action](std::tuple<size_t, Action, std::function<void(size_t, Action)>> actionTuple) {
                   return std::get<1>(actionTuple).actionId == action.actionId;
            }
        );
        
        if (foundDispatchInProcess == m_dispatchesInProcess.end()) {
            callback(dispatchId, action);
        }

        
    }

    size_t Dispatcher::GetNextActionTypeId() {
        return m_nextActionTypeId.fetch_add(1);
    }

    void Dispatcher::ProcessActions() {
        for (StoreBase *store : m_stores) {
            store->ProcessActions();
        }
    }

    void Dispatcher::RegisterEntityStore(EntityStoreBase *store) {
        m_stores.push_back(store);
        m_entityStores.push_back(store);
    }

    void Dispatcher::RegisterMainEntityStore(EntityStoreBase *store) {
        m_stores.push_back(store);
        m_mainEntityStore = store;
    }

    EntityStoreBase* Dispatcher::GetMainEntityStore() {
        return m_mainEntityStore;
    }

    void Dispatcher::RegisterEntityManager(EntityManagerBase* entityManager) {
        m_entityManager = entityManager;
    }


}
