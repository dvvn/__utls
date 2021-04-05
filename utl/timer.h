#pragma once

#include "utils core.h"

#include <chrono>

namespace utl
{
    class timer
    {
        static auto _Now( )
        {
            return std::chrono::high_resolution_clock::now( );
        }

    public:

        timer(bool start = 0)
        {
            if (start)
                this->set_start( );
        }

        bool started( ) const
        {
            return start_.has_value( );
        }

        bool updated( ) const
        {
            return end_.has_value( );
        }

        void set_start( )
        {
            start_.emplace(_Now( ));
            end_.reset( );
        }

        void set_end( )
        {
            _STL_ASSERT(started( ), "Timer not started");
            end_.emplace(_Now( ));
        }

        auto elapsed( ) const
        {
            _STL_ASSERT(updated( ), "Timer not updated");
            return *end_ - *start_;
        }

    private:

        std::optional<std::chrono::time_point<std::chrono::high_resolution_clock> > start_, end_;
    };

    class benchmark_timer final: protected timer
    {
    public:
        benchmark_timer( ) = default;

        template <class T, typename ...P>
        benchmark_timer(T& fn, P&&...params)
        {
            work(FWD(fn), FWD(params)...);
        }

        template <class T, typename ...P>
        void work(T&& fn, P&&...params)
        {
            this->set_start( );
            std::invoke(FWD(fn), FWD(params)...);
            this->set_end( );
        }

        template <class T, typename ...P>
        void work(size_t count, T&& fn, P&&...params)
        {
            _STL_ASSERT(count == 0, "bad count found");

            if (count == 1)
                return work(FWD(fn), FWD(params)...);;

            this->set_start( );
            while (count--)
                std::invoke(FWD(fn), FWD(params)...);
            this->set_end( );
        }

        using timer::elapsed;
        using timer::updated ;
    };

    template <typename ...T>
    std::chrono::nanoseconds benchmark_invoke(T&&...args)
    {
        benchmark_timer timer;
        timer.work(FWD(args)...);
        return timer.elapsed( );
    }
}
