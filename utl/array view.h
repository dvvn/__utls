#pragma once

#include "string view ex.h"

#include <algorithm>
#include <functional>

namespace utl
{
    template <typename _Ty>
    struct array_view_default_value
    {
        static constexpr _Ty value = _Ty(0);
    };

    template <typename _Ty, size_t _Size, typename _Def_val = array_view_default_value<_Ty>>
    class array_view: public std::array<_Ty, _Size>
    {
        template <typename _Ty_current, typename ..._Ty_next>
        constexpr void _Fill_part(size_t& offset, _Ty_current&& item, _Ty_next&&...args)
        {
            using namespace std;

            auto start = this->begin( ) + offset;

            if constexpr (ranges::random_access_range<_Ty_current>)
            {
                for (decltype(auto) r: item)
                    _Fill_part(offset, FWD(r));
            }
            else if constexpr (constructible_from<_Ty, _Ty_current>)
            {
                using val_t = remove_cvref_t<_Ty_current>;
                constexpr bool rvalue = is_rvalue_reference_v<decltype(item)>;
                if constexpr (rvalue && (same_as<_Ty, val_t> || derived_from<_Ty, val_t>))
                    *start = FWD(item);
                else
                    *start = _Ty(item);

                offset += 1;
            }
            else
            {
                static_assert(_Always_false<_Ty_current>,"Unsupported type");
                throw;
            }

            if constexpr (sizeof...(args) > 0)
                _Fill_part(offset, FWD(args)...);
        }

    public:

        using arr_base = std::array<_Ty, _Size>;

        template <typename ..._Args>
        constexpr array_view(_Args&&...args) : arr_base{ }
        {
            size_t offset = 0;
            if constexpr (sizeof...(args) > 0)
                _Fill_part(offset, FWD(args)...);
            std::fill(this->begin( ) + offset, this->end( ), _Def_val::value);
        }

        //ALLOW_COPY_AND_MOVE(array_view);

        //-----------------

        template <typename Q>
        constexpr bool operator==(Q&& val) const
        {
            using namespace std;

            auto& this_arr = *static_cast<const arr_base*>(this);

            if constexpr (ranges::random_access_range<Q>)
            {
                static_assert(constructible_from<_Ty, ranges::range_value_t<Q> >);
                return ranges::equal(this_arr, val);
            }
            else
            {
                static_assert(constructible_from<_Ty, remove_cvref_t<Q> >);
                //return ranges::equal_range(this_arr, val).size( ) == Size;
                for (auto& v: this_arr)
                {
                    if (v != val)
                        return false;
                }
                return true;
            }
        }

        template <typename Q>
        constexpr bool operator!=(Q&& val) const
        {
            return this->operator==(FWD(val)) == false;
        }

    private:

        template <typename Q, typename Fn>
        constexpr array_view& _Operator_helper(Q&& val, Fn&& fn)
        {
            using namespace std;

            auto& this_arr = *static_cast<arr_base*>(this);

            if constexpr (!ranges::random_access_range<Q>)
            {
                using val_t = remove_cvref_t<Q>;
                static_assert(constructible_from<_Ty, val_t>);
                static_assert(copyable<val_t>);

                if constexpr (_Size == 1)
                {
                    fn(this_arr.front( ),FWD(val));
                }
                else
                {
                    for (auto& v: this_arr)
                        fn(v, val);
                }
            }
            else
            {
                static_assert(constructible_from<_Ty, ranges::range_value_t<Q> >);
                //ranges::transform(this_arr, FWD(val), this_arr.begin( ), fn);

                constexpr auto _Get_element = []<typename W>(W& v)-> decltype(auto)
                {
                    if constexpr (std::is_rvalue_reference_v<decltype(val)>)
                        return static_cast<W&&>(v);
                    else
                        return FWD(v);
                };

                const auto this_size = this_arr.size( );
                const auto val_size  = val.size( );

                auto itr     = this_arr.begin( );
                auto val_itr = val.begin( );

                if (val_size >= this_size)
                {
                    auto last_itr = this_arr.end( );
                    while (true)
                    {
                        fn(*itr, _Get_element(*val_itr));
                        ++itr;
                        if (itr == last_itr)
                            break;
                        ++val_itr;
                    }
                }
                else
                {
                    auto last_val_itr = val.end( );
                    while (true)
                    {
                        fn(*itr, _Get_element(*val_itr));
                        ++val_itr;
                        if (val_itr == last_val_itr)
                            break;
                        ++itr;
                    }
                }
            }

            return *this;
        }

        struct _Plus_fn
        {
            template <typename Q>
            constexpr void operator()(_Ty& to, Q&& val) const
            {
                to += FWD(val);
            }
        };
        struct _Minus_fn
        {
            template <typename Q>
            constexpr void operator()(_Ty& to, Q&& val) const
            {
                to -= FWD(val);
            }
        };
        struct _Multiplies_fn
        {
            template <typename Q>
            constexpr void operator()(_Ty& to, Q&& val) const
            {
                to *= FWD(val);
            }
        };
        struct _Divides_fn
        {
            template <typename Q>
            constexpr void operator()(_Ty& to, Q&& val) const
            {
                to /= FWD(val);
            }
        };

    public:

        template <typename Q>
        constexpr array_view& operator+=(Q&& val)
        {
            return _Operator_helper(FWD(val), _Plus_fn{ });
        }

        template <typename Q>
        constexpr array_view& operator-=(Q&& val)
        {
            return _Operator_helper(FWD(val), _Minus_fn{ });
        }

        template <typename Q>
        constexpr array_view& operator*=(Q&& val)
        {
            return _Operator_helper(FWD(val), _Multiplies_fn{ });
        }

        template <typename Q>
        constexpr array_view& operator/=(Q&& val)
        {
            return _Operator_helper(FWD(val), _Divides_fn{ });
        }

        //---------------

        template <typename Q>
        constexpr array_view operator+(Q&& val) const
        {
            array_view copy = *this;
            copy += FWD(val);
            return copy;
        }

        template <typename Q>
        constexpr array_view operator-(Q&& val) const
        {
            array_view copy = *this;
            copy -= FWD(val);
            return copy;
        }

        template <typename Q>
        constexpr array_view operator*(Q&& val) const
        {
            array_view copy = *this;
            copy *= FWD(val);
            return copy;
        }

        template <typename Q>
        constexpr array_view operator/(Q&& val) const
        {
            array_view copy = *this;
            copy /= FWD(val);
            return copy;
        }
    };
}
