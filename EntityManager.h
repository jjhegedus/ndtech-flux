#pragma once

#include "Entity.h"
#include "TypeUtilities.h"
#include <tuple>
#include "Store.h"
#include "System.h"
#include "Dispatcher.h"

namespace ndtech {
    struct EntityManagerBase{
        virtual void SynchSystems() {};
        virtual void Process() {};
    };

    template<typename SystemAndDependenciesTypelist, typename EntitiesAndDependenciesTypelist>
    struct EntityManager: public EntityManagerBase {


        //struct EntityStore {
        //    std::vector<std::any>                                                       entities;
        //};


        //template<typename FunctionType, typename... Ts>
        //void ForEachArg(FunctionType func, Ts&&... args) {
        //    (func(args), ...);
        //}

        //template<typename FunctionType, typename TupleType>
        //void ForTuple(FunctionType&& func, TupleType tuple) {
        //    std::apply(
        //        [func](auto&&... tupleItems) {
        //        ForEachArg(
        //            func,
        //            std::forward<decltype(tupleItems)>(tupleItems)...
        //        );
        //    },
        //        std::forward<TupleType>(tuple)
        //        );
        //}

        template<typename EntityStoresTuple>
        void InitializeEntityStores(EntityStoresTuple entityStores) {

            TypeUtilities::Impl::ForTuple(
                [this](auto entityStore) mutable {
                    Dispatcher::RegisterEntityStore(&(std::get<decltype(entityStore)>(m_entityStores)));
                },
                m_entityStores
            );

        }


        std::vector<std::any>                                                       m_entities;
        //Store<EntityStore>                                                          m_store;
        using                                                                       SortedSystemAndTypeDependenciesTypelist = ndtech::TypeUtilities::SortTypeDependencies<SystemAndDependenciesTypelist>;
        using                                                                       SystemsTypelist = ndtech::TypeUtilities::GetPrimaryTypes<SortedSystemAndTypeDependenciesTypelist>;
        using                                                                       SortedEntitiesAndDependenciesTypelist = ndtech::TypeUtilities::SortTypeDependencies<EntitiesAndDependenciesTypelist>;
        using                                                                       EntitiesTypelist = ndtech::TypeUtilities::GetPrimaryTypes<SortedEntitiesAndDependenciesTypelist>;
        using                                                                       EntityStoresTypelist = ndtech::TypeUtilities::Convert<EntitiesTypelist, EntityStore>;
        ndtech::TypeUtilities::Convert<SystemsTypelist, std::tuple>                 m_systems;
        ndtech::TypeUtilities::Convert<EntitiesTypelist, std::tuple>                m_specificEntities;
        std::mutex                                                                  m_entitiesMutex;

        template<typename ...Ts>
        using TupleOfEntityStores = std::tuple<EntityStore<Ts> ...>;

        using EntityStoreTypes = TypeUtilities::Convert<EntitiesTypelist, TupleOfEntityStores>;

        EntityStoreTypes m_entityStores;

        EntityManager() {
            TraceLogWrite("ndtech::EntityManager\n");
            //TypeUtilities::PrintTypes<EntitiesAndDependenciesTypelist>(EntitiesAndDependenciesTypelist{}, "EntitiesAndDependenciesTypelist");
            //TypeUtilities::PrintTypes(EntitiesTypelist{}, "EntitiesTypelist");
            TypeUtilities::PrintTypes(m_entityStores, "PrintTypes of EntityManager::m_entityStores");
            //Dispatcher::RegisterStore(&m_store);

            TypeUtilities::Impl::ForTuple(
                    [this](auto entityStore) mutable {
                        Dispatcher::RegisterEntityStore(&(std::get<decltype(entityStore)>(m_entityStores)));
                    },
                    m_entityStores
            );

            //std::apply([](auto entityStore) {
            //    //Dispatcher::RegisterEntityStore(entityStore)
            //    std::cout << "test\n";
            //}, m_entityStores);

        }

        void SynchSystems() override {

            TypeUtilities::Impl::ForTuple(
                [this](auto system) mutable {
                    auto systemStore = Dispatcher::template GetEntityStore<decltype(system)::InternalEntityType>();

                    using DependentEntityTypes = typename TypeUtilities::Impl::GetDependentTypesImpl<decltype(system), SortedEntitiesAndDependenciesTypelist>::type;
                    using DependentEntitiesTupleType = TypeUtilities::Convert<DependentEntityTypes, std::tuple>;

                    DependentEntitiesTupleType dependentEntitiesTuple;

                    TypeUtilities::Impl::ForTuple(
                        [this](auto entities) mutable {
                            auto entityStore = Dispatcher::template GetEntityStore<decltype(entities)>();
                        },
                        dependentEntitiesTuple
                    );
                },
                m_systems
            );

        }

        void Process() override {

            TypeUtilities::Impl::ForTuple(
                    [this](auto system) mutable {
                        auto thisSystem = std::get<decltype(system)>(m_systems);

                        this->Synch(thisSystem);

                        using DependentEntityTypes = typename TypeUtilities::Impl::GetDependentTypesImpl<decltype(system), SortedEntitiesAndDependenciesTypelist>::type;
                        using DependentEntityTypesTuple = TypeUtilities::Convert<DependentEntityTypes, std::tuple>;

                        DependentEntityTypesTuple dependentEntitiesTuple;

                        TypeUtilities::Impl::ForTuple(
                            [this](auto entities) mutable {
                                auto thisEntities = std::get<decltype(entities)>(m_specificEntities);
                            },
                            dependentEntitiesTuple
                        );

                    },
                    m_systems
            );

        }

        template <typename SystemType>
        void Synch(SystemType system) {
            
        }

        void ReverseSynch()  {

            //TypeUtilities::Impl::ForTuple(
            //        [entityId, entity, this](auto system) mutable {
            //            std::get<decltype(system)>(m_systems).AddEntity(entityId, entity);
            //        },
            //        m_systems
            //);

        }

        template<typename ExternalEntityType>
        size_t AddEntity(ExternalEntityType entity) {
            using VectorType = std::vector<ExternalEntityType>;
            size_t entityId;

            {
                // lock across getting the entity id and inserting into the systems
                std::lock_guard<std::mutex> entitiesLockGuard(m_entitiesMutex);
                entityId = nextEntityId.fetch_add(1);
                m_entities.emplace_back(entity);

                //std::apply([entityId, entity](auto system) {
                //    system.AddEntity(entityId, entity); 
                //    std::cout << "test\n";
                //}, m_systems);

                //TypeUtilities::Impl::ForTuple(
                //        [entityId, entity, this](auto system) mutable {
                //            std::get<decltype(system)>(m_systems).AddEntity(entityId, entity);
                //        },
                //        m_systems
                //);

                //for (size_t index = 0; index < std::tuple_size<ndtech::TypeUtilities::Convert<SystemTypeList, std::tuple>>::value; index++) {
                //    std::get<index>(m_systems).AddEntity(entityId, entity);
                //    std::cout << "test\n";
                //}

            }

            return entityId;
        }

        template<typename ExternalEntityType>
        ExternalEntityType GetEntity(size_t entityId) {
            return std::any_cast<ExternalEntityType>(m_entities[entityId]);
        }

    };

}