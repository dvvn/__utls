#pragma once
#include "function info.h"
#include <functional>

//#include <d3d9.h>

namespace utl
{
    template <typename _Ty>
    class delete_helper
    {
    public:
        using pointer = std::conditional_t<std::is_pointer_v<_Ty>, _Ty, _Ty*>;

        template <std::invocable<pointer> _TyFn>
        delete_helper(_TyFn&& fn) : fn_(FWD(fn)) {}

        void operator()(pointer ptr) const
        {
            fn_(ptr);
        }

    private:
        std::function<void(pointer)> fn_;
    };

    template <typename _Ty>
    class delete_helper_dummy
    {
    public:
        using pointer = typename delete_helper<_Ty>::pointer;

        template <std::invocable<pointer> _TyFn>
        delete_helper_dummy(_TyFn&&) {}

        delete_helper_dummy( ) = default;

        void operator()(pointer) const { }
    };

    template <typename _Ty>
    class unique_ptr_ex: public std::unique_ptr<_Ty, delete_helper<_Ty> >
    {
    public:

        using base_t = std::unique_ptr<_Ty, delete_helper<_Ty> >;

        using pointer = typename base_t::pointer;

        PREVENT_COPY_FORCE_MOVE(unique_ptr_ex);

        template <std::invocable<pointer> _TyFn>
        unique_ptr_ex(pointer ptr, _TyFn&& deleter) : base_t(ptr, delete_helper<_Ty>(FWD(deleter))) {}

        unique_ptr_ex(std::nullptr_t) : unique_ptr_ex(static_cast<pointer>(nullptr), delete_helper_dummy<_Ty>( )) {}

        unique_ptr_ex( ) : unique_ptr_ex(nullptr) {}

        operator pointer( ) const
        {
            return this->get( );
        }

        [[nodiscard]] auto to_shared( ) -> std::shared_ptr<typename base_t::element_type>
        {
            return std::move(*this);
        }
    };
#if 0
#define CLASS_UNIQUE_PTR_EX1(name, type)\
	class name : public unique_ptr_ex<type>{\
	public :\
		 PREVENT_COPY_FORCE_MOVE(name);\
         name(pointer ptr);

#define CLASS_UNIQUE_PTR_EX_BASE(name, type, deleter,  ...)\
	class name : public ::utl::unique_ptr_ex<type>{\
	public :\
		name(pointer ptr):\
		unique_ptr_ex(ptr, [](pointer ptr2){std::invoke(deleter,ptr2,##__VA_ARGS__);} ) {}\
        PREVENT_COPY_FORCE_MOVE(name);

#define CLASS_UNIQUE_PTR_EX(name, type, deleter,  ...)\
	CLASS_UNIQUE_PTR_EX_BASE(name, type, deleter, ##__VA_ARGS__)\
	}

    namespace unique
    {
#ifdef _INC_WTSAPI
CLASS_UNIQUE_PTR_EX(wts_process_info, WTS_PROCESS_INFO, WTSFreeMemory);
#endif
    }

#undef CLASS_UNIQUE_PTR_EX_BASE
#undef CLASS_UNIQUE_PTR_EX
#endif
}
