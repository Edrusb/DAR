/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2052 Denis Corbin
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
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// to contact the author : dar.linux@free.fr
/*********************************************************************/
// $Id: limitint.hpp,v 1.7.4.6 2004/07/25 20:38:03 edrusb Exp $
//
/*********************************************************************/

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
} // end extern "C"

#include <typeinfo>
#include "integers.hpp"
#include "erreurs.hpp"
#include "special_alloc.hpp"
#include "int_tools.hpp"

namespace libdar
{

    class generic_file;
    class fichier;

    template<class B> class limitint
    {
    public :

#if SIZEOF_OFF_T > SIZEOF_TIME_T
#if SIZEOF_OFF_T > SIZEOF_SIZE_T
        limitint(off_t a = 0)
            { E_BEGIN; limitint_from(a); E_END("limitint::limitint", "off_t"); };
#else
        limitint(size_t a = 0)
            { E_BEGIN; limitint_from(a); E_END("limitint::limitint", "size_t"); };
#endif
#else
#if SIZEOF_TIME_T > SIZEOF_SIZE_T
        limitint(time_t a = 0)
            { E_BEGIN; limitint_from(a); E_END("limitint::limitint", "time_t"); };
#else
        limitint(size_t a = 0)
            { E_BEGIN; limitint_from(a); E_END("limitint::limitint", "size_t"); };
#endif
#endif

        limitint(int *fd, generic_file *x); // read an limitint from a file


        void dump(int fd) const; // write byte sequence to file
        void dump(generic_file &x) const; // write byte sequence to file
        void read(generic_file &f) { build_from_file(f); };

        limitint & operator += (const limitint & ref);
        limitint & operator -= (const limitint & ref);
        limitint & operator *= (const limitint & ref);
	template <class T> limitint power(const T & exponent) const;
        limitint & operator /= (const limitint & ref);
        limitint & operator %= (const limitint & ref);
        limitint & operator >>= (U_32 bit);
        limitint & operator >>= (limitint bit);
        limitint & operator <<= (U_32 bit);
        limitint & operator <<= (limitint bit);
        limitint operator ++(int a)
            { E_BEGIN; limitint ret = *this; ++(*this); return ret; E_END("limitint::operator ++", "int"); };
        limitint operator --(int a)
            { E_BEGIN; limitint ret = *this; --(*this); return ret; E_END("limitint::operator --", "int"); };
        limitint & operator ++()
            { E_BEGIN; return *this += 1; E_END("limitint::operator ++", "()"); };
        limitint & operator --()
            { E_BEGIN; return *this -= 1; E_END("limitint::operator --", "()"); };

        U_32 operator % (U_32 arg) const;

            // increment the argument up to a legal value for its storage type and decrement the object in consequence
            // note that the initial value of the argument is not ignored !
            // when the object is null the value of the argument stays the same as before
        template <class T>void unstack(T &v)
            { E_BEGIN; limitint_unstack_to(v); E_END("limitint::unstack", typeid(v).name()); };

        bool operator < (const limitint &x) const { return field < x.field; };
        bool operator == (const limitint &x) const { return field == x.field; };
        bool operator > (const limitint &x) const { return field > x.field; };
        bool operator <= (const limitint &x) const { return field <= x.field; };
        bool operator != (const limitint &x) const { return field != x.field; };
        bool operator >= (const limitint &x) const { return field >= x.field; };

#ifdef LIBDAR_SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(limitint);
#endif

        B debug_get_max() const { return max_value; };
        B debug_get_bytesize() const { return bytesize; };

    private :
        static const int TG = 4;

        enum endian { big_endian, little_endian, not_initialized };
        typedef unsigned char group[TG];

        B field;

        void build_from_file(generic_file & x);
        template <class T> void limitint_from(T a);
        template <class T> void limitint_unstack_to(T &a);

            /////////////////////////
            // static statments
            //
        static endian used_endian;
        static const U_I bytesize = sizeof(B);
        static const B max_value = ~B(0) > 0 ? ~B(0) : ~(B(1) << (bytesize*8 - 1));
        static void setup_endian();
    };

    template <class B> limitint<B> operator + (const limitint<B> &, const limitint<B> &);
    template <class B> inline limitint<B> operator + (const limitint<B> & a, U_I b)
    { return a + limitint<B>(b); };
    template <class B> limitint<B> operator - (const limitint<B> &, const limitint<B> &);
    template <class B> inline limitint<B> operator - (const limitint<B> & a, U_I b)
    { return a - limitint<B>(b); };
    template <class B> limitint<B> operator * (const limitint<B> &, const limitint<B> &);
    template <class B> inline limitint<B> operator * (const limitint<B> & a, U_I b)
    { return a * limitint<B>(b); };
    template <class B> limitint<B> operator / (const limitint<B> &, const limitint<B> &);
    template <class B> limitint<B> operator / (const limitint<B> & a, U_I b)
    { return a / limitint<B>(b); };
    template <class B> limitint<B> operator % (const limitint<B> &, const limitint<B> &);
    template <class B> limitint<B> operator >> (const limitint<B> & a, U_32 bit);
    template <class B> limitint<B> operator >> (const limitint<B> & a, const limitint<B> & bit);
    template <class B> limitint<B> operator << (const limitint<B> & a, U_32 bit);
    template <class B> limitint<B> operator << (const limitint<B> & a, const limitint<B> & bit);

    template <class T> inline void euclide(T a, T b, T & q, T &r)
    {
        E_BEGIN;
        q = a/b; r = a%b;
        E_END("euclide", "");
    }

    template <class B> inline void euclide(limitint<B> a, U_I b, limitint<B> & q, limitint<B> &r)
    {
        euclide(a, limitint<B>(b), q, r);
    }

	///////////////////////////////////////////////////////////////////////
	/////////  template implementation ////////////////////////////////////
	///////////////////////////////////////////////////////////////////////

    template <class B> typename limitint<B>::endian limitint<B>::used_endian = not_initialized;

    template <class B> limitint<B>::limitint(S_I *fd, generic_file *x)
    {
        if(fd != NULL && x != NULL)
            throw Erange("limitint::limitint(file, file)", "both arguments are not NULL, please choose for me one or the other");
        if(fd != NULL)
        {
            fichier f = dup(*fd);
            build_from_file(f);
        }
        else
            if(x != NULL)
                build_from_file(*x);
            else
                throw Erange("limitint::limitint(file, file)", "cannot read from file, both arguments are NULL");
    }

    template <class B> void limitint<B>::dump(S_I fd) const
    {
        fichier f = dup(fd);
        dump(f);
    }

    template <class B> void limitint<B>::build_from_file(generic_file & x)
    {
        E_BEGIN;
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
                throw Erange("limitint::build_from_file(generic_file)", "reached end of file before all data could be read");

            if(a == 0)
                skip++;
            else // end of size field
            {
                    // computing the size to read
                U_I pos = 0;

                int_tools_expand_byte(a, bf);
                for(S_I i = 0; i < 8; i++)
                    pos = pos + bf[i];
                if(pos != 1)
                    throw Erange("limitint::build_from_file(generic_file)", "badly formed infinint or not supported format"); // more than 1 bit is set to 1

                pos = 0;
                while(bf[pos] == 0)
                    pos++;
                pos += 1; // bf starts at zero, but bit zero means 1 TG of length

                skip *= 8;
                skip += pos;
                skip *= TG;

                if(skip.field > bytesize)
                    throw Elimitint();

                field = 0; // important to also clear "unread" bytes by the following call
                lu = x.read(ptr, skip.field);

                if(used_endian == not_initialized)
                    setup_endian();
                if(used_endian == big_endian)
                    int_tools_swap_bytes((unsigned char *)ptr, skip.field);
                else
                    field >>= (bytesize - skip.field)*8;
                fin = true;
            }
        }
        E_END("limitint::read_from_file", "generic_file");
    }


    template <class B> void limitint<B>::dump(generic_file & x) const
    {
        E_BEGIN;
        B width = bytesize;
        B pos;
        unsigned char last_width;
        B justification;
        S_I direction = +1;
        unsigned char *ptr, *fin;


        if(used_endian == not_initialized)
            setup_endian();

        if(used_endian == big_endian)
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
            width--;
        }
        if(width == 0)
            width = 1; // minimum size of information is 1 byte

            // "width" is the informational field size in byte
            // TG is the width in TG, thus the number of bit that must have
            // the preamble
        euclide(width, (const B)(TG), width, justification);
        if(justification != 0)
                // in case we need to add some bytes to have a width multiple of TG
            width++;  // we need then one more group to have a width multiple of TG

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

            // now we write the preamble except the last byte. All theses are zeros.

        unsigned char u = 0x00;

        while(width-- > 0)
            if(x.write((char *)(&u), 1) < 1)
                throw Erange("limitint::dump(generic_file)", "can't write data to file");


            // now we write the last byte of the preambule, which as only one bit set

        if(x.write((char *)&last_width, 1) < 1)
            throw Erange("limitint::dump(generic_file)", "can't write data to file");

            // we need now to write some justification byte to have an informational field multiple of TG

        if(justification != 0)
        {
            justification = TG - justification;
            while(justification-- > 0)
                if(x.write((char *)(&u), 1) < 1)
                    throw Erange("limitint::dump(generic_file)", "can't write data to file");
        }

            // now we continue dumping the informational bytes:
        if(ptr == fin) // field is equal to zero
        {
            if(x.write((char *)(&u), 1) < 1)
		throw Erange("limitint::dump(generic_file)", "can't write data to file");
        }
        else // we have some bytes to write down
            while(ptr != fin)
            {
                if(x.write((char *)ptr, 1) < 1)
                    throw Erange("limitint::dump(generic_file)", "can't write data to file");
                else
                    ptr += direction;
            }

        E_END("limitint::dump", "generic_file");
    }

    template<class B> limitint<B> & limitint<B>::operator += (const limitint & arg)
    {
        E_BEGIN;
        B res = field + arg.field;
        if(res < field || res < arg.field)
            throw Elimitint();
        else
            field = res;

        return *this;
        E_END("limitint::operator +=", "");
    }

    template <class B> limitint<B> & limitint<B>::operator -= (const limitint & arg)
    {
        E_BEGIN;
        if(field < arg.field)
            throw Erange("limitint::operator", "subtracting a infinint greater than the first, infinint can't be negative");

            // now processing the operation

        field -= arg.field;
        return *this;
        E_END("limitint::operator -=", "");
    }


    template <class B> limitint<B> & limitint<B>::operator *= (const limitint & arg)
    {
        E_BEGIN;
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
        E_END("limitint::operator *=", "");
    }

    template <class B> template<class T> limitint<B> limitint<B>::power(const T & exponent) const
    {
	limitint ret = 1;
	for(T count = 0; count < exponent; count++)
	    ret *= *this;

	return ret;
    }

    template <class B> limitint<B> & limitint<B>::operator /= (const limitint & arg)
    {
        E_BEGIN;
        if(arg == 0)
            throw Einfinint("limitint.cpp : operator /=", "division by zero");

        field /= arg.field;
        return *this;
        E_END("limitint::operator /=", "");
    }

    template <class B> limitint<B> & limitint<B>::operator %= (const limitint & arg)
    {
        E_BEGIN;
        if(arg == 0)
            throw Einfinint("limitint.cpp : operator %=", "division by zero");

        field %= arg.field;
        return *this;
        E_END("limitint::operator /=", "");
    }

    template <class B> limitint<B> & limitint<B>::operator >>= (U_32 bit)
    {
        E_BEGIN;
        field >>= bit;
        return *this;
        E_END("limitint::operator >>=", "U_32");
    }

    template <class B> limitint<B> & limitint<B>::operator >>= (limitint bit)
    {
        E_BEGIN;
        field >>= bit.field;
        return *this;
        E_END("limitint::operator >>=", "limitint");
    }

    template <class B> limitint<B> & limitint<B>::operator <<= (U_32 bit)
    {
        E_BEGIN;
        if(bit + int_tools_higher_power_of_2(field) >= bytesize*8)
            throw Elimitint();
        field <<= bit;
        return *this;
        E_END("limitint::operator <<=", "U_32");
    }

    template <class B> limitint<B> & limitint<B>::operator <<= (limitint bit)
    {
        E_BEGIN;
        if(bit.field + int_tools_higher_power_of_2(field) >= bytesize*8)
            throw Elimitint();
        field <<= bit.field;
        return *this;
        E_END("limitint::operator <<=", "limitint");
    }

    template <class B> U_32 limitint<B>::operator % (U_32 arg) const
    {
        E_BEGIN;
        return U_32(field % arg);
        E_END("limitint::modulo", "");
    }

    template <class B> template <class T> void limitint<B>::limitint_from(T a)
    {
        E_BEGIN;
        if(sizeof(a) <= bytesize || a <= (T)(max_value))
	    field = B(a);
        else
            throw Elimitint();
        E_END("limitint::limitint_from", "");
    }

    template <class B> template <class T> void limitint<B>::limitint_unstack_to(T &a)
    {
        E_BEGIN;
            // T is supposed to be an unsigned "integer"
            // (ie.: sizeof returns the width of the storage bit field  and no sign bit is present)
            // Note : static here avoids the recalculation of max_T at each call
        static const T max_T = ~T(0) > 0 ? ~T(0) : ~int_tools_rotate_right_one_bit(T(1));
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

        E_END("limitint::limitint_unstack_to", "");
    }

    template <class B> void limitint<B>::setup_endian()
    {
        E_BEGIN;
        U_16 u = 1;
        unsigned char *ptr = (unsigned char *)(&u);

        if(ptr[0] == 1)
            used_endian = big_endian;
        else
            used_endian = little_endian;
        E_END("limitint::setup_endian", "");
    }


///////////////////////////////////////////////////////////////////////
///////////////// friends and not friends of limitint /////////////////
///////////////////////////////////////////////////////////////////////

    template <class B> limitint<B> operator + (const limitint<B> & a, const limitint<B> & b)
    {
        E_BEGIN;
        limitint<B> ret = a;
        ret += b;

        return ret;
        E_END("operator +", "limitint");
    }

    template <class B> limitint<B> operator - (const limitint<B> & a, const limitint<B> & b)
    {
        E_BEGIN;
        limitint<B> ret = a;
        ret -= b;

        return ret;
        E_END("operator -", "limitint");
    }

    template <class B> limitint<B> operator * (const limitint<B> & a, const limitint<B> & b)
    {
        E_BEGIN;
        limitint<B> ret = a;
        ret *= b;

        return ret;
        E_END("operator *", "limitint");
    }

    template <class B> limitint<B> operator / (const limitint<B> & a, const limitint<B> & b)
    {
        E_BEGIN;
        limitint<B> ret = a;
        ret /= b;

        return ret;
        E_END("operator / ", "limitint");
    }

    template <class B> limitint<B> operator % (const limitint<B> & a, const limitint<B> & b)
    {
        E_BEGIN;
        limitint<B> ret = a;
        ret %= b;

        return ret;
        E_END("operator %", "limitint");
    }

    template <class B> limitint<B> operator >> (const limitint<B> & a, U_32 bit)
    {
        E_BEGIN;
        limitint<B> ret = a;
        ret >>= bit;
        return ret;
        E_END("operator >>", "limitint, U_32");
    }

    template <class B> limitint<B> operator >> (const limitint<B> & a, const limitint<B> & bit)
    {
        E_BEGIN;
        limitint<B> ret = a;
        ret >>= bit;
        return ret;
        E_END("operator >>", "limitint");
    }

    template <class B> limitint<B> operator << (const limitint<B> & a, U_32 bit)
    {
        E_BEGIN;
        limitint<B> ret = a;
        ret <<= bit;
        return ret;
        E_END("operator <<", "limitint, U_32");
    }

    template <class B> limitint<B> operator << (const limitint<B> & a, const limitint<B> & bit)
    {
        E_BEGIN;
        limitint<B> ret = a;
        ret <<= bit;
        return ret;
        E_END("operator <<", "limitint");
    }


} // end of namespace

#endif
