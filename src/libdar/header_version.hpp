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

    /// \file header_version.hpp
    /// \brief archive global header structure is defined here
    /// \ingroup Private

#ifndef HEADER_VERSION_HPP
#define HEADER_VERSION_HPP

#include "../my_config.h"
#include "infinint.hpp"
#include "generic_file.hpp"
#include "tools.hpp"
#include "archive_version.hpp"
#include "on_pool.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

    const U_I VERSION_FLAG_SAVED_EA_ROOT = 0x80;      //< no more used since version "05"
    const U_I VERSION_FLAG_SAVED_EA_USER = 0x40;      //< no more used since version "05"
    const U_I VERSION_FLAG_SCRAMBLED     = 0x20;      //< scrambled or strong encryption used
    const U_I VERSION_FLAG_SEQUENCE_MARK = 0x10;      //< escape sequence marks present for sequential reading
    const U_I VERSION_FLAG_INITIAL_OFFSET = 0x08;     //< whether the header contains the initial offset (size of clear data before encrypted) NOTE : This value is set internally by header_version, no need to set flag with it! But that's OK to set it or not, it will be updated according to initial_offset's value.
    const U_I VERSION_FLAG_HAS_AN_EXTENDED_SIZE = 0x01; //< reserved for future use
    const U_I VERSION_SIZE = 3;                       //< size of the version field
    const U_I HEADER_CRC_SIZE = 2;                    //< size of the CRC (deprecated, now only used when reading old archives)


	/// manages of the archive header and trailer
    struct header_version : public on_pool
    {
        archive_version edition;
        char algo_zip;
        std::string cmd_line; // used long ago to store cmd_line, then abandonned, then recycled as a user comment field
        unsigned char flag; // added at edition 02
	infinint initial_offset; // not dropped to archive if set to zero (at dump() time, the flag is also updated with VERSION_FLAG_INITIAL_OFFSET accordingly to this value)

	header_version()
	{
	    algo_zip = ' ';
	    cmd_line = "";
	    flag = 0;
	    initial_offset = 0;
	}

        void read(generic_file &f)
	{
	    crc *ctrl = NULL;

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
	};

        void write(generic_file &f) const
	{
	    header_version *me = const_cast <header_version *>(this);
	    crc *ctrl = NULL;

	    if(me == NULL)
		throw SRC_BUG;

		// preparing the data

	    if(initial_offset != 0)
		me->flag |= VERSION_FLAG_INITIAL_OFFSET; // adding it to the flag
	    else
		me->flag &= ~VERSION_FLAG_INITIAL_OFFSET; // removing it from the flag

		// writing down the data

	    f.reset_crc(HEADER_CRC_SIZE);
	    edition.dump(f);
	    f.write(&algo_zip, sizeof(algo_zip));
	    tools_write_string(f, cmd_line);
	    f.write((char *)&flag, 1);
	    if(initial_offset != 0)
		initial_offset.dump(f);

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
	};
    };

} // end of namespace

#endif
