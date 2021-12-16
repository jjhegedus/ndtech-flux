#pragma once

#include "pch.h"

#include "Action.h"
#include "Store.h"
#include "EntityManager.h"


namespace ndtech {

    class Dispatcher
    {
    public:
        Dispatcher() = delete;
        ~Dispatcher() = delete;

        static void RegisterStore(StoreBase *store);
        static void Dispatch(Action action);
        static void AwaitDispatch(Action action, std::function<void(size_t, Action)> callback);
        static void DispatchComplete(size_t dispatchId, Action action);
        static size_t GetNextActionTypeId();
        static void ProcessActions();

        template<typename StoreType>
        static StoreType* GetStore() {
            for (StoreBase *store : m_stores) {
                if (dynamic_cast<StoreType*>(store) != nullptr) {
                    return dynamic_cast<StoreType*>(store);
                }
            }
            return nullptr;
        }

        template<typename EntityType>
        static EntityStore<EntityType>* GetEntityStore() {
            for (EntityStoreBase *entityStore : m_entityStores) {
                if (dynamic_cast<EntityStore<EntityType>*>(entityStore) != nullptr) {
                    return dynamic_cast<EntityStore<EntityType>*>(entityStore);
                }
            }
            return nullptr;
        }

        template<typename EntityType>
        static size_t DispatchCreateEntityAction(Action action) {
            action.entityId = nextEntityId.fetch_add(1);
            for (StoreBase *store : m_entityStores) {
                store->Dispatch(action);
            }

            return action.entityId;
        }

        static void RegisterEntityStore(EntityStoreBase *store);
        static void RegisterMainEntityStore(EntityStoreBase *store);
        static EntityStoreBase* GetMainEntityStore();

        static void RegisterEntityManager(EntityManagerBase *entityManager);

    private:
        static std::vector<StoreBase*>                                                              m_stores;
        static EntityStoreBase*                                                                     m_mainEntityStore;
        static std::vector<EntityStoreBase*>                                                        m_entityStores;
        static std::atomic<size_t>                                                                  m_nextActionTypeId;
        static std::atomic<size_t>                                                                  m_dispatchId;
        static EntityManagerBase*                                                                   m_entityManager;
        static std::vector<std::tuple<size_t, Action, std::function<void(size_t, Action)>>>         m_dispatchesInProcess;
        static std::mutex                                                                           m_dispatchesInProcessMutex;
    };

}
