#pragma once

#include "hash core.h"

namespace utl::hash
{
    namespace detail
    {
        template <_Integral_exclude_bool _Ty>
        class _Fnv1a_updater
        {
        public:
            constexpr _Fnv1a_updater(_Ty offset_basis, _Ty prime) : offset_basis_(offset_basis), prime_(prime) {}

            constexpr void operator()(_Ty value, _Ty& hash) const
            {
                hash ^= value;
                hash *= prime_;
            }

            constexpr const _Ty& offset_basis() const { return offset_basis_; }
            constexpr const _Ty& prime() const { return prime_; }

        private:
            _Ty offset_basis_;
            _Ty prime_;
        };

        template <_Integral_exclude_bool _Ty>
        class _Fnv1a_fn
        {
        public:

            using processer_type = _Hash_processer<_Ty, _Ty, _Fnv1a_updater<_Ty> >;

            constexpr _Fnv1a_fn(_Ty offset_basis, _Ty prime) : processer_({ offset_basis, prime }) {}

            template <typename ..._Args>
            constexpr _Ty operator()(_Args&&...values) const
            {
                _Hash_init_value<_Ty> init = processer_.get_updater().offset_basis();
                return std::invoke(processer_, FWD(values)..., init).value;
            }

        private:
            processer_type processer_;
        };
    }

    static constexpr auto fnv1a_32 = detail::_Fnv1a_fn(0x811c9dc5u, 0x01000193u);
    static constexpr auto fnv1a_64 = detail::_Fnv1a_fn(0xcbf29ce484222325u, 0x00000100000001B3u);

#ifdef UTILS_X32
    static constexpr auto fnv1a = fnv1a_32;
#else
    static constexpr auto fnv1a = fnv1a_64;
#endif

    namespace literals
    {
        //todo: consteval this
        //

#define FNV_LITERALS(type)\
        constexpr auto operator ""_fnv32(const type* c, size_t size) { return fnv1a_32(get_data(c, size)); }\
        constexpr auto operator ""_fnv64(const type* c, size_t size) { return fnv1a_64(get_data(c, size)); }\
        constexpr auto operator ""_fnv(const type* c, size_t size) { return fnv1a(get_data(c, size)); }

        FNV_LITERALS(char);
#ifdef __cpp_char8_t
        FNV_LITERALS(char8_t);
#endif
        FNV_LITERALS(char16_t);
        FNV_LITERALS(char32_t);
        FNV_LITERALS(wchar_t);

#undef FNV_LITERALS
    }
}
