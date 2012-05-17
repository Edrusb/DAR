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
// $Id: crc.cpp,v 1.12 2011/06/02 13:17:37 edrusb Rel $
//
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
} // end extern "C"

#include <iostream>
#include <sstream>

#include "generic_file.hpp"
#include "crc.hpp"

using namespace std;

#define INFININT_MODE_START 10240

namespace libdar
{

    static void n_compute(const char *buffer, U_I length, unsigned char * begin, unsigned char * & pointer, unsigned char * end, U_I crc_size);

	/////////////////////////////////////////////
	// some TEMPLATES and static routines first
	//

    template <class P> string T_crc2str(P begin, P end)
    {
	ostringstream ret;
	P curs = begin;

	while(curs != end)
	{
	    ret << hex << ((*curs & 0xF0) >> 4);
	    ret << hex << (*curs & 0x0F);
	    ++curs;
	}

	return ret.str();
    }

    template <class P> void T_old_read(P & pointer, P begin, P end, const char *buffer, U_I size)
    {
	U_I pos = 0;

	while(pointer != end && pos < size)
	{
	    *pointer = buffer[pos];
	    ++pointer;
	    ++pos;
	}

	if(pointer != end || pos < size)
	    throw SRC_BUG; // should reach both ends at the same time

	pointer = begin;
    }

    template <class B> void B_compute_block(B anonymous, const char *buffer, U_I length, unsigned char * begin, unsigned char * & pointer, unsigned char * end, U_I & cursor)
    {
	B *buf_end = (B *)(buffer + length - sizeof(anonymous) + 1);
	B *buf_ptr = (B *)(buffer);
	B *crc_end = (B *)(end);
	B *crc_ptr = (B *)(begin);

	if(begin >= end)
	    throw SRC_BUG;
	else
	{
	    U_I crc_size = end - begin;

	    if(crc_size % sizeof(anonymous) != 0)
		throw SRC_BUG;
	    if(crc_size / sizeof(anonymous) == 0)
		throw SRC_BUG;
	}


	while(buf_ptr < buf_end)
	{
	    *crc_ptr ^= *buf_ptr;
	    ++buf_ptr;
	    ++crc_ptr;
	    if(crc_ptr >= crc_end)
		crc_ptr = (B *)(begin);
	}

	cursor = (char *)(buf_ptr) - buffer;
	pointer = (unsigned char *)(crc_ptr);
    }

    template <class P> void T_compute(const char *buffer, U_I length, P begin, P & pointer, P end)
    {
	if(pointer == end)
	    throw SRC_BUG;

	for(register U_I cursor = 0; cursor < length; ++cursor)
	{
	    *pointer ^= buffer[cursor];
	    if(++pointer == end)
		pointer = begin;
	}
    }

    static void dummy_call(char *x)
    {
        static char id[]="$Id: crc.cpp,v 1.12 2011/06/02 13:17:37 edrusb Rel $";
        dummy_call(id);
    }

    static void n_compute(const char *buffer, U_I length, unsigned char * begin, unsigned char * & pointer, unsigned char * end, U_I crc_size)
    {
	U_I register cursor = 0; //< index of next byte to read from buffer

	    // initial bytes

	if(pointer != begin)
	{
	    while(pointer != end && cursor < length)
	    {
		*pointer ^= buffer[cursor];
		++cursor;
		++pointer;
	    }

	    if(pointer == end) // we had enough data to have pointer reach the end of the crc_field
		pointer = begin;
	}

		// block bytes

	if(pointer == begin && cursor < length) // we can now use the optimized rountine relying on operation by block of bytes
	{
	    U_I partial_cursor = 0;

	    if(crc_size % 8 == 0)
		B_compute_block(U_64(0), buffer + cursor, length - cursor, begin, pointer, end, partial_cursor);
	    else if(crc_size % 4 == 0)
		B_compute_block(U_32(0), buffer + cursor, length - cursor, begin, pointer, end, partial_cursor);
	    else if(crc_size % 2 == 0)
		B_compute_block(U_16(0), buffer + cursor, length - cursor, begin, pointer, end, partial_cursor);

	    cursor += partial_cursor;
	}

            // final bytes

	if(cursor < length)
	    T_compute(buffer + cursor, length - cursor, begin, pointer, end);
    }

    template <class P> bool T_compare(P me_begin, P me_end, P you_begin, P you_end)
    {
	P me = me_begin;
	P you = you_begin;

	while(me != me_end && you != you_end && *me == *you)
	{
	    ++me;
	    ++you;
	}
	return me == me_end && you == you_end;
    }

	/////////////////////////////////////////////
	// Class implementation follows
	//

    crc::crc(U_I width) : i_cyclic(1)
    {
	if(width == 0)
	    throw Erange("crc::crc", gettext("Invalid size for CRC width"));
	infinint_mode = true; // to be able to use resive_non_infinint()
	i_size = 0;
	resize_non_infinint(width);
    }

    crc::crc(const infinint & width) : i_cyclic(1)
    {
	if(width == 0)
	    throw Erange("crc::crc", gettext("Invalid size for CRC width"));
	infinint_mode = true;
	i_size = 0;
	resize(width);
    }

    bool crc::operator == (const crc & ref) const
    {
	if(infinint_mode != ref.infinint_mode)
	    return false;
	if(infinint_mode)
	{
	    if(i_size != ref.i_size)
		return false;
	    else // same size
		return T_compare(i_cyclic.begin(), i_cyclic.end(), ref.i_cyclic.begin(), ref.i_cyclic.end());
	}
	else // not infinint mode
	{
	    if(n_size != ref.n_size)
		return false;
	    else // same size
		return T_compare(n_cyclic, n_cyclic + n_size, ref.n_cyclic, ref.n_cyclic + ref.n_size);
	}
    }

    void crc::compute(const infinint & offset, const char *buffer, U_I length)
    {
	infinint tmp = offset % (infinint_mode ? i_size : n_size);

	    // first we skip the cyclic at the correct position

	if(infinint_mode)
	    i_pointer.skip_to(i_cyclic, tmp);
	else
	{
	    U_I s_offset = 0;

	    tmp.unstack(s_offset);
	    if(tmp != 0)
		throw SRC_BUG; // tmp does not fit in a U_I variable !
	    n_pointer = n_cyclic + s_offset;
	}

	    // now we can compute the CRC

	compute(buffer, length);
    }

    void crc::compute(const char *buffer, U_I length)
    {
	if(infinint_mode)
	    T_compute(buffer, length, i_cyclic.begin(), i_pointer, i_cyclic.end());
	else
	    n_compute(buffer, length, n_cyclic, n_pointer, n_cyclic + n_size, n_size);
    }

    void crc::clear()
    {
	if(infinint_mode)
	{
	    i_cyclic.clear();
	    i_pointer = i_cyclic.begin();
	}
	else
	{
	    memset(n_cyclic, 0, n_size);
	    n_pointer = n_cyclic;
	}
    }

    void crc::dump(generic_file & f) const
    {
	if(infinint_mode)
	{
	    i_size.dump(f);
	    i_cyclic.dump(f);
	}
	else
	{
	    infinint tmp = n_size;
	    tmp.dump(f);
	    f.write((const char *)n_cyclic, n_size);
	}
    }

    void crc::read(generic_file & f)
    {
	try
	{
	    i_size.read(f);
	    resize(i_size);

	    if(infinint_mode)
	    {
		i_cyclic = storage(f, i_size);
		i_pointer = i_cyclic.begin();
	    }
	    else
	    {
		f.read((char*)n_cyclic, n_size);
		n_pointer = n_cyclic;
	    }
	}
	catch(...)
	{
	    resize(1);
	    throw;
	}
    }

    void crc::old_read(generic_file & f)
    {
	char buffer[OLD_CRC_SIZE];

	if(f.read(buffer, OLD_CRC_SIZE) < (S_I)OLD_CRC_SIZE)
	    throw Erange("crc::read", gettext("Reached End of File while reading CRC data"));

	resize(OLD_CRC_SIZE);
	if(infinint_mode)
	    T_old_read(i_pointer, i_cyclic.begin(), i_cyclic.end(), buffer, OLD_CRC_SIZE);
	else
	    T_old_read(n_pointer, n_cyclic, n_cyclic + n_size, buffer, OLD_CRC_SIZE);
    }

    string crc::crc2str() const
    {
	if(infinint_mode)
	    return T_crc2str(i_cyclic.begin(), i_cyclic.end());
	else
	    return T_crc2str(n_cyclic, n_cyclic + n_size);
    }

    void crc::resize(const infinint & width)
    {
	if(width == 0)
	    throw Erange("crc::crc", gettext("Invalid size for CRC width"));
	if(width == (infinint_mode ? i_size : n_size))
	    clear();
	else
	{
	    if(width > INFININT_MODE_START)
		resize_infinint(width);
	    else
	    {
		infinint tmp = width;
		U_I n_tmp = 0;
		tmp.unstack(n_tmp);
		if(tmp > 0)
		    SRC_BUG; // INFININT_MODE_START not possible to be held in a U_I variable
		resize_non_infinint(n_tmp);
	    }
	}
    }

    void crc::resize_infinint(const infinint & width)
    {
	destroy();
	infinint_mode = true;
	i_size = width;
	i_cyclic = storage(width);
	i_cyclic.clear();
	i_pointer = i_cyclic.begin();
    }

    void crc::resize_non_infinint(U_I width)
    {
	destroy();
	infinint_mode = false;
	n_size = width;
	n_cyclic = new unsigned char[n_size];
	if(n_cyclic == NULL)
	    throw Ememory("crc::resize");
	n_pointer = n_cyclic;
	(void)memset(n_cyclic, 0, n_size);
    }

    void crc::set_crc_pointer(crc * & dst, const crc * src)
    {
	if(src == NULL)
	{
	    if(dst != NULL)
	    {
		delete dst;
		dst = NULL;
	    }
	}
	else // src != NULL
	{
	    if(dst == NULL)
	    {
		dst = new crc(*src);
		if(dst == NULL)
		    throw Ememory("crc::set_crc_pointer");
	    }
	    else
		*dst = *src;
	}
    }

    void crc::copy_from(const crc & ref)
    {
	infinint_mode = ref.infinint_mode;

	if(infinint_mode)
	{
	    i_size = ref.i_size;
	    i_cyclic = ref.i_cyclic;
	    i_pointer = i_cyclic.begin();
	}
	else
	{
	    n_size = ref.n_size;
	    n_cyclic = new unsigned char[n_size];
	    if(n_cyclic == NULL)
		throw Ememory("crc::copy_from");
	    n_pointer = n_cyclic;
	    (void)memcpy(n_cyclic, ref.n_cyclic, n_size);
	}
    }

    void crc::destroy()
    {
	if(!infinint_mode)
	{
	    if(n_cyclic != NULL)
	    {
		delete [] n_cyclic;
		n_cyclic = NULL;
	    }
	    n_size = 0;
	    n_pointer = NULL;
	}
    }


} // end of namespace

