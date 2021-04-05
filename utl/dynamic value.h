#pragma once

#include "bytes_view.h"
#include "value_setter.h"

namespace utl
{
    template <std::default_initializable _Ty>
    class dynamic_value
    {
        void _Fill_last( )
        {
            last_value_   = current_value_;
            auto last_val = _Bytes_view((last_value_));

            for (auto& v: last_val)
                v += 1;

            ///ranges abuse
            //_RANGES for_each(last_val, [](uint8_t& val) { val += 1; });
        }

    public:
        dynamic_value( )
        {
            auto new_val = _Bytes_view(current_value_);
            _RANGES fill(new_val, static_cast<uint8_t>(0)); //ranges call memset so no abuse here
            _Fill_last( );
        }

        dynamic_value(_Ty&& def_value) : current_value_(std::move(def_value))
        {
            _Fill_last( );
        }

        dynamic_value(const _Ty& def_value) : current_value_(def_value)
        {
            _Fill_last( );
        }

        PREVENT_COPY_FORCE_MOVE(dynamic_value);

    private:

        template <typename _Ty_tmp>
        void _Set_value(_Ty_tmp&& val)
        {
            last_value_    = std::move(current_value_);
            current_value_ = _Ty(FWD(val));
        }

        /*void _Make_unchanged( )
        {
            if (!changed_)
                return;

            last_value_ = current_value_;
            changed_    = false;

            changed_ = false;
        }*/

        class _Lock_helper
        {
        public:
            _Lock_helper(dynamic_value* owner): owner_(owner)
            {
                ++this->owner_->locks_;
            }

            ~_Lock_helper( )
            {
                --this->owner_->locks_;
            }

            _Lock_helper(const _Lock_helper& other): COPY(owner_)
            {
                ++this->owner_->locks_;
            }

            _Lock_helper& operator==(const _Lock_helper&) = delete;

            bool operator()( ) const
            {
                return this->owner_->locks_ > 1;
            }

        private:

            dynamic_value* owner_;
        };

        class _Set_helper
        {
        public:

            _Set_helper(dynamic_value* owner): owner_(owner)
            {
                ++owner_->setters_;
            }

            ~_Set_helper( )
            {
                --owner_->setters_;
                if (owner_->setters_ > 0)
                    return;

                this->owner_->changed_ = called_;
            }

            _Set_helper(const _Set_helper& other):COPY(owner_), COPY(called_)
            {
                ++owner_->setters_;
            }

            _Set_helper& operator==(const _Set_helper&) = delete;

            void operator()(_Ty&& temp_value)
            {
                this->owner_->_Set_value(FWD(temp_value));
                called_ = true;
            }

        private:
            dynamic_value* owner_  = nullptr;
            bool           called_ = false;
        };

    public:

        _NODISCARD setter_native<_Ty> new_value( )
        {
            return {_Lock_helper(this), current_value_, _Set_helper(this)};
        }

        template <typename _Ty_tmp>
        bool set_new_value(_Ty_tmp&& new_val) requires(std::constructible_from<_Ty, _Ty_tmp>)
        {
            _STL_ASSERT(locks_ == 0, "Updater is locked.");

            if (current_value_ == new_val)
                changed_ = false;
            else
            {
                _Set_value(FWD(new_val));
                changed_ = true;
            }
            return changed_;
        }

        const _Ty& last( ) const
        {
            return this->last_value_;
        }

        const _Ty& current( ) const
        {
            return this->current_value_;
        }

        bool changed( ) const
        {
            return changed_;
        }

    private:
        _Ty last_value_;
        _Ty current_value_;

        bool   changed_ = true;
        size_t locks_   = 0, setters_ = 0;
    };
}
