#pragma once

#include "utils core.h"

#ifdef _DEBUG
#include <stdexcept>
#endif
namespace utl
{
    /*template <typename _Ty, typename ...Ts>
    concept all_same = sizeof...(Ts) > 0 && (std::same_as<_Ty, Ts> && ...);

    template <typename ..._Ty>
    concept all_different = sizeof...(_Ty) <= 1 || !all_same<_Ty...>;*/

    template <typename A, typename B = A>
    concept bitflag_native_support = requires(A a, B b)
    {
        { ~a }->std::_Boolean_testable;
        { ~b }->std::_Boolean_testable;
        { a & b }->std::_Boolean_testable;
        { a ^ b }->std::_Boolean_testable;
        { a | b }->std::_Boolean_testable;
    };

    /*template <typename A, typename B = A>
    concept bits_shift_supported = requires(A&& a, B&& b)
    {
        a >> b << a;
    };*/

    namespace detail
    {
        template <typename Tested, typename Current, typename ..._Ty_next>
        constexpr auto _Size_selector( )
        {
            if constexpr (sizeof(Tested) == sizeof(Current))
                return std::type_identity<Current>{ };
            else if constexpr (sizeof...(_Ty_next) > 0)
                return _Size_selector<Tested, _Ty_next...>( );
        }

        template <typename _Ty>
        using signed_selector = decltype(_Size_selector<_Ty, std::int8_t, std::int16_t, std::int32_t, std::int64_t>( ));
        template <typename _Ty>
        using unsigned_selector = decltype(_Size_selector<_Ty, std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t>( ));

        template <typename _Ty>
        constexpr auto _Raw_bitflag_type( )
        {
            if constexpr (std::is_enum_v<_Ty>)
                return std::type_identity<std::underlying_type_t<_Ty> >{ };
            else
                return std::type_identity<_Ty>{ };
        }

        template <typename _Ty>
        constexpr bool _Is_bitflag_constructible( )
        {
            if constexpr (std::is_enum_v<_Ty>)
                return true;
            else if constexpr (std::integral<_Ty>)
                return !std::same_as<_Ty, bool>;
            else
                return bitflag_native_support<_Ty>;
        }

        template <typename _Ty>
        concept bitflag_constructible = detail::_Is_bitflag_constructible<_Ty>( );
    }

    template <detail::bitflag_constructible _Ty>
    class bitflag
    {
    public:

        using value_type = _Ty;
        using raw_type = typename decltype(detail::_Raw_bitflag_type<_Ty>( ))::type;

    private:

        template <typename _Ty_other>
        static constexpr raw_type _To_raw(const _Ty_other& val)
        {
            if constexpr (std::same_as<_Ty_other, bitflag>)
                return val.get_raw( );
            else
                return static_cast<raw_type>(val);
        }

        template <typename ..._Ty_others>
        static constexpr raw_type _Combine(const _Ty_others& ...args)
        {
            return (_To_raw(args) | ...);
        }

        template <typename ..._Ty_others>
        constexpr void _Set(const _Ty_others& ...args)
        {
            raw_type temp = _Combine(args...);
            flags_        = static_cast<value_type>(temp);
        }

        _Ty flags_ = { };

    public:

        constexpr value_type get( ) const
        {
            return flags_;
        }

        constexpr raw_type get_raw( ) const
        {
            return _To_raw(flags_);
        }

        static constexpr bool native = bitflag_native_support<value_type>;

        constexpr auto convert( ) const
        {
            if constexpr (!native)
                return get_raw( );
            else
                return get( );
        }

        /* constexpr bitflag( )
             : flags_((_Ty)0) {}

         constexpr bitflag(_Ty&& val)
             : flags_(FWD(val)) {}*/

        ALLOW_COPY(bitflag);

        constexpr bitflag(const _Ty& val)
        {
            _Set(val);
        }

        template <typename ..._Ty_others>
        constexpr bitflag(const _Ty_others& ...args) requires((std::convertible_to<_Ty_others, value_type> || std::convertible_to<_Ty_others, raw_type>) && ...)
        {
            constexpr auto args_count = sizeof...(_Ty_others);
            if constexpr (args_count == 0)
                _Set(0);
            else
                _Set(args...);
        }

        /*template <typename ...Args>
        requires (_Safe_convertible<Args...>( ))
        constexpr bitflag combine(Args ...a) const
        {
            return bitflag{flags_, a...};
        }*/

    private:

        template <typename _Ty_other>
        static constexpr void _Validate_shift(const _Ty_other& value)
        {
#ifdef _DEBUG
            const auto value2 = static_cast<raw_type>(value);
            if (value != value2)
                throw std::logic_error("incorrect shift value!");
#endif
        }

    public:

        constexpr bitflag shift_rignt(const raw_type& val) const
        {
            const auto value = this->convert( ) >> val;
            _Validate_shift(value);
            return value;
        }

        constexpr bitflag shift_left(const raw_type& val) const
        {
            const auto value = this->convert( ) << val;
            _Validate_shift(value);
            return value;
        }

        template <typename ..._Ty_others>
        constexpr bool has(const _Ty_others& ...args) const
        {
            auto raw = this->get_raw( );

            return ((raw & _To_raw(args)) || ...);
        }

        template <typename ..._Ty_others>
        constexpr bool has_all(const _Ty_others& ...args) const
        {
            auto raw   = this->get_raw( );
            auto flags = _Combine(args...);

            return raw & flags;
        }

        template <typename ..._Ty_others>
        constexpr bitflag& add(const _Ty_others& ...args)
        {
            _Set(this->flags_, args...);
            return *this;
        }

        template <typename ..._Ty_others>
        constexpr bitflag& set(const _Ty_others& ...args)
        {
            *this = { };
            _Set(args...);
            return *this;
        }

        template <typename ..._Ty_others>
        constexpr bitflag& remove(const _Ty_others& ...args)
        {
            auto raw   = this->get_raw( );
            auto flags = _Combine(FWD(args)...);

            auto result = raw & ~flags;
            _Set(result);

            return *this;
        }

        constexpr bool operator ==(const bitflag& other) const = default;
    };

    namespace detail
    {
        template <typename _Ty, typename ..._Ty_next>
        constexpr auto _Get_first_enum( )
        {
            if constexpr (std::is_enum_v<_Ty>)
                return std::type_identity<_Ty>{ };
            else if constexpr (sizeof...(_Ty_next) == 0)
                return std::type_identity<void>{ };
            else
                return _Get_first_enum<_Ty_next...>( );
        }

        template <typename _Ty1, typename _Ty2, typename ..._Ty_next>
        constexpr auto _Biggest_type_selector( )
        {
            using type = std::conditional_t<sizeof(_Ty1) < sizeof(_Ty2), _Ty2, _Ty1>;
            if constexpr (sizeof...(_Ty_next) == 0)
                return std::type_identity<type>{ };
            else
                return _Biggest_type_selector<type, _Ty_next...>( );
        }

        template <typename _Ty, typename ..._Ty_next>
        constexpr bool _Contains_different_enums( )
        {
            if constexpr (std::is_enum_v<_Ty>)
                return ((std::is_enum_v<_Ty_next> && !std::same_as<_Ty, _Ty_next>) || ...);
            else if constexpr (sizeof...(_Ty_next) == 0)
                return false;
            else
                return _Contains_different_enums<_Ty_next...>( );
        }
    }

    namespace detail
    {
        template <typename _Ty = void, typename ..._Ty_others>
        constexpr bool _Is_all_same( ) requires(sizeof...(_Ty_others) > 0)
        {
            return (std::same_as<_Ty, _Ty_others> && ...);
        }
    }

    template <typename ..._Ty>
    constexpr auto make_bitflag(const _Ty& ...args) requires(std::same_as<_Ty, std::remove_cvref_t<_Ty> > && ...)
    {
        using namespace detail;

        if constexpr (sizeof...(_Ty) == 1)
        {
            return bitflag<first_element_t<_Ty...> >(args...);
        }
        else if constexpr (_Is_all_same<_Ty...>( ))
        {
            //select enum type or raw type

            using type = first_element_t<_Ty...>;
            if constexpr (_Is_bitflag_constructible<type>( ))
                return bitflag<type>(args...);
            else
                static_assert(std::_Always_false<type>, "Unsupported bitflag type");
        }
        else if constexpr (_Contains_different_enums<_Ty...>( ))
        {
            static_assert(std::_Always_false<_Ty>, "Two or more different enums same time");
        }
        else
        {
            //select enum type or generate raw type

            using enum_t = typename decltype(_Get_first_enum<_Ty...>( ))::type;
            if constexpr (!std::is_void_v<enum_t>)
            {
                return bitflag<enum_t>(args...);
            }
            else //enum not found
            {
                using biggest_type = typename decltype(_Biggest_type_selector<_Ty...>( ))::type;
                constexpr bool prefer_unsigned = (std::is_unsigned_v<_Ty> || ...);
                using selector = std::conditional_t<prefer_unsigned, unsigned_selector<biggest_type>, signed_selector<biggest_type> >;
                using selected_raw_type = typename selector::type;
                return bitflag<selected_raw_type>(args...);
            }
        }
    }
}
