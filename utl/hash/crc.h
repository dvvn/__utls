#pragma once

#include "hash core.h"

namespace utl::hash
{
    namespace detail
    {
        //crc logic found here: https://simplycalc.com/crc32-source.php
        //code modified and now it works with all possible bit'S
        //ps: github and stackoverflow suck, i found only unreadable ugly pre c++11 solutions there, or uselles and wrong

        template <_Integral_exclude_bool _Ty>
        struct _Crc_init_value: _Hash_init_value<_Ty>
        {
            constexpr _Crc_init_value(_Ty v = _Max( ), _Ty final_value = _Max( ))
                : _Hash_init_value<_Ty>(v),
                  final_value_(final_value) {}

            /*constexpr auto final(_Ty crc) const
            {
                return crc ^ final_value;;
            }*/
            constexpr _Ty finalize( ) const
            {
                return this->value ^ final_value_;
            }

        private:
            _Ty final_value_;

            static constexpr auto _Max( ) { return std::numeric_limits<std::make_unsigned_t<_Ty> >::max( ); }
        };

        template <_Integral_exclude_bool _Ty_poly>
        struct _Crc_table
        {
            using poly_type = _Ty_poly;
            using table_type = std::array<_Ty_poly, 256>;

            constexpr _Crc_table(_Ty_poly polynomial) : poly_(std::move(polynomial)), table_{ }, updated_(false)
            {
                _RANGES fill(table_, 0);
            }

            constexpr void reverse_poly( )
            {
                _Ty_poly poly_reversed = 0;
                for (auto i = 0u; i < std::numeric_limits<_Ty_poly>::digits; ++i)
                {
                    poly_reversed = poly_reversed << 1;
                    poly_reversed = poly_reversed | poly_ >> i & 1;
                }
                poly_ = poly_reversed;

                build_table(updated_ == true);
            }

            constexpr const table_type& get_table( )
            {
                build_table( );
                return table_;
            }

            constexpr const table_type& get_table( ) const
            {
                _STL_ASSERT(updated_ == true, "Table isnt generated!");
                return table_;
            }

            constexpr _Ty_poly get_poly( ) const
            {
                return poly_;
            }

            constexpr void build_table(bool force = false)
            {
                if (force || !updated_)
                {
                    _Generate_table( );
                    updated_ = true;
                }
            }

        private:

            constexpr void _Generate_table( )
            {
                //_STL_ASSERT(!table.has_value(), "Table already generated!");

                for (auto i = 0u; i < 256; ++i)
                {
                    _Ty_poly crc = i;

                    for (auto j = 0u; j < 8; ++j)
                    {
                        // is current coefficient set?
                        if (crc & 1)
                        {
                            // yes, then assume it gets zero'd (by implied x^64 coefficient of dividend)
                            crc >>= 1;

                            // and add rest of the divisor
                            crc ^= poly_;
                        }
                        else
                        {
                            // no? then move to next coefficient
                            crc >>= 1;
                        }
                    }

                    table_[i] = crc;
                }
            }

            poly_type  poly_;
            table_type table_;
            bool       updated_;
        };

        template <byte_only _Ty, _Integral_exclude_bool _Poly>
        struct _Crc_updater
        {
            constexpr _Crc_updater(_Poly polynomial, bool reverse) : table_{ }
            {
                auto temp_table = _Crc_table<_Poly>(polynomial);

                if (reverse)
                    temp_table.reverse_poly( );

                table_ = temp_table.get_table( );
            }

            constexpr void operator()(_Ty byte, _Poly& hash) const
            {
                /*uint8_t index = byte ^ hash;
               auto lookup   = table[index];

               hash >>= 8;
               hash ^= lookup;

               return hash;*/

                hash = table_[(hash ^ /*(std::uint8_t)*/byte) & UCHAR_MAX] ^ hash >> 8;;
            }

        private:
            typename _Crc_table<_Poly>::table_type table_;
        };

        template <byte_only _Bty, _Integral_exclude_bool _Ty>
        class _Crc_fn
        {
        public:
            using processer_type = _Hash_processer<_Bty, _Ty, _Crc_updater<_Bty, _Ty> >;
            using init_t = _Crc_init_value<_Ty>;

            constexpr _Crc_fn(_Ty polynomial, bool reverse) : processer_({polynomial, reverse}) { }

        private:
            template <typename ..._Args>
            static constexpr bool _Contains_init_value( )
            {
                using last_element_t = typename last_element<_Args...>::type;
                return std::same_as<std::remove_cvref_t<last_element_t>, init_t>;
            }

        public:

            template <typename ..._Args>
            constexpr _Ty operator()(_Args&&...values) const
            {
                if constexpr (_Contains_init_value<_Args...>( ))
                {
                    return std::invoke(processer_, FWD(values)...).finalize( );
                }
                else
                {
                    init_t init_value;
                    return std::invoke(processer_, FWD(values)..., init_value).finalize( );
                }
            }

        private:
            processer_type processer_;
        };
    }

#define PRESET_CRC(name, polynomial, reverse)\
    static constexpr auto crc_##name = detail::_Crc_fn<uint8_t, decltype(polynomial)>(polynomial, reverse)

    //warning!! RefIn must be same as RefOut

    PRESET_CRC(32, 0x04C11DB7U, true);           //IEEE 802.3
    PRESET_CRC(64, 0x000000000000001BUll, true); //ISO

    PRESET_CRC(32c, 0x1EDC6F41U, true);  //Castagnoli    
    PRESET_CRC(32k, 0x741B8CD7U, true);  //Koopman
    PRESET_CRC(32q, 0x814141ABU, false); //aviation; AIXM

    PRESET_CRC(64ecma, 0x42F0E1EBA9EA3693Ull, true); //ECMA

#undef PRESET_CRC

#ifdef UTILS_X32
    constexpr auto crc = crc_32;
#elif
    constexpr auto crc = get_64;
#endif

    namespace literals
    {
        //todo: consteval this

#define CRC_LITERALS(type)\
        constexpr auto operator ""_crc32(const type* c, size_t size) { return crc_32(get_data(c, size)); }\
        constexpr auto operator ""_crc64(const type* c, size_t size) { return crc_64(get_data(c, size)); }\
        constexpr auto operator ""_crc(const type* c, size_t size) { return crc(get_data(c, size)); }

        CRC_LITERALS(char);
#ifdef __cpp_char8_t
        CRC_LITERALS(char8_t);
#endif
        CRC_LITERALS(char16_t);
        CRC_LITERALS(char32_t);
        CRC_LITERALS(wchar_t);

#undef CRC_LITERALS

#if 0

        //constexpr auto test_table = detail::make_crc_table(79764919);

        //constexpr auto generator = detail::crc_generator{/*impl::crc_polynomial::reverse*/(unsigned int(79764919)) };
        //constexpr size_t asd = generator.update(std::string_view("123456789"), decltype(generator)::update_info{ 4294967295,4294967295 });

        constexpr auto a = "0x22887DC2CAAF2000"_crc32;
        constexpr auto aa = a == 0xE353439F;

        constexpr auto b = "qwerty"_crc64;
        constexpr auto bb = b == 0x22887DC2CAAF2000;
#endif
    }
}
