#pragma once

//fixed by visual studio
#if 0
#if __cplusplus > 201703L

#ifndef __cpp_concepts
#define __cpp_concepts 201907
#pragma (message, __cpp_concepts manually defined )
#endif

#endif
#endif

#include <cassert>
#include <concepts>
#include <filesystem>
#include <string_view>
#include <ranges>
#include <stdexcept>

#if defined(_MSVC_TRADITIONAL) && _MSVC_TRADITIONAL == 1

#error "Old preprocessor detected"

#else

#if defined(_M_IX86) || defined(__i386__)
#define UTILS_X32
#define UTILS_X86 UTILS_X32
#elif defined(_M_X64) || defined(__x86_64__)
#define UTILS_X64
#else
#error Unkonown system
#endif

#define FORCE_CONSTEXPR(x) ([]{constexpr auto val = x; return val;}())

//#define ARGS_TO_STR1(...)#__VA_ARGS__
//#define ARGS_TO_STR(...) ARGS_TO_STR1(__VA_ARGS__)
///use _STRINGIZE from std

//#define MACRO_ARGS_CONCAT1(a, b) a##b
//#define MACRO_ARGS_CONCAT(a, b) MACRO_ARGS_CONCAT1(a, b)
///use _CONCAT from std

#define DUMMY_VAR _CONCAT(dummy,__LINE__)
#define DUMMY_VAR_EX _CONCAT(dummy,__COUNTER__)

#define MAKE_DUMMY_VAR_EX(type, ...) [[maybe_unused]] const type DUMMY_VAR = __VA_ARGS__;(const void)(DUMMY_VAR);
#define MAKE_DUMMY_VAR(...) MAKE_DUMMY_VAR_EX(auto,__VA_ARGS__)

#define CALL_ONCE(...) MAKE_DUMMY_VAR_EX(static auto, [&]{ __VA_ARGS__ return true; }() );

#define FWD(obj) _STD forward<decltype(obj)>(obj)

#define PREVENT_COPY(x)\
	constexpr x(x&)       = delete;\
	constexpr x(const x&) = delete;\
	constexpr x& operator =(x&)       = delete;\
	constexpr x& operator =(const x&) = delete;

#define ALLOW_COPY(x)\
	constexpr x(const x&) = default;\
	constexpr x& operator =(const x&) = default;

#define PREVENT_MOVE(x)\
	constexpr x(x&&)      = delete;\
	constexpr x& operator =(x&&) = delete;

#define FORCE_MOVE(x)\
	constexpr x(x&&)      = default;\
	constexpr x& operator =(x&&) = default;

#define PREVENT_COPY_AND_MOVE(x) PREVENT_COPY(x) PREVENT_MOVE(x)
#define PREVENT_COPY_FORCE_MOVE(x) PREVENT_COPY(x) FORCE_MOVE(x)
#define ALLOW_COPY_AND_MOVE(x) ALLOW_COPY(x) FORCE_MOVE(x)

#define MOVE(x)\
	x{std::move(other.x)}

#define MOVE_ASSIGN(x)\
	this->x = std::move(other.x)

#define COPY(x)\
	x{(other.x)}

#define COPY_ASSIGN(x)\
	this->x = other.x

#if 1

#define PP_THIRD_ARG(a,b,c,...) c
#define VA_OPT_SUPPORTED_I(...) PP_THIRD_ARG(__VA_OPT__(,),true,false,)
#define VA_OPT_SUPPORTED VA_OPT_SUPPORTED_I(?)

#if VA_OPT_SUPPORTED

#define MACRO_GET_ARGS(...)\
	MACRO_ARGS_CONCAT(MACRO_ARGS_, PP_NARG(__VA_ARGS__))(__VA_ARGS__)

#elif 0

#define MACRO_ARGS_CONCAT1(a, b) a##b
#define MACRO_ARGS_CONCAT(a, b) MACRO_ARGS_CONCAT1(a, b)

#define PP_NARG(...) \
         PP_NARG_(__VA_ARGS__,PP_RSEQ_N())
#define PP_NARG_(...) \
         PP_ARG_N(__VA_ARGS__)
#define PP_ARG_N( \
	_1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
	_11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
	_21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
	_31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
	_41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
	_51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
	_61,_62,_63,N,...) N
#define PP_RSEQ_N() \
         63,62,61,60,                   \
         59,58,57,56,55,54,53,52,51,50, \
         49,48,47,46,45,44,43,42,41,40, \
         39,38,37,36,35,34,33,32,31,30, \
         29,28,27,26,25,24,23,22,21,20, \
         19,18,17,16,15,14,13,12,11,10, \
         9,8,7,6,5,4,3,2,1,0

#define MACRO_ARGS_1(a, s1) MACRO_ARGS_CONCAT(s1, a)
#define MACRO_ARGS_2(a, s1, s2) MACRO_ARGS_1(a, s1) , MACRO_ARGS_1(a, s2)

//#define MACRO_ARGS_X(a, s1, ...)\
//	MACRO_ARGS_1(a, s1) , MACRO_ARGS_CONCAT(MACRO_ARGS_, PP_NARG(__VA_ARGS__))(a, __VA_ARGS__)

#define MACRO_ARGS_3(a, s1, ...) MACRO_ARGS_1(a, s1) , MACRO_ARGS_2(a, __VA_ARGS__)
#define MACRO_ARGS_4(a, s1, ...) MACRO_ARGS_1(a, s1) , MACRO_ARGS_3(a, __VA_ARGS__)
#define MACRO_ARGS_5(a, s1, ...) MACRO_ARGS_1(a, s1) , MACRO_ARGS_4(a, __VA_ARGS__)
#define MACRO_ARGS_6(a, s1, ...) MACRO_ARGS_1(a, s1) , MACRO_ARGS_5(a, __VA_ARGS__)
#define MACRO_ARGS_7(a, s1, ...) MACRO_ARGS_1(a, s1) , MACRO_ARGS_6(a, __VA_ARGS__)
#define MACRO_ARGS_8(a, s1, ...) MACRO_ARGS_1(a, s1) , MACRO_ARGS_7(a, __VA_ARGS__)

#define MACRO_GET_ARGS(a, ...)\
	MACRO_ARGS_CONCAT(MACRO_ARGS_, PP_NARG(__VA_ARGS__))(a, __VA_ARGS__)

#endif
#endif

#endif

namespace utl
{
    template <typename _Ty = void, typename ..._Other>
    using first_element = std::type_identity<_Ty>;
    template <typename _Ty = void, typename ..._Other>
    using first_element_t = _Ty;

    template <typename ..._Args>
    struct last_element
    {
        using type = typename decltype((std::type_identity<_Args>{ }, ...))::type;
    };

    //---------

    class movable_class
    {
    public:
        constexpr movable_class( ) = default;
        PREVENT_COPY(movable_class)

        constexpr movable_class(movable_class&& other) noexcept
        {
            this->operator=(std::move(other));
        }

        constexpr movable_class& operator=(movable_class&& other) noexcept
        {
            _STL_ASSERT(!this->moved_, "this already moved.");
            _STL_ASSERT(!other.moved_, "other already moved.");

            other.moved_ = true;

            return *this;
        }

        void mark_moved( )
        {
            this->moved_ = true;
        }

    protected:
        [[nodiscard]]
        constexpr bool moved( ) const
        {
            return moved_;
        }

    private:
        bool moved_ = false;
    };

    class copyable_class
    {
    public:
        constexpr copyable_class( ) = default;
        PREVENT_MOVE(copyable_class)

        constexpr copyable_class(const copyable_class& other) noexcept
        {
            this->operator=(other);
        }

        constexpr copyable_class& operator=(const copyable_class&) noexcept
        {
            copied_ = true;
            return *this;
        }

        void mark_copied( )
        {
            this->copied_ = true;
        }

    protected:
        [[nodiscard]]
        constexpr bool copied( ) const
        {
            return copied_;
        }

    private:
        bool copied_ = false;
    };

    class copyable_movable_class: public copyable_class, public movable_class
    {
    public:
        constexpr copyable_movable_class( ) = default;

        constexpr copyable_movable_class(const copyable_movable_class& other) : copyable_class(other) { }
        constexpr copyable_movable_class(copyable_movable_class&& other) noexcept : movable_class(std::move(other)) { }

        constexpr copyable_movable_class& operator=(const copyable_movable_class& other)
        {
            copyable_class::operator=(other);
            return *this;
        }

        constexpr copyable_movable_class& operator=(copyable_movable_class&& other) noexcept
        {
            movable_class::operator=(std::move(other));
            return *this;
        }
    };

    template <typename _Ty>
    concept simple_type = std::is_bounded_array_v<_Ty> || !std::is_pointer_v<_Ty> && std::is_trivially_destructible_v<_Ty>;

    template <typename _Ty>
    concept byte_only = sizeof(_Ty) == sizeof(std::byte) && std::integral<_Ty>;

    template <typename _Ty>
    concept tuple_only = std::is_base_of_v<std::tuple<>, std::remove_cvref_t<_Ty> >;

    template <typename _Ty, size_t _Idx>
    concept _Get_possible = requires(_Ty&& val)
    {
        { std::get<_Idx>(FWD(val)) }->std::_Can_reference;
    };

    template <typename _Ty, typename _Ty_result, size_t _Idx>
    concept _Get_possible_to = requires(_Ty&& val)
    {
        { std::get<_Idx>(FWD(val)) }->std::convertible_to<_Ty_result>;
    };

    //-----

    //todo: move next to separate file

    //template <typename _Ty>
    //concept _Path_constructible = std::constructible_from<std::filesystem::path, _Ty>;

    /*template <_Path_constructinle _Ty>
    void _Assign_dir_entry(std::filesystem::directory_entry& e, _Ty&& path_from)
    {
        auto& path_to = const_cast<std::filesystem::path&>(e.path( ));
        path_to       = FWD(path_from);
        e.refresh( );
    }*/

    class _Unpack_path_fn
    {
        template <class _Ty>
        static std::wstring _Impl(_Ty&& str)
        {
            // ReSharper disable once CppLocalVariableMayBeConst
            auto   p        = std::filesystem::path(FWD(str));
            auto&& unpacked = const_cast<std::wstring&&>(p.native( ));
            return unpacked;
        }

    public:

        template <typename ..._Args>
        auto operator()(_Args&& ...str) const requires(std::constructible_from<std::filesystem::path, decltype(str)> && ...)
        {
            constexpr auto args_count = sizeof...(_Args);
            if constexpr (args_count == 1)
            {
                return _Impl(FWD(str)...);
            }
            else
            {
                return std::array<std::wstring, args_count>{_Impl(FWD(str))...};
            }
        }
    };
    inline static constexpr auto _Unpack_path = _Unpack_path_fn( );

    //tag system added to force select overloaded function, when normally same concept give use multiple overloads

    struct _Tag_base {};

    template <size_t _Idx>
    struct _Tag_number: _Tag_base
    {
        static constexpr auto index = _Idx;

        template <size_t _Idx_other>
        constexpr bool operator==(_Tag_number<_Idx_other>) const { return _Idx == _Idx_other; }

        template <size_t _Idx_other>
        constexpr bool operator!=(_Tag_number<_Idx_other>) const { return _Idx != _Idx_other; }
    };

    using _Tag_unknown = _Tag_number<-1>;

    /*template <typename _Ty_checked, template<typename _Ty> typename _Tag_getter, typename _Tag>
    concept _Tag_check = requires(_Tag_getter<_Ty_checked> getter)
    {
        { getter( ) }->std::same_as<_Tag>;
    };*/

    template <typename _Ty_checked, template<typename _Ty> typename _Tag_getter, typename _Tag>
    concept _Tag_check = std::invoke_result_t<_Tag_getter<_Ty_checked> >( ) == _Tag( );
}
