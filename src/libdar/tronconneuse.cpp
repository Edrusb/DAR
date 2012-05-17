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
// $Id: tronconneuse.cpp,v 1.5.2.2 2005/03/13 20:07:54 edrusb Rel $
//
/*********************************************************************/

#include "../my_config.h"

#include "tronconneuse.hpp"
#include "tools.hpp"

using namespace std;

namespace libdar
{

    tronconneuse::tronconneuse(user_interaction & dialog, U_32 block_size, generic_file & encrypted_side) : generic_file(dialog, encrypted_side.get_mode())
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
	initial_shift = encrypted_side.get_position();
	block_num = 0;
	encrypted = & encrypted_side;
	encrypted_buf = NULL; // cannot invoke pure virtual methods from constructor
	encrypted_buf_size = 0;
	weof = false;

	    // buffers cannot be initialized here as they need result from pure virtual methods
	    // the inherited class constructor part has not yet been initialized
	    // for this reason C++ forbids us to call virtual methods (they may rely on data that
	    // has not been initialized.
    }

    tronconneuse & tronconneuse::operator = (const tronconneuse & ref)
    {
	generic_file *moi = this;
	const generic_file *toi = &ref;

	detruit();
	*moi = *toi;
	copy_from(ref);
	return *this;
    }

    bool tronconneuse::skip(const infinint & pos)
    {
	bool ret;

	if(encrypted->get_mode() != gf_read_only)
	    throw SRC_BUG;
	current_position = pos;
	ret = check_current_position();
	if(!ret)
	    skip_to_eof();
	return ret;
    }

    bool tronconneuse::skip_to_eof()
    {
	bool ret;

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
	    fill_buf();
	    current_position = buf_offset + infinint(buf_byte_data); // we are now at the eof
	}

	return ret;
    }

    bool tronconneuse::skip_relative(S_I x)
    {
	bool ret;

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

    S_I tronconneuse::inherited_read(char *a, size_t size)
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

    S_I tronconneuse::inherited_write(char *a, size_t size)
    {
	size_t lu = 0;

	if(weof)
	    throw SRC_BUG; // write_end_of_file has been called no further write is allowed
	init_buf();

	while(lu < size)
	{
	    while(buf_byte_data < clear_block_size && lu < size)
		buf[buf_byte_data++] = a[lu++];
	    if(buf_byte_data >= clear_block_size) // we have a complete buffer to encrypt
	    {
		flush();
		block_num++;
	    }
	}
	current_position += infinint(size);
	return size;
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
    }

    void tronconneuse::copy_from(const tronconneuse & ref)
    {
	buf = NULL;
	encrypted_buf = NULL;
	try
	{
	    initial_shift = ref.initial_shift;
	    buf_offset = ref.buf_offset;
	    buf_byte_data = ref.buf_byte_data;
	    buf_size = ref.buf_size;
	    buf = new char[buf_size];
	    if(buf == NULL)
		throw Ememory("tronconneuse::copy_from");
	    for(U_I i = 0; i < buf_byte_data; i++)
		buf[i] = ref.buf[i];
	    clear_block_size = ref.clear_block_size;
	    current_position = ref.current_position;
	    block_num = ref.block_num;

	    encrypted = ref.encrypted;
		// objets share the same generic_file reference

	    encrypted_buf_size = ref.encrypted_buf_size;
	    encrypted_buf = new char[encrypted_buf_size];
	    if(encrypted_buf == NULL)
		throw Ememory("tronconneuse::copy_from");
	    for(U_I i = 0; i < encrypted_buf_size; i++)
		encrypted_buf[i] = ref.encrypted_buf[i];
	    weof = ref.weof;
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

	if(current_position < buf_offset || (buf_offset + infinint(buf_byte_data)) <= current_position)
	{
	    position_clear2crypt(current_position, crypt_offset, buf_offset, tmp_ret, block_num);
	    if(encrypted->skip(crypt_offset + initial_shift))
		buf_byte_data = decrypt_data(block_num, encrypted_buf, encrypted->read(encrypted_buf, encrypted_buf_size), buf, clear_block_size);
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

	if(buf_byte_data > 0)
	{
	    init_buf();
	    encrypted->write(encrypted_buf, encrypt_data(block_num, buf, buf_byte_data, buf_size, encrypted_buf, encrypted_buf_size));
	    buf_byte_data = 0;
	    buf_offset += infinint(clear_block_size);
	}
    }

    void tronconneuse::init_buf()
    {
	if(encrypted_buf == NULL)
	{
	    encrypted_buf_size = encrypted_block_size_for(clear_block_size);
	    encrypted_buf = new char[encrypted_buf_size];
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
		throw SRC_BUG; // buf_size must be larger than clear_block_size
	    buf = new char[buf_size];
	    if(buf == NULL)
	    {
		buf_size = 0;
		throw Ememory("tronconneuse::init_encrypte_buf_size");
	    }
	}
    }

    static void dummy_call(char *x)
    {
        static char id[]="$Id: tronconneuse.cpp,v 1.5.2.2 2005/03/13 20:07:54 edrusb Rel $";
        dummy_call(id);
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

} // end of namespace
