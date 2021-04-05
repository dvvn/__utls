#pragma once

#include "utils core.h"

namespace utl
{
    template <std::default_initializable T, size_t Index = 0>
    class one_instance
    {
    public:
        constexpr one_instance( ) = default;

        using value_type = T;
        using reference = T&;
        using const_reference = const T&;
        using pointer = T*;
        using const_pointer = const T*;

        static reference get( )
        {
            static T cache = T( );
            return cache;
        }

        auto operator->( ) -> reference { return get( ); }
        auto operator->( ) const -> const_reference { return get( ); }
        auto operator*( ) -> reference { return get( ); }
        auto operator*( ) const -> const_reference { return get( ); }
        //constexpr auto operator()( ) -> reference { return get( ); }
        //constexpr auto operator()( ) const -> const_reference { return get( ); }

        operator reference( ) { return get( ); }
        operator const_reference( ) const { return get( ); }
    };
}
