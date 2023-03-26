/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2023 Denis Corbin
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// to contact the author, see the AUTHOR file
/*********************************************************************/

    /// \file limitint.hpp
    /// \brief the reviewed implementation of infinint based on system limited integers
    /// \ingroup API
    ///
    /// the limitint template class implementation defined in this module can
    /// handle positive integers and detect overflow. It shares with infinint the same
    /// interface, so it can be use in place of it, but throw Elimitint exceptions if
    /// overflow is detected.


#ifndef LIMITINT_HPP
#define LIMITINT_HPP

#include "../my_config.h"

extern "C"
{
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_STRINGS_H
#include <strings.h>
#endif
} // end extern "C"

#include <typeinfo>
#include "integers.hpp"
#include "erreurs.hpp"
#include "int_tools.hpp"
#include "proto_generic_file.hpp"


#define ZEROED_SIZE 50

namespace libdar
{

	/// \addtogroup API
	/// @{

	/// limitint template class
	/// \ingroup Private
	///
	/// the limitint template class implementation can
	/// handle positive integers and detect overflow. It shares with infinint the same
	/// interface, so it can be use in place of it, but throw Elimitint exceptions if
	/// overflow is detected.
	/// this template class receive as argument a positive integer atomic type
	/// In particular it is assumed that the sizeof() operator gives the amount of
	/// byte of information that this type can handle, and it is also assumed that
	/// the bytes of information are contiguous.
	/// \note if you just want to convert an infinint
	/// integer to its decimal representation see the
	/// class libdar::deci


    template<class B> class limitint
    {
    public :

#if SIZEOF_OFF_T > SIZEOF_TIME_T
#if SIZEOF_OFF_T > SIZEOF_SIZE_T
        limitint(off_t a = 0)
	{ limitint_from(a); };
#else
        limitint(size_t a = 0)
	{ limitint_from(a); };
#endif
#else
#if SIZEOF_TIME_T > SIZEOF_SIZE_T
        limitint(time_t a = 0)
	{ limitint_from(a); };
#else
        limitint(size_t a = 0)
	{ limitint_from(a); };
#endif
#endif

	    // read an limitint from a file
	limitint(proto_generic_file & x);

	limitint(const limitint & ref) = default;
	limitint(limitint && ref) noexcept = default;
	limitint & operator = (const limitint & ref) = default;
	limitint & operator = (limitint && ref) noexcept = default;

	    // for coherent footprint with real infinint
	~limitint() = default;

        void dump(proto_generic_file &x) const; // write byte sequence to file
        void read(proto_generic_file &f) { build_from_file(f); };

        limitint & operator += (const limitint & ref);
        limitint & operator -= (const limitint & ref);
        limitint & operator *= (const limitint & ref);
	template <class T> limitint power(const T & exponent) const;
        limitint & operator /= (const limitint & ref);
        limitint & operator %= (const limitint & ref);
	limitint & operator &= (const limitint & ref);
	limitint & operator |= (const limitint & ref);
	limitint & operator ^= (const limitint & ref);
        limitint & operator >>= (U_32 bit);
        limitint & operator >>= (limitint bit);
        limitint & operator <<= (U_32 bit);
        limitint & operator <<= (limitint bit);
        limitint operator ++(int a)
	{ limitint ret = *this; ++(*this); return ret; };
        limitint operator --(int a)
	{ limitint ret = *this; --(*this); return ret; };
        limitint & operator ++()
	{ return *this += 1; };
        limitint & operator --()
	{ return *this -= 1; };

        U_32 operator % (U_32 arg) const;

            // increment the argument up to a legal value for its storage type and decrement the object in consequence
            // note that the initial value of the argument is not ignored !
            // when the object is null the value of the argument stays the same as before
        template <class T>void unstack(T &v)
	{ limitint_unstack_to(v); }

	limitint get_storage_size() const;
	    // it returns number of byte of information necessary to store the integer

	unsigned char operator [] (const limitint & position) const;
	    // return in little endian order the information bytes storing the integer

	bool is_zero() const { return field == 0; };

        bool operator < (const limitint &x) const { return field < x.field; };
        bool operator == (const limitint &x) const { return field == x.field; };
        bool operator > (const limitint &x) const { return field > x.field; };
        bool operator <= (const limitint &x) const { return field <= x.field; };
        bool operator != (const limitint &x) const { return field != x.field; };
        bool operator >= (const limitint &x) const { return field >= x.field; };
	static bool is_system_big_endian();

        B debug_get_max() const { return max_value; };
        B debug_get_bytesize() const { return bytesize; };
	B debug_get_field() const { return field; };

    private :

        B field;

        void build_from_file(proto_generic_file & x);
        template <class T> void limitint_from(T a);
	template <class T> T max_val_of(T x);
        template <class T> void limitint_unstack_to(T &a);

            /////////////////////////
            // static statments
            //
        static const int TG = 4;
	static const U_32 sizeof_field = sizeof(B); // number of bytes

        enum endian { big_endian, little_endian, not_initialized };
	using group = unsigned char[TG];

        static endian used_endian;
        static const U_I bytesize = sizeof(B);
        static const B max_value = ~B(0) > 0 ? ~B(0) : ~(B(1) << (bytesize*8 - 1));
	static U_8 zeroed_field[ZEROED_SIZE];

        static void setup_endian();
    };

    template <class B> U_8 limitint<B>::zeroed_field[ZEROED_SIZE];

    template <class B> limitint<B> operator + (const limitint<B> &, const limitint<B> &);
    template <class B> inline limitint<B> operator + (const limitint<B> & a, U_I b)
    { return a + limitint<B>(b); }
    template <class B> limitint<B> operator - (const limitint<B> &, const limitint<B> &);
    template <class B> inline limitint<B> operator - (const limitint<B> & a, U_I b)
    { return a - limitint<B>(b); }
    template <class B> limitint<B> operator * (const limitint<B> &, const limitint<B> &);
    template <class B> inline limitint<B> operator * (const limitint<B> & a, U_I b)
    { return a * limitint<B>(b); }
    template <class B> limitint<B> operator / (const limitint<B> &, const limitint<B> &);
    template <class B> limitint<B> operator / (const limitint<B> & a, U_I b)
    { return a / limitint<B>(b); }
    template <class B> limitint<B> operator % (const limitint<B> &, const limitint<B> &);
    template <class B> limitint<B> operator >> (const limitint<B> & a, U_32 bit);
    template <class B> limitint<B> operator >> (const limitint<B> & a, const limitint<B> & bit);
    template <class B> limitint<B> operator << (const limitint<B> & a, U_32 bit);
    template <class B> limitint<B> operator << (const limitint<B> & a, const limitint<B> & bit);
    template <class B> limitint<B> operator & (const limitint<B> & a, U_32  bit);
    template <class B> limitint<B> operator & (const limitint<B> & a, const limitint<B> & bit);
    template <class B> limitint<B> operator | (const limitint<B> & a, U_32  bit);
    template <class B> limitint<B> operator | (const limitint<B> & a, const limitint<B> & bit);
    template <class B> limitint<B> operator ^ (const limitint<B> & a, U_32  bit);
    template <class B> limitint<B> operator ^ (const limitint<B> & a, const limitint<B> & bit);

    template <class T> inline void euclide(T a, T b, T & q, T &r)
    {

        q = a/b; r = a%b;
    }

    template <class B> inline void euclide(limitint<B> a, U_I b, limitint<B> & q, limitint<B> &r)
    {
        euclide(a, limitint<B>(b), q, r);
    }

#ifndef INFININT_BASE_TYPE
#error INFININT_BASE_TYPE not defined cannot instantiate template
#else
    using infinint = limitint<INFININT_BASE_TYPE>;
#endif
} // end of namespace
    ///////////////////////////////////////////////////////////////////////
    /////////  template implementation ////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////

namespace libdar
{

    template <class B> typename limitint<B>::endian limitint<B>::used_endian = not_initialized;


    template <class B> limitint<B>::limitint(proto_generic_file & x)
    {
	build_from_file(x);
    }


    template <class B> void limitint<B>::build_from_file(proto_generic_file & x)
    {
        unsigned char a;
        bool fin = false;
        limitint<B> skip = 0;
        char *ptr = (char *)&field;
        S_I lu;
        int_tools_bitfield bf;

        while(!fin)
        {
            lu = x.read((char *)&a, 1);

            if(lu <= 0)
                throw Erange("limitint::build_from_file(proto_generic_file)", gettext("Reached end of file before all data could be read"));

            if(a == 0)
                ++skip;
            else // end of size field
            {
                    // computing the size to read
                U_I pos = 0;

                int_tools_expand_byte(a, bf);
                for(S_I i = 0; i < 8; ++i)
                    pos += bf[i];
                if(pos != 1)
                    throw Erange("limitint::build_from_file(proto_generic_file)", gettext("Badly formed \"infinint\" or not supported format")); // more than 1 bit is set to 1

                pos = 0;
                while(bf[pos] == 0)
                    ++pos;
                pos += 1; // bf starts at zero, but bit zero means 1 TG of length

                skip *= 8;
                skip += pos;
                skip *= TG;

                if(skip.field > bytesize)
                    throw Elimitint();

                field = 0; // important to also clear "unread" bytes by this call
                lu = x.read(ptr, skip.field);

                if(used_endian == not_initialized)
                    setup_endian();
                if(used_endian == little_endian)
                    int_tools_swap_bytes((unsigned char *)ptr, skip.field);
                else
                    field >>= (bytesize - skip.field)*8;
                fin = true;
            }
        }
    }


    template <class B> void limitint<B>::dump(proto_generic_file & x) const
    {
        B width = bytesize;
        B pos;
        unsigned char last_width;
        B justification;
        S_I direction = +1;
        unsigned char *ptr, *fin;


        if(used_endian == not_initialized)
            setup_endian();

        if(used_endian == little_endian)
        {
            direction = -1;
            ptr = (unsigned char *)(&field) + (bytesize - 1);
            fin = (unsigned char *)(&field) - 1;
        }
        else
        {
            direction = +1;
            ptr = (unsigned char *)(&field);
            fin = (unsigned char *)(&field) + bytesize;
        }

        while(ptr != fin && *ptr == 0)
        {
            ptr += direction;
            --width;
        }
        if(width == 0)
            width = 1; // minimum size of information is 1 byte

            // "width" is the informational field size in byte
            // TG is the width in TG, thus the number of bit that must have
            // the preamble
        euclide(width, (const B)(TG), width, justification);
        if(justification != 0)
                // in case we need to add some bytes to have a width multiple of TG
            ++width;  // we need then one more group to have a width multiple of TG

        euclide(width, (const B)(8), width, pos);
        if(pos == 0)
        {
            width--; // division is exact, only last bit of the preambule is set
            last_width = 0x80 >> 7;
                // as we add the last byte separately width gets shorter by 1 byte
        }
        else // division non exact, the last_width (last byte), make the rounding
        {
            U_16 pos_s = (U_16)(0xFFFF & pos);
            last_width = 0x80 >> (pos_s - 1);
        }

            // now we write the preamble except the last byte. All these are zeros.

        while(width != 0)
	    if(width > ZEROED_SIZE)
	    {
		x.write((char *)zeroed_field, ZEROED_SIZE);
		width -= ZEROED_SIZE;
	    }
	    else
	    {
		x.write((char *)zeroed_field, width);
		width = 0;
	    }

            // now we write the last byte of the preambule, which has only one bit set

        x.write((char *)&last_width, 1);

            // we need now to write some justification byte to have an informational field multiple of TG

        if(justification != 0)
        {
            justification = TG - justification;
	    if(justification > ZEROED_SIZE)
		throw SRC_BUG;
	    else
                x.write((char *)zeroed_field, justification);
        }

            // now we continue dumping the informational bytes:
        if(ptr == fin) // field is equal to zero
            x.write((char *)zeroed_field, 1);
        else // we have some bytes to write down
            while(ptr != fin)
            {
                x.write((char *)ptr, 1);
		ptr += direction;
            }
    }

    template<class B> limitint<B> & limitint<B>::operator += (const limitint & arg)
    {
        B res = field + arg.field;
        if(res < field || res < arg.field)
            throw Elimitint();
        else
            field = res;

        return *this;
    }

    template <class B> limitint<B> & limitint<B>::operator -= (const limitint & arg)
    {
        if(field < arg.field)
            throw Erange("limitint::operator", gettext("Subtracting an \"infinint\" greater than the first, \"infinint\" cannot be negative"));

            // now processing the operation

        field -= arg.field;
        return *this;
    }


    template <class B> limitint<B> & limitint<B>::operator *= (const limitint & arg)
    {
        static const B max_power = bytesize*8 - 1;

        B total = int_tools_higher_power_of_2(field) + int_tools_higher_power_of_2(arg.field) + 1; // for an explaination about "+2" see NOTES
        if(total > max_power) // this is a bit too much restrictive, but unless remaking bit by bit, the operation,
                // I don't see how to simply (and fast) know the result has not overflowed.
                // of course, it would be fast and easy to access the CPU flag register to check for overflow,
                // but that would not be portable, and unfortunately I haven't found any standart C++ expression that
                // could transparently access to it.
            throw Elimitint();

        total = field*arg.field;
        if(field != 0 && arg.field != 0)
            if(total < field || total < arg.field)
                throw Elimitint();
        field = total;
        return *this;
    }

    template <class B> template<class T> limitint<B> limitint<B>::power(const T & exponent) const
    {
	limitint ret = 1;
	for(T count = 0; count < exponent; ++count)
	    ret *= *this;

	return ret;
    }

    template <class B> limitint<B> & limitint<B>::operator /= (const limitint & arg)
    {
        if(arg == 0)
            throw Einfinint("limitint.cpp : operator /=", gettext("Division by zero"));

        field /= arg.field;
        return *this;
    }

    template <class B> limitint<B> & limitint<B>::operator %= (const limitint & arg)
    {
        if(arg == 0)
            throw Einfinint("limitint.cpp : operator %=", gettext("Division by zero"));

        field %= arg.field;
        return *this;
    }

    template <class B> limitint<B> & limitint<B>::operator >>= (U_32 bit)
    {
	if(bit >= sizeof_field*8)
	    field = 0;
	else
	    field >>= bit;
        return *this;
    }

    template <class B> limitint<B> & limitint<B>::operator >>= (limitint bit)
    {
        field >>= bit.field;
        return *this;
    }

    template <class B> limitint<B> & limitint<B>::operator <<= (U_32 bit)
    {
        if(bit + int_tools_higher_power_of_2(field) >= bytesize*8)
            throw Elimitint();
        field <<= bit;
        return *this;
    }

    template <class B> limitint<B> & limitint<B>::operator <<= (limitint bit)
    {
        if(bit.field + int_tools_higher_power_of_2(field) >= bytesize*8)
            throw Elimitint();
        field <<= bit.field;
        return *this;
    }

    template <class B> limitint<B> & limitint<B>::operator &= (const limitint & arg)
    {
        field &= arg.field;
        return *this;
    }

    template <class B> limitint<B> & limitint<B>::operator |= (const limitint & arg)
    {
        field |= arg.field;
        return *this;
    }

    template <class B> limitint<B> & limitint<B>::operator ^= (const limitint & arg)
    {
        field ^= arg.field;
        return *this;
    }

    template <class B> U_32 limitint<B>::operator % (U_32 arg) const
    {
        return U_32(field % arg);
    }

    template <class B> template <class T> void limitint<B>::limitint_from(T a)
    {
        if(sizeof(a) <= bytesize || a <= (T)(max_value))
	    field = B(a);
        else
            throw Elimitint();
    }

    template <class B> template <class T> T limitint<B>::max_val_of(T x)
    {
	x = 0;
	x = ~x;

	if(x < 1) // T is a signed integer type, we are not comparing to zero to avoid compiler warning when the template is used against unsigned integers
	{
	    x = 1;
	    x = int_tools_rotate_right_one_bit(x);
	    x = ~x;
	}

	return x;
    }

    template <class B> template <class T> void limitint<B>::limitint_unstack_to(T &a)
    {

            // T is supposed to be an unsigned "integer"
            // (ie.: sizeof returns the width of the storage bit field  and no sign bit is present)
            // Note : static here avoids the recalculation of max_T at each call
        static const T max_T = max_val_of(a);
        T step = max_T - a;

        if(field < (B)(step) && (T)(field) < step)
        {
            a += field;
            field = 0;
        }
        else
        {
            field -= step;
            a = max_T;
        }
    }

    template <class B> limitint<B> limitint<B>::get_storage_size() const
    {
	B tmp = field;
	B ret = 0;

	while(tmp != 0)
	{
	    tmp >>= 8;
	    ret++;
	}

	return limitint<B>(ret);
    }

    template <class B> unsigned char limitint<B>::operator [] (const limitint & position) const
    {
	B tmp = field;
	B index = position.field; // C++ has only class protection, not object protection

	while(index > 0)
	{
	    tmp >>= 8;
	    index--;
	}

	return (unsigned char)(tmp & 0xFF);
    }

    template <class B> void limitint<B>::setup_endian()
    {
	if(integers_system_is_big_endian())
            used_endian = big_endian;
        else
            used_endian = little_endian;

	(void)memset(zeroed_field, 0, ZEROED_SIZE);
    }


    template <class B> bool limitint<B>::is_system_big_endian()
    {
	if(used_endian == not_initialized)
	    setup_endian();

	switch(used_endian)
	{
	case big_endian:
	    return true;
	case little_endian:
	    return false;
	case not_initialized:
	    throw SRC_BUG;
	default:
	    throw SRC_BUG;
	}
    }


///////////////////////////////////////////////////////////////////////
///////////////// friends and not friends of limitint /////////////////
///////////////////////////////////////////////////////////////////////

    template <class B> limitint<B> operator + (const limitint<B> & a, const limitint<B> & b)
    {
        limitint<B> ret = a;
        ret += b;

        return ret;
    }

    template <class B> limitint<B> operator - (const limitint<B> & a, const limitint<B> & b)
    {
        limitint<B> ret = a;
        ret -= b;

        return ret;
    }

    template <class B> limitint<B> operator * (const limitint<B> & a, const limitint<B> & b)
    {
        limitint<B> ret = a;
        ret *= b;

        return ret;
    }

    template <class B> limitint<B> operator / (const limitint<B> & a, const limitint<B> & b)
    {
        limitint<B> ret = a;
        ret /= b;

        return ret;
    }

    template <class B> limitint<B> operator % (const limitint<B> & a, const limitint<B> & b)
    {
        limitint<B> ret = a;
        ret %= b;

        return ret;
    }

    template <class B> limitint<B> operator >> (const limitint<B> & a, U_32 bit)
    {
        limitint<B> ret = a;
        ret >>= bit;
        return ret;
    }

    template <class B> limitint<B> operator >> (const limitint<B> & a, const limitint<B> & bit)
    {
        limitint<B> ret = a;
        ret >>= bit;
        return ret;
    }

    template <class B> limitint<B> operator << (const limitint<B> & a, U_32 bit)
    {
        limitint<B> ret = a;
        ret <<= bit;
        return ret;
    }

    template <class B> limitint<B> operator << (const limitint<B> & a, const limitint<B> & bit)
    {
        limitint<B> ret = a;
        ret <<= bit;
        return ret;
    }

    template <class B> limitint<B> operator & (const limitint<B> & a, U_32 bit)
    {
        limitint<B> ret = a;
        ret &= bit;
        return ret;
    }

    template <class B> limitint<B> operator & (const limitint<B> & a, const limitint<B> & bit)
    {
        limitint<B> ret = a;
	ret &= bit;
        return ret;
    }

    template <class B> limitint<B> operator | (const limitint<B> & a, U_32 bit)
    {
        limitint<B> ret = a;
        ret |= bit;
        return ret;
    }

    template <class B> limitint<B> operator | (const limitint<B> & a, const limitint<B> & bit)
    {
        limitint<B> ret = a;
        ret |= bit;
        return ret;
    }

    template <class B> limitint<B> operator ^ (const limitint<B> & a, U_32 bit)
    {
        limitint<B> ret = a;
        ret ^= bit;
        return ret;
    }

    template <class B> limitint<B> operator ^ (const limitint<B> & a, const limitint<B> & bit)
    {
        limitint<B> ret = a;
        ret ^= bit;
        return ret;
    }

	/// @}

} // end of namespace

#endif
