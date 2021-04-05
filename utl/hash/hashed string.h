#pragma once

#include "fnv.h"

#include <string>

namespace utl
{
    class hashed_string_tag
    {
    public:

        static constexpr auto& _Compute_hash = hash::fnv1a;

        static constexpr auto unset_value = _Compute_hash(nullptr);
        using value_type = std::remove_const_t<decltype(unset_value)>;

        constexpr hashed_string_tag(value_type val) //basically for operator== only
            : hash_(val) { }

    protected:
        constexpr hashed_string_tag( ):
            hash_(_Compute_hash(nullptr)) {}

    public:

        template <string_viewable _Ty_chars_rng>
        /*explicit*/ constexpr hashed_string_tag(_Ty_chars_rng&& val) //explicit to prevent hashed_string_tag == "text"
            : hash_(_Compute_hash(val)) { }

        constexpr hashed_string_tag(const hashed_string_tag&) = default;

        constexpr bool operator==(const hashed_string_tag& other) const = default;

        _NODISCARD constexpr value_type hash( ) const
        {
            return hash_;
        }

        constexpr void _Set_hash(value_type h)
        {
            hash_ = h;
        }

    private:

        value_type hash_;
    };

    //constexpr bool operator==()

    template <character _Chr>
    class hashed_basic_string_view: public hashed_string_tag
    {
    public:

        using str_view = std::basic_string_view<_Chr>;
        using value_type = _Chr;

        constexpr hashed_basic_string_view( ) : hashed_string_tag( ), str_( ) {}

        template <string_viewable_exact<_Chr> _Ty_text>
        constexpr hashed_basic_string_view(_Ty_text&& text) : hashed_basic_string_view( )
        {
            _Set_string(to_string_view((text)));
            _Set_hash(_Compute_hash(str_));
        }

        _NODISCARD constexpr str_view text( ) const
        {
            return str_;
        }

        constexpr void _Set_string(const str_view& s)
        {
            str_ = s;
        }

    private:
        str_view str_;
    };

    using hashed_string_view = hashed_basic_string_view<char>;
    using hashed_wstring_view = hashed_basic_string_view<wchar_t>;

    template <character _Chr>
    class hashed_basic_string: public hashed_string_tag
    {
    public:
        using str_view = std::basic_string_view<_Chr>;
        using value_type = _Chr;
        using std_string = std::basic_string<_Chr>;

        static hashed_basic_string _Hash_only(const hashed_string_tag& tag)
        {
            hashed_basic_string dummy;
            dummy._Set_hash(tag.hash( ));
            return dummy;
        }

#pragma region constructors

        PREVENT_COPY(hashed_basic_string)

        hashed_basic_string(hashed_basic_string&& other) noexcept : hashed_string_tag(other), MOVE(str_)
        {
            other._Set_hash(_Compute_hash(nullptr));
        }

        hashed_basic_string& operator =(hashed_basic_string&& other) noexcept
        {
            this->_Set_hash(other.hash( ));
            this->_Set_string(std::move(other.str_));
            other._Set_hash(_Compute_hash(nullptr));
            return *this;
        }

        hashed_basic_string( ) = default;

        //idk why but sometimes std::string have different c_str() ptr after move....

        template <string_viewable_exact_no_rvalue<_Chr> _Ty_text>
        hashed_basic_string(_Ty_text&& str): hashed_basic_string( )
        {
            _Set_string(FWD(str));
            _Set_hash(_Compute_hash(str_));
        }

        hashed_basic_string(const hashed_basic_string_view<_Chr>& text) : hashed_basic_string( )
        {
            _Set_string(utl::to_string(text.text( )));
            _Set_hash(text.hash( ));
        }

#pragma endregion

        operator hashed_basic_string_view<_Chr>( ) const
        {
            hashed_basic_string_view<_Chr> ret;
            ret._Set_string(this->text( ));
            ret._Set_hash(this->hash( ));
            return ret;
        }

        const std_string& text( ) const
        {
            return str_;
        }

        template <string_viewable_exact_no_rvalue<_Chr> _Ty_other>
        void _Set_string(_Ty_other&& s)
        {
            std_string str = utl::to_string(FWD(s));
            //str.shrink_to_fit( );
            str_ = std::move(str);
        }

    private:
        std_string str_;
    };

    using hashed_string = hashed_basic_string<char>;
    using hashed_wstring = hashed_basic_string<wchar_t>;

    /*constexpr hashed_string_view a{"a"};
    constexpr hashed_string_view b{"a"};
    constexpr auto c = a == b;*/
}

_STD_BEGIN
    template <std::derived_from<utl::hashed_string_tag> T>
    struct std::hash<T>
    {
        _NODISCARD _CONSTEXPR17 size_t operator()(const T& _Keyval) const noexcept
        {
            return _Keyval.hash( );
        }
    };
#if 0
	template <utl::character C>
	struct std::hash<utl::hashed_basic_string<C> >
	{
		_NODISCARD size_t operator()(const utl::hashed_basic_string<C>& _Keyval) const noexcept
		{
			return _Keyval.hash( );
		}
	};

	template < >
	struct std::hash<utl::hashed_string_tag>
	{
		_NODISCARD size_t operator()(const utl::hashed_string_tag& _Keyval) const noexcept
		{
			return _Keyval.hash( );
		}
	};
#endif

_STD_END


