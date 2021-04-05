#pragma once

#include "utils core.h"

#include <array>
#include <span>
#include <string>
#include <xutility>

namespace utl
{
    //template <typename _Ty>
    //concept character = sizeof(_Ty) == sizeof(char) && !std::is_enum_v<_Ty> && !std::is_class_v<_Ty> && !std::same_as<_Ty, bool>;

    template <typename _Ty>
    concept character = !std::same_as<typename std::char_traits<_Ty>::int_type, long>;
    static_assert(std::same_as<std::char_traits<void*>::int_type, long>, " std::char_traits<unknown>::int_type changed!");

    template <typename _Ty>
    concept characters_range = _RANGES random_access_range<std::remove_cvref_t<_Ty> > && character<_RANGES range_value_t<_Ty> >;

    template <typename _Ty, typename _Chr>
    concept characters_range_exact = characters_range<_Ty> && std::same_as<_RANGES range_value_t<_Ty>, _Chr>;

    template <typename _Ty>
    concept raw_array = std::is_bounded_array_v<std::remove_cvref_t<_Ty> >;

    template <typename _Ty>
    concept std_array = std::_Is_std_array_v<std::remove_cvref_t<_Ty> >;

    template <typename _Ty>
    concept known_array_size = raw_array<_Ty> || std_array<_Ty>;

    template <typename _Ty>
    concept raw_array_of_chars = raw_array<_Ty> && characters_range<_Ty>;

    template <typename _Ty, typename _Ty_wanted>
    concept raw_array_exact = raw_array<_Ty> && std::same_as<std::remove_all_extents_t<std::remove_cvref_t<_Ty> >, _Ty_wanted>;

#if 0

    namespace detail
    {
        //for internal use only
        template <typename _Ty, size_t size>
        constexpr size_t _Array_size(std::type_identity<_Ty[size]>)
        {
            return size;
        }

        //for internal use only
        template <typename _Ty, size_t size>
        constexpr size_t _Array_size(std::type_identity<std::array<_Ty, size> >)
        {
            return size;
        }
    }

    template <typename _Ty>
    constexpr size_t array_size()
    {
        return detail::_Array_size(std::type_identity<std::remove_cvref_t<_Ty> >{ });
    }
#endif

    template <typename _Ty>
    concept string_view_std = characters_range<_Ty> && std::same_as<std::basic_string_view<_RANGES range_value_t<_Ty> >, std::remove_cvref_t<_Ty> >;
    template <typename _Ty>
    concept string_std = characters_range<_Ty> && std::same_as<std::basic_string<_RANGES range_value_t<_Ty> >, std::remove_cvref_t<_Ty> >;

    namespace detail
    {
        struct _Strings_combine_tag { };

        template <character _Ty, size_t size>
        //last char must be '\0'
        struct _Strings_combine_result: std::array<_Ty, size>, _Strings_combine_tag
        {
            constexpr _Strings_combine_result( ) = default;

            using std_array_t = std::array<_Ty, size>;

            constexpr std_array_t&       _Unwrap( ) { return *this; }
            constexpr const std_array_t& _Unwrap( ) const { return *this; }

            constexpr void _Check_correctly_combined( ) const
            {
                _STL_ASSERT(this->back() == '\0', "Wrong string's end");
            }
        };

        template <typename _Ty>
        concept _Tagged_with_strings_combine = std::derived_from<std::remove_cvref_t<_Ty>, _Strings_combine_tag>;
    }

    class _To_string_view_convert_fn
    {
        template <typename _Ty_from>
        static constexpr auto _Find_type_fn( )
        {
#define TO_STRING_VIEW_CONVERT(chr)\
         constexpr(std::constructible_from<std::basic_string_view<chr>, _Ty_from>)\
            return std::basic_string_view<chr>( );

            if TO_STRING_VIEW_CONVERT(char)
#ifdef __cpp_char8_t
            else if TO_STRING_VIEW_CONVERT(char8_t)
#endif
            else if TO_STRING_VIEW_CONVERT(char16_t)
            else if TO_STRING_VIEW_CONVERT(char32_t)
            else if TO_STRING_VIEW_CONVERT(wchar_t)
            else
            {
                //static_assert(std::_Always_false<_Ty_from>, __FUNCSIG__": Unable to view type as string!");
            }

#undef TO_STRING_VIEW_CONVERT
        }

        template <typename _Ty_from>
        using _Find_type = decltype(_Find_type_fn<_Ty_from>( ));

    public:

        template <typename _Ty_from,
            std::constructible_from<_Ty_from> _Ty_to = _Find_type<_Ty_from>>
        constexpr auto operator()(const _Ty_from& val) const
        {
            return _Ty_to(val);
        }

        template <typename _Ty_from,
            std::constructible_from<_Ty_from, size_t> _Ty_to = _Find_type<_Ty_from>>
        constexpr auto operator()(const _Ty_from& val, size_t chars_count) const requires(std::is_pointer_v<_Ty_from>)
        {
            return _Ty_to(val, chars_count);
        }

        template <typename _Ty_from_rng, std::invocable<_Ty_from_rng> _Fn_unwrap = decltype(_RANGES data),
                  typename _Ty_from = std::invoke_result_t<_Fn_unwrap, _Ty_from_rng>,
                  std::constructible_from<_Ty_from, size_t> _Ty_to = _Find_type<_Ty_from>>
        constexpr auto operator()(_Ty_from_rng&& val, size_t chars_count) const
        {
            return _Ty_to(_RANGES data(val), chars_count);
        }
    };

    constexpr auto _To_string_view_convert = _To_string_view_convert_fn( );

    enum _To_string_view_mode
    {
        /**
         * \brief construct from string/string_view
         */
        native,
        /**
         * \brief {text, text.size - 1}
         */
        direct,
        /**
        * \brief from pointer
        */
        bruteforce,
        /**
         * \brief from range with known size
         */
        clamp_back,
        unknown,
    };

    template <_To_string_view_mode>
    class _To_string_view_impl;

    template < >
    class _To_string_view_impl<native>
    {
    public:

        template <character _Chr>
        constexpr auto operator()(const std::basic_string_view<_Chr>& text) const
        {
            return text;
        }

        template <character _Chr>
        auto operator()(const std::basic_string<_Chr>& text) const
        {
            return _To_string_view_convert(text);
        }

        template <character _Chr>
        std::basic_string_view<_Chr> operator()(std::basic_string<_Chr>&& text) const = delete;
    };

    template < >
    class _To_string_view_impl<direct>
    {
    public:

        template <characters_range _Ty>
        constexpr auto operator()(const _Ty& text) const
        {
            return _To_string_view_convert(text, _RANGES size(text) - 1);
        }
    };

    template < >
    class _To_string_view_impl<bruteforce>
    {
    public:

        template <character _Ty>
        constexpr auto operator()(const _Ty* text) const
        {
            return _To_string_view_convert(text);
        }
    };

    template < >
    class _To_string_view_impl<clamp_back>
    {
    public:

        template <characters_range _Ty>
        constexpr auto operator()(const _Ty& text) const -> std::basic_string_view<_RANGES range_value_t<_Ty> >
        {
            auto rng_begin = _RANGES rbegin(text);
            auto rng_end   = _RANGES rend(text);

            for (auto char_wrapped = rng_begin; char_wrapped != rng_end; ++char_wrapped)
            {
                if (*char_wrapped == '\0')
                    continue;

                auto start = rng_end.base( );
                auto end   = char_wrapped.base( );

                return {start, end};
            }

            return { };
        }
    };

    class _To_string_view_fn
    {
        template <typename _Ty>
        static constexpr _To_string_view_mode _Get_mode( )
        {
            if constexpr (detail::_Tagged_with_strings_combine<_Ty> || raw_array<_Ty>)
                return direct;
            else if constexpr (std::is_pointer_v<std::remove_cvref_t<_Ty> >)
                return bruteforce;
            else if constexpr (string_view_std<_Ty> || string_std<_Ty>)
                return native;
            else if constexpr (_RANGES range<_Ty>)
                return clamp_back;
            else
                return unknown;
        }

    public:

        template <typename _Ty_from, _To_string_view_mode _Mode, character _Chr>
        class extra_info: public std::basic_string_view<_Chr>
        {
            using _Sv_type = std::basic_string_view<_Chr>;

        public:
            using source_type = _Ty_from;
            using value_type = _Chr;

            static constexpr _To_string_view_mode mode = _Mode;

            constexpr extra_info(const _Sv_type& sv) : _Sv_type(sv) {}

            constexpr auto _Unwrap( ) const { return *static_cast<const _Sv_type*>(this); }
            auto           _Unwrap_copy( ) const { return std::basic_string<_Chr>(_Unwrap( )); }
        };

    private:

        struct _Tags
        {
            static constexpr auto nothing    = _Tag_number<0>( );
            static constexpr auto extra_info = _Tag_number<1>( );
        };

    public:

        static constexpr auto tags = _Tags( );

        template <typename _Ty_from, std::derived_from<_Tag_base> _Tag = _Tag_number<tags.nothing.index>,
                  _To_string_view_mode _Mode = _Get_mode<_Ty_from>( ),
                  std::default_initializable _Impl = _To_string_view_impl<_Mode>>
        constexpr auto operator()(_Ty_from&& text, _Tag tag = { }) const requires(std::invocable<_Impl, decltype(text)>)
        {
            constexpr auto impl = _Impl( );

            auto result = impl(text);

            if constexpr (tag == tags.nothing)
            {
                return result;
            }
            else if constexpr (tag == tags.extra_info)
            {
                using _Chr = typename decltype(result)::value_type;
                return extra_info<_Ty_from, _Mode, _Chr>(result);
            }
        }
    };

    inline constexpr auto to_string_view = _To_string_view_fn( );

    namespace detail
    {
        template <typename _Ty>
        using _Rvalue_ignored = std::conditional_t<std::is_rvalue_reference_v<_Ty> || !std::is_reference_v<_Ty>, std::add_lvalue_reference_t<_Ty>, _Ty>;
    }

    template <typename _Ty>
    concept string_viewable = std::invocable<_To_string_view_fn, _Ty>;

    template <typename _Ty_from, typename _Chr>
    concept string_viewable_exact = string_viewable<_Ty_from> && std::same_as<_Chr, typename std::invoke_result_t<_To_string_view_fn, _Ty_from>::value_type>;

    template <typename _Ty>
    concept string_viewable_no_rvalue = string_viewable<detail::_Rvalue_ignored<_Ty> >;

    template <typename _Ty_from, typename _Chr>
    concept string_viewable_exact_no_rvalue = string_viewable_exact<detail::_Rvalue_ignored<_Ty_from>, _Chr>;

    struct _To_string_fn
    {
        template <string_viewable_no_rvalue _Ty>
        auto operator()(_Ty&& val) const
        {
            if constexpr (string_std<_Ty>)
                return FWD(val);
            else
                return to_string_view(val, to_string_view.tags.extra_info)._Unwrap_copy( );
        }
    };

    struct _To_string_ex_fn: _To_string_fn
    {
        template <typename _Ty>
        auto operator()(_Ty val) const requires(std::is_arithmetic_v<_Ty>)
        {
            return std::to_string(val);
        }
    };

    inline constexpr auto to_string    = _To_string_fn( );
    inline constexpr auto to_string_ex = _To_string_ex_fn( );

    template <typename _Ty>
    struct string_type_t: std::type_identity<typename std::invoke_result_t<_To_string_view_fn, _Ty>::value_type> {};

    template <typename _Ty>
    using string_type = typename string_type_t<_Ty>::type;

    //template <typename _Ty>
    //using string_type = typename decltype(to_string_view(std::declval<_Ty>()))::value_type;

    namespace detail
    {
        class _Strings_combine_complite_time_fn
        {
            template <typename _Ty>
            static constexpr size_t _String_size_for_buffer( )
            {
                //constexpr auto size   = _Array_size_impl<_Ty>( );

                constexpr auto size   = sizeof(_Ty) / sizeof(_RANGES range_value_t<_Ty>);
                constexpr auto result = raw_array<_Ty> || _Tagged_with_strings_combine<_Ty> ? size - 1 : size;
                return result;
            }

            template <character _Chr, size_t _Buff_size, string_viewable_exact<_Chr> _Ty_txt, typename ..._Next_txt>
            static constexpr void _Add_string_to_buffer(size_t& offset, _Strings_combine_result<_Chr, _Buff_size>& buffer, const _Ty_txt& text, _Next_txt&& ...other)
            {
                constexpr auto str_length  = _String_size_for_buffer<_Ty_txt>( );
                const auto     temp_string = std::basic_string_view<_Chr>(std::data(text), str_length);

                temp_string.copy(buffer.data( ) + offset, str_length);
                offset += str_length;

                if constexpr (sizeof...(_Next_txt) > 0)
                    _Add_string_to_buffer(offset, buffer, other...);
            }

        public:
            template <typename ..._Txt_args>
            constexpr auto operator()(_Txt_args&&...args) const requires(
                (string_viewable<decltype(args)> && (known_array_size<_Txt_args> || _Tagged_with_strings_combine<_Txt_args>)) && ...)
            {
                using char_type = _RANGES range_value_t<first_element_t<_Txt_args...> >;

                constexpr auto buffer_size = (_String_size_for_buffer<_Txt_args>( ) + ...);

                size_t offset = 0;
                auto   buffer = _Strings_combine_result<char_type, buffer_size + 1>( );
                _Add_string_to_buffer(offset, buffer, FWD(args)...);

                return buffer;
            }
        };

        class _Strings_combine_runtime_fn
        {
            struct _Add_string_to_buffer_v1
            {
                template <character _Chr, string_viewable_exact_no_rvalue<_Chr> _Ty, typename ..._Ty_next>
                void operator()(std::basic_string<_Chr>& buffer, _Ty&& text, _Ty_next&& ...other) const
                {
                    if (buffer.empty( ))
                        buffer = to_string(FWD(text));
                    else
                        buffer += to_string_view(text);

                    if constexpr (sizeof...(_Ty_next) > 0)
                        (*this)(buffer, FWD(other)...);
                }
            };

            class _Add_string_to_buffer_v2 //reserve buffer size before
            {
                template <typename _Ty>
                static auto _Move_string_or_make_view(_Ty&& str)
                {
                    if constexpr (string_std<_Ty> && std::is_rvalue_reference_v<decltype(str)>)
                        return to_string(FWD(str));
                    else
                        return to_string_view(str);
                }

                template <tuple_only _Tpl, size_t ..._I>
                static size_t _All_strings_size(_Tpl&& tpl, std::index_sequence<_I...>)
                {
                    return ((std::get<_I>(tpl).size( )) + ...);
                }

                static constexpr auto _Add_helper = _Add_string_to_buffer_v1( );

                template <character _Chr, tuple_only _Tpl, size_t _Idx, size_t ..._Idx_next>
                static void _Add_string_to_buffer(std::basic_string<_Chr>& buffer, _Tpl&& tpl, [[maybe_unused]] std::index_sequence<_Idx, _Idx_next...> seq)
                {
                    _Add_helper(buffer, std::move(std::get<_Idx>(tpl)));

                    constexpr auto next_seq = std::index_sequence<_Idx_next...>( );
                    if constexpr (next_seq.size( ) > 0)
                        _Add_string_to_buffer(buffer, tpl, next_seq);
                }

            public:

                template <character _Chr, typename ..._Args>
                void operator()(std::basic_string<_Chr>& buffer, _Args&&...args) const requires(string_viewable_exact_no_rvalue<_Args, _Chr> && ...)
                {
                    static constexpr auto tpl_seq = std::make_index_sequence<sizeof...(_Args)>( );

                    auto       all_strings = std::make_tuple(_Move_string_or_make_view(FWD(args))...);
                    const auto buffer_size = _All_strings_size(all_strings, tpl_seq);
                    buffer.reserve(buffer_size);
                    _Add_string_to_buffer(buffer, all_strings, tpl_seq);
                }
            };

        public:

            template <string_viewable_no_rvalue _Ty, typename ..._Ty_next>
            auto operator()(_Ty&& text, _Ty_next&&...args) const requires(string_viewable_no_rvalue<_Ty_next> && ...)
            {
                using char_type = string_type<_Rvalue_ignored<_Ty> >;
                auto buffer = std::basic_string<char_type>( );

                static constexpr auto generator = _Add_string_to_buffer_v2( );

                generator(buffer, FWD(text), FWD(args)...);
                return std::move(buffer);
            }
        };
    }

    inline static constexpr auto combine_strings_complie_time = detail::_Strings_combine_complite_time_fn( );
    inline static constexpr auto combine_strings_runtime      = detail::_Strings_combine_runtime_fn( );

    namespace detail
    {
        struct _Strings_combine_fn //:public _Strings_combine_runtime_fn,public _Strings_combine_complite_time_fn
        {
#if 0
            using _Tag_ct = _Tag_number<0>;
            using _Tag_rt = _Tag_number<1>;

            struct _Tag
            {
                template <typename _Ty>
                struct select
                {
                    constexpr auto operator()() const
                    {
                        if constexpr ((known_array_size<_Ty> || _Tagged_with_strings_combine<_Ty>) && string_viewable<_Ty>)
                            return _Tag_ct();
                        else if constexpr (string_viewable_no_rvalue<_Ty>)
                            return _Tag_rt();
                        else
                            return _Tag_unknown();
                    }
                };
            };

        public:


            template <typename ..._Args>
            constexpr auto operator()(_Args&&...args) const requires(
                _Tag_check<decltype(args), _Tag::select, _Tag_ct> && ...)
            {
                return combine_strings_complie_time(args...);
            }

            template <typename ..._Args>
            auto operator()(_Args&&...args) const requires(
                _Tag_check<decltype(args), _Tag::select, _Tag_rt> || ...)
            {
                return combine_strings_runtime(FWD(args)...);
            }
#endif
            template <typename ..._Args>
            constexpr auto operator()(_Args&&...args) const requires(std::invocable<
                _Strings_combine_complite_time_fn, decltype(args)> && ...)
            {
                return combine_strings_complie_time(args...);
            }

            template <typename ..._Args> requires(std::invocable<
                _Strings_combine_runtime_fn, _Args> && ...)
            auto operator()(_Args&&...args) const requires(!std::invocable<
                _Strings_combine_complite_time_fn, decltype(args)> || ...)
            {
                return combine_strings_runtime(FWD(args)...);
            }
        };
    }

    inline static constexpr auto combine_strings = detail::_Strings_combine_fn( );

    template <typename _Ty>
    struct type_name
    {
    private:
        static constexpr auto _Name( )
        {
            std::basic_string_view raw_name = to_string_view(__FUNCTION__);

            raw_name.remove_prefix(raw_name.find('<') + 1);
            raw_name = {raw_name.data( ), raw_name.find('>')};

            const auto space = raw_name.find(' ');
            if (space != raw_name.npos)
                raw_name.remove_prefix(space + 1);

            return raw_name;
        }

    public:
        static constexpr auto name = _Name( );
    };

    template <typename _Ty, string_viewable_exact_no_rvalue<char> _Chr>
    decltype(auto) _Nullptr_throw(_Ty&& ptr, _Chr&& msg)
    {
        if (ptr == nullptr)
            throw std::runtime_error(to_string(FWD(msg)));
        return FWD(ptr);
    }

    template <typename _Ty, string_viewable_exact_no_rvalue<char> _Chr = std::string_view>
    decltype(auto) _Nullptr_assert(_Ty&& ptr, _Chr&& msg = to_string_view("Null pointer detected!"))
    {
#ifdef _DEBUG
        if (ptr == nullptr)
        {
            const auto msg_fixed = to_string_view(msg);
            if (msg_fixed.empty( ) || *msg_fixed._Unchecked_end( ) != '\0')
                _RPTF0(_CRT_ASSERT, "DEFAULT NULLPTR ASSERT (wrong message data)");
            else
                _RPTF0(_CRT_ASSERT, msg_fixed.data());
        }
        return FWD(ptr);
#else
        return _Nullptr_throw(FWD(ptr), FWD(msg));
#endif
    }
}
