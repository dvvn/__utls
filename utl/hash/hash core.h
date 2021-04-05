#pragma once

//@note: type_traits have own non-constexpr fnv1a funcs

#include "utl/bytes_view.h"
#include "utl/string view ex.h"

#include "utl/function info.h"

#include <algorithm>
#include <array>
#include <functional>

namespace utl::hash
{
    namespace detail
    {
        template <typename _Ty>
        concept _Integral_exclude_bool = std::integral<_Ty> && !std::same_as<_Ty, bool>; //todo: 128+ bits check

        //template <typename _Ty>
        //concept signed_integral = std::signed_integral<_Ty>; //todo: 128+ bits check
        //
        //template <typename _Ty>
        //concept unsigned_integral = integral<_Ty> && !signed_integral<_Ty>;

        //template <typename _Ty>
        //concept trivial = std::is_trivial_v<_Ty> && !std::is_pointer_v<_Ty> && !std::is_reference_v<_Ty>;

        //----------------

        template <_Integral_exclude_bool _Ty>
        struct _Hash_init_value
        {
            constexpr _Hash_init_value(_Ty v) : value(std::move(v)) {}

            _Ty value;
        };

        //template <typename _Ty, typename Value_type, typename Hash>
        ///concept hash_processer_func = std::is_void_v<function_return_t<_Ty> > && function_info_t<_Ty>::args::template _Exact_to<Value_type, Hash&>( ) ;
        //concept hash_processer_func = check_function_raw<_Ty, Value_type, Hash&>;

        template <typename _Ty_value, _Integral_exclude_bool _Ty_hash, typename _Ty_updater>
        requires(check_function_raw<_Ty_updater, void, _Ty_value, _Ty_hash&>)
        class _Hash_processer
        {
        public:

            using value_type = _Ty_value;
            using hash_type = _Ty_hash;
            using updater_type = _Ty_updater;

            using init_object = _Hash_init_value<_Ty_hash>;

            //virtual constexpr Hash update(Value_type value, Hash &hash) const = 0;

            constexpr _Hash_processer(const _Ty_updater& update_fn) : updater_((update_fn)) {}
            constexpr _Hash_processer(_Ty_updater&& update_fn) : updater_(std::move(update_fn)) {}

        private:
            _Ty_updater updater_;

            constexpr void _Update(const _Ty_value& value, _Ty_hash& hash) const
            {
                std::invoke(updater_, value, hash);
            }

            template <_RANGES range _Rng>
            constexpr void _Process_range(_Rng&& rng, init_object& initial_value) const
            {
                if constexpr (raw_array_of_chars<_Rng>)
                {
                    this->_Process(to_string_view((rng)), initial_value);
                }
                else
                {
                    using _Rng_value = _RANGES range_value_t<_Rng>;
                    for (const auto& item: rng)
                    {
                        if constexpr (std::convertible_to<_Rng_value, _Ty_value>)
                            this->_Update(item, initial_value.value);
                        else
                            this->_Process(item, initial_value);
                    }
                }
            }

            template <tuple_only _Tpl, size_t _Idx_current, size_t ..._Indexes>
            constexpr void _Process_tuple(_Tpl&& tpl, init_object& initial_value, std::index_sequence<_Idx_current, _Indexes...>) const
            {
                decltype(auto) item = std::get<_Idx_current>((tpl));
                this->_Process((item), initial_value);
                if constexpr (sizeof...(_Indexes) > 0)
                    _Process_tuple((tpl), initial_value, std::index_sequence<_Indexes...>( ));
            }

            template <simple_type V>
            constexpr void _Process_simple(V&& value, init_object& initial_value) const
            {
                if constexpr (sizeof(V) == sizeof(std::byte))
                {
                    auto tmp = (const uint8_t&)value;
                    this->_Update(_Ty_value(tmp), initial_value.value);
                }
                else
                {
                    const auto bytes = _Bytes_view(value);
                    this->_Process_range(bytes, initial_value);
                }
            }

            template <typename _Ty>
            constexpr void _Process(_Ty&& value, init_object& initial_value) const
            {
                using namespace std;
                if constexpr (ranges::range<_Ty>)
                    _Process_range((value), initial_value);
                else if constexpr (tuple_only<_Ty>)
                    _Process_tuple((value), initial_value, make_index_sequence<tuple_size_v<_Ty> >{ });
                else if constexpr (string_viewable<_Ty>) //for char*
                    _Process_range(to_string_view((value)), initial_value);
                else if constexpr (is_null_pointer_v<std::remove_cvref_t<_Ty> >)
                    return;
                else if constexpr (simple_type<_Ty>)
                    _Process_simple((value), initial_value);
                else
                {
                    static_assert(_Always_false<_Ty>, "Utl_hash: unsipported value type!");
                }
            }

        public:

            constexpr const _Ty_updater& get_updater( ) const { return updater_; }

            template <typename ..._Args>
            constexpr decltype(auto) operator()(_Args&& ...args) const requires(std::convertible_to<typename last_element<_Args...>::type, init_object>)
            {
                constexpr auto args_count = sizeof...(_Args);
                constexpr auto indexes    = std::make_index_sequence<args_count - 1>{ };

                decltype(auto) tpl  = std::forward_as_tuple(FWD(args)...);
                decltype(auto) init = std::get<args_count - 1>((tpl));

                if constexpr (!std::is_const_v<std::remove_const_t<decltype(init)> >)
                {
                    _Process_tuple((tpl), (init), indexes);
                    return FWD(init);
                }
                else
                {
                    init_object init_copy = init;
                    _Process_tuple((tpl), init_copy, indexes);
                    return init_copy;
                }
            }
        };
    }

    namespace literals
    {
        template <character _Chr>
        constexpr auto get_data(const _Chr* c, size_t size)
        {
            const std::basic_string_view<_Chr> str(c, size);
            return str;
        }
    }
}
