#pragma once
#include "hashed string.h"

namespace utl::old
{

    //hashed_vector created. this is uselles now

    namespace hashed_string_vec_data
    {
        template <typename T>
        concept have_hash_outside = requires(T&& val) { val.first.hash( ); };

        //template <typename T>
        //concept have_hash_inside = requires(T&& val) { val.hash( ); };

        template <typename T>
        concept have_hash_inside/*_hidden*/ = std::is_class_v<std::remove_reference_t<T> > && std::convertible_to<T, hashed_string_tag>;

        template <typename T>
        concept maybe_pointer = requires(T&& val)
        {
            *val;
        };

        // _RANGES 

#if 0
        template <typename T>
        struct _Get_hash;

        template <have_hash_outside T>
        struct _Get_hash<T>
        {
            auto operator()(const T& val) const { return val.first.hash( ); }
        };

        /*template <have_hash_inside T>
        struct _Get_hash<T>
        {
            auto operator()(const T& val) const { return val.hash( ); }
        };*/

        template <have_hash_inside_hidden T>
        struct _Get_hash<T>
        {
            auto operator()(const T& val) const { return hashed_string_tag(val).hash( ); }
        };

        template <maybe_pointer T>
        struct _Get_hash<T> //: _Get_hash<std::remove_cvref_t<decltype(*std::declval<T>( ))> >
        {
            auto operator()(const T& val) const
            {
                auto& unpacked = *val;
                using val_t = std::remove_cvref_t<decltype(unpacked)>;
                static constexpr auto fn = _Get_hash<val_t>{ };
                return fn(unpacked);
                /*if constexpr (have_hash_inside_hidden<val_t> && have_hash_inside<val_t>)
                    return unpacked.hash( );
                else
                    std::invoke(_Get_hash<std::remove_cvref_t<decltype(*std::declval<T>( ))> >(),unpacked);*/
                //return (*this)(unpacked);
            }
        };
#endif
        struct _Get_hash
        {
            template <have_hash_outside T>
            auto operator()(const T& val) const { return val.first.hash( ); }

            //template <have_hash_inside T>
            //auto operator()(const T& val) const { return val.hash( ); }

            template <have_hash_inside T>
            auto operator()(const T& val) const { return hashed_string_tag(val).hash( ); }

            template <maybe_pointer T>
            auto operator()(const T& val) const
            {
                auto& unpacked = *val;
                using val_t = std::remove_cvref_t<decltype(unpacked)>;
                if constexpr (have_hash_inside<val_t>) //awoid  operator* overloading
                    return unpacked.hash( );
                else
                    return (*this)(unpacked);
            }
        };
        struct _Get_text
        {
            template <have_hash_outside T>
            decltype(auto) operator()(const T& val) const { return val.first.text( ); }

            template <have_hash_inside T>
            decltype(auto) operator()(const T& val) const
            {
                if constexpr (std::derived_from<T, hashed_string_tag>)
                    return val.text( );
                else
                    return utl::to_string_view(val);
            }

            /*template <have_hash_inside_hidden T>
            decltype(auto) operator()(const T& val) const
            {
                if constexpr (std::convertible_to<T, hashed_basic_string_view<char> >)
                    return hashed_basic_string_view<char>(val).text( );
                else if constexpr (std::convertible_to<T, hashed_basic_string_view<wchar_t> >)
                    return hashed_basic_string_view<wchar_t>(val).text( );
                else
                {
                    static_assert(std::_Always_false<T>, __FUNCTION__": add extra type!");
                }
            }*/

            /*template <have_hash_outside_ptr T>
            decltype(auto) operator()(const T& val) const { return val->first.text( ); }

            template <have_hash_inside_ptr T>
            decltype(auto) operator()(const T& val) const { return val->text( ); }*/

            template <maybe_pointer T>
            decltype(auto) operator()(const T& val) const { return (*this)(*val); }
        };
        struct _Get_val
        {
            template <have_hash_outside T>
            auto& operator()(T&& val) const { return val.second; }

            template <typename T>
            requires(have_hash_inside<T> /*|| have_hash_inside_hidden<T>*/)
            auto& operator()(T&& val) const { return val; }

            /*template <have_hash_outside_ptr T>
            auto& operator()(T&& val) const { return val->second; }

            template <have_hash_inside_ptr T>
            auto& operator()(T&& val) const { return *val; }*/

            template <maybe_pointer T>
            decltype(auto) operator()(const T& val) const { return (*this)(*val); }
        };

        template <typename T>
        auto& unpack(T&& val)
        {
            if constexpr (/*have_hash_inside_ptr<T> || have_hash_outside_ptr<T>*/maybe_pointer<T>)
                return *val;
            else
                return val;
        }

        template <typename T>
        decltype(auto) unpack_ptr(T&& val)
        {
            return std::addressof(unpack(val));
        }

        template <have_hash_outside T, typename S, typename ...Args>
        auto create(S&& name, Args&&...args)
        {
            using first_type = typename T::first_type;
            using second_type = typename T::second_type;

            return std::make_pair(first_type(FWD(name)), second_type(FWD(args)...));
        }

        template <typename T, typename ...Args>
        requires(have_hash_inside<T> /*|| have_hash_inside_hidden<T>*/)
        auto create(Args&&...args)
        {
            return T(FWD(args)...);
        }

        template <maybe_pointer T, typename ...Args>
        requires(!std::is_abstract_v<typename T::element_type>)
        auto create(Args&&...args)
        {
            return std::make_unique<T::element_type>(FWD(args)...);
        }

        /*template <have_hash_inside_ptr P, typename T, typename ...Args>
         requires(std::is_abstract_v<typename P::element_type>)
         decltype(auto) create(Args&&...args)
         {
             return std::make_unique<T>(FWD(args)...);
         }*/
    }

    template <typename Vata_type>
    class hashed_string_vec
    {
    public:
        //using container_type = std::vector<std::pair<hashed_string, T> >;
        using container_type = std::vector<Vata_type>;

        hashed_string_vec(size_t wish_size = 0) : container_( )
        {
            if (wish_size > 0)
                container_.reserve(wish_size);
        }

        void reserve(size_t wish_size)
        {
            container_.reserve(wish_size);
        }

        void shrink_to_fit( )
        {
            container_.shrink_to_fit( );
        }

    private:

        // ReSharper disable CppInconsistentNaming

        template <typename T, bool is_const>
        class _Unpacker_view_impl;

        template <typename T>
        class _Unpacker_view_impl<T, true>
        {
        public:

            _Unpacker_view_impl(const T& ref) : ref_(ref) {}

            auto get_hash( ) const -> decltype(auto) { return get_hash__(this->ref( )); }
            auto get_text( ) const -> decltype(auto) { return get_text__(this->ref( )); }
            auto get_val( ) const -> decltype(auto) { return get_val__(this->ref( )); }

        protected:

            auto ref( ) const -> const T& { return ref_; }
            auto ref( ) -> T& { return const_cast<T&>(ref_); }

            const T& ref_;
        };

        template <typename T>
        class _Unpacker_view_impl<T, false>: public _Unpacker_view_impl<T, true>
        {
        public:

            _Unpacker_view_impl(T& ref) : _Unpacker_view_impl<T, true>(ref) {}

            auto get_val( ) -> decltype(auto) { return get_val__(this->ref( )); }
        };

        struct _Unpacker_view
        {
            template <typename T>
            auto operator()(T& val) const
            {
                return _Unpacker_view_impl<T, false>(val);
            }

            template <typename T>
            auto operator()(const T& val) const
            {
                return _Unpacker_view_impl<T, true>(val);
            }
        };
        inline static constexpr _Unpacker_view unpacker_view__{ };

        /*struct _Get_hash_fn
        {
            template <typename T>
            decltype(auto) operator()(const T& val) const
            {
                using hash_fn = hashed_string_vec_data::_Get_hash<T>;
                return std::invoke(hash_fn( ), val);
            }
        };*/

        inline static constexpr hashed_string_vec_data::_Get_hash get_hash__{ };
        inline static constexpr hashed_string_vec_data::_Get_text get_text__{ };
        inline static constexpr hashed_string_vec_data::_Get_val  get_val__{ };

        // ReSharper restore CppInconsistentNaming

        template <typename Q>
        static auto _Find(const hashed_string_tag& tag, Q&& data) -> decltype(hashed_string_vec_data::unpack_ptr(data[0]))
        {
            auto found = _RANGESfind(data, tag, get_hash__);
            if (found != data.end( ))
                return hashed_string_vec_data::unpack_ptr(*found);

            return nullptr;
        }

    public:

        auto find(const hashed_string_tag& tag)
        {
            return _Find(tag, container_);
        }

        auto find(const hashed_string_tag& tag) const
        {
            return _Find(tag, container_);
        }

        bool contains(const hashed_string_tag& tag) const
        {
            return this->find(tag) != nullptr;
        }

    private:

        template <typename Q>
        static auto& _At(const hashed_string_tag& tag, Q&& data) //-> decltype(get_val__(data[0]))
        {
            auto found = _Find(tag, data);
            if (!found)
                std::_Xout_of_range("invalid data tag!");
            return get_val__(found);
        }

    public:

        auto& at(const hashed_string_tag& tag)
        {
            return _At(tag, container_);
        }

        auto& at(const hashed_string_tag& tag) const
        {
            return _At(tag, container_);
        }

        auto& operator[](const hashed_string_tag& tag) const
        {
#ifdef _DEBUG
#if _CONTAINER_DEBUG_LEVEL == 0
            return this->at(tag);
#else
            auto found = this->find(tag);
            _STL_ASSERT(found != nullptr, "Tag not found");
            return get_val__(found);
#endif
#else
            auto found = this->find(tag);
            return get_val__(found);
#endif
        }

        template <typename Q>
        auto& operator[](Q&& name)
        {
            return this->try_emplace(FWD(name));
        }

        size_t size( ) const
        {
            return container_.size( );
        }

        bool empty( ) const
        {
            return container_.empty( );
        }

        void clear( )
        {
            container_.clear( );
        }

    private:

        template <typename D = Vata_type, typename ...Args>
        static auto _Construct(Args&&...args)
        {
            return (hashed_string_vec_data::create<D>(FWD(args)...));
        }

        template <std::movable Q>
        auto& _Emplace(Q&& val)
        {
            return container_.emplace_back(FWD(val));
        }

    public:

        template <typename D = Vata_type, typename ...Args>
        auto& emplace(Args&&...args)
        {
            auto temp = _Construct<D>(FWD(args)...);

#if _CONTAINER_DEBUG_LEVEL>=1
            const auto stored = this->find(get_hash__(temp));
            if (stored != nullptr)
                _STL_REPORT_ERROR("Duplicate detected");
#endif
            auto& placed = _Emplace(std::move(temp));
            return get_val__(placed);
        }

        template <typename D = Vata_type>
        void push(D&& val)
        {
            this->template emplace<std::remove_cvref_t<D> >(FWD(val));
        }

        template <typename D = Vata_type, typename N, typename ...Args>
        requires(std::invocable<decltype(get_text__), N>)
        auto& try_emplace(N&& name, Args&&...args)
        {
            //warning! if value not found this sometimes calculate hash twice!

            auto stored = this->find(name);
            if (stored != nullptr)
                return get_val__(stored);

            auto  temp   = _Construct<D>(FWD(args)...);
            auto& placed = _Emplace(std::move(temp));
            return get_val__(placed);
        }

    private:
        template <typename Q>
        static auto _Iterate(Q&& source)
        {
            return std::views::transform(source, unpacker_view__);
        }

    public:

        auto iterate_raw( )
        {
            Vata_type* data_ptr = container_.data( );
            return std::span{data_ptr, container_.size( )};
        }

        auto iterate_raw( ) const
        {
            const Vata_type* data_ptr = container_.data( );
            return std::span{data_ptr, container_.size( )};
        }

        auto iterate( ) { return _Iterate(container_); }
        auto iterate( ) const { return _Iterate(container_); }

        auto iterate_names( ) { return std::views::transform(this->iterate_raw( ), get_text__); }
        auto iterate_names( ) const { return std::views::transform(this->iterate_raw( ), get_text__); }

        auto iterate_values( ) { return std::views::transform(this->iterate_raw( ), get_val__); }
        auto iterate_values( ) const { return std::views::transform(this->iterate_raw( ), get_val__); }

    private:

        container_type container_;
    };
}
