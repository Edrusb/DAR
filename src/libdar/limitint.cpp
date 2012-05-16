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
// $Id: limitint.cpp,v 1.8.4.5 2004/05/21 08:33:02 edrusb Rel $
//
/*********************************************************************/

#pragma implementation

#include "../my_config.h"

#include "erreurs.hpp"
#include "generic_file.hpp"
#include "limitint.hpp"

namespace libdar
{
    typedef unsigned char bitfield[8];

    static void swap_bytes(unsigned char &a, unsigned char &b) throw();
    static void swap_bytes(unsigned char *a, U_I size) throw();
    static void expand_byte(unsigned char a, bitfield &bit) throw();
    template <class B> static B higher_power_of_2(B val);
    template <class T> static T rotate_right_one_bit(T v);

    template <class B> typename limitint<B>::endian limitint<B>::used_endian = not_initialized;

    template <class B> limitint<B>::limitint(S_I *fd, generic_file *x) throw(Erange, Ememory, Ebug, Elimitint)
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

    template <class B> void limitint<B>::dump(S_I fd) const throw(Einfinint, Ememory, Erange, Ebug)
    {
        fichier f = dup(fd);
        dump(f);
    }

    template <class B> void limitint<B>::build_from_file(generic_file & x) throw(Erange, Ememory, Ebug, Elimitint)
    {
        E_BEGIN;
        unsigned char a;
        bool fin = false;
        limitint<B> skip = 0;
        char *ptr = (char *)&field;
        S_I lu;
        bitfield bf;

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

                expand_byte(a, bf);
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
                    swap_bytes((unsigned char *)ptr, skip.field);
                else
                    field >>= (bytesize - skip.field)*8;
                fin = true;
            }
        }
        E_END("limitint::read_from_file", "generic_file");
    }


    template <class B> void limitint<B>::dump(generic_file & x) const throw(Einfinint, Ememory, Erange, Ebug)
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

    template<class B> limitint<B> & limitint<B>::operator += (const limitint & arg) throw(Ememory, Erange, Ebug, Elimitint)
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

    template <class B> limitint<B> & limitint<B>::operator -= (const limitint & arg) throw(Ememory, Erange, Ebug)
    {
        E_BEGIN;
        if(field < arg.field)
            throw Erange("limitint::operator", "subtracting a infinint greater than the first, infinint can't be negative");

            // now processing the operation

        field -= arg.field;
        return *this;
        E_END("limitint::operator -=", "");
    }


    template <class B> limitint<B> & limitint<B>::operator *= (const limitint & arg) throw(Ememory, Erange, Ebug, Elimitint)
    {
        E_BEGIN;
        static const B max_power = bytesize*8 - 1;

        B total = higher_power_of_2(field) + higher_power_of_2(arg.field) + 1; // for an explaination about "+2" see NOTES
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

    template <class B> limitint<B> & limitint<B>::operator /= (const limitint & arg) throw(Einfinint, Erange, Ememory, Ebug)
    {
        E_BEGIN;
        if(arg == 0)
            throw Einfinint("limitint.cpp : operator /=", "division by zero");

        field /= arg.field;
        return *this;
        E_END("limitint::operator /=", "");
    }

    template <class B> limitint<B> & limitint<B>::operator %= (const limitint & arg) throw(Einfinint, Erange, Ememory, Ebug)
    {
        E_BEGIN;
        if(arg == 0)
            throw Einfinint("limitint.cpp : operator %=", "division by zero");

        field %= arg.field;
        return *this;
        E_END("limitint::operator /=", "");
    }

    template <class B> limitint<B> & limitint<B>::operator >>= (U_32 bit) throw(Ememory, Erange, Ebug)
    {
        E_BEGIN;
        field >>= bit;
        return *this;
        E_END("limitint::operator >>=", "U_32");
    }

    template <class B> limitint<B> & limitint<B>::operator >>= (limitint bit) throw(Ememory, Erange, Ebug)
    {
        E_BEGIN;
        field >>= bit.field;
        return *this;
        E_END("limitint::operator >>=", "limitint");
    }

    template <class B> limitint<B> & limitint<B>::operator <<= (U_32 bit) throw(Ememory, Erange, Ebug, Elimitint)
    {
        E_BEGIN;
        if(bit + higher_power_of_2(field) >= bytesize*8)
            throw Elimitint();
        field <<= bit;
        return *this;
        E_END("limitint::operator <<=", "U_32");
    }

    template <class B> limitint<B> & limitint<B>::operator <<= (limitint bit) throw(Ememory, Erange, Ebug, Elimitint)
    {
        E_BEGIN;
        if(bit.field + higher_power_of_2(field) >= bytesize*8)
            throw Elimitint();
        field <<= bit.field;
        return *this;
        E_END("limitint::operator <<=", "limitint");
    }

    template <class B> U_32 limitint<B>::operator % (U_32 arg) const throw(Einfinint, Ememory, Erange, Ebug)
    {
        E_BEGIN;
        return U_32(field % arg);
        E_END("limitint::modulo", "");
    }

    template <class B> template <class T> void limitint<B>::limitint_from(T a) throw(Ememory, Erange, Ebug, Elimitint)
    {
        E_BEGIN;
        if(sizeof(a) <= bytesize || a <= (T)(max_value))
	    field = B(a);
        else
            throw Elimitint();
        E_END("limitint::limitint_from", "");
    }

    static void dummy_call(char *x)
    {
        static char id[]="$Id: limitint.cpp,v 1.8.4.5 2004/05/21 08:33:02 edrusb Rel $";
        dummy_call(id);
    }

    template <class T> static T rotate_right_one_bit(T v)
    {
        bool retenue = (v & 1) != 0;

        v >>= 1;
        if(retenue)
            v |= T(1) << (sizeof(v)*8 - 1);

        return v;
    }

    template <class B> template <class T> void limitint<B>::limitint_unstack_to(T &a) throw(Ememory, Erange, Ebug)
    {
        E_BEGIN;
            // T is supposed to be an unsigned "integer"
            // (ie.: sizeof returns the width of the storage bit field  and no sign bit is present)
            // Note : static here avoids the recalculation of max_T at each call
        static const T max_T = ~T(0) > 0 ? ~T(0) : ~rotate_right_one_bit(T(1));
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

//////////////////////////////////////////////////////
///////////////// local functions ////////////////////
//////////////////////////////////////////////////////

    static void swap_bytes(unsigned char &a, unsigned char &b) throw()
    {
        E_BEGIN;
        unsigned char c = a;
        a = b;
        b = c;
        E_END("limitint.cpp : swap_bytes", "unsigned char &, unsigned char &");
    }

    static void swap_bytes(unsigned char *a, U_I size) throw()
    {
        E_BEGIN;
        if(size <= 1)
            return;
        else
        {
            swap_bytes(a[0], a[size-1]);
            swap_bytes(a+1, size-2); // terminal recursivity
        }
        E_END("limitint.cpp : swap_bytes", "unsigned char *");
    }

    static void expand_byte(unsigned char a, bitfield &bit) throw()
    {
        E_BEGIN;
        unsigned char mask = 0x80;

        for(register S_I i = 0; i < 8; i++)
        {
            bit[i] = (a & mask) >> 7 - i;
            mask >>= 1;
        }
        E_END("limitint.cpp : expand_byte", "");
    }

    template <class B> static B higher_power_of_2(B val)
    {
        B i = 0;

        while((val >> i) > 1)
            i++;

        return i;
    }

///////////////////////////////////////////////////////////////////////
///////////////// friends and not friends of limitint /////////////////
///////////////////////////////////////////////////////////////////////

    template <class B> limitint<B> operator + (const limitint<B> & a, const limitint<B> & b) throw(Erange, Ememory, Ebug, Elimitint)
    {
        E_BEGIN;
        limitint<B> ret = a;
        ret += b;

        return ret;
        E_END("operator +", "limitint");
    }

    template <class B> limitint<B> operator - (const limitint<B> & a, const limitint<B> & b) throw(Erange, Ememory, Ebug)
    {
        E_BEGIN;
        limitint<B> ret = a;
        ret -= b;

        return ret;
        E_END("operator -", "limitint");
    }

    template <class B> limitint<B> operator * (const limitint<B> & a, const limitint<B> & b) throw(Erange, Ememory, Ebug, Elimitint)
    {
        E_BEGIN;
        limitint<B> ret = a;
        ret *= b;

        return ret;
        E_END("operator *", "limitint");
    }

    template <class B> limitint<B> operator / (const limitint<B> & a, const limitint<B> & b) throw(Einfinint, Erange, Ememory, Ebug)
    {
        E_BEGIN;
        limitint<B> ret = a;
        ret /= b;

        return ret;
        E_END("operator / ", "limitint");
    }

    template <class B> limitint<B> operator % (const limitint<B> & a, const limitint<B> & b) throw(Einfinint, Erange, Ememory, Ebug)
    {
        E_BEGIN;
        limitint<B> ret = a;
        ret %= b;

        return ret;
        E_END("operator %", "limitint");
    }

    template <class B> limitint<B> operator >> (const limitint<B> & a, U_32 bit) throw(Erange, Ememory, Ebug)
    {
        E_BEGIN;
        limitint<B> ret = a;
        ret >>= bit;
        return ret;
        E_END("operator >>", "limitint, U_32");
    }

    template <class B> limitint<B> operator >> (const limitint<B> & a, const limitint<B> & bit) throw(Erange, Ememory, Ebug)
    {
        E_BEGIN;
        limitint<B> ret = a;
        ret >>= bit;
        return ret;
        E_END("operator >>", "limitint");
    }

    template <class B> limitint<B> operator << (const limitint<B> & a, U_32 bit) throw(Erange, Ememory, Ebug, Elimitint)
    {
        E_BEGIN;
        limitint<B> ret = a;
        ret <<= bit;
        return ret;
        E_END("operator <<", "limitint, U_32");
    }

    template <class B> limitint<B> operator << (const limitint<B> & a, const limitint<B> & bit) throw(Erange, Ememory, Ebug, Elimitint)
    {
        E_BEGIN;
        limitint<B> ret = a;
        ret <<= bit;
        return ret;
        E_END("operator <<", "limitint");
    }


    template <class B> static void template_instance(B x)
    {
        limitint<B> t;
        unsigned char a = 0;
        U_16 b = 0;
        U_32 c = 0;
        U_I d = 0;
        U_32 e = 0;
        off_t f = 0;
        size_t g = 0;
        time_t h = 0;
        off_t i = 0;

            // this forces template instanciation
        t.unstack(a);
        t.unstack(b);
        t.unstack(c);
        t.unstack(d);
        t.unstack(e);
        t.unstack(f);
        t.unstack(g);
        t.unstack(h);
        t = infinint(g);
        t = infinint(h);
        t = infinint(i);
	t.power(d);
	t.power(t);
        template_instance(x); // this call is never used,
            // this line is only to avoid compilation warning
            // of ... never used function :-)
    }


    template <class B> static void mega_template_instance(B u)
    {
            // this code has not the vocation to be executed !
            // but it forces the instanciation of the templates
            // becasue of the #pragma interface/implementation used
        limitint<B> x,y;
        B a;
        fichier *ptr = NULL;
        U_32 ent = 0;
        U_I ent2 = 0;
        time_t da = (time_t)0;
        off_t ta = (off_t)0;
        size_t si = (size_t)0;

        x = limitint<B>(si);
        x = limitint<B>(da);
        x = limitint<B>(ta);
        x = limitint<B>(NULL, NULL);
        x.dump(0);
        x.dump(*ptr);
        x.read(*ptr);
        x += y;
        x -= y;
        x *= y;
        x /= y;
        x %= y;
        x >>= ent;
        x >>= y;
        x <<= ent;
        x <<= y;
        ent = x % ent;
        x++; ++x;
        x--; --x;
        if(x < y || x == y || x > y || x <= y || x != y || x >= y)
            x++;
        x = x + y;
        x = x + ent2;
        x = x - y;
        x = x - ent2;
        x = x * y;
        x = x * ent2;
        x = x / y;
        x = x % y;
        x = x >> y;
        x = x >> ent;
        x = x << y;
        x = x << ent;
        euclide(x, y, x, y);
        euclide(x, ent2, y, x);
        template_instance(a);
    }

    static void maxi_mega_template()
    {
#if LIBDAR_MODE == 32
        U_32 x;
        mega_template_instance(x);
#endif
#if LIBDAR_MODE == 64
        U_64 x;
        mega_template_instance(x);
#endif
        maxi_mega_template(); // same remark as template_instance()
    }

} // end of namespace
