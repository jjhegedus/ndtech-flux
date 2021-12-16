#pragma once

#include <string>
#include <tuple>
#include <typeinfo>

namespace ndtech {

    namespace TypeUtilities {

        struct NullType {
            std::string GetTypeName() { return "NullType"; }
        };

        template <typename T>
        struct Sizeof
        {
            static constexpr size_t value = std::is_same<T, NullType>::value ? 0 : sizeof(T);
        };

        struct EmptyType {};


        template <typename...>
        using void_t = void;

        struct nonesuch
        {
            nonesuch() = delete;
            ~nonesuch() = delete;
            nonesuch(nonesuch const&) = delete;
            void operator=(nonesuch const&) = delete;
        };

        namespace Impl {

            template<typename CallbackType, typename... Ts>
            void ForEachArg(CallbackType func, Ts&&... args) {
                (func(args), ...);
            }

            template<typename CallbackType, typename TupleType>
            void ForTuple(CallbackType&& func, TupleType tuple) {
                std::apply(
                    [func](auto&&... tupleItems) {
                    ForEachArg(
                        func,
                        std::forward<decltype(tupleItems)>(tupleItems)...
                    );
                },
                    std::forward<TupleType>(tuple)
                    );
            }

            template <typename V, typename D>
            struct detect_impl
            {
                using value_t = V;
                using type = D;
            };

            template <typename D, template <typename...> class Check, typename... Args>
            auto detect_check(char)
                ->detect_impl<std::false_type, D>;

            template <typename D, template <typename...> class Check, typename... Args>
            auto detect_check(int)
                -> decltype(void_t<Check<Args...>>(),
                    detect_impl<std::true_type, Check<Args...>>{});

            template <typename D, typename Void, template <typename...> class Check, typename... Args>
            struct detect : decltype(detect_check<D, Check, Args...>(0)) {};

            template<typename... Ts>
            struct TypelistImpl {
                using type = TypelistImpl;
                static constexpr size_t size() noexcept { return sizeof...(Ts); }
            };



            template <typename SourceType, template <typename...> typename DestinationType>
            struct ConvertImpl;

            // "Converts" `SourceType<Ts...>` to `DestinationType<Ts...>`.
            template <template <typename...> typename SourceType, template <typename...> typename DestinationType, typename... Ts>
            struct ConvertImpl<SourceType<Ts...>, DestinationType>
            {
                using type = DestinationType<Ts...>;
            };


            template<typename... Ts>
            void PrintTypesInTypelistImpl(TypelistImpl<Ts...> typelist, std::string header = "") {
                std::cout << header << "\n";
                size_t index = 0;
                ForTuple(
                    [index](const auto& tupleItem) mutable {std::cout << "\ttypeid(tupleItem #" << index << ").name() = " << typeid(tupleItem).name() << "\n"; index++; },
                    ConvertImpl<TypelistImpl<Ts...>, std::tuple>::type{}
                );
            }

            template<typename T>
            void PrintTypesImpl(T t, std::string header = "") {
                std::cout << header << "\n";
                std::cout << typeid(T).name() << "\n";
                TraceLogWrite("PrintTypesImpl1",
                    TraceLoggingString(typeid(T).name(), "TypeName"));
            }

            template<typename... Ts>
            void PrintTypesImpl(Ts... typelist, std::string header = "") {
                std::cout << header << "\n";
                size_t index = 0;
                ForTuple(
                    [index](const auto& tupleItem) mutable {
                        std::cout << "\ttypeid(tupleItem #" << index << ").name() = " << typeid(tupleItem).name() << "\n"; index++;
                        TraceLogWrite("PrintTypesImpl2",
                            TraceLoggingString(typeid(tupleItem).name(), "TypeName"));
                    },
                    ConvertImpl<Ts..., std::tuple>::type{}
                );
            }


            template<typename... Ts>
            struct PushFrontImpl {
                using type = typename TypelistImpl<>::type;
            };

            template<typename... Ts>
            struct PushFrontImpl<Ts...> {
                using type = typename TypelistImpl<Ts...>::type;
            };

            template <typename... Ts>
            struct PushFrontImpl<TypelistImpl<Ts...>> {
                using type = typename TypelistImpl<Ts...>::type;
            };

            template<typename... Ts, typename... newTs>
            struct PushFrontImpl<TypelistImpl<Ts...>, newTs...> {
                using type = typename TypelistImpl<newTs..., Ts...>::type;
            };

            template<typename... Ts, typename... newTs>
            struct PushFrontImpl<TypelistImpl<Ts...>, TypelistImpl<newTs...>> {
                using type = typename TypelistImpl<newTs..., Ts...>::type;
            };

            template<typename T, typename... newTs>
            struct PushFrontImpl<T, TypelistImpl<newTs...>> {
                using type = typename TypelistImpl<newTs..., T>::type;
            };




            template<typename... Ts>
            struct PushBackImpl {
                using type = typename TypelistImpl<>::type;
            };

            template<typename... Ts>
            struct PushBackImpl<Ts...> {
                using type = typename TypelistImpl<Ts...>::type;
            };

            template <typename... Ts>
            struct PushBackImpl<TypelistImpl<Ts...>> {
                using type = typename TypelistImpl<Ts...>::type;
            };

            template<typename... Ts, typename... newTs>
            struct PushBackImpl<TypelistImpl<Ts...>, newTs...> {
                using type = typename TypelistImpl<Ts..., newTs...>::type;
            };

            template<typename... Ts, typename... newTs>
            struct PushBackImpl<TypelistImpl<Ts...>, TypelistImpl<newTs...>> {
                using type = typename TypelistImpl<Ts..., newTs...>::type;
            };





            template<typename... Ts>
            struct RemoveTypeImpl {
                using type = typename TypelistImpl<>::type;
                static inline std::string debug = "RemoveTypeImpl:1:";
            };

            template<typename T, typename... Ts>
            struct RemoveTypeImpl<T, TypelistImpl<T, Ts...>> {
                using type = typename TypelistImpl<Ts...>::type;
                static inline std::string debug = "RemoveTypeImpl:3:";
            };

            template<typename T, typename H, typename... Ts>
            struct RemoveTypeImpl<T, TypelistImpl<H, Ts...>> {
                using type = typename PushFrontImpl<typename RemoveTypeImpl<T, TypelistImpl<Ts...>>::type, H>::type;
                static std::string inline debug = std::string("RemoveTypeImpl:3:").append(RemoveTypeImpl<T, TypelistImpl<Ts...>>::debug);
            };






            template<typename... Ts>
            struct ConcatImpl;

            template<typename... T1s, typename... T2s>
            struct ConcatImpl<TypelistImpl<T1s...>, TypelistImpl<T2s...>> {
                using type = typename TypelistImpl<T1s..., T2s...>::type;
                static inline std::string value = "ConcatImpl:1:";
            };

            template<typename T, typename... Ts>
            struct ConcatImpl<T, TypelistImpl<Ts...>> {
                using type = typename TypelistImpl<T, Ts...>::type;
                static inline std::string value = "ConcatImpl:2:";
            };

            template<typename T, typename... Ts>
            struct ConcatImpl<TypelistImpl<Ts...>, T> {
                using type = typename PushBackImpl<TypelistImpl<Ts...>, T>::type;
                static inline std::string value = "ConcatImpl:3:";
            };

            template<typename... Ts>
            struct ConcatImpl<TypelistImpl<Ts...>> {
                using type = typename TypelistImpl<Ts...>::type;
                static inline std::string value = "ConcatImpl:4:";
            };





            template<typename... ListOfLists>
            struct FlattenImpl;

            template<typename... Ts>
            struct FlattenImpl<TypelistImpl<Ts...>> {
                using type = typename FlattenImpl<Ts...>::type;
                static std::string inline value = std::string("FlattenImpl:1:").append(FlattenImpl<Ts...>::value);
            };

            template<typename Typelist1, typename Typelist2>
            struct FlattenImpl<Typelist1, Typelist2> {
                using type = typename ConcatImpl<Typelist1, Typelist2>::type;
                static std::string inline value = std::string("FlattenImpl:2:").append(ConcatImpl<Typelist1, Typelist2>::value);
            };

            template<typename Typelist1, typename Typelist2, typename... RemainingTypelists>
            struct FlattenImpl<Typelist1, Typelist2, RemainingTypelists...> {
                using type = typename FlattenImpl<typename ConcatImpl<Typelist1, Typelist2>::type, RemainingTypelists...>::type;
                static std::string inline value = std::string("FlattenImpl:3:").append(ConcatImpl<Typelist1, Typelist2>::value).append(FlattenImpl<typename ConcatImpl<Typelist1, Typelist2>::type, RemainingTypelists...>::value);
            };




            template <size_t index, typename... Ts>
            struct RemoveAtImpl;

            template <typename T, typename... Ts>
            struct RemoveAtImpl<0, TypelistImpl<T, Ts...>> {
                using type = typename TypelistImpl<Ts...>::type;
                static inline std::string value = "RemoveAtImpl:1:";
            };

            template <size_t index, typename T, typename... Ts>
            struct RemoveAtImpl<index, TypelistImpl<T, Ts...>> {
                using type = typename ConcatImpl<T, typename RemoveAtImpl<index - 1, TypelistImpl<Ts...>>::type >::type;
                static inline std::string value = std::string("RemoveAtImpl:2:").append(RemoveAtImpl<index - 1, TypelistImpl<Ts...>>::value);
            };






            template <std::size_t index, typename typeList>
            struct TypeAtImpl;

            template <typename T, typename... Ts>
            struct TypeAtImpl < 0, TypelistImpl<T, Ts...> > {
                using type = T;
            };

            template <std::size_t index, typename T, typename... Ts>
            struct TypeAtImpl <index, TypelistImpl<T, Ts...>> {
                using type = typename TypeAtImpl< index - 1, TypelistImpl<Ts...> >::type;
            };



            template <typename... Ts>
            struct PopFrontImpl;

            template <typename... Ts>
            struct PopFrontImpl {
                using type = typename RemoveAtImpl<0, Ts...>::type;
            };


            template<typename... typelist>
            struct PopBackImpl;

            template<typename... Ts>
            struct PopBackImpl<TypelistImpl<Ts...>> {
                using type = typename RemoveAtImpl<sizeof...(Ts) - 1, TypelistImpl<Ts...>>::type;
                static size_t constexpr value = sizeof...(Ts);
            };


            template <size_t index, typename... Ts>
            struct IndexOfImpl {
            };

            template <size_t index, typename T, typename... Ts>
            struct IndexOfImpl<index, T, TypelistImpl<T, Ts...>> {
                static constexpr size_t value = index;
                static inline std::string debug = "IndexOfImpl:1:";
            };

            template <size_t index, typename T, typename... Ts>
            struct IndexOfImpl<index, T, TypelistImpl<Ts...>> {
                static constexpr size_t value = IndexOfImpl<index + 1, T, typename RemoveAtImpl<0, TypelistImpl<Ts...>>::type>::value;
                static inline std::string debug = std::string("IndexOfImpl:2:index = ").append(std::to_string(index)).append(":").append("size = ").append(std::to_string(sizeof...(Ts))).append(":").append(IndexOfImpl<index + 1, T, typename RemoveAtImpl<0, TypelistImpl<Ts...>>::type>::debug);
            };

            template<size_t index, typename T>
            struct IndexOfImpl<index, T, TypelistImpl<>> {
                static constexpr size_t value = -1;
                static inline std::string debug = "IndexOfImpl:3:";
            };


            template<typename T, typename... Ts>
            struct ContainsImpl;

            template<typename T, typename... Ts>
            struct ContainsImpl<T, Ts...> {
                static constexpr bool value = !(IndexOfImpl<0, T, Ts...>::value == -1);
                static constexpr size_t index = IndexOfImpl<0, T, Ts...>::value;
                static inline std::string debug = IndexOfImpl<0, T, Ts...>::debug;
            };


            template<typename... Ts>
            struct ContainsAnyOfImpl;

            template<typename... TsToTestAgainst>
            struct ContainsAnyOfImpl<TypelistImpl<>, TypelistImpl<TsToTestAgainst...>> {
                static constexpr bool value = false;

                static std::string inline debug = std::string("ContainsAnyOfImpl:1:False");
            };

            template<typename T, typename... AdditionalTsToTest, typename... TsToTestAgainst>
            struct ContainsAnyOfImpl<TypelistImpl<T, AdditionalTsToTest...>, TypelistImpl<TsToTestAgainst...>> {
                static constexpr bool value = std::conditional<
                    ContainsImpl<T, TypelistImpl<TsToTestAgainst...>>::value,
                    std::true_type,
                    ContainsAnyOfImpl<TypelistImpl<AdditionalTsToTest...>, TypelistImpl<TsToTestAgainst...>>>::type::value;

                static std::string inline debug = ContainsImpl<T, TypelistImpl<TsToTestAgainst...>>::value ? std::string("ContainsAnyOfImpl:2:True") : std::string("ContainsAnyOfImpl:3:").append(ContainsAnyOfImpl<TypelistImpl<AdditionalTsToTest...>, TypelistImpl<TsToTestAgainst...>>::debug);
            };



            template<typename... Ts>
            struct RemoveAllOfImpl {
                using type = typename TypelistImpl<>::type;
                static inline std::string debug = "RemoveAllOfImpl:1:";
            };

            template<typename T, typename... Ts>
            struct RemoveAllOfImpl<T, TypelistImpl<T, Ts...>> {
                using type = typename RemoveAllOfImpl<T, TypelistImpl<Ts...>>::type;
                static inline std::string debug = "RemoveAllOfImpl:3:";
            };

            template<typename T, typename H, typename... Ts>
            struct RemoveAllOfImpl<T, TypelistImpl<H, Ts...>> {
                using type = typename PushFrontImpl<typename RemoveAllOfImpl<T, TypelistImpl<Ts...>>::type, H>::type;
                static std::string inline debug = std::string("RemoveAllOfImpl:3:").append(RemoveAllOfImpl<T, TypelistImpl<Ts...>>::debug);
            };



            template<typename typelist>
            struct RemoveDuplicatesImpl;

            template<>
            struct RemoveDuplicatesImpl<TypelistImpl<>> {
                using type = TypelistImpl<>;
                static inline std::string debug = "RemoveDuplicatesImpl:1|";
            };

            template<typename H, typename... Ts>
            struct RemoveDuplicatesImpl<TypelistImpl<H, Ts...>> {
            private:
                using unique_t = typename RemoveDuplicatesImpl<TypelistImpl<Ts...>>::type;
                using new_t = typename RemoveTypeImpl<H, unique_t>::type;
            public:
                using type = typename PushFrontImpl<new_t, H>::type;
                static std::string inline debug = std::string("RemoveDuplicatesImpl:2|").append(RemoveDuplicatesImpl<TypelistImpl<Ts...>>::debug);
            };


            template<typename OriginalType, typename NewType, typename... Ts>
            struct ReplaceFirstImpl;

            template<typename OriginalType, typename NewType, typename... Ts>
            struct ReplaceFirstImpl;

            template<typename OriginalType, typename NewType>
            struct ReplaceFirstImpl<OriginalType, NewType, TypelistImpl<>> {
                using type = TypelistImpl<>;
            };

            template<typename OriginalType, typename NewType, typename... Ts>
            struct ReplaceFirstImpl<OriginalType, NewType, TypelistImpl<OriginalType, Ts...>> {
                using type = TypelistImpl<NewType, Ts...>;
            };

            template<typename OriginalType, typename NewType, typename OtherType, typename... Ts>
            struct ReplaceFirstImpl<OriginalType, NewType, TypelistImpl<OtherType, Ts...>> {
                using type = typename PushFrontImpl<typename ReplaceFirstImpl<OriginalType, NewType, TypelistImpl<Ts...>>::type, OtherType>::type;
            };



            template<typename OriginalType, typename NewType, typename... Ts>
            struct ReplaceAllOfTypeImpl;

            template<typename OriginalType, typename NewType>
            struct ReplaceAllOfTypeImpl<OriginalType, NewType, TypelistImpl<>> {
                using type = TypelistImpl<>;
            };

            template<typename OriginalType, typename NewType, typename... Ts>
            struct ReplaceAllOfTypeImpl<OriginalType, NewType, TypelistImpl<OriginalType, Ts...>> {
                using type = typename PushFrontImpl<typename ReplaceAllOfTypeImpl<OriginalType, NewType, TypelistImpl<Ts...>>::type, NewType>::type;
            };

            template<typename OriginalType, typename NewType, typename OtherType, typename... Ts>
            struct ReplaceAllOfTypeImpl<OriginalType, NewType, TypelistImpl<OtherType, Ts...>> {
                using type = typename PushFrontImpl<typename ReplaceAllOfTypeImpl<OriginalType, NewType, TypelistImpl<Ts...>>::type, OtherType>::type;
            };



            template <typename T, typename... Dependencies>
            struct TypeDependenciesImpl {
                using type = T;
                using dependencies = typename TypelistImpl<Dependencies...>;
            };

            template<typename T, typename... Dependencies>
            struct TypeDependenciesImpl<T, TypelistImpl<Dependencies...>> {
                using type = T;
                using dependencies = typename TypelistImpl<Dependencies...>;
            };

            template<typename... TypeDependencies>
            struct GetPrimaryTypesImpl;

            template<>
            struct GetPrimaryTypesImpl<TypelistImpl<>> {
                using type = TypelistImpl<>;
            };

            template<typename T, typename... Dependencies, typename... RemainingTypeDependencies>
            struct GetPrimaryTypesImpl<TypelistImpl<TypeDependenciesImpl<T, Dependencies...>, RemainingTypeDependencies...>> {
                using type = typename PushFrontImpl<typename GetPrimaryTypesImpl<TypelistImpl<RemainingTypeDependencies...>>::type, T>::type;
            };

            //template<typename... Ts>
            //struct HasDependenciesImpl;

            //template<typename T>
            //struct HasDependenciesImpl<TypeDependenciesImpl<T, TypelistImpl<>>, TypelistImpl<>> {
            //    static constexpr bool value = false;
            //};

            //template<typename T, typename... Ts>
            //struct HasDependenciesImpl<TypeDependenciesImpl<T, TypelistImpl<>>, TypelistImpl<Ts...>> {
            //    static constexpr bool value = false;
            //};

            //template<typename T, typename... Dependencies, typename... Ts>
            //struct HasDependenciesImpl<TypeDependenciesImpl<T, TypelistImpl<Dependencies...>, TypelistImpl<Ts...>>{
            //    static constexpr bool value = ContainsAnyOfImpl<TypelistImpl<Dependencies...>, TypelistImpl<Ts...>>::value;
            //};


            template<typename... Ts>
            struct LeastDependentImpl;

            template<>
            struct LeastDependentImpl<TypelistImpl<>> {
                using type = NullType;

                static inline std::string debug = "LeastDependentImpl:0:";
            };

            template<typename T, typename... Dependencies>
            struct LeastDependentImpl < TypeDependenciesImpl<T, TypelistImpl<Dependencies...>, TypelistImpl<>>> {
                using type = TypeDependenciesImpl<T, TypelistImpl<Dependencies...>>;

                static inline std::string debug = "LeastDependentImpl:1:";
            };

            template<typename T>
            struct LeastDependentImpl < TypeDependenciesImpl<T, TypelistImpl<>>, TypelistImpl<>> {
                using type = TypeDependenciesImpl<T, TypelistImpl<>>;

                static inline std::string debug = "LeastDependentImpl:2:";
            };

            template<typename T, typename... RemainingTypeDependencies>
            struct LeastDependentImpl < TypeDependenciesImpl<T, TypelistImpl<>>, TypelistImpl<RemainingTypeDependencies...>> {
                using type = TypeDependenciesImpl<T, TypelistImpl<>>;

                static inline std::string debug = "LeastDependentImpl:3:";
            };


            template<typename T, typename... Dependencies, typename... RemainingTypeDependencies>
            struct LeastDependentImpl <TypelistImpl<TypeDependenciesImpl<T, Dependencies...>, RemainingTypeDependencies...>> {
                using primaryTypes = typename GetPrimaryTypesImpl<TypelistImpl<RemainingTypeDependencies...>>::type;
                using remainingTypeDependencies = TypelistImpl<RemainingTypeDependencies...>;
                using type = typename std::conditional<
                    ContainsAnyOfImpl<TypelistImpl<Dependencies...>, primaryTypes>::value,
                    //typename LeastDependentImpl<TypelistImpl<RemainingTypeDependencies...>, primaryTypes>::type,
                    T,
                    TypeDependenciesImpl<T, Dependencies...>>::type;

                static inline std::string debug = std::string("LeastDependentImpl:4:").append("test type = ").append(typeid(TypeDependenciesImpl<T, Dependencies...>).name()).append(":").append(ContainsAnyOfImpl<TypelistImpl<Dependencies...>, typename GetPrimaryTypesImpl<TypelistImpl<RemainingTypeDependencies...>>::type>::debug).append(":").append(ContainsAnyOfImpl<TypelistImpl<Dependencies...>, typename GetPrimaryTypesImpl<TypelistImpl<RemainingTypeDependencies...>>::type>::value ? LeastDependentImpl<TypelistImpl<RemainingTypeDependencies...>>::debug : "LeastDependentImpl returning TypeDependenciesImpl");
            };

            template<typename T, typename... Dependencies, typename... PrimaryTypes>
            struct LeastDependentImpl <TypelistImpl<TypeDependenciesImpl<T, Dependencies...>>, TypelistImpl<PrimaryTypes...>> {
                using remainingTypeDependencies = NullType;
                using type = TypeDependenciesImpl<T, Dependencies...>;

                static inline std::string debug = std::string("LeastDependentImpl:5:");
            };

            template<typename T, typename... Dependencies, typename... RemainingTypeDependencies, typename... PrimaryTypes>
            struct LeastDependentImpl <TypelistImpl<TypeDependenciesImpl<T, Dependencies...>, RemainingTypeDependencies...>, TypelistImpl<PrimaryTypes...>> {
                using remainingTypeDependencies = TypelistImpl<RemainingTypeDependencies...>;
                using type = typename std::conditional<
                    ContainsAnyOfImpl<TypelistImpl<Dependencies...>, TypelistImpl<PrimaryTypes...>>::value,
                    typename LeastDependentImpl<TypelistImpl<RemainingTypeDependencies...>, TypelistImpl<PrimaryTypes...>>::type,
                    //T,
                    TypeDependenciesImpl<T, Dependencies...>>::type;

                static inline std::string debug = std::string("LeastDependentImpl:6:").append("test type = ").append(typeid(TypeDependenciesImpl<T, Dependencies...>).name()).append(":").append(ContainsAnyOfImpl<TypelistImpl<Dependencies...>, typename GetPrimaryTypesImpl<TypelistImpl<RemainingTypeDependencies...>>::type>::debug).append(":").append(ContainsAnyOfImpl<TypelistImpl<Dependencies...>, typename GetPrimaryTypesImpl<TypelistImpl<RemainingTypeDependencies...>>::type>::value ? LeastDependentImpl<TypelistImpl<RemainingTypeDependencies...>>::debug : "LeastDependentImpl returning TypeDependenciesImpl");
            };


            template<typename... TypeDependencies>
            struct SortTypeDependenciesImpl;

            template<>
            struct SortTypeDependenciesImpl<TypelistImpl<>> {
                using type = TypelistImpl<>;
            };

            template<typename T, typename... RemainingTypeDependencies>
            struct SortTypeDependenciesImpl<TypelistImpl<TypeDependenciesImpl<T, TypelistImpl<>>, RemainingTypeDependencies...>> {
                using type = typename PushFrontImpl<typename SortTypeDependenciesImpl<TypelistImpl<RemainingTypeDependencies...>>::type, TypeDependenciesImpl<T, TypelistImpl<>>>::type;
            };

            template<typename... TypeDependencies>
            struct SortTypeDependenciesImpl<TypelistImpl<TypeDependencies...>> {
                using primaryTypes = typename GetPrimaryTypesImpl<TypelistImpl<TypeDependencies...>>::type;
                //using leastDependent = typename LeastDependentImpl<TypelistImpl<TypeDependencies...>>::type;
                using leastDependent = typename LeastDependentImpl<TypelistImpl<TypeDependencies...>, primaryTypes>::type;
                using remainder = typename RemoveTypeImpl<leastDependent, TypelistImpl<TypeDependencies...>>::type;
                using type = typename PushFrontImpl< typename SortTypeDependenciesImpl<remainder>::type, leastDependent >::type;
            };

            template<typename... Ts>
            struct IsDependentImpl;

            template<typename T, typename Dependent, typename... Dependencies>
            struct IsDependentImpl<TypeDependenciesImpl<Dependent, TypelistImpl<Dependencies...>>, T> {
                static constexpr bool value = ContainsImpl<T, TypelistImpl<Dependencies...>>::value;
                static inline std::string debug = std::string("");
                using dependencies = TypelistImpl<Dependencies...>;
            };

            template<typename... Ts>
            struct GetDependentTypesImpl;

            template<typename T>
            struct GetDependentTypesImpl<T> {
                using type = TypelistImpl<>;

                static inline std::string debug = std::string("GetDependentTypesImpl#1:");
            };

            template<typename T>
            struct GetDependentTypesImpl<T, TypelistImpl<>> {
                using type = TypelistImpl<>;

                static inline std::string debug = std::string("GetDependentTypesImpl#2:");
            };

            template<typename T, typename T1>
            struct GetDependentTypesImpl<T, TypeDependenciesImpl<T1, TypelistImpl<>>> {

                using type = TypelistImpl<>;

                static inline std::string debug = std::string("GetDependentTypesImpl#3:");
            };

            template<typename T, typename T1>
            struct GetDependentTypesImpl<T, TypelistImpl<TypeDependenciesImpl<T1, TypelistImpl<>>>> {

                using type = TypelistImpl<>;

                static inline std::string debug = std::string("GetDependentTypesImpl#4:");
            };

            template<typename T, typename T1, typename... Dependencies>
            struct GetDependentTypesImpl<T, TypeDependenciesImpl<T1, TypelistImpl<Dependencies...>>> {
                static constexpr bool value = IsDependentImpl<TypeDependenciesImpl<T1, TypelistImpl<Dependencies...>>, T>::value;

                using type = typename std::conditional<
                    value,
                    TypelistImpl<T1>,
                    TypelistImpl<>>::type;

                static inline std::string debug = std::string("GetDependentTypesImpl#5:");
            };

            template<typename T, typename T1, typename... Dependencies>
            struct GetDependentTypesImpl<T, TypelistImpl<TypeDependenciesImpl<T1, TypelistImpl<Dependencies...>>>> {
                static constexpr bool value = IsDependentImpl<TypeDependenciesImpl<T1, TypelistImpl<Dependencies...>>, T>::value;

                using type = typename std::conditional<
                    value,
                    TypelistImpl<T1>,
                    TypelistImpl<>>::type;

                static inline std::string debug = value ? std::string("GetDependentTypesImpl#6:true") : std::string("GetDependentTypesImpl#6:false");
            };

            template<typename T, typename T1, typename... RemainingTypeDependencies>
            struct GetDependentTypesImpl<T, TypelistImpl<TypeDependenciesImpl<T1, TypelistImpl<>>, RemainingTypeDependencies...>> {
                static constexpr bool value = IsDependentImpl<TypeDependenciesImpl<T1, TypelistImpl<>>, T>::value;

                using type = typename std::conditional<
                    value,
                    typename ConcatImpl<T1, typename GetDependentTypesImpl<T, TypelistImpl<RemainingTypeDependencies...>>::type>::type,
                    typename GetDependentTypesImpl<T, TypelistImpl<RemainingTypeDependencies...>>::type>::type;

                static inline std::string debug = value ? std::string("GetDependentTypesImpl#7:true:").append(GetDependentTypesImpl<T, TypelistImpl<RemainingTypeDependencies...>>::debug) : std::string("GetDependentTypesImpl#7:false:").append(GetDependentTypesImpl<T, TypelistImpl<RemainingTypeDependencies...>>::debug);
            };

            template<typename T, typename T1, typename... Dependencies, typename... RemainingTypeDependencies>
            struct GetDependentTypesImpl<T, TypelistImpl<TypeDependenciesImpl<T1, TypelistImpl<Dependencies...>>, RemainingTypeDependencies...>> {
                static constexpr bool value = IsDependentImpl<TypeDependenciesImpl<T1, TypelistImpl<Dependencies...>>, T>::value;

                using type = typename std::conditional<
                    value,
                    typename ConcatImpl<T1, typename GetDependentTypesImpl<T, TypelistImpl<RemainingTypeDependencies...>>::type>::type,
                    typename GetDependentTypesImpl<T, TypelistImpl<RemainingTypeDependencies...>>::type>::type;

                static inline std::string debug = value ? std::string("GetDependentTypesImpl#8:true:").append(GetDependentTypesImpl<T, TypelistImpl<RemainingTypeDependencies...>>::debug) : std::string("GetDependentTypesImpl#8:false:").append(GetDependentTypesImpl<T, TypelistImpl<RemainingTypeDependencies...>>::debug);
            };

        }

        template<typename... Ts>
        void PrintTypesInTypelist(Ts&&... args) {
            Impl::PrintTypesInTypelistImpl(std::forward<Ts>(args)...);
        }

        template<typename... Ts>
        void PrintTypes(Ts&&... args) {
            Impl::PrintTypesImpl(std::forward<Ts>(args)...);
        }


        template <template< typename... > class Check, typename... Args>
        using is_detected = typename Impl::detect<nonesuch, void, Check, Args...>::value_t;


        template <typename... Ts>
        using Typelist = Impl::TypelistImpl<Ts...>;

        using EmptyTypeList = Typelist<>;


        template <typename SourceType, template <typename...> typename DestinationType>
        using Convert = typename Impl::ConvertImpl<SourceType, DestinationType>::type;


        template<typename... Ts>
        using PushFront = typename Impl::PushFrontImpl<Ts...>::type;


        template<typename... Ts>
        using PushBack = typename Impl::PushBackImpl<Ts...>::type;


        template<typename... Ts>
        using RemoveType = typename Impl::RemoveTypeImpl<Ts...>::type;


        template <typename Typelist1, typename Typelist2>
        using Concat = typename Impl::ConcatImpl<Typelist1, Typelist2>::type;


        template<typename... ListOfLists>
        using Flatten = typename Impl::FlattenImpl<ListOfLists...>::type;

        template <size_t index, typename... Ts>
        using RemoveAt = typename Impl::RemoveAtImpl<index, Ts...>::type;

        template <std::size_t index, typename typeList>
        using TypeAt = typename Impl::TypeAtImpl<index, typeList>::type;

        template <typename... Ts>
        using PopFront = typename Impl::PopFrontImpl<Ts...>::type;

        template <typename... Ts>
        using PopBack = typename Impl::PopBackImpl<Ts...>::type;

        template<typename T, typename... Ts>
        size_t IndexOf() {
            return Impl::IndexOfImpl<0, T, Ts...>::value;
        }

        template<typename T, typename... Ts>
        bool Contains() {
            return Impl::ContainsImpl<T, Ts...>::value;
        }

        template<typename... TsToTest, typename... TsToTestAgainst>
        bool ContainsAnyOf(Typelist<TsToTest...> tsToTest, Typelist<TsToTestAgainst...> tsToTestAgainst) {
            return Impl::ContainsAnyOfImpl<Typelist<TsToTest...>, Typelist<TsToTestAgainst...>>::value;
        };

        template<typename T, typename... Ts>
        using RemoveAllOf = typename Impl::RemoveAllOfImpl<T, Ts...>::type;


        template<typename typelist>
        using RemoveDuplicates = typename Impl::RemoveDuplicatesImpl<typelist>::type;

        template<typename OriginalType, typename NewType, typename... Ts>
        using ReplaceFirst = typename Impl::ReplaceFirstImpl<OriginalType, NewType, Ts...>::type;

        template<typename OriginalType, typename NewType, typename... Ts>
        using ReplaceAllOfType = typename Impl::ReplaceAllOfTypeImpl<OriginalType, NewType, Ts...>::type;

        template <typename T, typename... Dependencies>
        using TypeDependencies = Impl::TypeDependenciesImpl<T, Typelist<Dependencies...>>;

        template<typename... TypeDependencies>
        using GetPrimaryTypes = typename Impl::GetPrimaryTypesImpl<TypeDependencies...>::type;

        template<typename... TypeDependencies>
        using SortTypeDependencies = typename Impl::SortTypeDependenciesImpl<TypeDependencies...>::type;

        template<typename T>
        struct is_copy_assignable {
        private:
            template<typename U, typename = decltype(std::declval<U&>() = std::declval<U const&>())>
            static std::true_type try_assignment(U&&);
            static std::false_type try_assignment(...);

        public:
            using type = decltype(try_assignment(std::declval<T>()));
        };

    }

}