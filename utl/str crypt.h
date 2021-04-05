#pragma once
#include "xor shift.h"
#include "string view ex.h"

#include "function info.h"

//#define STR_CRYPTOR_MUTABLE

#if __cpp_nontype_template_args >= 201911
#pragma message ("__cpp_nontype_template_args updated")//https://en.cppreference.com/w/cpp/language/user_literal
#endif

namespace utl
{
    namespace detail
    {
#pragma pack(push, 1)

        template <character _Chr, size_t _Str_length, size_t _Pad_before, size_t _Pad_after>
        class _Str_cryptor_cache: protected std::array<_Chr, _Pad_before + _Str_length + _Pad_after + 1>
        {
        protected:

            constexpr void setup(const _Chr (&str)[_Str_length])
            {
                _RANGES copy(str, this->get_text( ).data( ));
                this->back( ) = '\0'; //fake null-terminated string

                uint64_t   state{__time__.seconds.count( ) + 1};
                const auto crypt_dummy = [&](_Chr& c)
                {
                    c = xor_shift::xorshift::xorshift64(state);
                };
                _RANGES for_each(this->get_pad_before( ), crypt_dummy);
                _RANGES for_each(this->get_pad_after( ), crypt_dummy);
            }

        private:

            //dont abuse std::ranges. they fuck ur pc

            template <typename _Ty>
            static constexpr auto _Get_text_impl(_Ty&& cache)
            {
                //auto dropped = std::views::drop(cache, Pad_before);
                //auto taken   = std::views::take(dropped, Str_length);
                //return taken;

                auto data = cache->data( ) + _Pad_before;
                return std::span(data, _Str_length);
            }

            template <typename _Ty>
            static constexpr auto _Get_pad_before_impl(_Ty&& cache)
            {
                //auto taken = std::views::take(cache, Pad_before);
                //return taken;

                auto data = cache->data( );
                return std::span(data, _Pad_before);
            }

            template <typename _Ty>
            static constexpr auto _Get_pad_after_impl(_Ty&& cache)
            {
                //return cache_ | std::views::drop(Pad_before + Str_length) | std::views::take(Pad_after);

                //auto dropped = std::views::drop(cache, Pad_before + Str_length);
                //auto taken   = std::views::take(dropped, Pad_after);
                //return taken;

                auto data = cache->data( ) + (_Pad_before + _Str_length);
                return std::span(data, _Pad_after);
            }

            template <typename _Ty>
            static constexpr auto _Get_whole_cache_impl(_Ty&& cache)
            {
                //auto taken = std::views::take(cache, Pad_before + Str_length + _pad_size2);
                //return taken;

                auto data = cache->data( );
                return std::span(data, _Pad_before + _Str_length + _Pad_after);
            }

        protected:
            constexpr _NODISCARD auto get_text( ) { return _Get_text_impl(this); }
        public:
            constexpr _NODISCARD auto get_text( ) const { return _Get_text_impl(this); }

        protected:
            constexpr _NODISCARD auto get_pad_before( ) { return _Get_pad_before_impl(this); }
        public:
            constexpr _NODISCARD auto get_pad_before( ) const { return _Get_pad_before_impl(this); }

        protected:
            constexpr _NODISCARD auto get_pad_after( ) { return _Get_pad_after_impl(this); }
        public:
            constexpr _NODISCARD auto get_pad_after( ) const { return _Get_pad_after_impl(this); }

        protected:
            constexpr _NODISCARD auto get_whole_cache( ) { return _Get_whole_cache_impl(this); }
        public:
            constexpr _NODISCARD auto get_whole_cache( ) const { return _Get_whole_cache_impl(this); }
        };

        constexpr size_t _Str_crypt_pad_size(size_t char_size, size_t magic, size_t limit)
        {
#ifdef UTILS_X32
            const auto shift_fn = xor_shift::xorshift::xorshift32_;
#else
            const auto shift_fn = xor_shift::xorshift::xorshift64_;
#endif

            const auto val = shift_fn(magic) % (limit / char_size);
            if (val == 0)
                return char_size + 1;
            return val;
        }

        template <character _Chr>
        class _Str_crypt_fn
        {
        public:
            constexpr _Str_crypt_fn(size_t state) : state_(state) {}

            constexpr void operator()(_Chr& c)
            {
#ifdef UTILS_X32
                auto magic = xor_shift::xorshift::xorshift32(state_);
#else
            auto magic = xor_shift::xorshift::xorshift64(state_);
#endif
                c ^= magic;
            }

        private:
            size_t state_;
        };

        template <character _Chr, size_t _Str_length>
        class _Str_cryptor: public _Str_cryptor_cache<_Chr, _Str_length,
                                                      _Str_crypt_pad_size(sizeof(_Chr), __time__.seconds.count( ), 128) + 1,
                                                      _Str_crypt_pad_size(sizeof(_Chr), __time__.hours.count( ), 128) + 1>
        {
        public:

            constexpr _Str_cryptor(const _Chr (&str)[_Str_length], _Str_crypt_fn<_Chr> crypt, _Str_crypt_fn<_Chr> decrypt) :
                crypt_(std::move(crypt)), decrypt_(std::move(decrypt))
            {
                this->setup(str);
            }

            constexpr bool crypt( )
            {
                if (crypted_)
                    return false;

                if (!precompile_.has_value( ))
                    precompile_ = std::is_constant_evaluated( );

                _Apply_func(crypt_);
                crypted_ = true;

                return true;
            }

            /*_NODISCARD constexpr auto crypt_copy( ) const
            {
                _Str_cryptor copy = *this;
                copy.crypt( );
                return copy;
            }*/

            constexpr bool decrypt( )
            {
                if (!crypted_)
                    return false;

                _Apply_func(decrypt_);
                crypted_ = false;

                return true;
            }

            /*_NODISCARD constexpr _Str_cryptor decrypt_copy( ) const
            {
                _Str_cryptor copy = *this;
                copy.decrypt( );
                return copy;
            }*/

        private:

            
            /**
             * \brief 
             * \tparam _Store_as_array : true - store to array, false - store to std::string
             */
            template <bool _Store_as_array>

            struct _Unwrap_text_fn;

            // ReSharper disable CppExplicitSpecializationInNonNamespaceScope

            template < >
            struct _Unwrap_text_fn<true>
            {
                template <typename T>
                _Strings_combine_result<_Chr, _Str_length> operator()(T&& text) const
                {
                    _Strings_combine_result<_Chr, _Str_length> buffer{ };
                    _RANGES   copy(text, buffer.begin( ));
                    buffer.back( ) = '\0'; //force last char to 0 if we return crypted string
                    return buffer;
                }
            };

            template < >
            struct _Unwrap_text_fn<false>
            {
                template <typename T>
                std::basic_string<_Chr> operator()(T&& text) const
                {
                    return utl::to_string(text);
                }
            };

            // ReSharper restore CppExplicitSpecializationInNonNamespaceScope

        public:

            template <bool _Store_as_array = true>
            _NODISCARD auto unwrap_text(bool decrypt = true) const
            {
                static constexpr auto _Unwrap_text = _Unwrap_text_fn<_Store_as_array>( );
                if (decrypt && crypted_)
                {
                    _Str_cryptor copy = *this;
                    copy.decrypt( );
                    return _Unwrap_text(copy.get_text( ));
                }

                return _Unwrap_text(this->get_text( ));
            }

            template <bool _Store_as_array = true>
            _NODISCARD auto unwrap_text(bool decrypt = true, bool self = true)
            {
                if (decrypt && self && crypted_)
                    this->decrypt( );

                const _Str_cryptor& cthis = *this;
                return cthis.unwrap_text<_Store_as_array>(decrypt);
            }

        private:

            template <typename _Tyfn>
            constexpr void _Apply_func(_Tyfn&& fn)
            {
                if (*precompile_)
                    _RANGES for_each(this->get_whole_cache( ), fn);
                else
                    _RANGES for_each(this->get_text( ), fn);
            }

            bool crypted_ = false;

            std::optional<bool> precompile_;

            _Str_crypt_fn<_Chr> crypt_;
            _Str_crypt_fn<_Chr> decrypt_;
        };

#pragma pack(pop)

        struct _Str_cryptor_fn
        {
            template <character _Chr, size_t _Str_length>
            constexpr auto operator()(const _Chr (&str)[_Str_length], bool crypt = true, size_t magic_add = 0) const -> _Str_cryptor<_Chr, _Str_length>
            {
                auto           str_hash = hash::fnv1a(str);
                constexpr auto time_val = static_cast<size_t>(__time__.seconds.count( ) + __time__.hours.count( ));

                auto state = str_hash + magic_add + time_val;

                auto fn     = _Str_crypt_fn<_Chr>(state);
                auto result = _Str_cryptor<_Chr, _Str_length>(str, fn, fn);
                if (crypt)
                    result.crypt( );
                return result;
            }
        };
    }

    inline static constexpr auto crypt_str = detail::_Str_cryptor_fn( );
}

#ifndef _DEBUG
#define CRYPT_STR(str) ( []{\
        constexpr auto result = utl::to_string_view_ex(str);\
        return result;\
    }() )
#else
#define CRYPT_STR(str) ( []()->auto&{\
        static constexpr auto crypted = utl::crypt_str(str/*, utl::hash::fnv1a(__FILE__)+__LINE__*/);\
        static const auto decrypted   = crypted.unwrap_text();\
        static const auto result      = utl::to_string_view_ex(decrypted);\
        return result;\
    }() )
#endif

//#define CRYPT_STR_COPY(str) ::utl::to_string_ex(CRYPT_STR(str))
//#define CRYPT_STR_PTR(str)  CRYPT_STR(str).data()

//inline auto aa = []( )-> auto&
//{
//    static constexpr auto crypted = utl::crypt_str("aaa");
//    static const auto decrypted   = crypted.decrypt( );
//    static const auto result      = utl::any_string_view(decrypted);
//    return result;
//}( );
