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
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

#include "../my_config.h"

#include <iostream>
#include "deci.hpp"
#include "erreurs.hpp"
#include "integers.hpp"
#include "nls_swap.hpp"

using namespace std;

namespace libdar
{

    typedef unsigned char chiffre;

    static const U_I PAS = 5;
    static inline chiffre get_left(unsigned char a) { return (a & 0xF0) >> 4; }
    static inline chiffre get_right(unsigned char a) { return a & 0x0F; }
    static inline void set_left(unsigned char & a, chiffre val) { val <<= 4; a &= 0x0F; a |= val; }
    static inline void set_right(unsigned char & a, chiffre val) { val &= 0x0F; a &= 0xF0; a |= val; }

    static inline chiffre digit_htoc(unsigned char c)
    {
        if(c < '0' || c > '9')
            throw Edeci("deci.cpp : digit_htoc", gettext("invalid decimal digit"));
        return chiffre(c - '0');
    }

    static inline unsigned char digit_ctoh(chiffre c)
    {
        if(c > 9) throw SRC_BUG;
        return '0' + c;
    }

    template <class T> void decicoupe(storage * &decimales, T x, memory_pool *p)
    {
	NLS_SWAP_IN;

	decimales = nullptr;
	try
	{
	    chiffre r;
	    const T d_t = 10;
	    T r_t;
	    storage::iterator it;
	    bool recule = false;
	    unsigned char tmp = 0;

	    decimales = new (p) storage(PAS);
	    if(decimales == nullptr)
		throw Ememory("template deci::decicoupe");

	    decimales->clear(0xFF);
	    it = decimales->rbegin();

	    while(x > 0 || recule)
	    {
		if(x > 0)
		{
		    euclide(x,d_t,x,r_t);
		    r = 0;
		    r_t.unstack(r);
		}
		else
		    r = 0xF; // not significative information
		if(recule)
		{
		    set_left(tmp, chiffre(r));
		    if(it == decimales->rend())
		    {
			decimales->insert_const_bytes_at_iterator(decimales->begin(), 0xFF, PAS);
			it = decimales->begin() + PAS - 1;
		    }
		    *(it--) = tmp;
		}
		else
		    set_right(tmp, chiffre(r));
		recule = ! recule;
	    }
	}
	catch(...)
	{
	    if(decimales != nullptr)
	    {
		delete decimales;
		decimales = nullptr;
	    }
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    deci::deci(string s)
    {
	NLS_SWAP_IN;

	decimales = nullptr;
	try
	{
	    string::reverse_iterator it = s.rbegin();
	    storage::iterator ut;
	    bool recule = false;
	    unsigned char tmp = 0xFF;

	    U_I size = s.size() / 2;
	    if(s.size() % 2 != 0)
		size++;

	    if(size == 0) // empty string
		throw Erange("deci::deci(string s)", gettext("an empty string is an invalid argument"));

	    decimales = new (get_pool()) storage(size);
	    if(decimales == nullptr)
		throw Ememory("deci::deci(string s)");
	    decimales->clear(0xFF); // FF is not a valid couple of decimal digit

	    ut = decimales->rbegin();
	    while(it != s.rend() || recule)
	    {
		if(recule)
		{
		    if(it != s.rend())
			set_left(tmp, digit_htoc(*it));
		    else
			set_left(tmp, 0xF);
		    if(ut == decimales->rend())
			throw SRC_BUG;
		    *(ut--) = tmp;
		}
		else
		    set_right(tmp, digit_htoc(*it));

		recule = ! recule;
		if(it != s.rend())
		    it++; // it is a reverse iterator thus ++ for going backward
	    }

	    reduce();
	}
	catch(...)
	{
	    if(decimales != nullptr)
		delete decimales;
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    deci::deci(const infinint & x)
    {
	try
	{
	    decicoupe(decimales, x, get_pool());
	    reduce();
	}
	catch(...)
	{
	    if(decimales != nullptr)
		delete decimales;
	    throw;
	}
    }

    void deci::copy_from(const deci & ref)
    {
        if(decimales != nullptr)
            throw SRC_BUG;

        decimales = new (get_pool()) storage(*ref.decimales);
	if(decimales == nullptr)
	    throw SRC_BUG;
    }

    void deci::detruit()
    {
        if(decimales != nullptr)
        {
            delete decimales;
            decimales = nullptr;
        }
    }


    void deci::reduce()
    {
        bool avance = false, leading_zero = true;
        chiffre tmp;
        infinint justif_size = 0;

        if(decimales == nullptr)
            throw SRC_BUG;

        storage::iterator it = decimales->begin();

        while(it != decimales->end() && leading_zero)
        {
            if(avance)
                tmp = get_right(*it);
            else
                tmp = get_left(*it);

            if(tmp == 0 && leading_zero)
            {
                if(avance)
                    set_right(*it, 0xF);
                else
                    set_left(*it, 0xF);

                tmp = 0xF;
            }

            if(tmp == 0xF)
	    {
                if(leading_zero)
                {
                    if(avance)
                        ++justif_size;
                }
                else
                    throw SRC_BUG;
	    }

            if(tmp != 0 && tmp != 0xF)
                leading_zero = false;

            if(avance)
                ++it;

            avance = ! avance;
        }

        if(justif_size == decimales->size())
        {
            --justif_size;
            it = decimales->rbegin();
            *it = 0xF0; // need at least one digit
        }
        if(justif_size > 0)
            decimales->remove_bytes_at_iterator(decimales->begin(), justif_size);
    }

    string deci::human() const
    {
        string s = "";
        storage::iterator it = decimales->begin(), fin = decimales->end();
        bool avance = false;
        chiffre c;

        while(it != fin)
        {
            if(avance)
            {
                c = get_right(*it);
                ++it;
            }
            else
                c = get_left(*it);

            if(c != 0xF)
                s = s + string(1, digit_ctoh(c));

            avance = ! avance;
        }

        return s;
    }

    infinint deci::computer() const
    {
        infinint r = 0;
        storage::iterator it = decimales->begin(), fin = decimales->end();
        bool avance = false;
        chiffre c;

        while(it != fin)
        {
            if(avance)
            {
                c = get_right(*it);
                ++it;
            }
            else
                c = get_left(*it);

            if(c != 0xF)
            {
                r *= 10;
                r += c;
            }

            avance = ! avance;
        }

        return r;
    }

    ostream & operator << (ostream & ref, const infinint & arg)
    {
        deci tmp = arg;
        ref << tmp.human();

        return ref;
    }

} // end of namespace
