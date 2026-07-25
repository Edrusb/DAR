/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2026 Denis Corbin
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

#if HAVE_TIME_H
# include <time.h>
#endif

#if HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
} // end extern "C"

#include "header.hpp"
#include "tlv_list.hpp"
#include "tools.hpp"
#include "fichier_global.hpp"
#include "contextual.hpp"

using namespace std;
namespace libdar
{

    enum extension_type
    {
	extension_none = 'N',     ///< no extension (obsolete since format "08")
	extension_size = 'S',     ///< extension is the size of slices (obsolete since format "08")
	extension_tlv =  'T'      ///< extension is a TLV (systematic since format "08")
    };


    enum tlv_type
    {
	tlv_size = 1,             ///< TLV gives the size of slices (infinint)
	tlv_first_size = 2,       ///< TLV gives the size of first slice (infinint)
	tlv_data_name = 3,        ///< TLV gives the name of the data set
	tlv_header_size = 4,      ///< TLV gives the common slice header size (of archive format > 07), feature introduces with archive format 12
	tlv_reserved = 65535      ///< TLV reserved if 16 bits type space is exhausted to signal a new (larger type storage, to be implemented of course)
    };


	/*************************************************************/
	/******************* HEADER datastructure ********************/
	/*************************************************************/

    void header::read(user_interaction & ui,
		      generic_file & f,
		      bool lax)
    {
        magic_number tmp;
	tlv_list tempo;
	char extension;
	fichier_global *f_fic = dynamic_cast<fichier_global *>(&f); // we read a header from a slice header

	clear();

        if(f.read((char *)&tmp, sizeof(magic_number)) != sizeof(magic_number))
	    throw Erange("header::read", gettext("Reached end of file while reading slice header"));
        magic = ntohl(tmp);

	if(magic != SAUV_MAGIC_NUMBER)
	    throw Erange("header::read", tools_printf(gettext("not a valid dar file (wrong magic number), please provide the good file.")));

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
		// this extension was used in archives before release 2.4.0
		// when the first slice had the same size of the following ones.
		// The slice size of all slices was thus the one of the first which
		// was learnt by getting the size of the file.
		// This works also for single sliced archives.

	    if(f_fic != nullptr)
	    {
		sly.other_size = f_fic->get_size();
		if(sly.other_size <= min_size())
		{
		    if(!lax)
			throw Erange("header::read", gettext("Invalide slice size"));
		    else
		    {
			ui.message(gettext("LAX MODE: slice size is not possible to read, (lack of virtual memory?), continuing anyway..."));
			sly.other_size = 0;
		    }
		}

		sly.first_size = sly.other_size;
	    }
	    else // not reading from a file (but read from a pipe or reading an external header, for example)
	    {
		sly.other_size = 0; // means single sliced archive
		sly.first_size = 0; // means non-specific size for the first slice
	    }

	    sly.other_slice_header = min_size();
	    sly.first_slice_header = min_size(); // yes, this his the header we read, only sar/trivial sar have context on that
	    sly.older_sar_than_v8 = true;
            break;
        case extension_size:
	    sly.other_size.read(f);
	    if(sly.other_size <= min_size())
	    {
		if(!lax)
		    throw Erange("header::read", gettext("Invalide slice size"));
		else
		{
		    ui.message(gettext("LAX MODE: slice size is not possible to read, (lack of virtual memory?), continuing anyway..."));
		    sly.other_size = 0;
		}
	    }

	    if(f_fic != nullptr)
	    {
		sly.first_size = f_fic->get_size();
		if(sly.first_size <= min_size())
		{
		    if(!lax)
			throw Erange("header::read", gettext("Invalide first slice size"));
		    else
		    {
			ui.message(gettext("LAX MODE: first slice size is not possible to read, (lack of virtual memory?), continuing anyway..."));
			sly.first_size = 0;
		    }
			// note: the "extension_size" extension was used in archives before release 2.4.0
			// this option was only used in the first slice and contained the size of slices (not of the first slice)
			// when the first slice had a different size. This way, reading the size of the current file gives
			// the size of the first slice while the header extension gives the size of following slices.
		}
	    }
	    else
	    {
		if(!lax)
		    throw Erange("header::read", gettext("Archive format older than \"08\" (release 2.4.0) cannot be read through a single pipe. It only can be read using dar_slave or as normal plain files (slices)"));
		else
		{
		    ui.message(gettext("LAX MODE: first slice size is not possible to read, continuing anyway..."));
		    sly.first_size = 0; // no specific size for the first slice as we assume we read a single sliced archive
		}
	    }

	    sly.first_slice_header = f.get_position();
	    if(sly.first_slice_header < min_size())
		throw Erange("header::read", gettext("Invalid first slice header size for a slice of format 07 or older"));
	    sly.other_slice_header = min_size();
	    sly.older_sar_than_v8 = true;
            break;
	case extension_tlv:
	    sly.first_size = 0;
	    sly.other_size = 0;
	    sly.first_slice_header = 0; // will eventually be set from tlv
	    sly.other_slice_header = 0; // will eventually be set from tlv

	    tempo.read(f);        // read the list of TLV stored in the header
	    fill_from(ui, tempo); // from the TLV list, set the different fields of the current header object


		// header information was not stored as TLV
		// we obviously read a header at the start of a slice
		// not a header of a reference archive stored beside an isolated catalogue
		// thus we can the header size information from the generic_file we read from:
	    if(sly.other_slice_header.is_zero())
	    {
		sly.other_slice_header = f.get_position();

		if(sly.other_slice_header <= min_size())
		    throw Erange("header::read", gettext("Invalid slice header size for a slice of format 08 or more recent (below minimum)"));

		if(sly.first_slice_header.is_zero())
		    sly.first_slice_header = sly.other_slice_header;

		    // in format >= 08 all slice have the exact same header (and thus header size).
		if(sly.first_slice_header != sly.other_slice_header)
		    throw SRC_BUG;


		    // if slice_header were not present as TLV
		    // we know the slice header currently read is not a reference
		    // of another archive, so we can replace the zero-valued
		    // slice size by the size of the generic_file we read from
		    // if possible (when not reading from a pipe):
		if(f_fic != nullptr)
		{
			// if a single sliced archive was requested at creation time
		    if(sly.other_size.is_zero())
			sly.other_size = f_fic->get_size();
		}
	    }

		// handling the case of nospecific size for the first slice

	    if(sly.first_size.is_zero())
		sly.first_size = sly.other_size;

		// sanity checks

	    if(! sly.first_size.is_zero() && sly.first_size <= sly.first_slice_header)
		throw Erange("header::read", gettext("Incoherent slice header: first slice size too small"));

	    if(! sly.other_size.is_zero() && sly.other_size <= sly.other_slice_header)
		throw Erange("header::read", gettext("Incoherent slice header: slice size too small"));

	    sly.older_sar_than_v8 = false;
	    break;
        default:
	    if(!lax)
		throw Erange("header::read", gettext("Badly formatted SAR header (unknown TLV type in slice header)"));
	    else
	    {
		ui.message(gettext("LAX MODE: Unknown data in slice header, ignoring and continuing"));
		sly.other_size = 0;
	    }
        }

	if(data_name.is_cleared())
	    data_name = internal_name;

    }

    void header::write(user_interaction & ui,
		       generic_file & f,
		       bool as_first_slice,
		       bool with_header_size) const
    {
        magic_number tmp;
	char tmp_ext[] = { extension_tlv, '\0' };

	if(with_header_size && ! as_first_slice)
	    throw SRC_BUG;
	    // writing down with header size information
	    // is used in the context of isolated catalogue
	    // to avoid fetching the slicing information
	    // from the real archive but from the isolated
	    // catalogue. For old archive format, the
	    // first slice is requested (because it has
	    // informations the other don't have, like the
	    // first slice size), thus the as_first_slice
	    // should always be set in that context

        tmp = htonl(magic);
        f.write((char *)&tmp, sizeof(magic));
        internal_name.dump(f);
        f.write(&flag, 1);
	if(sly.older_sar_than_v8)
	{
	    if(sly.first_size != sly.other_size && as_first_slice)
	    {
		tmp_ext[0] = extension_size;
		f.write(tmp_ext, 1);
		sly.other_size.dump(f);
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
	    build_tlv_list(ui, with_header_size).dump(f);
	}

	    // we will record the current offset as the slice header size
	    // in case the archive is read from the start of an slice (not
	    // read from within a header_version).
	    // This condition is not met when whith_header_size is set,
	    // which instructs to write down the slice header with the header
	    // size information, information/field which are not used at the
	    // beginning of a slice, but precisely within a header_version in
	    // the context of an isolated catalogue.
	if(!with_header_size && sly.first_slice_header.is_zero() && sly.other_slice_header.is_zero())
	{
	    header* me = const_cast<header*>(this);

	    if(me == nullptr)
		throw SRC_BUG;

	    me->sly.first_slice_header = f.get_position();
	    if(sly.first_slice_header < header::min_size())
		throw SRC_BUG;

	    if(sly.older_sar_than_v8)
		me->sly.other_slice_header = header::min_size();
	    else
		me->sly.other_slice_header = sly.first_slice_header;

		// some additional sanity checks:

	    if(! sly.first_size.is_zero() && sly.first_slice_header >= sly.first_size)
		throw SRC_BUG; // header is larger than the slice size
	    if(! sly.other_size.is_zero() && sly.other_slice_header >= sly.other_size)
		throw SRC_BUG; // header is larger than the slice size
	}
    }

    void header::clear()
    {
	magic = 0;
	internal_name.clear();
	data_name.clear();
	flag = '\0';
	sly.clear();
    }

    bool header::check_same_slice_set(const header & ref) const
    {
	return internal_name == ref.internal_name;
    }


    bool header::check_same_data_set(const header & ref) const
    {
	return data_name == ref.data_name;
    }

    void header::fill_from(user_interaction & ui, const tlv_list & extension)
    {
	U_I taille = extension.size();

	sly.first_size = 0;
	sly.other_size = 0;
	for(U_I index = 0; index < taille; ++index)
	{
	    switch(extension[index].get_type())
	    {
	    case tlv_first_size:
		extension[index].skip(0);
		sly.first_size.read(extension[index]);
		break;
	    case tlv_size:
		extension[index].skip(0);
		sly.other_size.read(extension[index]);
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
	    case tlv_header_size:
		extension[index].skip(0);
		sly.first_slice_header.read(extension[index]);
		sly.other_slice_header.read(extension[index]);
		if(sly.first_slice_header.is_zero())
		    throw Erange("header::fill_from", gettext("Unexpected null size for first slice header size"));
		if(sly.other_slice_header.is_zero())
		    throw Erange("header::fill_from", gettext("Unexpected null size for slice header size"));
		break;
	    default:
		ui.pause(tools_printf(gettext("Unknown entry found in slice header (type = %d), option not supported. The archive you are reading may have been generated by a more recent version of libdar, ignore this entry and continue anyway?"), extension[index].get_type()));
	    }
	}
    }

    tlv_list header::build_tlv_list(user_interaction & ui, bool with_header_size) const
    {
	tlv_list ret;
	tlv tmp;

	if(! sly.first_size.is_zero())
	{
	    tmp.reset();
	    sly.first_size.dump(tmp);
	    tmp.set_type(tlv_first_size);
	    ret.add(tmp);
	}

	if(! sly.other_size.is_zero())
	{
	    tmp.reset();
	    sly.other_size.dump(tmp);
	    tmp.set_type(tlv_size);
	    ret.add(tmp);
	}

	if(with_header_size && sly.other_slice_header.is_zero())
	    throw SRC_BUG;

	if(with_header_size && sly.first_slice_header.is_zero())
	    throw SRC_BUG;

	if(with_header_size)
	{
	    tmp.reset();
	    sly.first_slice_header.dump(tmp);
	    sly.other_slice_header.dump(tmp);
	    tmp.set_type(tlv_header_size);
	    ret.add(tmp);
	}

	tmp.reset();
	data_name.dump(tmp);
	tmp.set_type(tlv_data_name);
	ret.add(tmp);

	return ret;
    }

} // end of namespace
