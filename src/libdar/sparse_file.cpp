/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2025 Denis Corbin
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

#if HAVE_LIMITS_H
#include <limits.h>
#endif

} // end extern "C"

#include "sparse_file.hpp"

#define BUFFER_SIZE 102400
#ifdef SSIZE_MAX
#if SSIZE_MAX < BUFFER_SIZE
#undef BUFFER_SIZE
#define BUFFER_SIZE SSIZE_MAX
#endif
#endif

#include "crc.hpp"

using namespace std;

namespace libdar
{

	// sparse_file class static members:

    bool sparse_file::initialized = false;
    unsigned char sparse_file::zeroed_field[SPARSE_FIXED_ZEROED_BLOCK];

	//

    sparse_file::sparse_file(generic_file *below,
			     const infinint & hole_size)
	: escape(below, std::set<sequence_type>()),
	  zero_count(0),
	  offset(0)
    {
	    // change the escape sequences fixed part to not collide with a possible underlying (the "below" object or below this "below" object) escape object
	change_fixed_escape_sequence(ESCAPE_FIXED_SEQUENCE_SPARSE_FILE);

	if(!initialized)
	{

		// we avoid the cost of using a mutex, as even if two or more threads execute this statement at the same time,
		// the first to finish will avoid other new thread to change these values executing this code,
		// while the thread already executing this code, will just redo the same thing as what has been done by the first thread to finish
		// thread (zero the area to zeros).
	    (void)memset(zeroed_field, 0, SPARSE_FIXED_ZEROED_BLOCK);
	    initialized = true;
	}

	reset();
	copy_to_no_skip = false;

	if(below == nullptr)
	    throw SRC_BUG;

	min_hole_size = hole_size;
	UI_min_hole_size = 0;
	min_hole_size.unstack(UI_min_hole_size);
	if(!min_hole_size.is_zero()) // hole size is larger than maximum buffer
	    UI_min_hole_size = 0; // disabling hole lookup inside buffers (faster execution)
	min_hole_size = hole_size; // setting back min_hole_size to its value
    }

    infinint sparse_file::get_position() const
    {
	if(is_terminated())
	    throw SRC_BUG;

	switch(get_mode())
	{
	case gf_read_only:
	    if(zero_count > offset)
		throw SRC_BUG;
	    return offset - zero_count;
	case gf_write_only:
	    return offset + zero_count;
	default:
	    throw SRC_BUG;
	}
    }

    U_I sparse_file::inherited_read(char *a, U_I size)
    {
	U_I lu = 0;
	bool eof = false;
	U_I tmp;
	U_I needed;

	if(escape_read)
	    return escape::inherited_read(a, size);

	while(lu < size && ! eof)
	{
	    needed = size - lu;

	    switch(mode)
	    {
	    case hole:
		if(zero_count.is_zero())  // start of a new hole
		{
		    if(!next_to_read_is_mark(seqt_file))
		    {
			sequence_type t;

			if(next_to_read_is_which_mark(t))
			    if(t == seqt_file)
				throw SRC_BUG;
			    else
				throw Erange("sparse_file::inherited_read", gettext("Incoherent structure in data carrying sparse files: unknown mark"));
			    // we were not at the end of file, while an escape sequence different from seqt_file was met
			else
			    eof = true; // no mark next to be read, thus we are at EOF for real
		    }
		    else
			if(skip_to_next_mark(seqt_file, false))
			{
			    read_as_escape(true);
			    try
			    {
				try
				{
				    zero_count.read(*this);
				}
				catch(Egeneric &e)
				{
				    e.prepend_message("Error while reading the size of a hole:");
				    throw;
				}
			    }
			    catch(...)
			    {
				read_as_escape(false);
				throw;
			    }
			    read_as_escape(false);
			    seen_hole = true;
			    offset += zero_count;
			}
			else
			    throw SRC_BUG; // the next to read mark was seqt_file, but could not skip forward to that mark !?!
		}
		else // zero_count > 0 , whe have not yet read all the bytes of the hole
		{
		    U_I available = 0;
		    zero_count.unstack(available);
		    if(available == 0)
			throw SRC_BUG; // could not unstack, but zero_count not equal to zero !?!
		    else
		    {
			if(needed < available)
			{
			    (void)memset(a + lu, 0, needed);
			    zero_count += available - needed;
			    lu += needed;
			}
			else
			{
			    (void)memset(a + lu, 0, available);
			    lu += available;
				// zero_count is already null, due to previous call to unstack()
			}
		    }

		    if(zero_count.is_zero())
			mode = normal;
		}
		break;
	    case normal:
		tmp = escape::inherited_read(a + lu, needed);
		if(has_escaped_data_since_last_skip())
		    data_escaped = true;
		offset += tmp;
		lu += tmp;

		if(tmp < needed)
		{
		    zero_count = 0;
		    mode = hole;
		}
		break;
	    default:
		throw SRC_BUG;
	    }
	}
	return lu;
    }

    void sparse_file::copy_to(generic_file &ref, const infinint & crc_size, crc * & value)
    {
	char buffer[BUFFER_SIZE];
	S_I lu;
	bool loop = true;
	bool last_is_skip = false;

	if(is_terminated())
	    throw SRC_BUG;

	if(!crc_size.is_zero())
	{
	    value = create_crc_from_size(crc_size);
	    if(value == nullptr)
		throw SRC_BUG;
	}
	else
	    value = nullptr;

	try
	{
	    do
	    {
		lu = escape::inherited_read(buffer, BUFFER_SIZE);
		if(has_escaped_data_since_last_skip())
		    data_escaped = true;

		if(lu > 0)
		{
		    if(!crc_size.is_zero())
			value->compute(offset, buffer, lu);
		    ref.write(buffer, lu);
		    offset += lu;
		    last_is_skip = false;
		}
		else // lu == 0
		    if(next_to_read_is_mark(seqt_file))
		    {
			if(!skip_to_next_mark(seqt_file, false))
			    throw SRC_BUG;
			else
			{
			    read_as_escape(true);
			    try
			    {
				zero_count.read(*this);
			    }
			    catch(...)
			    {
				read_as_escape(false);
				zero_count = 0;
				throw;
			    }
			    read_as_escape(false);

			    if(copy_to_no_skip)
			    {
				while(!zero_count.is_zero())
				{
				    U_I to_write = 0;
				    zero_count.unstack(to_write);
				    while(to_write > 0)
				    {
					U_I min = to_write > SPARSE_FIXED_ZEROED_BLOCK ? SPARSE_FIXED_ZEROED_BLOCK : to_write;

					ref.write((const char *)zeroed_field, min);
					to_write -= min;
				    }
				}
			    }
			    else  // using skip to restore hole into the copied-to generic_file
			    {
				offset += zero_count;
				zero_count = 0;
				if(!ref.skip(offset))
				    throw Erange("sparse_file::copy_to", gettext("Cannot skip forward to restore a hole"));
				last_is_skip = true;
				seen_hole = true;
			    }
			}
		    }
		    else // reached EOF ?
		    {
			sequence_type m;

			if(next_to_read_is_which_mark(m)) // this is not EOF, but unused mark is present
			    if(m == seqt_file)
				throw SRC_BUG; // should have been reported above by next_to_read_is_mark(seqt_file)
			    else
				throw Erange("sparse_file::copy", gettext("Data corruption or unknown sparse_file mark found in file's data"));
			else // Yes, this is the EOF
			{
			    if(last_is_skip)
			    {
				(void)ref.skip_relative(-1);
				ref.write((const char *)&zeroed_field, 1);
			    }
			    loop = false;
			}
		    }
	    }
	    while(loop);
	}
	catch(...)
	{
	    if(value != nullptr)
	    {
		delete value;
		value = nullptr;
	    }
	    throw;
	}
    }


    void sparse_file::inherited_write(const char *a, U_I size)
    {
	U_I written = 0;
	U_I hole_start = 0;
	U_I hole_length = 0;

	if(is_terminated())
	    throw SRC_BUG;

	if(escape_write)
	    return escape::inherited_write(a, size);

	while(written < size)
	{
	    switch(mode)
	    {
	    case normal:
		if(look_for_hole(a + written, size - written, UI_min_hole_size, hole_start, hole_length))
		{
		    U_I next_data = written + hole_start + hole_length;
		    if(hole_length < UI_min_hole_size)
			throw SRC_BUG; // bug in look for hole!

		    escape::inherited_write(a + written, hole_start);
		    if(has_escaped_data_since_last_skip())
			data_escaped = true;

		    if(next_data < size) // hole is inside "a"
		    {
			write_hole(hole_length);
			written = next_data;
		    }
		    else // hole is at the end of "a"
		    {
			mode = hole;
			zero_count = hole_length;
			offset += written + hole_start; // offset points at the start of the hole
			written = size; // this ends the while loop
		    }
		}
		else // no hole in the remaing data to inspect (either hole size is larger than any possible buffer or no hole could be found in buffer)
		{
		    escape::inherited_write(a + written, size - written);
		    offset += size;
		    written = size; // this ends the while loop
		    if(has_escaped_data_since_last_skip())
			data_escaped = true;
		}
		break;
	    case hole:
		if(written > 0)
		    throw SRC_BUG; // we should not pass from normal to hole inside a single call to this same method
		written = count_initial_zeros(a, size);
		if(written < size) // some normal data are present after the hole
		{
		    zero_count += written;
		    dump_pending_zeros();
		    offset -= written; // to have offset giving the offset of the first byte of "a"
			// instead of the first byte after the written hole (which is shifted in "a"
			// by an amount of "written" byte(s).
		}
		else // all data of the buffer is part of the current hole
		    zero_count += size;
		break;
	    default:
		throw SRC_BUG;
	    }
	}
    }

    void sparse_file::inherited_sync_write()
    {
	switch(mode)
	{
	case hole:
	    dump_pending_zeros();
	    break;
	case normal:
	    break;
	default:
	    throw SRC_BUG;
	}
	escape::inherited_sync_write();
    }


    void sparse_file::dump_pending_zeros()
    {
	if(mode != hole)
	    throw SRC_BUG;

	offset += zero_count;

	if(zero_count <= min_hole_size)
	{
	    U_I tmp = 0;

		// write down zeroed bytes normally (zero_count bytes)

	    do
	    {
		zero_count.unstack(tmp);

		while(tmp > 0)
		{
		    if(tmp > SPARSE_FIXED_ZEROED_BLOCK)
		    {
			escape::inherited_write((const char *)zeroed_field, SPARSE_FIXED_ZEROED_BLOCK);
			tmp -= SPARSE_FIXED_ZEROED_BLOCK;
		    }
		    else
		    {
			escape::inherited_write((const char *)zeroed_field, tmp);
			tmp = 0;
		    }
		}
	    }
	    while(!zero_count.is_zero());
	}
	else // enough data to record a hole
	{
	    write_hole(zero_count);
	}

	zero_count = 0;
	mode = normal;
    }


    void sparse_file::write_hole(const infinint & length)
    {
	add_mark_at_current_position(seqt_file);
	write_as_escape(true); // to avoid recursion dumping offset
	try
	{
	    length.dump(*this);
	}
	catch(...)
	{
	    write_as_escape(false); // back in normal writing mode
	    throw;
	}
	write_as_escape(false); // back in normal writing mode
	seen_hole = true;
    }

    void sparse_file::reset()
    {
	mode = normal;
	zero_count = 0;
	escape_write = false;
	escape_read = false;
	seen_hole = false;
	data_escaped = false;
    }


    bool sparse_file::look_for_hole(const char *a, U_I size, U_I min_hole_size, U_I & start, U_I & length)
    {
	U_I inspected = 0;
	length = 0;

	while(inspected < size)
	{
	    start = inspected;

	    while(start < size && a[start] != '\0')
		++start;

	    if(start < size)
	    {
		inspected = start + 1;
		while(inspected < size && a[inspected] == '\0')
		    ++inspected;
	    }
	    else
		inspected = start;

	    length = inspected - start;
	    if(min_hole_size > 0 && length > min_hole_size)
		inspected = size;
	    else
	    {
		++inspected;
		length = 0;
	    }
	}

	return length > 0;
    }


    U_I sparse_file::count_initial_zeros(const char *a, U_I size)
    {
	U_I curs = 0;

	while(curs < size && a[curs] == '\0')
	    ++curs;

	return curs;
    }

} // end of namespace
