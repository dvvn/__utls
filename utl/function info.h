#pragma once

#include "utils core.h"

#include <functional>

namespace utl
{
    template <typename _Ty>
    concept invocable = std::is_function_v<_Ty> || std::is_member_function_pointer_v<_Ty> || requires(_Ty&& val)
    {
        { std::function(FWD(val)) }->std::default_initializable;
    };

    namespace detail
    {
        enum call_cvs
        {
#ifdef UTILS_X32
            thiscall_,
            cdecl_,
            stdcall_,
            vectorcall_,
            fastcall_,
#else
			UNUSED
#endif
        };

        //todo: rewrite this using std::_Is_memfunptr

        template <typename ..._Args>
        struct function_args
        {
            using packed = std::tuple<_Args...>;
            static constexpr size_t count = sizeof...(_Args);

            template <size_t idx>
            using unpack = std::tuple_element_t<idx, packed>;

            //--

            template <typename ...T>
            static constexpr bool _Exact_to( ) { return (sizeof...(T) == count) && (std::same_as<std::tuple<T...>, packed>); }
        };

        template <class Type, bool Is_const>
        struct function_class
        {
            using type = Type;
            static constexpr bool member   = std::is_class_v<Type>;
            static constexpr bool is_const = Is_const;
        };

        template <typename Ret, call_cvs call, typename Class, bool Is_const, typename ...Args>
        struct function_info
        {
            using return_type = Ret;
            static constexpr call_cvs call_type = call;
            using class_info = function_class<Class, Is_const>;

            using args = function_args<Args...>;
            //removed, because different call conversions have different args. also too much constexpr shit destroy compiler and fuck ur pc
            //using invocable_args = std::conditional_t<class_info::member, function_args<Class*, Args...>, args>;
        };

#ifdef UTILS_X64
		template <typename Ret, typename Cl, typename ...Args>
		constexpr auto get_function_info_impl(Ret ( Cl::*)(Args ...) const) { return function_info<Ret, UNUSED, Cl, true, Args...>{ }; }
		template <typename Ret, typename Cl, typename ...Args>
		constexpr auto get_function_info_impl(Ret ( Cl::*)(Args ...)) { return function_info<Ret, UNUSED, Cl, false, Args...>{ }; }
		template <typename Ret, typename ...Args>
		constexpr auto get_function_info_impl(Ret ( *)(Args ...)) { return function_info<Ret, UNUSED, void, false, Args...>{ }; }
#else

#define FUNCTION_INFO_IMPL(call_type)\
		template <typename Ret, typename Cl, typename ...Args>\
		constexpr auto get_function_info_impl(Ret (__##call_type Cl::*)(Args ...) const) { return function_info<Ret, call_type##_, Cl, true, Args...>{ }; }\
		template <typename Ret, typename Cl, typename ...Args>\
		constexpr auto get_function_info_impl(Ret (__##call_type Cl::*)(Args ...)) { return function_info<Ret, call_type##_, Cl, false, Args...>{ }; }\
		template <typename Ret, typename ...Args>\
		constexpr auto get_function_info_impl(Ret (__##call_type *)(Args ...)) { return function_info<Ret, call_type##_, void, false, Args...>{ }; }

        FUNCTION_INFO_IMPL(thiscall);
        FUNCTION_INFO_IMPL(cdecl);
        FUNCTION_INFO_IMPL(stdcall);
        FUNCTION_INFO_IMPL(vectorcall);
        FUNCTION_INFO_IMPL(fastcall);

#undef FUNCTION_INFO_IMPL
#endif

        template <typename _Fn_obj>
        constexpr auto get_function_info(_Fn_obj&& raw_fn)
        {
            
            if constexpr (!std::is_class_v<_Fn_obj>)
                return detail::get_function_info_impl(raw_fn);
            else
            {
                //using real_fn = decltype(&T::operator());
                using real_fn = decltype(&std::remove_cvref_t<_Fn_obj>::operator());
                return detail::get_function_info_impl(std::declval<real_fn>( ));
            }
        }
    }

    template <typename T>
    using function_info_t = decltype(detail::get_function_info(std::declval<std::remove_cvref_t<T> >( )));

    template <typename _Fn_obj>
    using function_return_t = typename function_info_t<_Fn_obj>::return_type;

    template <typename _Fn_obj>
    using function_args_t = typename function_info_t<_Fn_obj>::args::packed;

    template <typename _Fn_obj, size_t _Arg_idx>
    using function_arg_t = typename function_info_t<_Fn_obj>::template args::template unpack<_Arg_idx>;

    namespace detail
    {
        template <invocable _Fn, typename Ret, typename ...Args>
        constexpr bool _Check_function_raw_fn( )
        {
            using info = function_info_t<_Fn>;
            constexpr bool same_ret  = std::same_as<typename info::return_type, Ret>;
            constexpr bool same_args = info::args::template _Exact_to<Args...>( );
            return same_ret && same_args;
        }

        template <invocable Fn, invocable Fn2, bool Cvs, bool Class, bool Const>
        constexpr bool _Check_function_fn( )
        {
            using info = function_info_t<Fn>;
            using info2 = function_info_t<Fn2>;

            constexpr bool same_ret  = std::same_as<typename info::return_type, typename info2::return_type>;
            constexpr bool same_args = std::same_as<typename info::args::packed, typename info2::args::packed>;

            if constexpr (!same_ret || !same_args)
                return false;
            else
            {
                constexpr bool same_cvs   = info::call_type == info2::call_type;
                constexpr bool same_const = info::class_info::is_const == info2::class_info::is_const;
                constexpr bool same_class = std::same_as<typename info::class_info::type, typename info2::class_info::type>;

                constexpr bool cvs_result   = !Cvs || same_cvs;
                constexpr bool const_result = !Const || same_const;
                constexpr bool class_result = !Class || same_class;

                constexpr bool static_funcs = !info::class_info::member && !info2::class_info::member;
                if constexpr (static_funcs)
                {
                    return cvs_result;
                }
                else
                {
                    constexpr bool result = cvs_result && const_result && class_result;
                    return result;
                }
            }
        }
    }

    template <typename T, typename Ret, typename ...Args>
    concept check_function_raw = detail::_Check_function_raw_fn<T, Ret, Args...>( );
    template <typename Fn, typename Fn2>
    concept check_function = detail::_Check_function_fn<Fn, Fn2, false, false, false>( );
    template <typename Fn, typename Fn2, bool Cvs, bool Class, bool Const>
    concept check_function_ex = detail::_Check_function_fn<Fn, Fn2, Cvs, Class, Const>( );
}
