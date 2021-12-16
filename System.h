#pragma once

#include "pch.h"

#include "TypeUtilities.h"
#include "Entity.h"
#include "Store.h"

namespace ndtech {

    template<typename ToType, typename FromType>
    constexpr bool CanConvert() {
        return std::is_same_v<ToType, decltype(Convert(std::declval<FromType>()))>;
    };

    //template <typename SystemType>
    //using InitializeEntityType = decltype(std::declval<SystemType>().InitializeEntity(std::declval<size_t>()));

    //template <typename SystemType>
    //constexpr bool HasInitializeEntity() {
    //    return ndtech::TypeUtilities::is_detected<InitializeEntityType, SystemType>::value;
    //}

    template <typename ConcreteSystem, typename InternalEntityType>
    struct System {

        static_assert(std::is_copy_assignable_v<InternalEntityType>);

        System() {
            TraceLogWrite("ndtech::System\n");
            Dispatcher::RegisterEntityStore(&m_store);
        }

        template<typename ExternalEntityType>
        void AddEntity(size_t entityId, ExternalEntityType externalEntity) {
            std::stringstream ss;
            ss << typeid(ConcreteSystem).name() << std::endl;

            TraceLogWrite(
                "ndtech::System::AddEntity",
                TraceLoggingString(ss.str().c_str(), "ConcreteSystem"));

            if constexpr (CanConvert<InternalEntityType, ExternalEntityType>()) {
                InternalEntityType internalEntity = Convert(externalEntity);

                //m_uninitializedEntities.emplace_back(std::pair<size_t, InternalEntityType>(entityId, internalEntity));

                //if constexpr(HasInitializeEntity<ConcreteSystem>()) {
                //    static_cast<ConcreteSystem*>(this)->InitializeEntity(entityId, internalEntity);
                //}
            }

        }

        //InternalEntityType GetEntity(size_t entityId) {
        //    auto foundEntityPair = std::find_if(
        //        m_internalEntities.begin(),
        //        m_internalEntities.end(),
        //        [entityId](std::pair<size_t, InternalEntityType> entityPair) {
        //        return entityPair.first == entityId;
        //    });

        //    if (foundEntityPair != m_internalEntities.end()) {
        //        return (*foundEntityPair).second;
        //    }
        //    else {
        //        TraceLogWrite("ndtech::System::GetEntity(): invalid entityId",
        //            TraceLoggingInt32(entityId, "entityId"));

        //        throw "ndtech::System::GetEntity(): invalid entityId\n";
        //    }
        //}

        std::vector<std::pair<size_t, InternalEntityType>> m_uninitializedEntities;
        std::vector<std::pair<size_t, InternalEntityType>> m_internalEntities;

        struct SystemStore {
            std::vector<std::pair<size_t, InternalEntityType>> uninitializedEntities;
            std::vector<std::pair<size_t, InternalEntityType>> internalEntities;
        };

        EntityStore<SystemStore>                                  m_store;

    };
}