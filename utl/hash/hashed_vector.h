// ReSharper disable CppRedundantAccessSpecifier

#pragma once
#include "hash core.h"

#include <vector>

namespace utl
{
    namespace detail
    {
        template <typename _Ty_pair>
        struct _Is_pair_impl;

        template <typename _Kty, typename _Ty>
        struct _Is_pair_impl<std::pair<_Kty, _Ty> > { };

        template <typename _Ty_pair>
        concept _Is_pair = std::default_initializable<_Is_pair_impl<std::remove_cvref_t<_Ty_pair> > >;

        template <typename _Ty1, typename _Ty2>
        concept _Same_as_clean = std::same_as<std::remove_cvref_t<_Ty1>, std::remove_cvref_t<_Ty2> >;
        template <typename _Ty1, typename _Ty2>
        concept _Not_same_as_clean = !std::same_as<std::remove_cvref_t<_Ty1>, std::remove_cvref_t<_Ty2> >;

        template <typename _Ty, typename _Ty_same>
        concept _Value_inside = requires(_Ty&& val)
        {
            { *val }->_Same_as_clean<_Ty_same>;
        };

        template <typename _Ty, typename _Ty_convertible>
        concept _Value_inside_maybe = requires(_Ty&& val)
        {
            { *val }->std::convertible_to<const _Ty_convertible&>;
        };

        template <typename _Ty, bool _Steal_value>
        struct _Unwrap_value_fn
        {
            template <_Same_as_clean<_Ty> _Ty_same>
            decltype(auto) operator()(_Ty_same&& val) const
            {
                if constexpr (_Steal_value && std::is_rvalue_reference_v<decltype(val)> && std::movable<_Ty>)
                    return _Ty(std::move(val));
                else
                    return FWD(val);
            }

            template <_Not_same_as_clean<_Ty> _Ty_other>
            _Ty operator()(_Ty_other&& val) const requires(std::constructible_from<_Ty, decltype(val)>)
            {
                if constexpr (_Steal_value)
                    return _Ty(FWD(val));
                else
                    return _Ty(val);
            }

            template <_Not_same_as_clean<_Ty> _Ty_wrapped>
            decltype(auto) operator()(_Ty_wrapped&& val) const requires(_Value_inside_maybe<_Ty_wrapped, _Ty>)
            {
                constexpr bool steal_possible = _Steal_value && _Value_inside<_Ty_wrapped, _Ty>;
                if constexpr (!steal_possible || !std::is_rvalue_reference_v<decltype(val)> || !std::movable<std::remove_cvref_t<_Ty_wrapped> >)
                {
                    if constexpr (!std::is_const_v<std::remove_reference_t<_Ty_wrapped> >)
                    {
                        return *val;
                    }
                    else
                    {
                        const auto& ref = *val;
                        return ref;
                    }
                }
                else if constexpr (std::is_class_v<std::remove_cvref_t<_Ty_wrapped> >)
                {
                    auto tmp = std::move(val); //destroy object
                    return _Ty(std::move(*tmp));
                }
                else
                {
                    return _Ty(std::move(*val));
                }
            }
        };

        template <class _Ty, size_t _Ty_index, bool _Steal_value>
        class _Unwrap_pair_fn
        {
            static constexpr auto _Unwrap_value = _Unwrap_value_fn<_Ty, _Steal_value>( );

        public:
            static_assert(_Ty_index <= 1, "Pair index must be 0 or 1");

            template <_Is_pair _Ty_inside>
            decltype(auto) operator()(_Ty_inside&& val) const requires(_Get_possible<_Ty_inside, _Ty_index>)
            {
                return _Unwrap_value(std::get<_Ty_index>(FWD(val)));
            }
        };
    }

#pragma region Hashed_vector_hash_fn

    template <bool _Stored_as_pair, class _Kty, class _Ty>
    struct _Hashed_vector_hash_fn;

    template <class _Kty, class _Ty>
    struct _Hashed_vector_hash_fn<true, _Kty, _Ty>: detail::_Unwrap_pair_fn<_Kty, 0, false> { };

    template <class _Kty, class _Ty>
    struct _Hashed_vector_hash_fn<false, _Kty, _Ty>: detail::_Unwrap_value_fn<_Kty, false> { };

#pragma endregion

#pragma region Hashed_vector_get_value_fn

    template <bool _Stored_as_pair, class _Kty, class _Ty>
    struct _Hashed_vector_get_value_fn;

    template <class _Kty, class _Ty>
    struct _Hashed_vector_get_value_fn<true, _Kty, _Ty>: detail::_Unwrap_pair_fn<_Ty, 1, false> { };

    template <class _Kty, class _Ty>
    struct _Hashed_vector_get_value_fn<false, _Kty, _Ty>: detail::_Unwrap_value_fn<_Ty, false> { };

#pragma endregion

#pragma region Hashed_vector_construct_helper_fn

    template <bool _Stored_as_pair, class _Kty, class _Ty>
    struct _Hashed_vector_construct_helper_fn;

    template <class _Kty, class _Ty>
    struct _Hashed_vector_construct_helper_fn<true, _Kty, _Ty>
    {
        using value_type = std::pair<_Kty, _Ty>;

        template <typename _Kty_tmp, typename ..._Ty_args>
        value_type operator ()(_Kty_tmp&& key_value, _Ty_args&&...args) const requires(
            std::constructible_from<_Kty, decltype(key_value)> && std::constructible_from<_Ty, decltype(args)...>)
        {
            _Kty key = _Kty(FWD(key_value));
            _Ty  val = _Ty(FWD(args)...);
            return value_type(std::move(key), std::move(val));
        }

        template <class _Kty_tmp, class _Ty_tmp>
        value_type operator()(std::pair<_Kty_tmp, _Ty_tmp>&& val) const requires(
            std::constructible_from<_Kty, _Kty_tmp&&> && std::constructible_from<_Ty, _Ty_tmp&&>)
        {
            return (*this)(std::move(val.first), std::move(val.second));
        }

        template <std::copyable _Kty_tmp, std::copyable _Ty_tmp_copyable>
        value_type operator()(const std::pair<_Kty_tmp, _Ty_tmp_copyable>& val) const requires(
            std::invocable<_Hashed_vector_construct_helper_fn, const _Kty_tmp&, const _Ty_tmp_copyable&>)
        {
            return (*this)(val.first, val.second);
        }
    };

    template <class _Kty, class _Ty>
    struct _Hashed_vector_construct_helper_fn<false, _Kty, _Ty>
    {
        using value_type = _Ty;

        template <typename ..._Ty_args>
        value_type operator ()(_Ty_args&&...args) const requires(std::constructible_from<value_type, decltype(args)...>)
        {
            return value_type(FWD(args)...);
        }

        /*value_type operator( )(value_type&& obj) const
        {
            return std::move(obj);
        }*/
    };

#pragma endregion

#pragma region Hashed_vector_construct_helper_wrapped_fn

    template <bool _Stored_as_pair, class _Kty, class _Ty, class _Wrapper>
    struct _Hashed_vector_construct_helper_wrapped_fn;

    template <class _Kty, class _Ty, class _Wrapper>
    struct _Hashed_vector_construct_helper_wrapped_fn<false, _Kty, _Ty, _Wrapper>
    {
        static constexpr auto wrapper = _Wrapper( );

        using _Ty_wrapped = typename _Wrapper::object_type;
        using value_type = _Ty_wrapped;

        template <typename ..._Ty_args>
        value_type operator ()(_Ty_args&&...args) const requires(std::is_invocable_r_v<_Ty_wrapped, _Wrapper, decltype(args)...>)
        {
            _Ty_wrapped result = wrapper(FWD(args)...);
            return std::move(result);
        }

        /*value_type operator( )(value_type&& obj) const
        {
            return std::move(obj);
        }*/
    };

    template <class _Kty, class _Ty, class _Wrapper>
    struct _Hashed_vector_construct_helper_wrapped_fn<true, _Kty, _Ty, _Wrapper>
    {
        using _Ty_constructor = _Hashed_vector_construct_helper_wrapped_fn<false, _Kty, _Ty, _Wrapper>;
        static constexpr auto wrapper = _Ty_constructor( );

        using _Ty_wrapped = typename _Wrapper::object_type;
        using value_type = std::pair<_Kty, _Ty_wrapped>;

        template <typename _Kty_tmp, typename ..._Ty_args>
        value_type operator ()(_Kty_tmp&& key, _Ty_args&&...args) const requires(
            std::constructible_from<_Kty, decltype(key)> && std::is_invocable_r_v<_Ty_wrapped, _Wrapper, decltype(args)...>)
        {
            _Kty        first  = _Kty(FWD(key));
            _Ty_wrapped second = wrapper(FWD(args)...);
            return value_type(std::move(first), std::move(second));
        }

        template <typename _Kty_tmp, typename _Ty_tmp>
        value_type operator( )(std::pair<_Kty_tmp, _Ty_tmp>&& data) const requires(std::invocable<
            _Hashed_vector_construct_helper_wrapped_fn, _Kty_tmp&&, _Ty_tmp&&>)
        {
            return (*this)(std::move(data.first), std::move(data.second));
        }

        template <typename _Kty_tmp, typename _Ty_tmp>
        value_type operator( )(const std::pair<_Kty_tmp, _Ty_tmp>& data) const requires(std::invocable<
            _Hashed_vector_construct_helper_wrapped_fn, const _Kty_tmp&, const _Ty_tmp&>)
        {
            return (*this)(data.first, data.second);
        }

        value_type operator( )(value_type&& obj) const
        {
            return std::move(obj);
        }
    };

#pragma endregion

    template <typename _Ty, class _Dx/*= std::default_delete<_Ty>*/>
    struct _Unique_ptr
    {
        using object_type = std::unique_ptr<_Ty, _Dx>;

        template <class ..._Ty_args>
        object_type operator( )(_Ty_args&&...args) const requires(std::constructible_from<_Ty, decltype(args)...>)
        {
            return std::make_unique<_Ty>(FWD(args)...);
        }

        object_type operator( )(object_type&& obj) const
        {
            return std::move(obj);
        }
    };

    template <typename _Ty>
    struct _Shared_ptr
    {
        using object_type = std::shared_ptr<_Ty>;

        template <class ..._Ty_args>
        object_type operator( )(_Ty_args&&...args) const requires(std::constructible_from<_Ty, decltype(args)...>)
        {
            return std::make_shared<_Ty>(FWD(args)...);
        }

        object_type operator( )(object_type&& obj) const
        {
            return std::move(obj);
        }
    };

    template <bool _Stored_as_pair, class _Kty, class _Ty, class _Dx = std::default_delete<_Ty>>
    struct _Hashed_vector_construct_helper_unique_fn: _Hashed_vector_construct_helper_wrapped_fn<_Stored_as_pair, _Kty, _Ty, _Unique_ptr<_Ty, _Dx> >
    {
        template <std::_Not_same_as<_Dx> _Dx_new>
        using switch_deleter = _Hashed_vector_construct_helper_unique_fn<_Stored_as_pair, _Kty, _Ty, _Dx_new>;
    };
    template <typename _Dx>
    struct _Hashed_vector_construct_helper_unique_builder
    {
        template <bool _Stored_as_pair, class _Kty, class _Ty>
        using unwrap = _Hashed_vector_construct_helper_unique_fn<_Stored_as_pair, _Kty, _Ty, _Dx>;
    };

    template <bool _Stored_as_pair, class _Kty, class _Ty>
    struct _Hashed_vector_construct_helper_shared_fn: _Hashed_vector_construct_helper_wrapped_fn<_Stored_as_pair, _Kty, _Ty, _Shared_ptr<_Ty> > {};

#define UTL_HASHED_VEC_DEBUG

    template <class _Kty, class _Ty,
              template <bool _Stored_as_pair, class _Key, class _Type> typename _Construct_fn = _Hashed_vector_construct_helper_fn,
              bool _Store_as_pair = !std::derived_from<_Ty, _Kty> && !std::convertible_to<_Ty, _Kty>,
              template <bool _Stored_as_pair, class _Key, class _Type> typename _Hash_getter_fn = _Hashed_vector_hash_fn,
              template <bool _Stored_as_pair, class _Key, class _Type> typename _Value_getter_fn = _Hashed_vector_get_value_fn>
    class hashed_vector
    {
#ifdef UTL_HASHED_VEC_DEBUG
    public:
#endif

        // ReSharper disable CppInconsistentNaming

        static constexpr auto _Get_hash    = _Hash_getter_fn<_Store_as_pair, _Kty, _Ty>( );
        static constexpr auto _Get_value   = _Value_getter_fn<_Store_as_pair, _Kty, _Ty>( );
        static constexpr auto _Constructor = _Construct_fn<_Store_as_pair, _Kty, _Ty>( );

    public:
        using _Get_hash_fn_t = decltype(_Get_hash);
        using _Get_value_fn_t = decltype(_Get_value);
        using _Construct_fn_t = decltype(_Constructor);

        using value_type = typename _Construct_fn_t::value_type;

#ifndef UTL_HASHED_VEC_DEBUG
    private:
#endif

        using container_type = std::vector<value_type>;
        container_type container_;

        // ReSharper restore CppInconsistentNaming

        template <typename _Ty_test>
        static auto _Get_value_ptr(_Ty_test&& val)
        {
            auto& ref = _Get_value(val);
            return std::addressof(ref);
        }

    public:
        hashed_vector(size_t wish_size = 0) : container_( )
        {
            if (wish_size > 0)
                container_.reserve(wish_size);
        }

        void reserve(size_t wish_size) { container_.reserve(wish_size); }
        void shrink_to_fit( ) { container_.shrink_to_fit( ); }

        bool   empty( ) const { return container_.empty( ); }
        size_t size( ) const { return container_.size( ); }

        void clear( ) { container_.clear( ); }

#ifndef UTL_HASHED_VEC_DEBUG
    private:
#endif

        template <typename _Ty_cont, bool _Unwrap = true>
        class _Find_fn
        {
#ifdef UTL_HASHED_VEC_DEBUG
        public:
#endif
            struct _Tag_unwrap: _Tag_number<0> {};
            struct _Tag_equal: _Tag_number<1> {};
            struct _Tag_equal_inversed: _Tag_number<2> {};

            struct _Tag
            {
                template <typename _Kty_test>
                struct select
                {
                    constexpr auto operator()( ) const
                    {
                        if constexpr (std::invocable<_Get_hash_fn_t, _Kty_test>)
                            return _Tag_unwrap( );
                        else if constexpr (std::_Half_equality_comparable<_Kty, _Kty_test>)
                            return _Tag_equal( );
                        else if constexpr (std::_Half_equality_comparable<_Kty_test, _Kty>)
                            return _Tag_equal_inversed( );
                        else
                            return _Tag_unknown( );
                    }
                };
            };

            _Ty_cont& container_;

            template <typename _Ty_item>
            static auto _Get_item(_Ty_item&& item)
            {
                if constexpr (_Unwrap)
                    return _Get_value_ptr(item);
                else
                    return std::addressof(item);
            }

        public:
            using return_type = decltype(_Get_item(container_[0]));

            _Find_fn(_Ty_cont& container) : container_(container) {}

            template <_Tag_check<typename _Tag::select, _Tag_unwrap> _Kty_test>
            return_type operator()(_Kty_test&& val) const
            {
                decltype(auto) val_hash = _Get_hash(val);
                for (auto& item: container_)
                {
                    if (_Get_hash(item) == val_hash)
                        return _Get_item(item);
                }
                return nullptr;
            }

            template <_Tag_check<typename _Tag::select, _Tag_equal> _Kty_test>
            return_type operator()(_Kty_test&& val) const
            {
                for (auto& item: container_)
                {
                    if (_Get_hash(item) == val)
                        return _Get_item(item);
                }
                return nullptr;
            }

            template <_Tag_check<typename _Tag::select, _Tag_equal_inversed> _Kty_test>
            return_type operator()(_Kty_test&& val) const
            {
                for (auto& item: container_)
                {
                    if (val == _Get_hash(item))
                        return _Get_item(item);
                }
                return nullptr;
            }
        };

        template <typename _Ty_cont>
        class _At_fn
        {
#ifdef UTL_HASHED_VEC_DEBUG
        public:
#endif
            using _Find_fn_t = _Find_fn<_Ty_cont>;

            _Find_fn_t find_obj_;
        public:

            using return_type = std::remove_pointer_t<typename _Find_fn_t::return_type>&;

            _At_fn(_Ty_cont& container) : find_obj_(container) {}

            template <typename _Kty_test>
            return_type operator()(_Kty_test&& val) const requires(std::invocable<_Find_fn_t, /*detail::_Rvalue_ignored<*/_Kty_test/*>*/>)
            {
                auto found = find_obj_(val);
                if (found == nullptr)
                    std::_Xout_of_range("invalid data tag!");
                return *found;
            }
        };

    public:

        template <class _Kty_test, std::invocable<_Kty_test> _Find = _Find_fn<const container_type&, false>>
        auto find_raw(_Kty_test&& key) const
        {
            auto find_obj = _Find(container_);
            return find_obj(key);
        }

        template <class _Kty_test, std::invocable<_Kty_test> _Find = _Find_fn<container_type&, false>>
        auto find_raw(_Kty_test&& key)
        {
            auto find_obj = _Find(container_);
            return find_obj(key);
        }

        template <class _Kty_test, std::invocable<_Kty_test> _Find = _Find_fn<const container_type&>>
        auto find(_Kty_test&& key) const
        {
            auto find_obj = _Find(container_);
            return find_obj(key);
        }

        template <class _Kty_test, std::invocable<_Kty_test> _Find = _Find_fn<container_type&>>
        auto find(_Kty_test&& key)
        {
            auto find_obj = _Find(container_);
            return find_obj(key);
        }

        template <class _Kty_test, std::invocable<_Kty_test> _At = _At_fn<const container_type&>>
        auto& at(_Kty_test&& key) const
        {
            auto at_obj = _At(container_);
            return at_obj(key);
        }

        template <class _Kty_test, std::invocable<_Kty_test> _At = _At_fn<container_type&>>
        auto& at(_Kty_test&& key)
        {
            auto at_obj = _At(container_);
            return at_obj(key);
        }

        template <class _Kty_test>
        auto find_safe(_Kty_test&& key) const
        {
#ifdef _DEBUG
            const auto stored = this->find(key);
            if (stored == nullptr)
                _STL_REPORT_ERROR("Key not found");
            return stored;
#else
            return std::addressof(this->at(key));
#endif
        }

        template <class _Kty_test>
        auto find_safe(_Kty_test&& key)
        {
#ifdef _DEBUG
            const auto stored = this->find(key);
            if (stored == nullptr)
                _STL_REPORT_ERROR("Key not found");
            return stored;
#else
            return std::addressof(this->at(key));
#endif
        }

        template <class _Kty_test>
        bool contains(_Kty_test&& key) const
        {
            return this->find(key) != nullptr;
        }

        template <class _Kty_test>
        bool erase(_Kty_test&& key)
        {
            auto find_obj = this->find_raw(key);
            auto result   = find_obj(key);
            if (!result)
                return false;

            auto begin     = container_.begin( );
            auto begin_ptr = std::addressof(*begin);

            auto offset = std::distance(begin_ptr, result);
            auto itr    = begin + offset;
            container_.erase(itr);

            return true;
        }

#ifndef UTL_HASHED_VEC_DEBUG
    private:
#endif

        template <typename _Ty_add>
        auto& _Emplace(_Ty_add&& obj) requires(std::constructible_from<value_type, decltype(obj)>)
        {
            auto& placed = container_.emplace_back(FWD(obj));
            return _Get_value(placed);
        }

    public:

        template <typename ..._Kty_TyArgs>
        auto& emplace(_Kty_TyArgs&&...args) requires(std::invocable<_Construct_fn_t, decltype(args)...>)
        {
            auto temp = _Constructor(FWD(args)...);

#if _CONTAINER_DEBUG_LEVEL >= 1 || defined(UTL_HASHED_VEC_DEBUG)
            const auto stored = this->find(temp);
            if (stored != nullptr)
                _STL_REPORT_ERROR("Duplicate detected");
#endif
            return this->_Emplace(std::move(temp));
        }

#ifndef UTL_HASHED_VEC_DEBUG
    private:
#endif
        template <typename ..._Kty_TyArgs>
        auto& _Construct_in_place(_Kty_TyArgs&&...args) requires(std::invocable<_Construct_fn_t, decltype(args)...>)
        {
            return this->_Emplace(_Constructor(FWD(args)...));
        }

    public:

        template <std::_Weakly_equality_comparable_with<_Kty> _Kty_tmp, typename ..._Ty_args>
        auto& try_emplace(_Kty_tmp&& test, _Ty_args&&...args)
        {
            //warning! if value not found this sometimes calculate hash twice! ///UNTESTED

            auto stored = this->find(test);
            if (stored != nullptr)
                return *stored;

            if constexpr (std::invocable<_Construct_fn_t, _Kty_tmp, _Ty_args...>)
            {
                static_assert(!std::invocable<_Construct_fn_t, _Ty_args...>, "Multiple constructors available");
                return this->_Construct_in_place(std::move(test), FWD(args)...);
            }
            else
            {
                return this->_Construct_in_place(FWD(args)...);
            }
        }

        void push(value_type&& obj)
        {
            this->emplace(std::move(obj));
        }

        void push(const value_type& obj) requires(std::copyable<value_type>)
        {
            this->emplace(obj);
        }

        auto iterate( ) { return std::span(container_.data( ), container_.size( )); }
        auto iterate( ) const { return std::span(container_.data( ), container_.size( )); }
    };
}
