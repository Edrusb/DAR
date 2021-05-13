/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2021 Denis Corbin
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
// to allow compilation under Cygwin we need <sys/types.h>
// else Cygwin's <netinet/in.h> lack __int16_t symbol !?!
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
} // end extern "C"

#include "header.hpp"
#include "tlv_list.hpp"
#include "tools.hpp"
#include "fichier_global.hpp"

using namespace std;
namespace libdar
{

    enum extension_type
    {
	extension_none = 'N',     //< no extension (obsolete since format "08")
	extension_size = 'S',     //< extension is the size of slices (obsolete since format "08")
	extension_tlv =  'T'      //< extension is a TLV (systematic since format "08")
    };


    enum tlv_type
    {
	tlv_size = 1,             //< TLV gives the size of slices (infinint)
	tlv_first_size = 2,       //< TLV gives the size of first slice (infinint)
	tlv_data_name = 3,        //< TLV gives the name of the data set
	tlv_reserved = 65535      //< TLV reserved if 16 bits type space is exhausted to signal a new (larger type storage, to be implemented of course)
    };


	/*************************************************************/
	/******************* HEADER datastructure ********************/
	/*************************************************************/

    header::header()
    {
        magic = 0;
	internal_name.clear();
	data_name.clear();
        flag = '\0';
	first_size = nullptr;
	slice_size = nullptr;
	old_header = false;
    }

    void header::read(user_interaction & ui, generic_file & f, bool lax)
    {
        magic_number tmp;
	tlv_list tempo;
	char extension;
	fichier_global *f_fic = dynamic_cast<fichier_global *>(&f);

	free_pointers();
	old_header = false;
        if(f.read((char *)&tmp, sizeof(magic_number)) != sizeof(magic_number))
	    throw Erange("header::read", gettext("Reached end of file while reading slice header"));
        magic = ntohl(tmp);
	try
	{
	    internal_name.read(f);
	}
	catch(Erange & e)
	{
	    throw Erange("header::read", gettext("Reached end of file while reading slice header"));
	}
        if(f.read(&flag, 1) != 1)
	    throw Erange("header::read", gettext("Reached end of file while reading slice header"));
        if(f.read(&extension, 1) != 1)
	    throw Erange("header::read", gettext("Reached end of file while reading slice header"));
	data_name.clear();
        switch(extension)
        {
        case extension_none:
	    if(f_fic != nullptr)
	    {
		slice_size = new (nothrow) infinint(f_fic->get_size());
		if(slice_size == nullptr)
		{
		    if(!lax)
			throw Ememory("header::read");
		    else
		    {
			ui.message(gettext("LAX MODE: slice size is not possible to read, (lack of virtual memory?), continuing anyway..."));
			slice_size = new (nothrow) infinint(0);
			if(slice_size == nullptr)
			    throw Ememory("header::read");
		    }
			// this extension was used in archives before release 2.4.0
			// when the first slice had the same size of the following ones
			// the slice size of all slices if thus the one of the first which
			// was learnt by getting the size of the file
			// this works also for single sliced archives.
		}
	    }
	    old_header = true;
            break;
        case extension_size:
	    slice_size = new (nothrow) infinint(f);
	    if(slice_size == nullptr)
	    {
		if(!lax)
		    throw Ememory("header::read");
		else
		{
		    ui.message(gettext("LAX MODE: slice size is not possible to read, (lack of virtual memory?), continuing anyway..."));
		    slice_size = new (nothrow) infinint(0);
		    if(slice_size == nullptr)
			throw Ememory("header::read");
		}
	    }
	    if(f_fic != nullptr)
	    {
		first_size = new (nothrow) infinint(f_fic->get_size());
		if(first_size == nullptr)
		{
		    if(!lax)
			throw Ememory("header::read");
		    else
		    {
			ui.message(gettext("LAX MODE: first slice size is not possible to read, (lack of virtual memory?), continuing anyway..."));
			first_size = new (nothrow) infinint(0);
			if(first_size == nullptr)
			    throw Ememory("header::read");
		    }
			// note: the "extension_size" extension was used in archives before release 2.4.0
			// this option was only used in the first slice and contained the size of slices (not of the first slice)
			// when the first slice had a different size. This way, reading the size of the current file gives
			// the size of the first slice while the header extension gives the size of following slices.
		}
	    }
	    else
		if(!lax)
		    throw Erange("header::read", gettext("Archive format older than \"08\" (release 2.4.0) cannot be read through a single pipe. It only can be read using dar_slave or normal plain file (slice)"));
		else
		    ui.message(gettext("LAX MODE: first slice size is not possible to read, continuing anyway..."));
	    old_header = true;
            break;
	case extension_tlv:
	    tempo.read(f);        // read the list of TLV stored in the header
	    fill_from(ui, tempo); // from the TLV list, set the different fields of the current header object
	    if(slice_size == nullptr && f_fic != nullptr)
	    {
		slice_size = new (nothrow) infinint(f_fic->get_size());
		if(slice_size == nullptr)
		    throw Ememory("header::read");
	    }
	    break;
        default:
	    if(!lax)
		throw Erange("header::read", gettext("Badly formatted SAR header (unknown TLV type in slice header)"));
	    else
	    {
		ui.message(gettext("LAX MODE: Unknown data in slice header, ignoring and continuing"));
		slice_size = new (nothrow) infinint(0);
		if(slice_size == nullptr)
		    throw Ememory("header::read");
	    }
        }
	if(data_name.is_cleared())
	    data_name = internal_name;
    }

    void header::write(user_interaction & ui, generic_file & f) const
    {
        magic_number tmp;
	char tmp_ext[] = { extension_tlv, '\0' };

        tmp = htonl(magic);
        f.write((char *)&tmp, sizeof(magic));
        internal_name.dump(f);
        f.write(&flag, 1);
	if(old_header)
	{
	    if(first_size != nullptr && slice_size != nullptr && *first_size != *slice_size)
	    {
		tmp_ext[0] = extension_size;
		f.write(tmp_ext, 1);
		slice_size->dump(f);
	    }
	    else
	    {
		tmp_ext[0] = extension_none;
		f.write(tmp_ext, 1);
	    }
	}
	else
	{
	    f.write(tmp_ext, 1); // since release 2.4.0, tlv is always used to store optional information
	    build_tlv_list(ui).dump(f);
	}
    }

    bool header::get_first_slice_size(infinint & size) const
    {
	if(first_size != nullptr)
	{
	    size = *first_size;
	    return true;
	}
	else
	    return false;
    }

    void header::set_first_slice_size(const infinint & size)
    {
	if(first_size == nullptr)
	{
	    first_size = new (nothrow) infinint();
	    if(first_size == nullptr)
		throw Ememory("header::set_first_file_size");
	}
	*first_size = size;
    }

    bool header::get_slice_size(infinint & size) const
    {
	if(slice_size != nullptr)
	{
	    size = *slice_size;
	    return true;
	}
	else
	    return false;
    }

    void header::set_slice_size(const infinint & size)
    {
	if(slice_size == nullptr)
	{
	    slice_size = new (nothrow) infinint();
	    if(slice_size == nullptr)
		throw Ememory("header::set_slice_size");
	}

	*slice_size = size;
    }


    void header::copy_from(const header & ref)
    {
        magic = ref.magic;
        internal_name = ref.internal_name;
 	data_name = ref.data_name;
        flag = ref.flag;
	first_size = nullptr;
	slice_size = nullptr;

	try
	{
	    if(ref.first_size != nullptr)
	    {
		first_size = new (nothrow) infinint();
		if(first_size == nullptr)
		    throw Ememory("header::copy_from");
		*first_size = *ref.first_size;
	    }

	    if(ref.slice_size != nullptr)
	    {
		slice_size = new (nothrow) infinint();
		if(slice_size == nullptr)
		    throw Ememory("header::copy_from");
		*slice_size = *ref.slice_size;
	    }

	    old_header = ref.old_header;
	}
	catch(...)
	{
	    free_pointers();
	    throw;
	}
    }

    void header::move_from(header && ref) noexcept
    {
	magic = move(ref.magic);
	internal_name = move(ref.internal_name);
	data_name = move(ref.data_name);
	flag = move(ref.flag);
	swap(first_size, ref.first_size);
	swap(slice_size, ref.slice_size);
	old_header = move(ref.old_header);
    }

    void header::free_pointers()
    {
	if(first_size != nullptr)
	{
	    delete first_size;
	    first_size = nullptr;
	}

	if(slice_size != nullptr)
	{
	    delete slice_size;
	    slice_size = nullptr;
	}
    }

    void header::fill_from(user_interaction & ui, const tlv_list & extension)
    {
	U_I taille = extension.size();

	free_pointers();
	for(U_I index = 0; index < taille; ++index)
	{
	    switch(extension[index].get_type())
	    {
	    case tlv_first_size:
		first_size = new (nothrow) infinint();
		if(first_size == nullptr)
		    throw Ememory("header::fill_from");
		extension[index].skip(0);
		first_size->read(extension[index]);
		break;
	    case tlv_size:
		slice_size = new (nothrow) infinint();
		if(slice_size == nullptr)
		    throw Ememory("header::fill_from");
		extension[index].skip(0);
		slice_size->read(extension[index]);
		break;
	    case tlv_data_name:
		try
		{
		    extension[index].skip(0);
		    data_name.read(extension[index]);
		}
		catch(Erange & e)
		{
		    throw Erange("header::fill_from", gettext("incomplete data set name found in a slice header"));
		}
		break;
	    default:
		ui.pause(tools_printf(gettext("Unknown entry found in slice header (type = %d), option not supported. The archive you are reading may have been generated by a more recent version of libdar, ignore this entry and continue anyway?"), extension[index].get_type()));
	    }
	}
    }

    tlv_list header::build_tlv_list(user_interaction & ui) const
    {
	tlv_list ret;
	tlv tmp;

	if(first_size != nullptr)
	{
	    tmp.reset();
	    first_size->dump(tmp);
	    tmp.set_type(tlv_first_size);
	    ret.add(tmp);
	}

	if(slice_size != nullptr)
	{
	    tmp.reset();
	    slice_size->dump(tmp);
	    tmp.set_type(tlv_size);
	    ret.add(tmp);
	}

	tmp.reset();
	data_name.dump(tmp);
	tmp.set_type(tlv_data_name);
	ret.add(tmp);

	return ret;
    }

} // end of namespace
