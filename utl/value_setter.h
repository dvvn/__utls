#pragma once

#include "utils core.h"
#include "function info.h"

#include <functional>

namespace utl
{
    namespace detail
    {
        template <typename _Ty>
        constexpr auto _Setter_base_fn( )
        {
            if constexpr (!std::is_class_v<_Ty> || !std::copyable<_Ty>)
                return std::type_identity<std::optional<_Ty> >{ };
            else
                return std::type_identity<_Ty>{ };
        }

        template <typename _Ty>
        using _Setter_base = typename decltype(_Setter_base_fn<_Ty>( ))::type;

        template <bool _Native_get, std::default_initializable _Ty>
        class value_setter: public _Setter_base<_Ty>
        {
            using _Base_type = _Setter_base<_Ty>;

            auto _Base( ) -> _Base_type& { return *static_cast<_Base_type*>(this); }
            auto _Base( ) const -> const _Base_type& { return *static_cast<const _Base_type*>(this); }

            static constexpr bool _Is_wrapped( ) { return sizeof(_Base_type) != sizeof(_Ty); }

            auto _Get_value( ) const -> const _Ty&
            {
                if constexpr (!_Is_wrapped( ))
                    return _Base( );
                else
                    return *_Base( );
            }

            auto _Get_value( ) -> _Ty&
            {
                const _Ty& val = static_cast<const value_setter*>(this)->_Get_value( );
                return const_cast<_Ty&>(val);
            }

            void _Set_value(_Ty&& val)
            {
                if constexpr (!_Is_wrapped( ))
                    _Base( ) = std::move(val);
                else
                    _Base( ).emplace(std::move(val));

                if (this->released( ) && this->_Is_modified( ))
                {
                    this->dont_apply_ = false;
                }
            }

            void _Set_value(const _Ty& val)
            {
                _Ty copy = val;
                _Set_value(std::move(copy));
            }

            bool _Is_modified( ) const
            {
                if constexpr (_Is_wrapped( ))
                {
                    if (this->_Base( ).has_value( ) == false)
                        return false;
                }
                return this->_Get_value( ) != getter_fn_( );
            }

        public:
            using checker_fn = std::function<bool(const _Ty&)>;
            using setter_fn = std::function<void(_Ty&&)>;
            using locked_fn = std::function<bool( )>;
            using getter_fn = std::function<std::conditional_t<_Native_get, _Ty&, _Ty>( )>;

            ~value_setter( )
            {
                this->apply( );
            }

        private:

            void _Init(locked_fn&& locked_fn, getter_fn&& getter_fn, setter_fn&& setter_fn)
            {
                locked_fn_ = std::move(locked_fn);
                _STL_ASSERT(locked_fn_ != nullptr, "Locked func is null!");
                getter_fn_ = std::move(getter_fn);
                _STL_ASSERT(getter_fn_ != nullptr, "Getter func is null!");
                setter_fn_ = std::move(setter_fn);
                _STL_ASSERT(setter_fn_ != nullptr, "Setter func is null!");

                if constexpr (!_Is_wrapped( ))
                    this->sync( );
            }

        public:
            PREVENT_COPY(value_setter)

            value_setter(value_setter&& other) noexcept: _Base_type( )
            {
                *this = std::move(other);
            }

            value_setter& operator =(value_setter&& other) noexcept
            {
                MOVE_ASSIGN(_Base());
                MOVE_ASSIGN(dont_apply_);
                MOVE_ASSIGN(locked_fn_);
                MOVE_ASSIGN(getter_fn_);
                MOVE_ASSIGN(setter_fn_);

                other.dont_apply_ = true;

                return *this;
            }

        private:
            getter_fn _Native_getter(_Ty& native_owner) const
            {
                return [&]( )-> _Ty&
                {
                    return native_owner;
                };
            }

        public:

            template <check_function<locked_fn> _FnL, check_function<checker_fn> _FnC = checker_fn>
            value_setter(_FnL&& locked_func, _Ty& native_owner, _FnC&& checker_func = { }) : _Base_type( )
            {
                setter_fn temp_setter = [&native_owner](_Ty&& value)
                {
                    native_owner = std::move(value);
                };
                setter_fn  setter_func;
                checker_fn temp_checker = FWD(checker_func);
                if (!temp_checker)
                    setter_func = std::move(temp_setter);
                else
                {
                    setter_func = [checker = std::move(temp_checker), setter = std::move(temp_setter)](_Ty&& val)
                    {
                        if (!std::invoke(checker, val))
                            throw std::logic_error("Unable to set value!");
                        std::invoke(setter, std::move(val));
                    };
                }

                _Init(locked_fn(FWD(locked_func)), _Native_getter(native_owner), std::move(setter_func));
            }

            template <check_function<locked_fn> _FnL, check_function<setter_fn> _FnS>
            value_setter(_FnL&& locked_func, _Ty& native_owner, _FnS&& setter_func) : _Base_type( )
            {
                _Init(locked_fn(FWD(locked_func)), _Native_getter(native_owner), setter_fn(FWD(setter_func)));
            }

            template <check_function<locked_fn> _FnL,check_function<getter_fn> _FnG, check_function<setter_fn> _FnS>
            value_setter(_FnL&& locked_func, _FnG&& getter_func, _FnS&& setter_func) : _Base_type( )
            {
                _Init(locked_fn(FWD(locked_func)), getter_fn(FWD(getter_func)), setter_fn(FWD(setter_func)));
            }

        private:
            value_setter( ) = default;
        public:
            value_setter& operator=(_Ty&& val)
            {
                this->_Set_value(std::move(val));
                return *this;
            }

            template <typename _Ty_other>
            value_setter& operator=(_Ty_other&& val) requires(std::constructible_from<_Ty, decltype(val)>)
            {
                _Ty temp(FWD(val));
                this->_Set_value(std::move(temp));
                return *this;
            }

        private:
            bool released( ) const { return this->dont_apply_; }

            _Ty release( )
            {
                _Ty result;
                if constexpr (!_Is_wrapped( ))
                {
                    result = std::move(this->_Get_value( ));
                }
                else
                {
                    auto& base = this->_Base( );
                    if (/*!dont_apply_ && */base.has_value( ))
                    {
                        result = std::move(this->_Get_value( ));
                        base.reset( );
                    }
                }
                dont_apply_     = true;
                _Ty&& result_rv = static_cast<_Ty&&>(result);
                return result_rv;
            }

            void destroy(bool apply = false)
            {
                if (!apply)
                    this->dont_apply_ = true;

                value_setter dummy;
                std::swap(*this, dummy);
            }

            bool apply( )
            {
                if (dont_apply_)
                    return false;
                if (!_Is_modified( ))
                    return false;

                if (locked_fn_( ))
                    throw std::runtime_error("Unable to modify locked setter!");

                setter_fn_(this->release( ));
                return true;
            }

            void sync( ) requires(std::copyable<_Ty>)
            {
                dont_apply_ = false;
                this->_Set_value(getter_fn_( ));
            }

#define VS_PROXY_FN(ret, name, ...)\
            ret name () __VA_ARGS__{\
                return this->owner( ).name( );\
            }

            class _Setter_proxy_const
            {
            public:
                _Setter_proxy_const(const value_setter& owner): owner_(owner) {}

                VS_PROXY_FN(bool, released, const)

            protected:
                const value_setter& owner( ) const
                {
                    return owner_;
                }

                value_setter& owner( )
                {
                    return const_cast<value_setter&>(owner_);
                }

            private:
                const value_setter& owner_;
            };

            class _Setter_proxy_1: public _Setter_proxy_const
            {
            public:
                _Setter_proxy_1(value_setter& owner): _Setter_proxy_const(owner) {}

                VS_PROXY_FN(_Ty, release)
                VS_PROXY_FN(void, destroy)
                VS_PROXY_FN(bool, apply)
            };

            class _Setter_proxy_2: public _Setter_proxy_1
            {
            public:
                _Setter_proxy_2(value_setter& owner): _Setter_proxy_1(owner) {}

                VS_PROXY_FN(void, sync)
            };

#undef VS_PROXY_FN

        public:

            _Setter_proxy_const get_value_setter( ) const
            {
                return (*this);
            }

            auto get_value_setter( )
            {
                if constexpr (!std::copyable<_Ty>)
                    return _Setter_proxy_1(*this);
                else
                    return _Setter_proxy_2(*this);
            }

        private:
            bool dont_apply_ = true;

            locked_fn locked_fn_;
            getter_fn getter_fn_;
            setter_fn setter_fn_;
        };
    }

    template <typename _Ty>
    using setter_native = detail::value_setter<true, _Ty>;

    template <typename _Ty>
    using setter_custom = detail::value_setter<false, _Ty>;
}
