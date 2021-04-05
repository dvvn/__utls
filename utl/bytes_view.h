#pragma once
#include "string view ex.h"

#include <algorithm>
#include <span>

namespace utl
{
    template <typename _Rng>
    concept range_of_bytes = _RANGES random_access_range<_Rng> && sizeof(_RANGES range_value_t<_Rng>) == sizeof(std::byte);

    namespace detail
    {
        class _As_bytes_fn
        {
            template <_RANGES random_access_range _Ty>
            static constexpr auto _As_bytes_std(_Ty&& span_v)
            {
                if constexpr (std::copyable<std::remove_reference_t<decltype(span_v[0])> >)
                    return std::as_writable_bytes(span_v);
                else
                    return std::as_bytes(span_v);
            }

            using _Tag_direct = _Tag_number<0>;
            using _Tag_unwrap = _Tag_number<1>;

            struct _Tag
            {
                template <typename _Ty>
                struct select
                {
                    constexpr auto operator()( ) const
                    {
                        if constexpr (!simple_type<_Ty>)
                            return _Tag_unknown( );
                        else
                        {
                            if constexpr (!_RANGES range<_Ty>)
                                return _Tag_direct( );
                            else if constexpr (_RANGES random_access_range<_Ty>)
                                return _Tag_unwrap( );
                            else
                                return _Tag_unknown( );
                        }
                    }
                };
            };

        public:
            template <_Tag_check<_Tag::select, _Tag_direct> _Ty>
            constexpr auto operator()(_Ty&& v) const
            {
                auto span_v = std::span(&v, 1u);
                return _As_bytes_std(span_v);
            }

            template <_Tag_check<_Tag::select, _Tag_unwrap> _Ty>
            constexpr auto operator()(_Ty&& v) const
            {
                auto span_v = std::span(_RANGES data(v), _RANGES size(v));
                return _As_bytes_std(span_v);
            }
        };

        inline constexpr auto _As_bytes = _As_bytes_fn( );

        template <typename _Transform_to /*= uint8_t*/>
        struct _Bytes_view_fn
        {
            template <typename _Source>
            struct _Transform_helper_ref
            {
                using ref_t = _RANGES range_reference_t<_Source>;
                using val_t = std::remove_reference_t<ref_t>;
                static constexpr bool writable = !std::is_const_v<val_t>;

                using transform_val_t = std::conditional_t<writable, _Transform_to, std::add_const_t<_Transform_to> >;
                using transform_ref_t = std::add_lvalue_reference_t<transform_val_t>;

                constexpr transform_ref_t operator()(ref_t val) const
                {
                    return (transform_ref_t)val;
                }
            };

            template <typename _Rng_val>
            struct _Transform_helper_convert
            {
                constexpr _Transform_to operator()(_Rng_val val) const
                {
                    auto b = static_cast<uint8_t>(val);
                    return _Transform_to(b);
                }
            };

            class _Transform_bytes_fn
            {
                using _Tag_unwrap_string = _Tag_number<0>;
                using _Tag_direct = _Tag_number<1>;
                using _Tag_transform_ref = _Tag_number<2>;
                using _Tag_transform_convert = _Tag_number<3>;

                struct _Tag
                {
                    //i made this hack because of compiler bugs

                    template <typename _Ty>
                    struct select
                    {
                        constexpr auto operator( )( ) const
                        {
                            if constexpr (!range_of_bytes<_Ty>)
                                return _Tag_unknown( );
                            else
                            {
                                using rng_val = _RANGES range_value_t<_Ty>;
                                if constexpr (raw_array_of_chars<_Ty>)
                                    return _Tag_unwrap_string( );
                                else if constexpr (std::same_as<rng_val, _Transform_to> || !std::destructible<_Transform_to>)
                                    return _Tag_direct( );
                                else if constexpr (sizeof(_Transform_to) == sizeof(rng_val))
                                    return _Tag_transform_ref( );
                                else if constexpr (std::convertible_to<rng_val, uint8_t> && std::constructible_from<_Transform_to, uint8_t>)
                                    return _Tag_transform_convert( );
                                else
                                    return _Tag_unknown( );
                            }
                        }
                    };
                };

            public:
                template <_Tag_check<typename _Tag::select, _Tag_unwrap_string> _Ty>
                constexpr auto operator()(_Ty&& v) const
                {
                    auto bytes = std::span(v, utl::to_string_view(v).size( ));
                    return (*this)(bytes);
                }

                template <_Tag_check<typename _Tag::select, _Tag_direct> _Ty>
                constexpr auto operator()(_Ty&& v) const
                {
                    return std::span(_RANGES begin(v), _RANGES size(v));
                }

                template <_Tag_check<typename _Tag::select, _Tag_transform_ref> _Ty>
                constexpr auto operator()(_Ty&& v) const
                {
                    constexpr auto transform_helper = _Transform_helper_ref<decltype(v)>( );
                    return std::views::transform(v, transform_helper);
                }

                template <_Tag_check<typename _Tag::select, _Tag_transform_convert> _Ty>
                constexpr auto operator()(_Ty&& v) const
                {
                    constexpr auto transform_helepr = _Transform_helper_convert<_RANGES range_value_t<_Ty> >( );
                    return std::views::transform(v, transform_helepr);
                }
            };
            static constexpr auto _Transform_bytes = _Transform_bytes_fn( );

            using _Tag_transform = _Tag_number<0>;
            using _Tag_convert_ptr = _Tag_number<1>;
            using _Tag_convert = _Tag_number<2>;

            struct _Tag
            {
                template <typename _Ty>
                struct select
                {
                    constexpr auto operator()( ) const
                    {
                        if constexpr (range_of_bytes<_Ty>)
                        {
                            if constexpr (std::invocable<_Transform_bytes_fn, _Ty>)
                                return _Tag_transform( );
                            else
                                return _Tag_unknown( );
                        }
                        else if constexpr (std::invocable<_To_string_view_impl<bruteforce>, _Ty>)
                            return _Tag_convert_ptr( );
                        else if constexpr (std::invocable<_As_bytes_fn, _Ty>)
                            return _Tag_convert( );
                        else
                            return _Tag_unknown( );
                    }
                };
            };

            template <_Tag_check<typename _Tag::select, _Tag_transform> _Ty>
            constexpr auto operator()(_Ty&& v) const
            {
                return _Transform_bytes(v);
            }

            template <_Tag_check<typename _Tag::select, _Tag_convert_ptr> _Ty>
            constexpr auto operator()(_Ty&& v) const
            {
                auto temp_bytes = to_string_view(v);
                auto bytes      = _As_bytes(temp_bytes);
                return _Transform_bytes(bytes);
            }

            template <_Tag_check<typename _Tag::select, _Tag_convert> _Ty>
            constexpr auto operator()(_Ty&& v) const
            {
                auto bytes = _As_bytes(v);
                return _Transform_bytes(bytes);
            }
        };
    }

    template <typename _Transform_to>
    inline static constexpr auto _Bytes_view_ex = detail::_Bytes_view_fn<_Transform_to>( );
    inline static constexpr auto _Bytes_view    = _Bytes_view_ex<uint8_t>;
}
