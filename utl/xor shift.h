#pragma once

#include <chrono>
#include "hash/fnv.h"

namespace utl
{
    namespace detail
    {
        constexpr auto _Unpack_number(const std::string_view& str, size_t idx, size_t length)
        {
            size_t val   = 0;
            size_t shift = 1;
            for (size_t i = 1; i < length; ++i)
            {
                shift *= 10;
            }

            for (auto chr: str.substr(idx, length))
            {
                const auto num         = chr - '0';
                const auto correct_num = static_cast<std::make_unsigned_t<decltype(num)>>(num) * shift;

                val += correct_num;
                shift /= 10;
            }

            return val;

            /* auto part1 = c2i(str[idx]);
             auto part2 = c2i(str[idx + 1]);
     
             return part1 * 10 + part2;*/
        }

        struct _Time_impl: std::string_view
        {
            constexpr _Time_impl( ) : std::string_view(to_string_view(__TIME__))
            {
                static_assert(sizeof(__TIME__) == 9);

                _Unpack_time(seconds, 6);
                _Unpack_time(minutes, 3);
                _Unpack_time(hours, 0);
            }

            std::chrono::hours   hours{ };
            std::chrono::minutes minutes{ };
            std::chrono::seconds seconds{ };

        private:

            template <typename T>
            constexpr void _Unpack_time(T& value, int idx)
            {
                value = T(_Unpack_number(*this, idx, 2));
            }
        };

        struct _Date_impl: std::string_view
        {
            constexpr _Date_impl( ) : std::string_view(to_string_view(__DATE__))
            {
                static_assert(sizeof(__DATE__) == 12);
            }

            std::string_view month_prefix      = this->substr(0, 3);;
            size_t           month_prefix_hash = hashed_string_view::_Compute_hash(month_prefix);;

            size_t day   = _Unpack_number(*this, 4, 2);
            size_t month = this->_Get_month_index( );
            size_t year  = _Unpack_number(*this, 7, 4);

            static constexpr auto month_names = std::array<hashed_string_view, 12>{
                "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
            };

        private:

            /*constexpr size_t get_month_index( ) const
            {
                auto& arr           = __month_names;
                const size_t number = _RANGES find(arr, month_prefix) - arr.begin( ) + 1;
                return number;
            }*/

            constexpr size_t _Get_month_index( ) const
            {
                const size_t number = _RANGES find(month_names, month_prefix_hash, &hashed_string_view::hash) - month_names.begin( ) + 1;
                return number;
            }
        };
    }

    // ReSharper disable CppInconsistentNaming

    inline constexpr auto __time__ = detail::_Time_impl{ };
    inline constexpr auto __date__ = detail::_Date_impl{ };

    // ReSharper restore CppInconsistentNaming

    namespace xor_shift
    {
        namespace xorwow
        {
            //Marsaglia suggested scrambling the output by combining it with a simple additive counter modulo 2^32
            //(which he calls a "Weyl sequence" after Weyl's equidistribution theorem). This also increases the period by a factor of 2^32, to 2^192−2^32: 

            struct xorwow_state
            {
                uint32_t a, b, c, d, e;
                uint32_t counter;
            };

            constexpr xorwow_state get_state( )
            {
                xorwow_state s;

                s.a = (uint32_t)__time__.minutes.count( );
                s.b = (uint32_t)__time__.seconds.count( );
                s.c = (uint32_t)__time__.hours.count( );
                s.d = static_cast<uint32_t>(__date__.month_prefix_hash ^ __date__.day);
                s.e = (uint32_t)(__date__.day ^ __date__.month);

                s.counter = 0;

                return s;
            }

            constexpr uint32_t update(xorwow_state& state)
            {
                /* Algorithm "xorwow" from p. 5 of Marsaglia, "Xorshift RNGs" */
                auto t  = state.e;
                auto s  = state.a;
                state.e = state.d;
                state.d = state.c;
                state.c = state.b;
                state.b = s;
                t ^= t >> 2;
                t ^= t << 1;
                t ^= s ^ (s << 4);
                state.a = t;
                state.counter += 362437;
                return t + state.counter;
            }
        };

        namespace xorshift64s
        {
            //A xorshift* generator takes a xorshift generator and applies an invertible multiplication (modulo the word size) to its output as a non-linear transformation, as suggested by Marsaglia.[1]
            //The following 64-bit generator with 64 bits of state has a maximal period of 2^64−1[8] and fails only the MatrixRank test of BigCrush:

            struct xorshift64s_state
            {
                uint64_t a;
            };

            constexpr uint64_t update(xorshift64s_state& state)
            {
                auto x = state.a; /* The state must be seeded with a nonzero value. */
                x ^= x >> 12;     // a
                x ^= x << 25;     // b
                x ^= x >> 27;     // c
                state.a = x;
                return x * (0x2545F4914F6CDD1DU);
            }
        }

        namespace xorshift1024s
        {
            /* The state must be seeded so that there is at least one non-zero element in array */
            struct xorshift1024s_state
            {
                std::array<uint64_t, 16> array;
                int                      index;
            };

            constexpr uint64_t update(xorshift1024s_state& state)
            {
                auto       index = state.index;
                auto const s     = state.array[index++];
                auto       t     = state.array[index &= 15];
                t ^= t << 31;       // a
                t ^= t >> 11;       // b
                t ^= s ^ (s >> 30); // c
                state.array[index] = t;
                state.index        = index;
                return t * 1181783497276652981U;
            }
        }

        namespace xorshift128p
        {
            struct xorshift128p_state
            {
                uint64_t a, b;
            };

            /* The state must be seeded so that it is not all zero */
            constexpr uint64_t xorshift128p(xorshift128p_state& state)
            {
                auto       t = state.a;
                auto const s = state.b;
                state.a      = s;
                t ^= t << 23;       // a
                t ^= t >> 17;       // b
                t ^= s ^ (s >> 26); // c
                state.b = t;
                return t + s;
            }
        }

        namespace xoshiro256
        {
            constexpr uint64_t rol64(uint64_t x, int k)
            {
                return (x << k) | (x >> (64 - k));
            }

            struct xoshiro256_state
            {
                std::array<uint64_t, 4> s;
            };

            constexpr uint64_t xoshiro256ss(xoshiro256_state& state)
            {
                auto& s      = state.s;
                auto  result = rol64(s[1] * 5, 7) * 9;
                auto  t      = s[1] << 17;

                s[2] ^= s[0];
                s[3] ^= s[1];
                s[1] ^= s[2];
                s[0] ^= s[3];

                s[2] ^= t;
                s[3] = rol64(s[3], 45);

                return result;
            }

            constexpr uint64_t xoshiro256p(xoshiro256_state& state)
            {
                auto& s = state.s;

                const auto result = s[0] + s[3];
                const auto t      = s[1] << 17;

                s[2] ^= s[0];
                s[3] ^= s[1];
                s[1] ^= s[2];
                s[0] ^= s[3];

                s[2] ^= t;
                s[3] = rol64(s[3], 45);

                return result;
            }
        }

        namespace xorshift
        {
            /* The state word must be initialized to non-zero */
            constexpr uint32_t xorshift32(uint32_t& state)
            {
                /* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
                uint32_t x = state;
                x ^= x << 13;
                x ^= x >> 17;
                x ^= x << 5;
                return state = x;
            }

            constexpr uint32_t xorshift32_(uint32_t init_value)
            {
                return xorshift32(init_value);
            }

            constexpr uint64_t xorshift64(uint64_t& state)
            {
                uint64_t x = state;
                x ^= x << 13;
                x ^= x >> 7;
                x ^= x << 17;
                return state = x;
            }

            constexpr uint64_t xorshift64_(uint64_t init_value)
            {
                return xorshift64(init_value);
            }

            struct xorshift128_state
            {
                uint32_t a, b, c, d;
            };

            /* The state array must be initialized to not be all zero */
            constexpr uint32_t xorshift128(xorshift128_state& state)
            {
                /* Algorithm "xor128" from p. 5 of Marsaglia, "Xorshift RNGs" */
                uint32_t t = state.d;

                const uint32_t s = state.a;

                state.d = state.c;
                state.c = state.b;
                state.b = s;

                t ^= t << 11;
                t ^= t >> 8;
                return state.a = t ^ s ^ (s >> 19);
            }
        }

        namespace splitmix64
        {
            constexpr uint64_t splitmix64(uint64_t& state)
            {
                uint64_t result = state;

                state  = result + 0x9E3779B97f4A7C15;
                result = (result ^ (result >> 30)) * 0xBF58476D1CE4E5B9;
                result = (result ^ (result >> 27)) * 0x94D049BB133111EB;
                return result ^ (result >> 31);
            }

            // as an example; one could do this same thing for any of the other generators
            constexpr xorshift::xorshift128_state xorshift128_init(uint64_t seed)
            {
                uint64_t                    smstate = seed;
                xorshift::xorshift128_state result{ };

                uint64_t tmp = splitmix64(smstate);
                result.a     = (uint32_t)tmp;
                result.b     = (uint32_t)(tmp >> 32);

                tmp      = splitmix64(smstate);
                result.c = (uint32_t)tmp;
                result.d = (uint32_t)(tmp >> 32);

                return result;
            }
        }
    }
}
