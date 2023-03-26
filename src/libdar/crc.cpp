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

} // end extern "C"

#include <iostream>
#include <sstream>

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

	for(U_I cursor = 0; cursor < length; ++cursor)
	{
	    *pointer ^= buffer[cursor];
	    if(++pointer == end)
		pointer = begin;
	}
    }

    static void n_compute(const char *buffer, U_I length, unsigned char * begin, unsigned char * & pointer, unsigned char * end, U_I crc_size)
    {
	U_I cursor = 0; //< index of next byte to read from buffer

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

		// But we cannot use optimized method on some systems if we are not aligned to the size boundary
	    if(crc_size % 8 == 0 && (U_I)(buffer + cursor) % 8 == 0)
		B_compute_block(U_64(0), buffer + cursor, length - cursor, begin, pointer, end, partial_cursor);
	    else if(crc_size % 4 == 0 && (U_I)(buffer + cursor) % 4 == 0)
		B_compute_block(U_32(0), buffer + cursor, length - cursor, begin, pointer, end, partial_cursor);
	    else if(crc_size % 2 == 0 && (U_I)(buffer + cursor) % 2 == 0)
		B_compute_block(U_16(0), buffer + cursor, length - cursor, begin, pointer, end, partial_cursor);
		/// warning, adding a new type here need modifying crc_n::alloc() to provide aligned crc storage

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
	// Class CRC_I implementation follows
	//


    crc_i::crc_i(const infinint & width) : size(width), cyclic(width)
    {
	if(width.is_zero())
	    throw Erange("crc::crc", gettext("Invalid size for CRC width"));
	clear();
    }

    crc_i::crc_i(const infinint & width, proto_generic_file & f) : size(width), cyclic(f, width)
    {
	pointer = cyclic.begin();
    }

    bool crc_i::operator == (const crc & ref) const
    {
	const crc_i *ref_i = dynamic_cast<const crc_i *>(&ref);
	if(ref_i == nullptr)
	    throw SRC_BUG;

	if(size != ref_i->size)
	    return false;
	else // same size
	    return T_compare(cyclic.begin(), cyclic.end(), ref_i->cyclic.begin(), ref_i->cyclic.end());
    }

    void crc_i::compute(const infinint & offset, const char *buffer, U_I length)
    {
	infinint tmp = offset % size;

	    // first we skip the cyclic at the correct position

	pointer.skip_to(cyclic, tmp);

	    // now we can compute the CRC

	compute(buffer, length);
    }

    void crc_i::compute(const char *buffer, U_I length)
    {
	T_compute(buffer, length, cyclic.begin(), pointer, cyclic.end());
    }

    void crc_i::clear()
    {
	cyclic.clear();
	pointer = cyclic.begin();
    }

    void crc_i::dump(proto_generic_file & f) const
    {
	size.dump(f);
	cyclic.dump(f);
    }

    string crc_i::crc2str() const
    {
	return T_crc2str(cyclic.begin(), cyclic.end());
    }

    void crc_i::copy_from(const crc_i & ref)
    {
	if(size != ref.size)
	{
	    size = ref.size;
	    cyclic = ref.cyclic;
	}
	else
	    copy_data_from(ref);
	pointer = cyclic.begin();
    }

    void crc_i::copy_data_from(const crc_i & ref)
    {
	if(ref.size == size)
	{
	    storage::iterator ref_it = ref.cyclic.begin();
	    storage::iterator it = cyclic.begin();

	    while(ref_it != ref.cyclic.end() && it != cyclic.end())
	    {
		*it = *ref_it;
		++it;
		++ref_it;
	    }
	    if(ref_it != ref.cyclic.end() || it != cyclic.end())
		throw SRC_BUG;
	}
	else
	    throw SRC_BUG;
    }


	/////////////////////////////////////////////
	// Class CRC_N implementation follows
	//


    crc_n::crc_n(U_I width)
    {
	pointer = nullptr;
	cyclic = nullptr;
	try
	{
	    if(width == 0)
		throw Erange("crc::crc", gettext("Invalid size for CRC width"));
	    alloc(width);
	    clear();
	}
	catch(...)
	{
	    destroy();
	    throw;
	}
    }

    crc_n::crc_n(U_I width, proto_generic_file & f)
    {
	pointer = nullptr;
	cyclic = nullptr;
	try
	{
	    alloc(width);
	    f.read((char*)cyclic, size);
	}
	catch(...)
	{
	    destroy();
	    throw;
	}
    }

    crc_n & crc_n::operator = (const crc_n & ref)
    {
	if(size != ref.size)
	{
	    destroy();
	    copy_from(ref);
	}
	else
	    copy_data_from(ref);

	return *this;
    }

    bool crc_n::operator == (const crc & ref) const
    {
	const crc_n *ref_n = dynamic_cast<const crc_n *>(&ref);
	if(ref_n == nullptr)
	    throw SRC_BUG;

	if(size != ref_n->size)
	    return false;
	else // same size
	    return T_compare(cyclic, cyclic + size, ref_n->cyclic, ref_n->cyclic + ref_n->size);
    }

    void crc_n::compute(const infinint & offset, const char *buffer, U_I length)
    {
	infinint tmp = offset % size;
	U_I s_offset = 0;

	    // first we skip the cyclic at the correct position

	tmp.unstack(s_offset);
	if(tmp != 0)
	    throw SRC_BUG; // tmp does not fit in a U_I variable !
	pointer = cyclic + s_offset;

	    // now we can compute the CRC

	compute(buffer, length);
    }

    void crc_n::compute(const char *buffer, U_I length)
    {
	n_compute(buffer, length, cyclic, pointer, cyclic + size, size);
    }

    void crc_n::clear()
    {
	(void)memset(cyclic, 0, size);
	pointer = cyclic;
    }

    void crc_n::dump(proto_generic_file & f) const
    {
	infinint tmp = size;
	tmp.dump(f);
	f.write((const char *)cyclic, size);
    }


    string crc_n::crc2str() const
    {
	return T_crc2str(cyclic, cyclic + size);
    }

    void crc_n::alloc(U_I width)
    {
	size = width;

	    //////////////////////////////////////////////////////////////////////
	    // the following trick is to have cyclic aligned at its boundary size
	    // (its allocated address is a multiple of it size)
	    // some CPU need that (sparc), and it does not hurt for other ones.

	if(width % 8 == 0)
	    cyclic = (unsigned char *)(new (nothrow) U_64[width/8]);
	else if(width % 4 == 0)
	    cyclic = (unsigned char *)(new (nothrow) U_32[width/4]);
	else if(width % 2 == 0)
	    cyclic = (unsigned char *)(new (nothrow) U_16[width/2]);
	else
	    cyclic = new (nothrow) unsigned char[size];
	    // end of the trick and back to default situation

	    //////////////////////////////////////////////////////////////////////
	    // WARNING! this trick allows the use of 2, 4 or 8 bytes operations //
	    // instead of byte by byte one, in n_compute calls B_compute_block  //
	    // CODE MUST BE ADAPTED THERE AND IN destroy() IF CHANGED HERE!!!   //
	    //////////////////////////////////////////////////////////////////////

	if(cyclic == nullptr)
	    throw Ememory("crc::copy_from");
	pointer = cyclic;
    }

    void crc_n::copy_from(const crc_n & ref)
    {
	alloc(ref.size);
	copy_data_from(ref);
    }

    void crc_n::copy_data_from(const crc_n & ref)
    {
	if(size != ref.size)
	    throw SRC_BUG;
	(void)memcpy(cyclic, ref.cyclic, size);
	pointer = cyclic;
    }

    void crc_n::destroy()
    {
	if(cyclic != nullptr)
	{
	    delete [] cyclic;
	    cyclic = nullptr;
	}
	size = 0;
	pointer = nullptr;
    }

	/////////////////////////////////////////////
	// exported routines implementation
	//

    crc *create_crc_from_file(proto_generic_file & f, bool old)
    {
	crc *ret = nullptr;
	if(old)
	    ret = new (nothrow) crc_n(crc::OLD_CRC_SIZE, f);
	else
	{
	    infinint taille = f; // reading the crc size

	    if(taille < INFININT_MODE_START)
	    {
		U_I s = 0;
		taille.unstack(s);
		if(!taille.is_zero())
		    throw SRC_BUG;
		ret = new (nothrow) crc_n(s, f);
	    }
	    else
		ret = new (nothrow) crc_i(taille, f);
	}

	if(ret == nullptr)
	    throw Ememory("create_crc_from_file");

	return ret;
    }

    crc *create_crc_from_size(infinint width)
    {
	crc *ret = nullptr;

	if(width < INFININT_MODE_START)
	{
	    U_I s = 0;
	    width.unstack(s);
	    if(!width.is_zero())
		throw SRC_BUG;

	    ret = new (nothrow) crc_n(s);
	}
	else
	    ret = new (nothrow) crc_i(width);

	if(ret == nullptr)
	    throw Ememory("create_crc_from_size");

	return ret;
    }

} // end of namespace
