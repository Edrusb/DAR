/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2019 Denis Corbin
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

#include "escape.hpp"
#include "tools.hpp"

extern "C"
{
#ifdef HAVE_STRIN_H
#include <string.h>
#endif
} // extern "C"

using namespace std;

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


namespace libdar
{

	//-- class static variable (well constant to be correct)

	// escape sequence structure
	//
	// fixed pattern (5 bytes)  + escape sequence type (1 byte)         = total length: 6 bytes
	// 0xAD 0xFD 0xEA 0x77 0x21 + { 'X' | 'H' | 'F' | 'E' | 'C' | ... }
	//
	// the fixed pattern may have its first byte (which default value is 0xAD) modified if necessary to
	// avoid escaping of escape sequences when using two escape objects, one writing its data to a second one.

    const unsigned char escape::usual_fixed_sequence[ESCAPE_SEQUENCE_LENGTH] = { ESCAPE_FIXED_SEQUENCE_NORMAL, 0xFD, 0xEA, 0x77, 0x21, 0x00 };
    const infinint escape::READ_BUFFER_SIZE_INFININT = MAX_BUFFER_SIZE;


	//-- class routines

    escape::escape(generic_file *below, const set<sequence_type> & x_unjumpable) : generic_file(below->get_mode())
    {
	x_below = below;
	if(below == nullptr)
	    throw SRC_BUG;
	write_buffer_size = 0;
	read_buffer_size = 0;
	read_eof = false;
	already_read = 0;
	escape_seq_offset_in_buffer = 0;
	escaped_data_count_since_last_skip = 0;
	below_position = x_below->get_position();
	unjumpable = x_unjumpable;
	for(U_I i = 0 ; i < ESCAPE_SEQUENCE_LENGTH; ++i)
	    fixed_sequence[i] = usual_fixed_sequence[i];

    }

    escape::~escape()
    {
	try
	{
	    terminate();
	}
	catch(...)
	{
		// ignore all exceptions
	}
    }

    escape & escape::operator = (const escape & ref)
    {
	generic_file *me = this;
	const generic_file *you = &ref;

	if(is_terminated())
	    throw SRC_BUG;

	*me = *you; // copying the generic_file data
	copy_from(ref); // copying the escape specific data

	return *this;
    }

    void escape::add_mark_at_current_position(sequence_type t)
    {
	    // some necessary sanity checks
	if(is_terminated())
	    throw SRC_BUG;

	if(get_mode() == gf_read_only)
	    throw SRC_BUG;
	check_below();

	    // sanity checks done

	if(t == seqt_not_a_sequence)
	    throw Erange("escape::add_mark_at_current_position", gettext("Adding an explicit escape sequence of type seqt_not_a_sequence is forbidden"));

	flush_write();
	escaped_data_count_since_last_skip = 0;
	set_fixed_sequence_for(t);
	x_below->write((const char*)fixed_sequence, ESCAPE_SEQUENCE_LENGTH);
	below_position += ESCAPE_SEQUENCE_LENGTH;
    }

    bool escape::skip_to_next_mark(sequence_type t, bool jump)
    {
	bool found = false;

	if(is_terminated())
	    throw SRC_BUG;

	    // check whether we are not in write mode !!!
	if(get_mode() != gf_read_only)
	    throw SRC_BUG; // escape implementation does not support this mode

	read_eof = false; // may be have been set because we reached a mark while reading data, so we now need to unset it
	escaped_data_count_since_last_skip = 0;

	do
	{
		// looking at data currently in the buffer

	    if(escape_seq_offset_in_buffer < read_buffer_size) // amorce found in buffer
	    {
		already_read = escape_seq_offset_in_buffer; // dropping data before that start of escape sequence

		    // at that time, this may be just the start of what seems to be an escape mark,
		    // so we need more data, which may turn out to show that this is not an escape mark
		if(!mini_read_buffer())
		{
		    read_eof = true; // not enough data available, thus
		    clean_read();
		}
		else // we could get more data to determine whether we have a mark in the buffer or not
		{
		    if(escape_seq_offset_in_buffer + ESCAPE_SEQUENCE_LENGTH - 1 < read_buffer_size)
		    {
			sequence_type found_type = char2type(read_buffer[escape_seq_offset_in_buffer + ESCAPE_SEQUENCE_LENGTH - 1]);
			if(found_type == seqt_not_a_sequence)
			{
				// this is just escaped data, so we skip it
			    already_read = escape_seq_offset_in_buffer + ESCAPE_SEQUENCE_LENGTH;
				// moving the marker to the next possible escape sequence
			    escape_seq_offset_in_buffer = already_read + trouve_amorce(read_buffer + already_read, read_buffer_size - already_read, fixed_sequence);
			}
			else  // real mark found
			    if(found_type == t) // found the expected type of mark
			    {
				found = true;
				already_read = escape_seq_offset_in_buffer + ESCAPE_SEQUENCE_LENGTH;
				escape_seq_offset_in_buffer = already_read + trouve_amorce(read_buffer + already_read, read_buffer_size - already_read, fixed_sequence);
			    }
			    else
				if(jump && unjumpable.find(found_type) == unjumpable.end()) // not an unjumpable mark or jump allowed for any mark
				{
				    already_read = escape_seq_offset_in_buffer + ESCAPE_SEQUENCE_LENGTH;
				    escape_seq_offset_in_buffer = already_read + trouve_amorce(read_buffer + already_read, read_buffer_size - already_read, fixed_sequence);
				}
				else // not an unjumpable mark
				    read_eof = true;
		    }
		    else // what seemed to be the start of a mark is not a mark
			already_read = escape_seq_offset_in_buffer;
		}
	    }
	    else // no mark in current data
	    {
		    // dropping all data in read_buffer, and filling it again with some new data
		read_buffer_size = x_below->read(read_buffer, READ_BUFFER_SIZE);
		below_position += read_buffer_size;
		if(read_buffer_size == 0)
		    read_eof = true;
		already_read = 0;
		escape_seq_offset_in_buffer = trouve_amorce(read_buffer, read_buffer_size, fixed_sequence);
	    }
	}
	while(!found && !read_eof);

	return found;
    }

    bool escape::next_to_read_is_mark(sequence_type t)
    {
	sequence_type toberead;

	if(is_terminated())
	    throw SRC_BUG;

	if(next_to_read_is_which_mark(toberead))
	    return t == toberead;
	else
	    return false;
    }

    void escape::remove_unjumpable_mark(sequence_type t)
    {
	set<sequence_type>::iterator it = unjumpable.find(t);

	if(is_terminated())
	    throw SRC_BUG;

	if(it != unjumpable.end())
	    unjumpable.erase(it);
    }

    bool escape::next_to_read_is_which_mark(sequence_type & t)
    {
	if(is_terminated())
	    throw SRC_BUG;

	check_below();

	if(get_mode() != gf_read_only)
	    throw SRC_BUG;

	if(escape_seq_offset_in_buffer > already_read) //no next to read mark
	    return false;

	    // if read_buffer size is less than ESCAPE_SEQUENCE MARK, then read some data for that
	if(mini_read_buffer())
	{
	    if(read_buffer_size - already_read < ESCAPE_SEQUENCE_LENGTH)
		throw SRC_BUG;

		// check the data in the read_buffer

	    if(escape_seq_offset_in_buffer == already_read)
	    {
		t = char2type(read_buffer[already_read + ESCAPE_SEQUENCE_LENGTH - 1]);
		if(t == seqt_not_a_sequence)
		    throw SRC_BUG; // mini_read_buffer did not made its job properly!
		return true;
	    }
	    else
		return false; // no escape sequence found next to be read
	}
	else // not enough data available in x_below to form a escape sequence mark (eof reached)
	    return false;
    }

    bool escape::skippable(skippability direction, const infinint & amount)
    {
	infinint new_amount = amount;

	switch(get_mode())
	{
	case gf_read_only:
	    return x_below->skippable(direction, new_amount);
	case gf_write_only:
	case gf_read_write:
	    switch(direction)
	    {
	    case skip_backward:
		new_amount += ESCAPE_SEQUENCE_LENGTH;
		    // we read some bytes before to check fo escape sequence
		break;
	    case skip_forward:
		break;
	    default:
		throw SRC_BUG;
	    }

	    if(direction == skip_forward)
		return false;
	    else
		return x_below->skippable(direction, new_amount);
	default:
	    throw SRC_BUG;
	}
    }

    bool escape::skip(const infinint & pos)
    {
	bool ret = true;

	if(is_terminated())
	    throw SRC_BUG;

	check_below();

	escaped_data_count_since_last_skip = 0;
	if(get_position() == pos)
	    return true;

	switch(get_mode())
	{
	case gf_read_only:
	    if(pos >= below_position - read_buffer_size
	       &&
	       pos < below_position)
	    {
		    // requested position is in read_buffer

		infinint delta = below_position - pos;
		already_read = 0;
		delta.unstack(already_read);
		if(!delta.is_zero())
		    throw SRC_BUG;
		already_read = read_buffer_size - already_read;
		    // this leads to the following:
		    // alread_read = read_buffer_size - (below_position - pos);

		escape_seq_offset_in_buffer = already_read + trouve_amorce(read_buffer + already_read, read_buffer_size - already_read, fixed_sequence);
		escaped_data_count_since_last_skip = 0;
		read_eof = false;
	    }
	    else
	    {

		    // requested position is out of read_buffer

		read_eof = false;
		flush_or_clean();
		ret = x_below->skip(pos);
		if(ret)
		    below_position = pos;
		else
		    below_position = x_below->get_position();
	    }
	    break;
	case gf_write_only:
	    if(get_position() != pos)
		throw Efeature("Skipping on write_only escape object");
	    else
		ret = true;
	    break;
	case gf_read_write:
		// only backward skipping is allowed in that mode
	    if(get_position() < pos)
		throw Efeature("Skipping forward not implemented in write mode for escape class");
	    else
	    {
		char tmp_buffer[WRITE_BUFFER_SIZE];
		infinint cur_below = below_position;
		U_I trouve;

		try
		{
		    if(pos >= ESCAPE_SEQUENCE_LENGTH)
		    {
			U_I lu = 0;

			below_position = pos - ESCAPE_SEQUENCE_LENGTH;
			ret = x_below->skip(below_position);

			if(ret)
			{
			    lu = x_below->read(tmp_buffer, ESCAPE_SEQUENCE_LENGTH);
			    below_position += lu;
			    write_buffer_size = lu;
			}
			else
			    below_position = x_below->get_position();
		    }
		    else // skipping very close after the start of file, no escape mark can take place there
		    {
			U_I width = 0;
			U_I lu = 0;
			infinint tmp = pos;
			tmp.unstack(width);
			if(tmp != 0)
			    throw SRC_BUG;
			width = ESCAPE_SEQUENCE_LENGTH - width;
			if(!x_below->skip(0))
			    throw SRC_BUG;  // should succeed or throw an exception in that situation (backward skipping)
			below_position = 0;
			lu = x_below->read(tmp_buffer, width); // may throw exception
			write_buffer_size = lu;
			below_position += lu;
			ret = true;
		    }
		}
		catch(...)
		{
		    x_below->skip(cur_below);
		    below_position = cur_below;
		    throw;
		}
		(void)memcpy(write_buffer, tmp_buffer, write_buffer_size);
		trouve = trouve_amorce(write_buffer, write_buffer_size, fixed_sequence);
		if(trouve == 0) // we read a whole escape sequence
		    write_buffer_size = 0; // so we know that we can restart the lookup process here from scratch
		else if(trouve == write_buffer_size) // no start of escape sequence found
		    write_buffer_size = 0; // so we know that we can restart the lookup process here from scratch
		else // partial escape sequence found, moving at the beginning of the write_buffer for further lookup
		{
		    (void)memmove(write_buffer, write_buffer + trouve, write_buffer_size - trouve);
		    write_buffer_size -= trouve;
		}
	    }
	    break;
	default:
	    throw SRC_BUG; // this mode is not allowed
	}

	return ret;
    }

    bool escape::skip_to_eof()
    {
	bool ret;

	if(is_terminated())
	    throw SRC_BUG;

	check_below();

	if(get_mode() != gf_read_only)
	    throw Efeature("Skipping not implemented in write mode for escape class");
	    // if the buffer is neither empty not full, we cannot know what to do with this date
	    // either place it asis in the below file, or escape it in the below file.
	flush_or_clean();
	read_eof = true;
	escaped_data_count_since_last_skip = 0;

	ret = x_below->skip_to_eof();
	below_position = x_below->get_position();

	return ret;
    }

    bool escape::skip_relative(S_I x)
    {
	bool ret;

	if(is_terminated())
	    throw SRC_BUG;

	if(x == 0)
	    return true;

	check_below();
	read_eof = false;
	escaped_data_count_since_last_skip = 0;
	if(get_mode() != gf_read_only)
	    throw Efeature("Skipping not implemented in write mode for escape class");
	    // if the buffer is neither empty not full, we cannot know what to do with this date
	    // either place it asis in the below file, or escape it in the below file.
	flush_or_clean();
	ret = x_below->skip_relative(x);
	if(ret) // skipping succeeded
	{
	    if(x >= 0)
		below_position += x;
	    else  // x is negative
	    {
		if(below_position < -x)
		    below_position = 0;
		else
		    below_position -= -x; // trick used, because infinint cannot be negative
	    }
	}
	else // skipping failed, need to consult x_below to know where we are now
	    below_position = x_below->get_position();

	return ret;
    }

    infinint escape::get_position() const
    {
	if(is_terminated())
	    throw SRC_BUG;

	check_below();
	if(get_mode() == gf_read_only)
	    return below_position - read_buffer_size + already_read - escaped_data_count_since_last_skip;
	else
	    return below_position + write_buffer_size - escaped_data_count_since_last_skip;
    }

    void escape::inherited_read_ahead(const infinint & amount)
    {
	if(is_terminated())
	    throw SRC_BUG;

	check_below();
	if(!read_eof)
	{
	    U_I avail = read_buffer_size - already_read;
	    infinint i_avail = avail;
	    if(i_avail < amount)
		x_below->read_ahead(amount - i_avail);
	}
    }


    U_I escape::inherited_read(char *a, U_I size)
    {
	U_I returned = 0;

	    // #############  if EOF -> stop
	if(read_eof && already_read == read_buffer_size)
	    return 0; // eof reached. (real eof or next to read is a real mark)

	    // ############# if read_buffer not empty (we copy as needed and available data from the buffer into "a") up to the first mark

	bool loop = true;

	do
	{
	    if(escape_seq_offset_in_buffer < already_read)
		throw SRC_BUG;

	    U_I avail = escape_seq_offset_in_buffer - already_read;
	    if(avail > 0)
	    {
		U_I needed = size - returned;
		U_I min_cp = avail > needed ? needed : avail;

		(void)memcpy(a + returned, read_buffer + already_read, min_cp);
		returned += min_cp;
		already_read += min_cp;
	    }

	    if(already_read == read_buffer_size)
	    {
		already_read = read_buffer_size = 0;
		escape_seq_offset_in_buffer = 0;
	    }

	    if(returned == size)
		return returned;

	    if(returned > size)
		throw SRC_BUG;

	    if(already_read != read_buffer_size)
	    {
		    // some data remains in the buffer (either more than requested, or due to a real or data mark found in it)

		if(already_read != escape_seq_offset_in_buffer)
		    throw SRC_BUG; // more data was requested but could not be delivered, while no mark is next to be read from read_buffer!?


		if(mini_read_buffer()) // se we complete it and eventually unescape data from data mark : mini_read_buffer() does this
		{
			// there is now enough data in buffer_size to tell that this is a real completed mark

		    if(escape_seq_offset_in_buffer == already_read) // there is a real mark next to be read in the buffer
		    {
			    // no more real data to be read
			read_eof = true;
			loop = false;
		    }
		    else
		    {
			    // the real mark is not the next to be read, some data have been unescaped before it
			loop = true;
		    }
		}
		else // data in buffer is not a mark, just a truncated mark at an EOF so we can take it as pure data
		{
		    escape_seq_offset_in_buffer = read_buffer_size;
		    loop = true;
		}
	    }
	    else
		loop = false;
	}
	while(loop);

	    // ############# OK, now, read_buffer is empty, no eof/mark met and still more data needed.
	    // reading data from "below" directly into "a" after already placed data (if there is enough place to detect marks)

	loop = !read_eof;
	    // if all data could be read, we already returned from this function, so we do not reach this statement,
	    // if data remains in read_buffer, this is thus because we need more, but either the buffer is empty
	    // or we met a real mark, so we reached "eof" and must not continue the reading in the following loop.

	while(loop)
	{
	    U_I needed = size - returned;
	    U_I read;

	    if(needed > ESCAPE_SEQUENCE_LENGTH)
	    {
		U_I delta;

		    // filling missing data in "a" from x_below

		read = x_below->read(a + returned, needed);
		below_position += read;
		if(read < needed)
		    read_eof = true;

		    // analyse the new data, unescape data sequences, (skip left for each escaped data) and stop at the first non data escape sequence
		    // we temporarily use the variable escape_seq_offset_in_buffer to point in "a" instead of "read_buffer"
		escape_seq_offset_in_buffer = remove_data_marks_and_stop_at_first_real_mark(a + returned, read, delta, fixed_sequence);
		escaped_data_count_since_last_skip += delta;
		read -= delta;
		if(escape_seq_offset_in_buffer > read)
		    throw SRC_BUG;
		returned += escape_seq_offset_in_buffer;

		if(escape_seq_offset_in_buffer < read)
		{
			// mark found in data

			// copy back the remaining data to read_buffer

		    if(READ_BUFFER_SIZE < read - escape_seq_offset_in_buffer)
			throw SRC_BUG; // read_buffer too small to store all in-transit data
		    read_buffer_size = read - escape_seq_offset_in_buffer;
		    escape_seq_offset_in_buffer = 0;
		    already_read = 0;
			// not setting yet read_eof, as it could be a false mark (starting like a mark, but not enough data to determin the real nature of the sequence)
		    (void)memcpy(read_buffer, a + returned, read_buffer_size);
		    read_eof = false; // because we moved out data from the one ready to be returned
			// eithet this was not the eof, and thus this call does not change anything
			// or it was EOF because data "read" was less than "needed"
			// but here some data will still be in read_buffer so EOF must be cleaned

			// be sure the mark is completed and return as much as requested data if not a complet mark

		    needed = size - returned;
		    if(needed > 0)
		    {
			read = escape::inherited_read(a + returned, needed); // recursive call <!>
			returned += read;
		    }
		    loop = false;
		}
		else // no mark found in data, all that got read is pure data and is directly sent to the upper layer
		{
		    escape_seq_offset_in_buffer = read_buffer_size; // both should be equal to zero now
		    loop = returned < size && !read_eof;
		}
	    }
	    else // too short space in "a" to put data and be sure there is not any mark in it, so we use read_buffer again
	    {
		(void)mini_read_buffer(); // filling the read_buffer
		if(escape_seq_offset_in_buffer > 0) // some more data available in read_buffer
		    returned += escape::inherited_read(a + returned, needed); // recursive call <!>
		else
		    read_eof = true;
		loop = false;
	    }
	}

	    // return the amount of data put into "a"
	return returned;
    }

    void escape::inherited_write(const char *a, U_I size)
    {
	U_I written = 0;
	U_I trouve;

	if(size == 0)
	    return; // nothing to do

	try
	{

	    if(write_buffer_size > 0) // some data are pending in transit
	    {
		U_I initial_buffer_size = write_buffer_size;

		if(write_buffer_size >= ESCAPE_SEQUENCE_LENGTH - 1)
		    throw SRC_BUG;

		    // filling the buffer

		U_I delta = WRITE_BUFFER_SIZE - write_buffer_size; // available room in write_buffer
		delta = delta > size ? size : delta;
		(void)memcpy(write_buffer + write_buffer_size, a, delta);
		write_buffer_size += delta;
		written += delta;

		    // checking for escape sequence in write_buffer

		trouve = trouve_amorce(write_buffer, write_buffer_size, fixed_sequence);
		if(trouve == write_buffer_size) // no escape sequence found
		{
		    x_below->write(write_buffer, write_buffer_size);
		    below_position += write_buffer_size;
		    write_buffer_size = 0;
		}
		else // start of escape sequence found
		{
		    if(trouve + ESCAPE_SEQUENCE_LENGTH - 1 <= write_buffer_size) // no doubt, we have a full escape sequence in data, we need to protect this data
		    {
			x_below->write(write_buffer, trouve);
			below_position += trouve;
			set_fixed_sequence_for(seqt_not_a_sequence);
			x_below->write((const char *)fixed_sequence, ESCAPE_SEQUENCE_LENGTH);
			below_position += ESCAPE_SEQUENCE_LENGTH;
			    // still remains valid data not yet written in write_buffer at offset 'trouve + ESCAPE_SEQUENCE_LENGTH - 1'
			    // however this data is also in the input write_buffer (a, size)
			written = (trouve + ESCAPE_SEQUENCE_LENGTH - 1) - initial_buffer_size;
			    // this way, we do not have to copy back to "a" the not yet written data
			++escaped_data_count_since_last_skip;
			write_buffer_size = 0; // dropping all supplementary data added
			    // it will be treated from the "a" buffer where they had been copied from
		    }
		    else // the escape sequence found is not complete
		    {
			U_I yet_in_a = size - written;
			U_I missing_for_sequence = trouve + (ESCAPE_SEQUENCE_LENGTH - 1) - write_buffer_size;

			if(write_buffer_size < WRITE_BUFFER_SIZE && yet_in_a > 0)
			    throw SRC_BUG; // write_buffer_size not filled while remains available data in "a" !

			    // either the escape sequence is entirely in "a" (and partially copied in write_buffer)
			    // or there is not enough data in "a" to determin whether this start of sequence is complete or not

			    // first, we can at least write down the data up to offset "trouve - 1" (that's "trouve" bytes).

			x_below->write(write_buffer, trouve);
			below_position += trouve;

			if(yet_in_a >= missing_for_sequence) // sequence entirely available with remaining data in "a"
			{
			    if(trouve < initial_buffer_size)
				throw SRC_BUG; // some original data of write_buffer are part of the escape sequence !!!
			    written = trouve - initial_buffer_size;
			    write_buffer_size = 0;
			}
			else // missing data to determine the nature of the sequence
			{
			    (void)memmove(write_buffer, write_buffer + trouve, write_buffer_size - trouve);
			    write_buffer_size -= trouve;
			    if(write_buffer_size >= ESCAPE_SEQUENCE_LENGTH - 1)
				throw SRC_BUG; // should never seen this if() condition
			    if(write_buffer_size + yet_in_a > WRITE_BUFFER_SIZE)
				throw SRC_BUG; // not possible to reach normally, because yet_in_a < missing_for_sequence < SEQUENCE_LENGTH
			    (void)memcpy(write_buffer + write_buffer_size, a+written, yet_in_a);
			    written = size;
			    write_buffer_size += yet_in_a;
			}
		    }
		}
	    }

		// now that we have eventually treated the write_buffer, we get two possibilities
		// either no escape sequence is pending in the write_buffer [write_buffer_size == 0] (escape sequence in "a" or non escape sequence found at all)
		// or an potential escape sequence is pending in the write_buffer, which only occurs if "a" does not contain any more
		// data to detemine the exact nature of this sequence [ written == size ]

	    if(written != size && write_buffer_size > 0)
		throw SRC_BUG; // anormal situation, seen the previous comment.

	    while(written < size)
	    {
		U_I remains = size - written;

		trouve = trouve_amorce(a + written, remains, fixed_sequence);
		if(trouve == remains)
		{
		    x_below->write(a + written, remains);
		    below_position += remains;
		    written = size;
		}
		else
		{
		    if(trouve > 0)
		    {
			x_below->write(a + written, trouve);
			below_position += trouve;
			written += trouve;
		    }

		    if(trouve + ESCAPE_SEQUENCE_LENGTH - 1 <= remains) // full escape sequence
		    {
			set_fixed_sequence_for(seqt_not_a_sequence);
			x_below->write((const char *)fixed_sequence, ESCAPE_SEQUENCE_LENGTH);
			below_position += ESCAPE_SEQUENCE_LENGTH;
			written += ESCAPE_SEQUENCE_LENGTH - 1;
			++escaped_data_count_since_last_skip;
		    }
		    else // not completed sequence
		    {
			remains = size - written;
			if(remains >= ESCAPE_SEQUENCE_LENGTH - 1)
			    throw SRC_BUG; // how possible is to not be able to fully determine the sequence ???
			(void)memcpy(write_buffer, a + written, remains);
			write_buffer_size = remains;
			written = size;
		    }
		}
	    }
	}
	catch(Ethread_cancel & e)
	{
	    below_position = x_below->get_position();
	    throw;
	}
    }

    void escape::inherited_truncate(const infinint & pos)
    {
	x_below->truncate(pos);
	if(pos < get_position())
	  if(!skip(pos))
	    throw SRC_BUG;
    }

    char escape::type2char(sequence_type x)
    {
	switch(x)
	{
	case seqt_not_a_sequence:
	    return 'X';
	case seqt_file:
	    return 'F';
	case seqt_ea:
	    return 'E';
	case seqt_catalogue:
	    return 'C';
	case seqt_data_name:
	    return 'D';
	case seqt_file_crc:
	    return 'R';
	case seqt_ea_crc:
	    return 'r';
	case seqt_changed:
	    return 'W';
	case seqt_dirty:
	    return 'I';
	case seqt_failed_backup:
	    return '!';
	case seqt_fsa:
	    return 'S';
	case seqt_fsa_crc:
	    return 's';
	case seqt_delta_sig:
	    return 'd';
	default:
	    throw SRC_BUG;
	}
    }

    escape::sequence_type escape::char2type(char x)
    {
	switch(x)
	{
	case 'X':
	    return seqt_not_a_sequence;
	case 'F':
	    return seqt_file;
	case 'E':
	    return seqt_ea;
	case 'C':
	    return seqt_catalogue;
	case 'D':
	    return seqt_data_name;
	case 'R':
	    return seqt_file_crc;
	case 'r':
	    return seqt_ea_crc;
	case 'W':
	    return seqt_changed;
	case 'I':
	    return seqt_dirty;
	case '!':
	    return seqt_failed_backup;
	case 'S':
	    return seqt_fsa;
	case 's':
	    return seqt_fsa_crc;
	case 'd':
	    return seqt_delta_sig;
	default:
	    throw Erange("escape::char2type", gettext("Unknown escape sequence type"));
	}
    }

    void escape::clean_read()
    {
	read_buffer_size = already_read = escape_seq_offset_in_buffer = 0;
	read_eof = false;
	escaped_data_count_since_last_skip = 0;
    }

    void escape::flush_write()
    {
	check_below();

	if(write_buffer_size > 0)
	{
	    x_below->write(write_buffer, write_buffer_size);
	    below_position += write_buffer_size;
	    write_buffer_size = 0;
	}
    }

    void escape::copy_from(const escape & ref)
    {
	x_below = ref.x_below;
	write_buffer_size = ref.write_buffer_size;
	if(write_buffer_size > WRITE_BUFFER_SIZE)
	    throw SRC_BUG;
	(void)memcpy(write_buffer, ref.write_buffer, write_buffer_size);
	read_buffer_size = ref.read_buffer_size;
	if(read_buffer_size > READ_BUFFER_SIZE)
	    throw SRC_BUG;
	(void)memcpy(read_buffer, ref.read_buffer, read_buffer_size);
	already_read = ref.already_read;
	read_eof = ref.read_eof;
	escaped_data_count_since_last_skip = ref.escaped_data_count_since_last_skip;
	below_position = ref.below_position;
	unjumpable = ref.unjumpable;
	(void)memcpy(fixed_sequence, ref.fixed_sequence, ESCAPE_SEQUENCE_LENGTH);
    }

    void escape::move_from(escape && ref) noexcept
    {
	swap(x_below, ref.x_below);
	write_buffer_size = move(ref.write_buffer_size);
	swap(write_buffer, ref.write_buffer);
	read_buffer_size = move(ref.read_buffer_size);
	already_read = move(ref.already_read);
	read_eof = move(ref.read_eof);
	escape_seq_offset_in_buffer = move(ref.escape_seq_offset_in_buffer);
	swap(read_buffer, ref.read_buffer);
	unjumpable = move(ref.unjumpable);
	swap(fixed_sequence, ref.fixed_sequence);
	escaped_data_count_since_last_skip = move(ref.escaped_data_count_since_last_skip);
	below_position = move(ref.below_position);
    }

    bool escape::mini_read_buffer()
    {
	U_I avail = read_buffer_size - already_read;

	if(avail < ESCAPE_SEQUENCE_LENGTH)
	{
		// we need more data

	    if(already_read + ESCAPE_SEQUENCE_LENGTH >= READ_BUFFER_SIZE)
	    {

		    // we need room to place more data, so we skip data at the beginning of the read_buffer

		if(already_read < ESCAPE_SEQUENCE_LENGTH)
		    throw SRC_BUG; // READ_BUFFER_SIZE is expected to be (much) larger than twice the escape sequence length,
		    // so now, we never have to use memmove in place of memcpy:
		(void)memcpy(read_buffer, read_buffer + already_read, avail);

		if(escape_seq_offset_in_buffer < already_read)
		    throw SRC_BUG; // escape_seq_offset_in_buffer, has not been updated sometime before
		escape_seq_offset_in_buffer -= already_read;
		already_read = 0;
		read_buffer_size = avail;
	    }

		// recording up to what point data had been unescaped

	    if(escape_seq_offset_in_buffer > read_buffer_size)
		throw SRC_BUG; // should not be greater than read_buffer_size
	    else
	    {
		U_I delta; // will receive the amount of byte to reduce the buffer (one byte for each data mark found)
		U_I delta_size; // will holds the size of the data to unescape
		U_I offset_in_buffer;
		U_I short_read;

		    // adding some data at the end of the buffer

		short_read = x_below->read(read_buffer + read_buffer_size, ESCAPE_SEQUENCE_LENGTH - avail);
		read_buffer_size += short_read;
		below_position += short_read;
		avail = read_buffer_size - already_read;

		    // we can continue unescaping the new data
		    // but only what has not yet been unescaped, thus no data before escape_seq_offset_in_buffer

		delta_size = read_buffer_size - escape_seq_offset_in_buffer;
		offset_in_buffer = remove_data_marks_and_stop_at_first_real_mark(read_buffer + escape_seq_offset_in_buffer, delta_size, delta, fixed_sequence);

		delta_size -= delta;
		escaped_data_count_since_last_skip += delta;
		read_buffer_size = escape_seq_offset_in_buffer + delta_size;
		escape_seq_offset_in_buffer += offset_in_buffer;
	    }
	}
	else // enough data, but removing any data mark found in the beginning of the read_buffer
	{
	    if(escape_seq_offset_in_buffer == already_read
	       && char2type(read_buffer[escape_seq_offset_in_buffer + ESCAPE_SEQUENCE_LENGTH - 1]) == seqt_not_a_sequence)
	    {

		    // next to read is a data mark, we must un-escape data

		U_I delta = 0;
		escape_seq_offset_in_buffer = already_read + remove_data_marks_and_stop_at_first_real_mark(read_buffer + already_read,
													   read_buffer_size - already_read,
													   delta,
													   fixed_sequence);
		escaped_data_count_since_last_skip += delta;
		read_buffer_size -= delta;
	    }
	}

	if(avail < ESCAPE_SEQUENCE_LENGTH)
	{
	    read_eof = true;
	    return false;
	}
	else
	    return true;
    }

    U_I escape::trouve_amorce(const char *a, U_I size, const unsigned char escape_sequence[ESCAPE_SEQUENCE_LENGTH])
    {
	U_I ret = 0; // points to the start of the escape sequence
	U_I curs = 0; // points to current byte considered
	U_I amorce = 0; // points to the byte to compare in the fixed sequence
	U_I found = ESCAPE_SEQUENCE_LENGTH - 1; // maximum number of byte to compare in fixed sequence

	while(curs < size && amorce < found)
	{
	    if((unsigned char)a[curs] == escape_sequence[amorce])
	    {
		if(amorce == 0)
		    ret = curs;
		++amorce;
	    }
	    else
		if(amorce > 0)
		{
		    curs -= amorce;
		    amorce = 0;
		}

	    ++curs;
	}

	if(curs >= size) // reached the end of the field
	    if(amorce == 0) // and no start of escape sequence could be found
		ret = size;
	    // else we return the start of the escape sequence found (or partial escape sequence if found at the end of the "a" buffer

	return ret;
    }


    U_I escape::remove_data_marks_and_stop_at_first_real_mark(char *a, U_I size, U_I & delta, const unsigned char escape_sequence[ESCAPE_SEQUENCE_LENGTH])
    {
	bool loop = false;
	U_I ret = 0;

	delta = 0;

	do
	{
	    ret += trouve_amorce(a + ret, size - ret, escape_sequence);

	    if(ret < size) // start of escape sequence found
		if(ret + ESCAPE_SEQUENCE_LENGTH <= size) // we can determin the nature of the escape sequence
		    if(char2type(a[ret + ESCAPE_SEQUENCE_LENGTH - 1]) == seqt_not_a_sequence)
		    {
			(void)memmove(a + ret + ESCAPE_SEQUENCE_LENGTH - 1, a + ret + ESCAPE_SEQUENCE_LENGTH, size - ret - ESCAPE_SEQUENCE_LENGTH);
			++delta;
			--size; // this modification is local to this method.
			loop = true;
			ret += ESCAPE_SEQUENCE_LENGTH - 1; // skipping one byte, as this is the escape data byte of which we just removed the protection
		    }
		    else
			loop = false; // real mark found
		else
		    loop = false; // cannot know whether this is a real mark or just escaped data
	    else
		loop = false; // no mark found
	}
	while(loop);

	return ret;
    }

} // end of namespace
