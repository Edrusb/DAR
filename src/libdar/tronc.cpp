/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2022 Denis Corbin
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
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_STRINGS_H
#include <strings.h>
#endif

#if STDC_HEADERS
# include <string.h>
#else
# if !HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
    char *strchr (), *strrchr ();
# if !HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

#if HAVE_ERRNO_H
#include <errno.h>
#endif
} // end extern "C"

#include "tronc.hpp"

namespace libdar
{

    tronc::tronc(generic_file *f, const infinint & offset, const infinint &size, bool own_f) : generic_file(f->get_mode())
    {
        ref = f;
        sz = size;
        start = offset;
        current = size; // forces skipping the first time
	own_ref = own_f;
	limited = true;
	check_pos = true;
    }

    tronc::tronc(generic_file *f, const infinint & offset, const infinint &size, gf_mode mode, bool own_f) : generic_file(mode)
    {
        ref = f;
        sz = size;
        start = offset;
        current = size; // forces skipping the firt time
	own_ref = own_f;
	limited = true;
	check_pos = true;
    }

    tronc::tronc(generic_file *f, const infinint &offset, bool own_f) : generic_file(f->get_mode())
    {
        ref = f;
        sz = 0;
        start = offset;
        current = 0;
	own_ref = own_f;
	limited = false;
	check_pos = true;
    }

    tronc::tronc(generic_file *f, const infinint &offset, gf_mode mode, bool own_f) : generic_file(mode)
    {
        ref = f;
        sz = 0;
        start = offset;
        current = 0;
	own_ref = own_f;
	limited = false;
	check_pos = true;
    }

    void tronc::modify(const infinint & new_offset, const infinint & new_size)
    {
	modify(new_offset);
	sz = new_size;
	limited = true;
	if(current > sz)
	    current = sz;
    }

    void tronc::modify(const infinint & new_offset)
    {
	current = current + start; // current now temporarily holds the absolute position
	start = new_offset;
	if(current <= start)
	    current = 0;
	else
	    current -= start; // now current points to the same byte but within the new scope
	limited = false;
    }


    bool tronc::skippable(skippability direction, const infinint & amount)
    {
	if(is_terminated())
	    throw SRC_BUG;

	return ref->skippable(direction, amount);
    }


    bool tronc::skip(const infinint & pos)
    {
	if(is_terminated())
	    throw SRC_BUG;

        if(current == pos && check_pos) // we skip anyway when check_pos is false
            return true;

        if(limited && pos > sz)
        {
            if(ref->skip(start + sz))
		current = sz;
	    else
		(void)ref->skip(start + current);
            return false;
        }
        else
        {
	    bool ret = ref->skip(start + pos);
	    if(ret)
		current = pos;
	    else
		(void)ref->skip(start + current);
	    return ret;
        }
    }

    bool tronc::skip_to_eof()
    {
	bool ret;

	if(is_terminated())
	    throw SRC_BUG;

	if(limited)
	{
	    ret = ref->skip(start + sz);
	    if(ret)
		current = sz;
	    else
		(void)ref->skip(start + current);
	}
	else
	{
	    ret = ref->skip_to_eof();
	    if(ret)
		set_back_current_position();
	    else
		(void)skip(start + current);
	}

	return ret;
    }

    bool tronc::skip_relative(S_I x)
    {
	if(is_terminated())
	    throw SRC_BUG;

        if(x < 0)
	{
            if(current < -x)
            {
                (void)ref->skip(start);
                current = 0;
                return false;
            }
            else
            {
                bool r = ref->skip_relative(x);
                if(r)
                    current -= -x;
                else
                    ref->skip(start + current);
                return r;
            }
	}

        if(x > 0)
	{
            if(limited && current + x >= sz)
            {
                current = sz;
                (void)ref->skip(start + sz);
                return false;
            }
            else
            {
                bool r = ref->skip_relative(x);
                if(r)
                    current += x;
                else
		    (void)ref->skip(start + current);
                return r;
            }
	}

        return true;
    }

    void tronc::inherited_read_ahead(const infinint & amount)
    {
	if(limited)
	{
	    infinint avail = sz - current;
	    if(avail > amount)
		ref->read_ahead(amount);
	    else
		ref->read_ahead(avail);
	}
	else
	    ref->read_ahead(amount);
    }



    U_I tronc::inherited_read(char *a, U_I size)
    {
        U_I lu = 0;
	infinint abso_pos = start + current;

	if(check_pos && ref->get_position() != abso_pos)
	{
	    if(!ref->skip(abso_pos))
		throw Erange("tronc::inherited_read", gettext("Cannot skip to the current position in \"tronc\""));
	}

	if(limited)
	{
	    infinint avail = sz - current;
	    U_32 macro_pas = 0, micro_pas;
	    U_I ret;

	    do
	    {
		avail.unstack(macro_pas);
		micro_pas = size - lu > macro_pas ? macro_pas : size - lu;
		if(micro_pas > 0)
		{
		    ret = ref->read(a+lu, micro_pas);
		    if(ret > 0)
		    {
			lu += ret;
			macro_pas -= ret;
		    } // else ret = 0 and this ends the while loop
		}
		else
		    ret = 0;
	    }
	    while(ret > 0);
	}
	else
	    lu = ref->read(a, size);

	current += lu;

        return lu;
    }

    void tronc::inherited_write(const char *a, U_I size)
    {
        U_I wrote = 0;

	if(check_pos)
	{
	    if(!ref->skip(start + current))
		throw Erange("tronc::inherited_read", gettext("Cannot skip to the current position in \"tronc\""));
	}

	if(limited)
	{
	    infinint avail = sz - current;
	    U_32 macro_pas = 0, micro_pas;

	    do
	    {
		avail.unstack(macro_pas);
		if(macro_pas == 0 && wrote < size)
		    throw Erange("tronc::inherited_write", gettext("Tried to write out of size limited file"));
		micro_pas = size - wrote > macro_pas ? macro_pas : size - wrote;
		ref->write(a+wrote, micro_pas);
		wrote += micro_pas;
		macro_pas -= micro_pas;
	    }
	    while(wrote < size);
	}
	else
	{
	    ref->write(a, size);
	    wrote = size;
	}

	current += wrote;
    }

    void tronc::set_back_current_position()
    {
	if(is_terminated())
	    throw SRC_BUG;

	infinint ref_pos = ref->get_position();

	if(ref_pos < start)
	    throw SRC_BUG;

	if(limited)
	{
	    if(ref_pos > start + sz)
		throw SRC_BUG;
	    else
		current = ref_pos - start;
	}
	else
	    current = ref_pos - start;
    }

} // end of namespace
