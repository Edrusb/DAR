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
} // end of extern "C"

#include "header_version.hpp"
#include "integers.hpp"

namespace libdar
{
    const U_I HEADER_CRC_SIZE = 2;                    //< size of the CRC (deprecated, now only used when reading old archives)


    static char sym_crypto_to_char(crypto_algo a);
    static crypto_algo char_to_sym_crypto(char a);

    header_version::header_version()
    {
	algo_zip = ' ';
	cmd_line = "";
	flag = 0;
	initial_offset = 0;
	sym = crypto_none;
	crypted_key = NULL;
    }

    void header_version::read(generic_file &f)
    {
	crc *ctrl = NULL;
	char tmp;

	f.reset_crc(HEADER_CRC_SIZE);
	edition.read(f);
	f.read(&algo_zip, sizeof(algo_zip));
	tools_read_string(f, cmd_line);
	if(edition > 1)
	    f.read((char *)&flag, 1);
	else
	    flag = 0;
	if((flag & VERSION_FLAG_INITIAL_OFFSET) != 0)
	    initial_offset.read(f);
	else
	    initial_offset = 0;

	if((flag & VERSION_FLAG_SCRAMBLED) != 0)
	    if(edition >= 9)
	    {
		f.read(&tmp, sizeof(tmp));
		sym = char_to_sym_crypto(tmp);
	    }
	    else
		    // unknown ciphering algorithm used or no encryption
		    // not coherent with flag which has the VERSION_FLAG_SCRAMBLED bit set
		    // but that this way we record that the crypto algo has
		    // to be provided by the user
		sym = crypto_none;
	else
	    sym = crypto_none; // no crypto used, coherent with flag

	if(crypted_key != NULL)
	{
	    delete crypted_key;
	    crypted_key = NULL;
	}
	if((flag & VERSION_FLAG_HAS_CRYPTED_KEY) != 0)
	{
	    infinint key_size = f;

	    crypted_key = new (get_pool()) memory_file(gf_read_write);
	    if(crypted_key == NULL)
		throw Ememory("header_version::read");
	    if(f.copy_to(*crypted_key, key_size) != key_size)
		throw Erange("header_version::read", gettext("Missing data for encrypted symmetrical key"));
	}

	ctrl = f.get_crc();
	if(ctrl == NULL)
	    throw SRC_BUG;
	try
	{
	    if((edition == empty_archive_version()))
		throw Erange("header_version::read", gettext("Consistency check failed for archive header"));
	    if(edition > 7)
	    {
		crc *coh = create_crc_from_file(f, get_pool());

		if(coh == NULL)
		    throw SRC_BUG;
		try
		{
		    if(typeid(*coh) != typeid(*ctrl))
		    {
			if(coh->get_size() != ctrl->get_size())
			    throw SRC_BUG;
			else
			    throw SRC_BUG; // both case lead to a bug, but we need to know which one is met
		    }
		    if(*coh != *ctrl)
			throw Erange("header_version::read", gettext("Consistency check failed for archive header"));
		}
		catch(...)
		{
		    if(coh != NULL)
			delete coh;
		    throw;
		}
		if(coh != NULL)
		    delete coh;
	    }
	    if(initial_offset == 0)
		initial_offset = f.get_position();
	}
	catch(...)
	{
	    if(ctrl != NULL)
		delete ctrl;
	    throw;
	}

	if(ctrl != NULL)
	    delete ctrl;
    }

    void header_version::write(generic_file &f) const
    {
	header_version *me = const_cast <header_version *>(this);
	crc *ctrl = NULL;
	char tmp;

	if(me == NULL)
	    throw SRC_BUG;

	    // preparing the data

	if(initial_offset != 0)
	    me->flag |= VERSION_FLAG_INITIAL_OFFSET; // adding it to the flag
	else
	    me->flag &= ~VERSION_FLAG_INITIAL_OFFSET; // removing it from the flag

	if(crypted_key != NULL)
	    me->flag |= VERSION_FLAG_HAS_CRYPTED_KEY;
	else
	    me->flag &= ~VERSION_FLAG_HAS_CRYPTED_KEY;

	    // writing down the data

	f.reset_crc(HEADER_CRC_SIZE);
	edition.dump(f);
	f.write(&algo_zip, sizeof(algo_zip));
	tools_write_string(f, cmd_line);
	f.write((char *)&flag, sizeof(flag));
	if(initial_offset != 0)
	    initial_offset.dump(f);
	if(sym != crypto_none)
	{
	    tmp = sym_crypto_to_char(sym);
	    f.write(&tmp, sizeof(tmp));
	}
	if((sym == crypto_none) ^ ((flag & VERSION_FLAG_SCRAMBLED) == 0))
	    throw SRC_BUG; // incoherence

	if(crypted_key != NULL)
	{
	    infinint key_size = crypted_key->get_data_size();

	    key_size.dump(f);
	    crypted_key->copy_to(f, key_size);
	}

	ctrl = f.get_crc();
	if(ctrl == NULL)
	    throw SRC_BUG;
	try
	{
	    ctrl->dump(f);
	}
	catch(...)
	{
	    if(ctrl != NULL)
		delete ctrl;
	    throw;
	}
	if(ctrl != NULL)
	    delete ctrl;
    }


    static char sym_crypto_to_char(crypto_algo a)
    {
	switch(a)
	{
	case crypto_none:
	    return 'n';
	case crypto_scrambling:
	    return 's';
	case crypto_blowfish:
	    return 'b';
	case crypto_aes256:
	    return 'a';
	case crypto_twofish256:
	    return 't';
	case crypto_serpent256:
	    return 'p';
	case crypto_camellia256:
	    return 'c';
	default:
	    throw SRC_BUG;
	}
    }

    static crypto_algo char_to_sym_crypto(char a)
    {
	switch(a)
	{
	case 'n':
	    return crypto_none;
	case 's':
	    return crypto_scrambling;
	case 'b':
	    return crypto_blowfish;
	case 'a':
	    return crypto_aes256;
	case 't':
	    return crypto_twofish256;
	case 'p':
	    return crypto_serpent256;
	case 'c':
	    return crypto_camellia256;
	default:
	    throw SRC_BUG;
	}
    }


} // end of namespace
