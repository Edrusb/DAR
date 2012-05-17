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
// $Id: real_infinint.hpp,v 1.14.2.2 2007/07/22 16:35:00 edrusb Rel $
//
/*********************************************************************/

    /// \file real_infinint.hpp
    /// \brief the original infinint class implementation
    ///
    /// the infinint class implementation defined in this module can
    /// handle arbitrary large positive integer numbers

#ifndef REAL_INFININT_HPP
#define REAL_INFININT_HPP

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
#include "int_tools.hpp"

namespace libdar
{
    class generic_file;
    class user_interaction;

	/// the arbitrary large positive integer class

	/// can only handle positive integer numbers
	/// \ingroup Private
    class infinint
    {
    public :

#if SIZEOF_OFF_T > SIZEOF_TIME_T
#if SIZEOF_OFF_T > SIZEOF_SIZE_T
        infinint(off_t a = 0)
            { E_BEGIN infinint_from(a); E_END("infinint::infinint", "off_t") };
#else
        infinint(size_t a = 0)
            { E_BEGIN infinint_from(a); E_END("infinint::infinint", "size_t") };
#endif
#else
#if SIZEOF_TIME_T > SIZEOF_SIZE_T
        infinint(time_t a = 0)
            { E_BEGIN infinint_from(a); E_END("infinint::infinint", "time_t") };
#else
        infinint(size_t a = 0)
            { E_BEGIN infinint_from(a); E_END("infinint::infinint", "size_t") };
#endif
#endif

        infinint(const infinint & ref)
            { E_BEGIN; copy_from(ref); E_END("infinint::infinint", "const infinint &"); }
        infinint(user_interaction & dialog, S_I *fd, generic_file *x); // read an infinint from a file
        ~infinint()
            { E_BEGIN detruit(); E_END("infinint::~infinint","") };

        infinint & operator = (const infinint & ref)
            { E_BEGIN detruit(); copy_from(ref); return *this; E_END("infinint::operator =","") };

        void dump(user_interaction & dialog, int fd) const; // write byte sequence to file
        void dump(generic_file &x) const; // write byte sequence to file
        void read(generic_file &f) { detruit(); build_from_file(f); };

        infinint & operator += (const infinint & ref);
        infinint & operator -= (const infinint & ref);
        infinint & operator *= (unsigned char arg);
        infinint & operator *= (const infinint & ref);
	template <class T> infinint power(const T & exponent) const;
        inline infinint & operator /= (const infinint & ref);
        inline infinint & operator %= (const infinint & ref);
	infinint & operator &= (const infinint & ref);
	infinint & operator |= (const infinint & ref);
	infinint & operator ^= (const infinint & ref);
        infinint & operator >>= (U_32 bit);
        infinint & operator >>= (infinint bit);
        infinint & operator <<= (U_32 bit);
        infinint & operator <<= (infinint bit);
        infinint operator ++(int a)
            { E_BEGIN infinint ret = *this; ++(*this); return ret; E_END("infinint::operator ++", "int") };
        infinint operator --(int a)
            { E_BEGIN infinint ret = *this; --(*this); return ret; E_END("infinint::operator --", "int") };
        infinint & operator ++()
            { E_BEGIN return *this += 1; E_END("infinint::operator ++", "()") };
        infinint & operator --()
            { E_BEGIN return *this -= 1; E_END("infinint::operator --", "()") };

        U_32 operator % (U_32 arg) const
            { E_BEGIN return modulo(arg); E_END("infinint::operator %","") };

            // increment the argument up to a legal value for its storage type and decrement the object in consequence
            // note that the initial value of the argument is not ignored !
            // when the object is null the value of the argument stays the same as before
        template <class T>void unstack(T &v)
            { E_BEGIN infinint_unstack_to(v); E_END("infinint::unstack", typeid(v).name()) }

	infinint get_storage_size() const { return field->size(); };
	    // it returns number of byte of information necessary to store the integer

	unsigned char operator [] (const infinint & position) const;
	    // return in big endian order the information byte storing the integer

        friend bool operator < (const infinint &, const infinint &);
        friend bool operator == (const infinint &, const infinint &);
        friend bool operator > (const infinint &, const infinint &);
        friend bool operator <= (const infinint &, const infinint &);
        friend bool operator != (const infinint &, const infinint &);
        friend bool operator >= (const infinint &, const infinint &);
        friend void euclide(infinint a, const infinint &b, infinint &q, infinint &r);

    private :
        static const int TG = 4;

        enum endian { big_endian, little_endian, not_initialized };
        typedef unsigned char group[TG];

        storage *field;

        bool is_valid() const;
        void build_from_file(generic_file & x);
        void reduce(); // put the object in canonical form : no leading byte equal to zero
        void copy_from(const infinint & ref);
        void detruit();
        void make_at_least_as_wider_as(const infinint & ref);
        template <class T> void infinint_from(T a);
        template <class T> void infinint_unstack_to(T &a);
        template <class T> T modulo(T arg) const;
        signed int difference(const infinint & b) const; // gives the sign of (*this - arg) but only the sign !

            /////////////////////////
            // static statments
            //
        static endian used_endian;
        static void setup_endian();
    };


#define OPERATOR(OP) inline bool operator OP (const infinint &a, const infinint &b) \
{ \
    E_BEGIN \
    return a.difference(b) OP 0; \
    E_END("operator OP", "infinint, infinint") \
}

    OPERATOR(<)
    OPERATOR(>)
    OPERATOR(<=)
    OPERATOR(>=)
    OPERATOR(==)
    OPERATOR(!=)

    infinint operator + (const infinint &, const infinint &);
    infinint operator - (const infinint &, const infinint &);
    infinint operator * (const infinint &, const infinint &);
    infinint operator * (const infinint &, const unsigned char);
    infinint operator * (const unsigned char, const infinint &);
    infinint operator / (const infinint &, const infinint &);
    infinint operator % (const infinint &, const infinint &);
    infinint operator & (const infinint & a, const infinint & bit);
    infinint operator | (const infinint & a, const infinint & bit);
    infinint operator ^ (const infinint & a, const infinint & bit);
    infinint operator >> (const infinint & a, U_32 bit);
    infinint operator >> (const infinint & a, const infinint & bit);
    infinint operator << (const infinint & a, U_32 bit);
    infinint operator << (const infinint & a, const infinint & bit);
    void euclide(infinint a, const infinint &b, infinint &q, infinint &r);
    template <class T> inline void euclide(T a, T b, T & q, T &r)
    {
        E_BEGIN
        q = a/b; r = a%b;
        E_END("euclide", "")
    }

    inline infinint & infinint::operator /= (const infinint & ref)
    {
        E_BEGIN
        *this = *this / ref;
        return *this;
        E_END("infinint::operator /=", "")
    }

    inline infinint & infinint::operator %= (const infinint & ref)
    {
        E_BEGIN
        *this = *this % ref;
        return *this;
        E_END("infinint::operator %=", "")
    }


	/////////////////////////////////////////////////////
	///////////////// TEMPLATE BODIES ///////////////////
	/////////////////////////////////////////////////////

    template <class T> infinint infinint::power(const T & exponent) const
    {
	infinint ret = 1;
	for(T count = 0; count < exponent; ++count)
	    ret *= *this;

	return ret;
    }

    template <class T> T infinint::modulo(T arg) const
    {
        E_BEGIN
        infinint tmp = *this % infinint(arg);
        T ret = 0;
        unsigned char *debut = (unsigned char *)(&ret);
        unsigned char *ptr = debut + sizeof(T) - 1;
        storage::iterator it = tmp.field->rbegin();

        while(it != tmp.field->rend() && ptr >= debut)
	{
            *ptr = *it;
            --ptr;
            --it;
        }

        if(it != tmp.field->rend())
            throw SRC_BUG; // could not put all the data in the returned value !

        if(used_endian == big_endian)
            int_tools_swap_bytes(debut, sizeof(T));

        return ret;
        E_END("infinint::modulo", "")
    }


    template <class T> void infinint::infinint_from(T a)
    {
        E_BEGIN
        U_I size = sizeof(a);
        S_I direction = +1;
        unsigned char *ptr, *fin;

        if(used_endian == not_initialized)
            setup_endian();

        if(used_endian == big_endian)
        {
            direction = -1;
            ptr = (unsigned char *)(&a) + (size - 1);
            fin = (unsigned char *)(&a) - 1;
        }
        else
        {
            direction = +1;
            ptr = (unsigned char *)(&a);
            fin = (unsigned char *)(&a) + size;
        }

        while(ptr != fin && *ptr == 0)
        {
            ptr += direction;
            --size;
        }

        if(size == 0)
        {
            size = 1;
            ptr -= direction;
        }

        field = new storage(size);
        if(field != NULL)
        {
            storage::iterator it = field->begin();

            while(ptr != fin)
            {
                *it = *ptr;
                ++it;
                ptr += direction;
            }
            if(it != field->end())
                throw SRC_BUG; // size mismatch in this algorithm
        }
        else
            throw Ememory("template infinint::infinint_from");

        E_END("infinint::infinint_from", "")
    }

    template <class T> void infinint::infinint_unstack_to(T &a)
    {
        E_BEGIN
            // T is supposed to be an unsigned "integer"
            // (ie.: sizeof() returns the width of the storage bit field  and no sign bit is present)
            // Note : static here avoids the recalculation of max_T at each call
        static const T max_T = int_tools_maxof_agregate(T(0));
        infinint step = max_T - a;

        if(*this < step)
        {
            T transfert = 0;
            unsigned char *debut = (unsigned char *)&transfert;
            unsigned char *ptr = debut + sizeof(transfert) - 1;
            storage::iterator it = field->rbegin();

            while(ptr >= debut && it != field->rend())
            {
                *ptr = *it;
                --ptr;
                --it;
	    }
            if(used_endian == big_endian)
                int_tools_swap_bytes(debut, sizeof(transfert));
            a += transfert;
            *this -= *this;
        }
        else
        {
            *this -= step;
            a = max_T;
        }
        E_END("infinint::infinint_unstack_to", "")
    }

} // end of namespace

#endif
