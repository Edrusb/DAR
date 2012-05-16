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
// $Id: limitint.hpp,v 1.7.4.4 2004/01/28 15:29:47 edrusb Rel $
//
/*********************************************************************/

#ifndef LIMITINT_HPP
#define LIMITINT_HPP

#pragma interface

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

namespace libdar
{
    class generic_file;

    template<class B> class limitint
        // this is the "limitint" implementation if infinint.
    {
    public :

#if SIZEOF_OFF_T > SIZEOF_TIME_T
#if SIZEOF_OFF_T > SIZEOF_SIZE_T
        limitint(off_t a = 0) throw(Ememory, Erange, Ebug, Elimitint)
            { E_BEGIN; limitint_from(a); E_END("limitint::limitint", "off_t"); };
#else
        limitint(size_t a = 0) throw(Ememory, Erange, Ebug, Elimitint)
            { E_BEGIN; limitint_from(a); E_END("limitint::limitint", "size_t"); };
#endif
#else
#if SIZEOF_TIME_T > SIZEOF_SIZE_T
        limitint(time_t a = 0) throw(Ememory, Erange, Ebug, Elimitint)
            { E_BEGIN; limitint_from(a); E_END("limitint::limitint", "time_t"); };
#else
        limitint(size_t a = 0) throw(Ememory, Erange, Ebug, Elimitint)
            { E_BEGIN; limitint_from(a); E_END("limitint::limitint", "size_t"); };
#endif
#endif

        limitint(int *fd, generic_file *x) throw(Erange, Ememory, Ebug, Elimitint); // read an limitint from a file


        void dump(int fd) const throw(Einfinint, Ememory, Erange, Ebug); // write byte sequence to file
        void dump(generic_file &x) const throw(Einfinint, Ememory, Erange, Ebug); // write byte sequence to file
        void read(generic_file &f) throw(Erange, Ememory, Ebug, Elimitint) { build_from_file(f); };

        limitint & operator += (const limitint & ref) throw(Ememory, Erange, Ebug, Elimitint);
        limitint & operator -= (const limitint & ref) throw(Ememory, Erange, Ebug);
        limitint & operator *= (const limitint & ref) throw(Ememory, Erange, Ebug, Elimitint);
	template <class T> limitint power(const T & exponent) const;
        limitint & operator /= (const limitint & ref) throw(Einfinint, Erange, Ememory, Ebug);
        limitint & operator %= (const limitint & ref) throw(Einfinint, Erange, Ememory, Ebug);
        limitint & operator >>= (U_32 bit) throw(Ememory, Erange, Ebug);
        limitint & operator >>= (limitint bit) throw(Ememory, Erange, Ebug);
        limitint & operator <<= (U_32 bit) throw(Ememory, Erange, Ebug, Elimitint);
        limitint & operator <<= (limitint bit) throw(Ememory, Erange, Ebug, Elimitint);
        limitint operator ++(int a) throw(Ememory, Erange, Ebug, Elimitint)
            { E_BEGIN; limitint ret = *this; ++(*this); return ret; E_END("limitint::operator ++", "int"); };
        limitint operator --(int a) throw(Ememory, Erange, Ebug, Elimitint)
            { E_BEGIN; limitint ret = *this; --(*this); return ret; E_END("limitint::operator --", "int"); };
        limitint & operator ++() throw(Ememory, Erange, Ebug, Elimitint)
            { E_BEGIN; return *this += 1; E_END("limitint::operator ++", "()"); };
        limitint & operator --() throw(Ememory, Erange, Ebug, Elimitint)
            { E_BEGIN; return *this -= 1; E_END("limitint::operator --", "()"); };

        U_32 operator % (U_32 arg) const throw(Einfinint, Ememory, Erange, Ebug);

            // increment the argument up to a legal value for its storage type and decrement the object in consequence
            // note that the initial value of the argument is not ignored !
            // when the object is null the value of the argument stays the same as before
        template <class T>void unstack(T &v) throw(Ememory, Erange, Ebug)
            { E_BEGIN; limitint_unstack_to(v); E_END("limitint::unstack", typeid(v).name()); };

        bool operator < (const limitint &x) const throw(Erange, Ebug) { return field < x.field; };
        bool operator == (const limitint &x) const throw(Erange, Ebug) { return field == x.field; };
        bool operator > (const limitint &x) const throw(Erange, Ebug) { return field > x.field; };
        bool operator <= (const limitint &x) const throw(Erange, Ebug) { return field <= x.field; };
        bool operator != (const limitint &x) const throw(Erange, Ebug) { return field != x.field; };
        bool operator >= (const limitint &x) const throw(Erange, Ebug) { return field >= x.field; };

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

        void build_from_file(generic_file & x) throw(Erange, Ememory, Ebug, Elimitint);
        template <class T> void limitint_from(T a) throw(Ememory, Erange, Ebug, Elimitint);
        template <class T> void limitint_unstack_to(T &a) throw(Ememory, Erange, Ebug);

            /////////////////////////
            // static statments
            //
        static endian used_endian;
        static const U_I bytesize = sizeof(B);
        static const B max_value = ~B(0) > 0 ? ~B(0) : ~(B(1) << (bytesize*8 - 1));
        static void setup_endian();
    };

    template <class B> limitint<B> operator + (const limitint<B> &, const limitint<B> &) throw(Erange, Ememory, Ebug, Elimitint);
    template <class B> inline limitint<B> operator + (const limitint<B> & a, U_I b) throw(Erange, Ememory, Ebug)
    { return a + limitint<B>(b); };
    template <class B> limitint<B> operator - (const limitint<B> &, const limitint<B> &) throw(Erange, Ememory, Ebug);
    template <class B> inline limitint<B> operator - (const limitint<B> & a, U_I b) throw(Erange, Ememory, Ebug)
    { return a - limitint<B>(b); };
    template <class B> limitint<B> operator * (const limitint<B> &, const limitint<B> &) throw(Erange, Ememory, Ebug, Elimitint);
    template <class B> inline limitint<B> operator * (const limitint<B> & a, U_I b) throw(Erange, Ememory, Ebug)
    { return a * limitint<B>(b); };
    template <class B> limitint<B> operator / (const limitint<B> &, const limitint<B> &) throw(Einfinint, Erange, Ememory, Ebug);
    template <class B> limitint<B> operator / (const limitint<B> & a, U_I b) throw(Einfinint, Erange, Ememory, Ebug)
    { return a / limitint<B>(b); };
    template <class B> limitint<B> operator % (const limitint<B> &, const limitint<B> &) throw(Einfinint, Erange, Ememory, Ebug);
    template <class B> limitint<B> operator >> (const limitint<B> & a, U_32 bit) throw(Erange, Ememory, Ebug);
    template <class B> limitint<B> operator >> (const limitint<B> & a, const limitint<B> & bit) throw(Erange, Ememory, Ebug);
    template <class B> limitint<B> operator << (const limitint<B> & a, U_32 bit) throw(Erange, Ememory, Ebug, Elimitint);
    template <class B> limitint<B> operator << (const limitint<B> & a, const limitint<B> & bit) throw(Erange, Ememory, Ebug, Elimitint);

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

} // end of namespace

#endif
