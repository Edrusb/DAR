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
}

#include "tronconneuse.hpp"
#include "tools.hpp"

using namespace std;

namespace libdar
{

    tronconneuse::tronconneuse(U_32 block_size, generic_file & encrypted_side, bool no_initial_shift, const archive_version & x_reading_ver) : generic_file(encrypted_side.get_mode())
    {
	if(&encrypted_side == NULL)
	    throw SRC_BUG;
	if(encrypted_side.get_mode() == gf_read_write)
	    throw SRC_BUG;
	if(block_size == 0)
	    throw Erange("tronconneuse::tronconneuse", tools_printf(gettext("%d is not a valid block size"), block_size));
	buf_offset = 0;
	buf_byte_data = 0;
	buf_size = 0;
	buf = NULL;  // cannot invoke pure virtual methods from constructor
	clear_block_size = block_size;
	current_position = 0;
	if(no_initial_shift)
	    initial_shift = 0;
	else
	    initial_shift = encrypted_side.get_position();
	block_num = 0;
	encrypted = & encrypted_side;
	encrypted_buf = NULL; // cannot invoke pure virtual methods from constructor
	encrypted_buf_data = 0;
	encrypted_buf_size = 0;
	weof = false;
	reof = false;
	reading_ver = x_reading_ver;
	trailing_clear_data = NULL;

	    // buffers cannot be initialized here as they need result from pure virtual methods
	    // the inherited class constructor part has not yet been initialized
	    // for this reason C++ forbids us to call virtual methods (they may rely on data that
	    // has not been initialized.
    }

    const tronconneuse & tronconneuse::operator = (const tronconneuse & ref)
    {
	generic_file *moi = this;
	const generic_file *toi = &ref;

	if(is_terminated())
	    throw SRC_BUG;

	detruit();
	*moi = *toi;
	copy_from(ref);
	return *this;
    }

    bool tronconneuse::skippable(skippability direction, const infinint & amount)
    {
	if(is_terminated())
	    throw SRC_BUG;

	if(encrypted->get_mode() != gf_read_only)
	    return false;
	else
	    return encrypted->skippable(direction, amount);
    }

    bool tronconneuse::skip(const infinint & pos)
    {
	bool ret = true;

	if(is_terminated())
	    throw SRC_BUG;

	if(encrypted->get_mode() != gf_read_only)
	    throw SRC_BUG;

	if(current_position != pos)
	{
	    if(pos < buf_offset)
		reof = false;
	    current_position = pos;
	    ret = check_current_position();
	    if(!ret)
		skip_to_eof();
	}
	else
	    ret = true;

	return ret;
    }

    bool tronconneuse::skip_to_eof()
    {
	bool ret;

	if(is_terminated())
	    throw SRC_BUG;

	if(encrypted->get_mode() != gf_read_only)
	    throw SRC_BUG;
	ret = encrypted->skip_to_eof();
	if(ret)
	{
	    infinint residu;

	    init_buf(); // to be sure encrypted_buf_size is defined
	    if(encrypted->get_position() < initial_shift)
		throw SRC_BUG; // eof is before the first encrypted byte
	    euclide(encrypted->get_position() - initial_shift, encrypted_buf_size, block_num, residu);
	    current_position = block_num * infinint(clear_block_size);
	    reof = false; // must be set to true to have fill_buf filling the clear buffer
	    (void)fill_buf();
	    reof = true; // now fill_buf has completed we can set it to the expected value
	    current_position = buf_offset + infinint(buf_byte_data); // we are now at the eof
	    ret = encrypted->skip_to_eof();
	}

	return ret;
    }

    bool tronconneuse::skip_relative(S_I x)
    {
	bool ret;

	if(is_terminated())
	    throw SRC_BUG;

	if(encrypted->get_mode() != gf_read_only)
	    throw SRC_BUG;
	if(x >= 0)
	    ret = skip(current_position + x);
	else
	{
	    x = -x;
	    if(current_position >= x)
		ret = skip(current_position - infinint(x));
	    else
	    {
		skip(0);
		ret = false;
	    }
	}
	return ret;
    }

    U_I tronconneuse::inherited_read(char *a, U_I size)
    {
	U_I lu = 0;
	bool eof = false;
	U_32 pos_in_buf;

	while(lu < size && ! eof)
	{
	    pos_in_buf = fill_buf();
	    if(pos_in_buf >= buf_byte_data)
		eof = true;
	    else // some data can be read
	    {
		while(pos_in_buf < buf_byte_data && lu < size)
		    a[lu++] = buf[pos_in_buf++];
		current_position = buf_offset + infinint(pos_in_buf);
	    }
	}

	return lu;
    }

    void tronconneuse::inherited_write(const char *a, U_I size)
    {
	U_I lu = 0;
	bool thread_stop = false;
	Ethread_cancel caught = Ethread_cancel(false, 0);

	if(weof)
	    throw SRC_BUG; // write_end_of_file has been called no further write is allowed
	init_buf();

	while(lu < size)
	{
	    U_I place = clear_block_size - buf_byte_data; // how much we are able to store before completing the current block
	    U_I avail = size - lu; // how much data is available
	    U_I min = avail > place ? place : avail; // the minimum(place, avail)

	    (void)memcpy(buf + buf_byte_data, a + lu, min);
	    buf_byte_data += min;
	    lu += min;

	    if(buf_byte_data >= clear_block_size) // we have a complete buffer to encrypt
	    {
		try
		{
		    flush();
		}
		catch(Ethread_cancel & e)
		{
		    thread_stop = true;
		    caught = e;
		}
		block_num++;
	    }
	}
	current_position += infinint(size);
	if(thread_stop)
	    throw caught;
    }

    void tronconneuse::detruit()
    {
	if(buf != NULL)
	{
	    delete [] buf;
	    buf = NULL;
	}
	if(encrypted_buf != NULL)
	{
	    delete [] encrypted_buf;
	    encrypted_buf = NULL;
	}
	buf_size = 0;
	encrypted_buf_size = 0;
	encrypted_buf_data = 0;
    }

    void tronconneuse::copy_from(const tronconneuse & ref)
    {
	buf = NULL;
	encrypted_buf = NULL;

	if(is_terminated())
	    throw SRC_BUG;

	try
	{
	    initial_shift = ref.initial_shift;
	    buf_offset = ref.buf_offset;
	    buf_byte_data = ref.buf_byte_data;
	    buf_size = ref.buf_size;
	    buf = new (nothrow) char[buf_size];
	    if(buf == NULL)
		throw Ememory("tronconneuse::copy_from");
	    (void)memcpy(buf, ref.buf, buf_byte_data);
	    clear_block_size = ref.clear_block_size;
	    current_position = ref.current_position;
	    block_num = ref.block_num;

	    encrypted = ref.encrypted;
		// objets share the same generic_file reference

	    encrypted_buf_size = ref.encrypted_buf_size;
	    encrypted_buf_data = ref.encrypted_buf_data;
	    encrypted_buf = new (nothrow) char[encrypted_buf_size];
	    if(encrypted_buf == NULL)
		throw Ememory("tronconneuse::copy_from");
	    (void)memcpy(encrypted_buf, ref.encrypted_buf, encrypted_buf_data);
	    weof = ref.weof;
	    reof = ref.reof;
	    reading_ver = ref.reading_ver;
	    trailing_clear_data = ref.trailing_clear_data;
	}
	catch(...)
	{
	    detruit();
	    throw;
	}
    }

    U_32 tronconneuse::fill_buf()
    {
	U_32 ret;
	infinint crypt_offset;
	infinint tmp_ret;

	if(current_position < buf_offset || (buf_offset + infinint(buf_byte_data)) <= current_position) // requested data not in current clear buffer
	{
	    position_clear2crypt(current_position, crypt_offset, buf_offset, tmp_ret, block_num);
	    if(!reof && encrypted->skip(crypt_offset + initial_shift))
	    {
		encrypted_buf_data = encrypted->read(encrypted_buf, encrypted_buf_size);

		if(encrypted_buf_data < encrypted_buf_size)
		{
		    reof = true;
		    remove_trailing_clear_data_from_encrypted_buf(crypt_offset);
		}

		try
		{
		    buf_byte_data = decrypt_data(block_num, encrypted_buf, encrypted_buf_data, buf, clear_block_size);
		}
		catch(Erange & e)
		{
		    if(!reof)
		    {
			remove_trailing_clear_data_from_encrypted_buf(crypt_offset);

			    // retrying but without trailing cleared data
			buf_byte_data = decrypt_data(block_num, encrypted_buf, encrypted_buf_data, buf, clear_block_size);
		    }
		    else // already tried to remove clear data at end of encrypted buffer
			throw;
		}

		if(buf_byte_data > buf_size)
		{
		    buf_byte_data = clear_block_size;
		    throw Erange("tronconneuse::fill_buff", gettext("Data corruption may have occurred, cannot decrypt data"));
		}
	    }
	    else
		buf_byte_data = 0; // no data could be read as the requested position could not be reached
	}
	else
	    tmp_ret = current_position - buf_offset;

	ret = 0;
	tmp_ret.unstack(ret);
	if(tmp_ret != 0)
	    throw SRC_BUG; // result is modulo clear_block_size which is U_32, thus it should fit in U_32 (=ret)

	return ret;
    }

    void tronconneuse::flush()
    {
	if(encrypted->get_mode() != gf_write_only)
	    return;

	if(weof)
	    return;

	if(buf_byte_data > 0)
	{
	    init_buf();
	    encrypted_buf_data = encrypt_data(block_num, buf, buf_byte_data, buf_size, encrypted_buf, encrypted_buf_size);
	    try
	    {
		encrypted->write(encrypted_buf, encrypted_buf_data);
		buf_byte_data = 0;
		buf_offset += infinint(clear_block_size);
	    }
	    catch(Ethread_cancel & e)
	    {
		    // at this step, the pending encrypted data could not be wrote to disk
		    // (Ethread_cancel exception has been thrown before).
		    // But we have returned to the upper layer the position of the corresponding clear
		    // data which may have already been recorded
		    // in the catalogue above, thus we cannot just drop this pending encrypted data
		    // as it would make reference to inexistant data in the catalogue.
		    // By chance the thread_cancellation mechanism in clean termination is now carried
		    // by the exception we just caugth here so we may retry to write the encrypted data
		    // no new Ethread_cancel exception will be thrown this time.

		encrypted->write(encrypted_buf, encrypted_buf_data);
		buf_byte_data = 0;
		buf_offset += infinint(clear_block_size);

		    // Now that we have cleanly completed the action at this level,
		    // we can propagate the exception toward the caller

		throw;
	    }
	}
    }

    void tronconneuse::init_buf()
    {
	if(encrypted_buf == NULL)
	{
	    encrypted_buf_data = 0;
	    encrypted_buf_size = encrypted_block_size_for(clear_block_size);
	    encrypted_buf = new (nothrow) char[encrypted_buf_size];
	    if(encrypted_buf == NULL)
	    {
		encrypted_buf_size = 0;
		throw Ememory("tronconneuse::init_encrypte_buf_size");
	    }
	}
	if(buf == NULL)
	{
	    buf_size = clear_block_allocated_size_for(clear_block_size);
	    if(buf_size < clear_block_size)
		throw SRC_BUG; // buf_size must be larger than or equal to clear_block_size
	    buf = new (nothrow) char[buf_size];
	    if(buf == NULL)
	    {
		buf_size = 0;
		throw Ememory("tronconneuse::init_encrypte_buf_size");
	    }
	}
    }

    void tronconneuse::position_clear2crypt(const infinint & pos, infinint & file_buf_start, infinint & clear_buf_start, infinint & pos_in_buf, infinint & block_num)
    {
	euclide(pos, clear_block_size, block_num, pos_in_buf);
	init_buf(); // needs to be placed before the following line as encrypted_buf_size is defined by this call
	file_buf_start = block_num * infinint(encrypted_buf_size);
	clear_buf_start = block_num * infinint(clear_block_size);
    }

    void tronconneuse::position_crypt2clear(const infinint & pos, infinint & clear_pos)
    {
	infinint block, residu;
	init_buf(); // needs to be placed before the following line as encrypted_buf_size is defined by this call
	euclide(pos, encrypted_buf_size, block, residu);
	clear_pos = block * infinint(clear_block_size) + residu;
    }

    void tronconneuse::remove_trailing_clear_data_from_encrypted_buf(const infinint & crypt_offset)
    {
	if(encrypted == NULL)
	    throw SRC_BUG;

	if(trailing_clear_data != NULL)
	{
	    infinint clear_offset = 0;

	    try
	    {
		encrypted->skip_to_eof();
		clear_offset = (*trailing_clear_data)(*encrypted, reading_ver);
		clear_offset -= initial_shift;
		encrypted->skip_to_eof(); // no more data to read from the encrypted layer
	    }
	    catch(Egeneric & e)
	    {
		throw Erange("tronconneuse::check_trailing_clear_data", gettext("Cannot determine location of the end of cyphered data: ") + e.get_message());
	    }
	    catch(...)
	    {
		throw SRC_BUG;
	    }

	    if(crypt_offset >= clear_offset)
		    // we already read cleared byte as if they were
		    // ciphered ones in a previous call to fill_buf
	    {
		    // here let the clear data that has been processed for decipherization
		    // in a previous call to fill_buf, assuming that the upper layer will not
		    // need and read them, or they are read, they will trigger a corruption
		    // based on CRC at a upper layer
		encrypted_buf_data = 0;
	    }
	    else // some data a the tail of encrypted_buf are cleared one, and must be removed from it
	    {
		U_I nouv_buf_data = 0;

		clear_offset -= crypt_offset; // max number of byte we should keep in the encrypted_buf
		clear_offset.unstack(nouv_buf_data);
		if(clear_offset > 0)
		    throw SRC_BUG; // cannot handle that integer as U_32 while this number should be less than encrypted_buf_size which is a U_32
		if(nouv_buf_data <= encrypted_buf_data)
		    encrypted_buf_data = nouv_buf_data;
		else
		    throw SRC_BUG; // more encrypted data than could be read so far!
	    }

	}
	    // else, nothing can be done
    }

} // end of namespace
