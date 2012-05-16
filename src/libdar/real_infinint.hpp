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
// $Id: real_infinint.hpp,v 1.4.4.3 2004/01/15 20:35:55 edrusb Rel $
//
/*********************************************************************/

#ifndef REAL_INFININT_HPP
#define REAL_INFININT_HPP

#pragma interface

#include "../my_config.h"

extern "C"
{
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
} // end extern "C"

#include <typeinfo>
#include "storage.hpp"
#include "integers.hpp"

namespace libdar
{

    class generic_file;

    class infinint
    {
    public :

#if SIZEOF_OFF_T > SIZEOF_TIME_T
#if SIZEOF_OFF_T > SIZEOF_SIZE_T
        infinint(off_t a = 0) throw(Ememory, Erange, Ebug)
            { E_BEGIN; infinint_from(a); E_END("infinint::infinint", "off_t"); };
#else
        infinint(size_t a = 0) throw(Ememory, Erange, Ebug)
            { E_BEGIN; infinint_from(a); E_END("infinint::infinint", "size_t"); };
#endif
#else
#if SIZEOF_TIME_T > SIZEOF_SIZE_T
        infinint(time_t a = 0) throw(Ememory, Erange, Ebug)
            { E_BEGIN; infinint_from(a); E_END("infinint::infinint", "time_t"); };
#else
        infinint(size_t a = 0) throw(Ememory, Erange, Ebug)
            { E_BEGIN; infinint_from(a); E_END("infinint::infinint", "size_t"); };
#endif
#endif

        infinint(const infinint & ref) throw(Ememory, Ebug)
            { E_BEGIN; copy_from(ref); E_END("infinint::infinint", "const infinint &"); };
        infinint(int *fd, generic_file *x) throw(Erange, Ememory, Ebug); // read an infinint from a file
        ~infinint() throw(Ebug)
            { E_BEGIN; detruit(); E_END("infinint::~infinint",""); };

        infinint & operator = (const infinint & ref) throw(Ememory, Ebug)
            { E_BEGIN; detruit(); copy_from(ref); return *this; E_END("infinint::operator =",""); };

        void dump(int fd) const throw(Einfinint, Ememory, Erange, Ebug); // write byte sequence to file
        void dump(generic_file &x) const throw(Einfinint, Ememory, Erange, Ebug); // write byte sequence to file
        void read(generic_file &f) throw(Erange, Ememory, Ebug) { detruit(); build_from_file(f); };

        infinint & operator += (const infinint & ref) throw(Ememory, Erange, Ebug);
        infinint & operator -= (const infinint & ref) throw(Ememory, Erange, Ebug);
        infinint & operator *= (unsigned char arg) throw(Ememory, Erange, Ebug);
        infinint & operator *= (const infinint & ref) throw(Ememory, Erange, Ebug);
	template <class T> infinint power(const T & exponent) const;
        inline infinint & operator /= (const infinint & ref) throw(Einfinint, Erange, Ememory, Ebug);
        inline infinint & operator %= (const infinint & ref) throw(Einfinint, Erange, Ememory, Ebug);
        infinint & operator >>= (U_32 bit) throw(Ememory, Erange, Ebug);
        infinint & operator >>= (infinint bit) throw(Ememory, Erange, Ebug);
        infinint & operator <<= (U_32 bit) throw(Ememory, Erange, Ebug);
        infinint & operator <<= (infinint bit) throw(Ememory, Erange, Ebug);
        infinint operator ++(int a) throw(Ememory, Erange, Ebug)
            { E_BEGIN; infinint ret = *this; ++(*this); return ret; E_END("infinint::operator ++", "int"); };
        infinint operator --(int a) throw(Ememory, Erange, Ebug)
            { E_BEGIN; infinint ret = *this; --(*this); return ret; E_END("infinint::operator --", "int"); };
        infinint & operator ++() throw(Ememory, Erange, Ebug)
            { E_BEGIN; return *this += 1; E_END("infinint::operator ++", "()"); };
        infinint & operator --() throw(Ememory, Erange, Ebug)
            { E_BEGIN; return *this -= 1; E_END("infinint::operator --", "()"); };

        U_32 operator % (U_32 arg) const throw(Einfinint, Ememory, Erange, Ebug)
            { E_BEGIN; return modulo(arg); E_END("infinint::operator %",""); };

            // increment the argument up to a legal value for its storage type and decrement the object in consequence
            // note that the initial value of the argument is not ignored !
            // when the object is null the value of the argument stays the same as before
        template <class T>void unstack(T &v) throw(Ememory, Erange, Ebug)
            { E_BEGIN; infinint_unstack_to(v); E_END("infinint::unstack", typeid(v).name()); };

        friend bool operator < (const infinint &, const infinint &) throw(Erange, Ebug);
        friend bool operator == (const infinint &, const infinint &) throw(Erange, Ebug);
        friend bool operator > (const infinint &, const infinint &) throw(Erange, Ebug);
        friend bool operator <= (const infinint &, const infinint &) throw(Erange, Ebug);
        friend bool operator != (const infinint &, const infinint &) throw(Erange, Ebug);
        friend bool operator >= (const infinint &, const infinint &) throw(Erange, Ebug);
        friend void euclide(infinint a, const infinint &b, infinint &q, infinint &r) throw(Einfinint, Erange, Ememory, Ebug);

    private :
        static const int TG = 4;

        enum endian { big_endian, little_endian, not_initialized };
        typedef unsigned char group[TG];

        storage *field;

        bool is_valid() const throw();
        void build_from_file(generic_file & x) throw(Erange, Ememory, Ebug);
        void reduce() throw(Erange, Ememory, Ebug); // put the object in canonical form : no leading byte equal to zero
        void copy_from(const infinint & ref) throw(Ememory, Ebug);
        void detruit() throw(Ebug);
        void make_at_least_as_wider_as(const infinint & ref) throw(Erange, Ememory, Ebug);
        template <class T> void infinint_from(T a) throw(Ememory, Erange, Ebug);
        template <class T> void infinint_unstack_to(T &a) throw(Ememory, Erange, Ebug);
        template <class T> T modulo(T arg) const throw(Einfinint, Ememory, Erange, Ebug);
        signed int difference(const infinint & b) const throw(Erange, Ebug); // gives the sign of (*this - arg) but only the sign !

            /////////////////////////
            // static statments
            //
        static endian used_endian;
        static void setup_endian();
    };

#define OPERATOR(OP) inline bool operator OP (const infinint &a, const infinint &b) throw(Erange, Ebug) \
{ \
    E_BEGIN; \
    return a.difference(b) OP 0; \
    E_END("operator OP", "infinint, infinint"); \
}

    OPERATOR(<);
    OPERATOR(>);
    OPERATOR(<=);
    OPERATOR(>=);
    OPERATOR(==);
    OPERATOR(!=);

    infinint operator + (const infinint &, const infinint &) throw(Erange, Ememory, Ebug);
    infinint operator - (const infinint &, const infinint &) throw(Erange, Ememory, Ebug);
    infinint operator * (const infinint &, const infinint &) throw(Erange, Ememory, Ebug);
    infinint operator * (const infinint &, const unsigned char) throw(Erange, Ememory, Ebug);
    infinint operator * (const unsigned char, const infinint &) throw(Erange, Ememory, Ebug);
    infinint operator / (const infinint &, const infinint &) throw(Einfinint, Erange, Ememory, Ebug);
    infinint operator % (const infinint &, const infinint &) throw(Einfinint, Erange, Ememory, Ebug);
    infinint operator >> (const infinint & a, U_32 bit) throw(Erange, Ememory, Ebug);
    infinint operator >> (const infinint & a, const infinint & bit) throw(Erange, Ememory, Ebug);
    infinint operator << (const infinint & a, U_32 bit) throw(Erange, Ememory, Ebug);
    infinint operator << (const infinint & a, const infinint & bit) throw(Erange, Ememory, Ebug);
    void euclide(infinint a, const infinint &b, infinint &q, infinint &r) throw(Einfinint, Erange, Ememory, Ebug);
    template <class T> inline void euclide(T a, T b, T & q, T &r)
    {
        E_BEGIN;
        q = a/b; r = a%b;
        E_END("euclide", "");
    };

    inline infinint & infinint::operator /= (const infinint & ref) throw(Einfinint, Erange, Ememory, Ebug)
    {
        E_BEGIN;
        *this = *this / ref;
        return *this;
        E_END("infinint::operator /=", "");
    }

    inline infinint & infinint::operator %= (const infinint & ref) throw(Einfinint, Erange, Ememory, Ebug)
    {
        E_BEGIN;
        *this = *this % ref;
        return *this;
        E_END("infinint::operator %=", "");
    }

} // end of namespace

#endif
