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
	fichier_global *f_fic = dynamic_cast<fichier_global *>(&f);

	clear();

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
		sly.other_size = f_fic->get_size();
		if(sly.other_size.is_zero())
		{
		    if(!lax)
			throw Erange("header::read", gettext("Invalide slice size"));
		    else
			ui.message(gettext("LAX MODE: slice size is not possible to read, (lack of virtual memory?), continuing anyway..."));

			// this extension was used in archives before release 2.4.0
			// when the first slice had the same size of the following ones.
			// The slice size of all slices was thus the one of the first which
			// was learnt by getting the size of the file.
			// This works also for single sliced archives.
		}
		else
		{
		    sly.first_size = 0;
			// header not the first slice or no
			// specific size for the first slice
		}
	    }
	    else // not reading from a file (but read from a pipe, for example)
	    {
		sly.other_size = 0; // means single sliced archive
		sly.first_size = 0; // means non-specific size for the first slice
	    }

	    sly.other_slice_header = min_size();
	    sly.first_slice_header = 0; // unknown slice header size of the first slice

	    sly.older_sar_than_v8 = true;
            break;
        case extension_size:
	    sly.other_size.read(f);
	    if(f_fic != nullptr)
	    {
		sly.first_size = f_fic->get_size();
		if(sly.first_size.is_zero())
		{
		    if(!lax)
			throw Erange("header::read", gettext("Invalide first slice size"));
		    else
			ui.message(gettext("LAX MODE: first slice size is not possible to read, (lack of virtual memory?), continuing anyway..."));

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
	    sly.other_slice_header = min_size();
	    if(sly.first_slice_header < min_size())
		throw Erange("header::read", gettext("Invalid first slice header size for a slice of format 07 or older"));

	    sly.older_sar_than_v8 = true;
            break;
	case extension_tlv:
	    sly.first_size = 0;
	    sly.other_size = 0;
	    sly.other_slice_header = 0; // we be eventually set from tlv
	    sly.first_slice_header = 0; // we be eventually set from tlv

	    tempo.read(f);        // read the list of TLV stored in the header
	    fill_from(ui, tempo); // from the TLV list, set the different fields of the current header object

		// specific case of trivial_sar (non-sliced archive)
		// where no slice nor first_slice info is set (surfacing both as zero)

	    if(sly.other_size.is_zero() && f_fic != nullptr)
		sly.other_size = f_fic->get_size();

	    if(sly.first_size.is_zero())
		sly.first_size = sly.other_size;

	    if(sly.other_slice_header.is_zero())
		sly.other_slice_header = f.get_position();

	    if(sly.other_slice_header < min_size())
		throw Erange("header::read", gettext("Invalid slice header size for a slice of format 08 or more recent (below minimum)"));

	    sly.first_slice_header = sly.other_slice_header;
		// in format >= 08 all slice have the exact same header (and thus header size).

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
		       bool with_header_size) const
    {
        magic_number tmp;
	char tmp_ext[] = { extension_tlv, '\0' };

        tmp = htonl(magic);
        f.write((char *)&tmp, sizeof(magic));
        internal_name.dump(f);
        f.write(&flag, 1);
	if(sly.older_sar_than_v8)
	{
	    if(sly.first_size != sly.other_size)
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
    }

    void header::clear()
    {
	magic = 0;
	internal_name.clear();
	data_name.clear();
	flag = '\0';
	sly.clear();
	header_tlv = false;
    }

    void header::check_validity(user_interaction & dialog,
				const infinint & slice_num,
				bool initial) const
    {
	bool external = slice_num.is_zero();

	    // old format related checks

	if(get_format_07_compatibility() && initial && slice_num != 1)
	    throw Erange("header::check_validity", gettext("This is an old archive, it can only be opened starting by the first slice"));

	    // external related checks

	if(external && get_format_07_compatibility())
	    throw Erange("header::check_validity", gettext("slice layout of an old archive stored in an isolated catalog cannot be used to avoid openning the first or last slice of the archive"));

	if(external && ! has_header_tlv())
	    throw Erange("header::check_validity", gettext("Corrupted external slice header: missing header size tlv"));

	if(external && ! initial)
	    throw SRC_BUG;

	    // checking against the magic number
	    //
	if(get_magic() != SAUV_MAGIC_NUMBER)
	    throw Erange("sar::check_header", tools_printf(gettext("not a valid dar file (wrong magic number), please provide the good file.")));

	    // sanity checks on slice sizes and header slice sizes

	if(get_common_slice_header_size() < min_size())
		throw Erange("header::check_validity", gettext("Incoherent slice header: header size too small"));

	if(get_slice_size() <= get_common_slice_header_size())
	    throw Erange("header::check_validity", gettext("Incoherent slice header: slice size too small"));

	if(slice_num == 1 || !get_format_07_compatibility())
	{
		// for non initial slice, the size of the first slice and first slice header size
		// may be unknown (slice > 1 with format <= 07) and would be set both to zero

	    if(get_first_slice_size() < min_size())
		throw Erange("header::check_validity", gettext("Incoherent slice header: first slice header size too small"));

	    if(get_first_slice_size() <= get_first_slice_header_size())
		throw Erange("header::check_validity", gettext("Incoherent slice header: First slice size too small"));
	}
    }

    void header::check_same_slice_set(const header & ref) const
    {
	if(get_internal_name() != ref.get_internal_name())
	    throw Erange("header::check_same_slice_set", gettext("this slice is from a different set of slices"));
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
		sly.other_slice_header.read(extension[index]);
		sly.first_slice_header = sly.other_slice_header;
		header_tlv = true;
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

	if(with_header_size && ! sly.other_slice_header.is_zero() && ! sly.older_sar_than_v8)
	{
	    if(sly.other_slice_header != sly.first_slice_header)
		throw SRC_BUG;
	    tmp.reset();
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
