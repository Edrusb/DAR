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
#ifdef STDC_HEADERS
#include <ctype.h>
#endif
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
# include <sys/time.h>
# else
# include <time.h>
# endif
#endif
#if HAVE_LIMITS_H
#include <limits.h>
#endif
} // end extern "C"

#include "cat_inode.hpp"
#include "cat_lien.hpp"

#define INODE_FLAG_EA_MASK  0x07
#define INODE_FLAG_EA_FULL  0x01
#define INODE_FLAG_EA_PART  0x02
#define INODE_FLAG_EA_NONE  0x03
#define INODE_FLAG_EA_FAKE  0x04
#define INODE_FLAG_EA_REMO  0x05

#define INODE_FLAG_FSA_MASK 0x18
#define INODE_FLAG_FSA_NONE 0x00
#define INODE_FLAG_FSA_PART 0x08
#define INODE_FLAG_FSA_FULL 0x10

using namespace std;

namespace libdar
{

    const ea_attributs cat_inode::empty_ea;

    cat_inode::cat_inode(const infinint & xuid, const infinint & xgid, U_16 xperm,
                 const datetime & last_access,
                 const datetime & last_modif,
                 const datetime & last_change,
                 const string & xname,
                 const infinint & fs_device) : cat_nomme(xname)
    {
	nullifyptr();
        uid = xuid;
        gid = xgid;
        perm = xperm;
        xsaved = s_not_saved;
        ea_saved = ea_none;
	fsa_saved = fsa_none;
        edit = 0;

        try
        {
            last_acc = last_access;
            last_mod = last_modif;
            last_cha = new (get_pool()) datetime(last_change);
            fs_dev = new (get_pool()) infinint(fs_device);
            if(last_cha == NULL || fs_dev == NULL)
                throw Ememory("cat_inode::cat_inode");
        }
        catch(...)
        {
	    destroy();
	    throw;
        }
    }

    cat_inode::cat_inode(user_interaction & dialog,
                 generic_file & f,
                 const archive_version & reading_ver,
                 saved_status saved,
                 compressor *efsa_loc,
                 escape *ptr) : cat_nomme(f)
    {
        U_16 tmp;
        unsigned char flag;

	nullifyptr();

	try
	{
	    xsaved = saved;
	    edit = reading_ver;
	    esc = ptr;

	    if(reading_ver > 1)
	    {
		f.read((char *)(&flag), 1);
		unsigned char ea_flag = flag & INODE_FLAG_EA_MASK;
		switch(ea_flag)
		{
		case INODE_FLAG_EA_FULL:
		    ea_saved = ea_full;
		    break;
		case INODE_FLAG_EA_PART:
		    ea_saved = ea_partial;
		    break;
		case INODE_FLAG_EA_NONE:
		    ea_saved = ea_none;
		    break;
		case INODE_FLAG_EA_FAKE:
		    ea_saved = ea_fake;
		    break;
		case INODE_FLAG_EA_REMO:
		    ea_saved = ea_removed;
		    break;
		default:
		    throw Erange("cat_inode::cat_inode", gettext("badly structured inode: unknown inode flag"));
		}
	    }
	    else
		ea_saved = ea_none;

	    if(reading_ver <= 7)
	    {
		    // UID and GID were stored on 16 bits each

		if(f.read((char *)&tmp, sizeof(tmp)) != sizeof(tmp))
		    throw Erange("cat_inode::cat_inode", gettext("missing data to build an inode"));
		uid = ntohs(tmp);
		if(f.read((char *)&tmp, sizeof(tmp)) != sizeof(tmp))
		    throw Erange("cat_inode::cat_inode", gettext("missing data to build an inode"));
		gid = ntohs(tmp);
	    }
	    else // archive format >= "08"
	    {
		uid = infinint(f);
		gid = infinint(f);
	    }

	    if(f.read((char *)&tmp, sizeof(tmp)) != sizeof(tmp))
		throw Erange("cat_inode::cat_inode", gettext("missing data to build an inode"));
	    perm = ntohs(tmp);

	    last_acc.read(f, reading_ver);
	    last_mod.read(f, reading_ver);

	    if(reading_ver >= 8)
	    {
		last_cha = new (get_pool()) datetime(f, reading_ver);
		if(last_cha == NULL)
		    throw Ememory("cat_inode::cat_inode(file)");

		if(ea_saved == ea_full)
		{
		    ea_size = new (get_pool()) infinint(f);
		    if(ea_size == NULL)
			throw Ememory("cat_inode::cat_inode(file)");
		}
	    }
	    else // archive format <= 7
	    {
		    // ea_size stays NULL meaning EA size unknown (old format)
	    }

	    if(esc == NULL) // reading a full entry from catalogue
	    {
		switch(ea_saved)
		{
		case ea_full:
		    ea_offset = new (get_pool()) infinint(f);
		    if(ea_offset == NULL)
			throw Ememory("cat_inode::cat_inode(file)");

		    if(reading_ver <= 7)
		    {
			ea_crc = create_crc_from_file(f, get_pool(), true);
			if(ea_crc == NULL)
			    throw SRC_BUG;

			last_cha = new (get_pool()) datetime(f, reading_ver);
			if(last_cha == NULL)
			    throw Ememory("cat_inode::cat_inode(file)");
		    }
		    else // archive format >= 8
		    {
			ea_crc = create_crc_from_file(f, get_pool(), false);
			if(ea_crc == NULL)
			    throw SRC_BUG;
		    }
		    break;
		case ea_partial:
		case ea_fake:
		    if(reading_ver <= 7)
		    {
			last_cha = new (get_pool()) datetime(f, reading_ver);
			if(last_cha == NULL)
			    throw Ememory("cat_inode::cat_inode(file)");
		    }
		    break;
		case ea_none:
		case ea_removed:
		    if(reading_ver <= 7)
		    {
			last_cha = new (get_pool()) datetime();
			if(last_cha == NULL)
			    throw Ememory("cat_inode::cat_inode(file)");
		    }
		    break;
		default:
		    throw SRC_BUG;
		}
	    }
	    else // reading a small dump using escape sequence marks
	    {
		    // header version is greater than or equal to "08" (small dump appeared at
		    // this version of archive format) ea_offset is not used (sequential read mode)
		    // while ea_CRC has been dumped a bit further in that case, we will fetch
		    // its value upon request by get_ea() or ea_get_crc()
		    // methods
	    }
	    ea = NULL; // in any case

		// to be able later to read EA and FSA from archive file
	    if(efsa_loc == NULL)
		throw SRC_BUG;
	    storage = efsa_loc;

	    if(reading_ver >= 9)
	    {
		unsigned char fsa_flag = flag & INODE_FLAG_FSA_MASK;

		switch(fsa_flag)
		{
		case INODE_FLAG_FSA_NONE:
		    fsa_saved = fsa_none;
		    break;
		case INODE_FLAG_FSA_PART:
		    fsa_saved = fsa_partial;
		    break;
		case INODE_FLAG_FSA_FULL:
		    fsa_saved = fsa_full;
		    break;
		default:
		    throw Erange("cat_inode::cat_inode", gettext("badly structured inode: unknown inode flag for FSA"));
		}

		if(fsa_saved != fsa_none)
		{
		    fsa_families = new (get_pool()) infinint(f);
		    if(fsa_families == NULL)
			throw Ememory("cat_inode::cat_inode(file)");
		}

		if(fsa_saved == fsa_full)
		{
		    fsa_size = new (get_pool()) infinint(f);
		    if(fsa_size == NULL)
			throw Ememory("cat_inode::cat_inode(file)");
		}

		if(esc == NULL)
		{
		    switch(fsa_saved)
		    {
		    case fsa_full:
			fsa_offset = new (get_pool()) infinint(f);
			fsa_crc = create_crc_from_file(f, get_pool());
			if(fsa_offset == NULL || fsa_crc == NULL)
			    throw Ememory("cat_inode::cat_inode(file)");
			break;
		    case fsa_partial:
		    case fsa_none:
			break;
		    default:
			throw SRC_BUG;
		    }
		}
		else  // reading a small dump using escape sequence marks
		{
			// fsa_offset is not used and fsa_CRC have been dumped a bit
			// further in that case (sequential read mode),
			// and will be fetched by get_fsa() or ea_get_crc()
			// methods
		}
	    }
	    else // older archive than version 9 do not support FSA
		fsa_saved = fsa_none;
        }
        catch(...)
        {
	    destroy();
	    throw;
        }
    }

    cat_inode::cat_inode(const cat_inode & ref) : cat_nomme(ref)
    {
	nullifyptr();

        try
        {
	    copy_from(ref);
        }
	catch(...)
	{
	    destroy();
	    throw;
	}
    }

    const cat_inode & cat_inode::operator = (const cat_inode & ref)
    {
        cat_nomme *me = this;
        const cat_nomme *nref = &ref;

        *me = *nref; // copying the "cat_nomme" part of the object

	destroy();
	copy_from(ref);

        return *this;
    }

    cat_inode::~cat_inode()
    {
	destroy();
    }

    bool cat_inode::same_as(const cat_inode & ref) const
    {
        return cat_nomme::same_as(ref) && compatible_signature(ref.signature(), signature());
    }

    bool cat_inode::is_more_recent_than(const cat_inode & ref, const infinint & hourshift) const
    {
        return ref.last_mod < last_mod && !tools_is_equal_with_hourshift(hourshift, ref.last_mod, last_mod);
    }

    bool cat_inode::has_changed_since(const cat_inode & ref, const infinint & hourshift, comparison_fields what_to_check) const
    {
        return (what_to_check != cf_inode_type && (hourshift > 0 ? ! tools_is_equal_with_hourshift(hourshift, ref.last_mod, last_mod) : ref.last_mod != last_mod))
            || (what_to_check == cf_all && uid != ref.uid)
            || (what_to_check == cf_all && gid != ref.gid)
            || (what_to_check != cf_mtime && what_to_check != cf_inode_type && perm != ref.perm);
    }

    void cat_inode::compare(const cat_inode &other,
                        const mask & ea_mask,
                        comparison_fields what_to_check,
                        const infinint & hourshift,
			bool symlink_date,
			const fsa_scope & scope,
			bool isolated_mode) const
    {
	bool do_mtime_test = dynamic_cast<const cat_lien *>(&other) == NULL || symlink_date;

        if(!same_as(other))
            throw Erange("cat_inode::compare",gettext("different file type"));
        if(what_to_check == cf_all && get_uid() != other.get_uid())
	{
	    infinint u1 = get_uid();
	    infinint u2 = other.get_uid();
            throw Erange("cat_inode.compare", tools_printf(gettext("different owner (uid): %i <--> %i"), &u1, &u2));
	}
        if(what_to_check == cf_all && get_gid() != other.get_gid())
	{
	    infinint g1 = get_gid();
	    infinint g2 = other.get_gid();
            throw Erange("cat_inode.compare", tools_printf(gettext("different owner group (gid): %i <--> %i"), &g1, &g2));
	}
        if((what_to_check == cf_all || what_to_check == cf_ignore_owner) && get_perm() != other.get_perm())
	{
	    string p1 = tools_int2octal(get_perm());
	    string p2 = tools_int2octal(other.get_perm());
            throw Erange("cat_inode.compare", tools_printf(gettext("different permission: %S <--> %S"), &p1, &p2));
	}
        if(do_mtime_test
	   && (what_to_check == cf_all || what_to_check == cf_ignore_owner || what_to_check == cf_mtime)
           && !tools_is_equal_with_hourshift(hourshift, get_last_modif(), other.get_last_modif()))
	{
	    string s1 = tools_display_date(get_last_modif());
	    string s2 = tools_display_date(other.get_last_modif());
            throw Erange("cat_inode.compare", tools_printf(gettext("difference of last modification date: %S <--> %S"), &s1, &s2));
	}

        sub_compare(other, isolated_mode);

        switch(ea_get_saved_status())
        {
        case ea_full:
            if(other.ea_get_saved_status() == ea_full)
            {
		if(!isolated_mode)
		{
		    const ea_attributs *me = get_ea(); // this pointer must not be freed
		    const ea_attributs *you = other.get_ea(); // this pointer must not be freed neither
		    if(me->diff(*you, ea_mask))
			throw Erange("cat_inode::compare", gettext("different Extended Attributes"));
		}
            }
            else
            {
#ifdef EA_SUPPORT
                throw Erange("cat_inode::compare", gettext("no Extended Attribute to compare with"));
#else
                throw Ecompilation(gettext("Cannot compare EA: EA support has not been activated at compilation time"));
#endif
            }
                // else we ignore the EA present in the argument,
                // this is not a symetrical comparison
                // we check that all data in current object are the same in the argument
                // but additional data can reside in the argument
            break;
        case ea_partial:
        case ea_fake:
            if(other.ea_get_saved_status() != ea_none && other.ea_get_saved_status() != ea_removed)
            {
                if(!tools_is_equal_with_hourshift(hourshift, get_last_change(), other.get_last_change())
                   && get_last_change() < other.get_last_change())
                    throw Erange("cat_inode::compare", gettext("inode last change date (ctime) greater, EA might be different"));
            }
            else
            {
#ifdef EA_SUPPORT
                throw Erange("cat_inode::compare", gettext("no Extended Attributes to compare with"));
#else
                throw Ecompilation(gettext("Cannot compare EA: EA support has not been activated at compilation time"));
#endif
            }
            break;
        case ea_none:
        case ea_removed:
            break;
        default:
            throw SRC_BUG;
        }

	switch(fsa_get_saved_status())
	{
	case fsa_full:
	    if(other.fsa_get_saved_status() == fsa_full)
	    {
		if(!isolated_mode)
		{
		    const filesystem_specific_attribute_list *me = get_fsa();
		    const filesystem_specific_attribute_list *you = other.get_fsa();

		    if(me == NULL)
			throw SRC_BUG;
		    if(you == NULL)
			throw SRC_BUG;

		    if(!me->is_included_in(*you, scope))
			throw Erange("cat_inode::compare", gettext("different Filesystem Specific Attributes"));
		}
	    }
	    else
		throw Erange("cat_inode::compare", gettext("No Filesystem Specific Attribute to compare with"));
	    break;
	case fsa_partial:
	    if(other.fsa_get_saved_status() != fsa_none)
	    {
		if(!tools_is_equal_with_hourshift(hourshift, get_last_change(), other.get_last_change())
                   && get_last_change() < other.get_last_change())
                    throw Erange("cat_inode::compare", gettext("inode last change date (ctime) greater, FSA might be different"));
	    }
	    else
		throw Erange("cat_inode::compare", gettext("Filesystem Specific Attribute are missing"));
	    break;
	case fsa_none:
	    break; // nothing to check
	default:
	    throw SRC_BUG;
	}
    }

    void cat_inode::inherited_dump(generic_file & r, bool small) const
    {
        U_16 tmp;
        unsigned char flag = 0;

        switch(ea_saved)
        {
        case ea_none:
            flag |= INODE_FLAG_EA_NONE;
            break;
        case ea_partial:
            flag |= INODE_FLAG_EA_PART;
            break;
        case ea_fake:
            flag |= INODE_FLAG_EA_FAKE;
            break;
        case ea_full:
            flag |= INODE_FLAG_EA_FULL;
            break;
        case ea_removed:
            flag |= INODE_FLAG_EA_REMO;
            break;
        default:
            throw SRC_BUG; // unknown value for ea_saved
        }

	switch(fsa_saved)
	{
	case fsa_none:
	    flag |= INODE_FLAG_FSA_NONE;
	    break;
	case fsa_partial:
	    flag |= INODE_FLAG_FSA_PART;
	    break;
	case fsa_full:
	    flag |= INODE_FLAG_FSA_FULL;
	    break;
	default:
	    throw SRC_BUG; // unknown value for fsa_saved
	}

        cat_nomme::inherited_dump(r, small);

	    // unix inode part

        r.write((char *)(&flag), 1);
        uid.dump(r);
        gid.dump(r);
        tmp = htons(perm);
        r.write((char *)&tmp, sizeof(tmp));
        last_acc.dump(r);
        last_mod.dump(r);
        if(last_cha == NULL)
            throw SRC_BUG;
        last_cha->dump(r);

	    // EA part

        if(ea_saved == ea_full)
            ea_get_size().dump(r);

        if(!small)
        {
            switch(ea_saved)
            {
            case ea_full:
		if(ea_offset == NULL)
		    throw SRC_BUG;
                ea_offset->dump(r);
                if(ea_crc == NULL)
                    throw SRC_BUG;
                ea_crc->dump(r);
                break;
            case ea_partial:
            case ea_fake:
            case ea_none:
            case ea_removed:
                break;
            default:
                throw SRC_BUG;
            }
        }

	    // FSA part

	if(fsa_saved != fsa_none)
	{
	    if(fsa_families == NULL)
		throw SRC_BUG;
	    fsa_families->dump(r);
	}
	if(fsa_saved == fsa_full)
	{
	    if(fsa_size == NULL)
		throw SRC_BUG;
	    fsa_size->dump(r);
	}

	if(!small)
	{
	    switch(fsa_saved)
	    {
	    case fsa_full:
		if(fsa_offset == NULL)
		    throw SRC_BUG;
		fsa_offset->dump(r);
		if(fsa_crc == NULL)
		    throw SRC_BUG;
		fsa_crc->dump(r);
		break;
	    case fsa_partial:
	    case fsa_none:
		break;
	    default:
		throw SRC_BUG;
	    }
	}
    }

    void cat_inode::ea_set_saved_status(ea_status status)
    {
        if(status == ea_saved)
            return;
        switch(status)
        {
        case ea_none:
        case ea_removed:
        case ea_partial:
        case ea_fake:
            if(ea != NULL)
            {
                delete ea;
                ea = NULL;
            }
	    if(ea_offset != NULL)
	    {
		delete ea_offset;
		ea_offset = NULL;
	    }
            break;
        case ea_full:
            if(ea != NULL)
                throw SRC_BUG;
	    if(ea_offset != NULL)
		throw SRC_BUG;
            break;
        default:
            throw SRC_BUG;
        }
        ea_saved = status;
    }

    void cat_inode::ea_attach(ea_attributs *ref)
    {
        if(ea_saved != ea_full)
            throw SRC_BUG;

        if(ref != NULL && ea == NULL)
        {
	    if(ea_size != NULL)
	    {
		delete ea_size;
		ea_size = NULL;
	    }
            ea_size = new (get_pool()) infinint(ref->space_used());
	    if(ea_size == NULL)
		throw Ememory("cat_inode::ea_attach");
            ea = ref;
        }
        else
            throw SRC_BUG;
    }

    const ea_attributs *cat_inode::get_ea() const
    {
        switch(ea_saved)
        {
        case ea_full:
            if(ea != NULL)
                return ea;
            else
                if(storage != NULL) // reading from archive
                {
		    crc *val = NULL;
		    const crc *my_crc = NULL;

		    try
		    {
			if(esc == NULL) // direct read mode
			{
			    if(ea_offset == NULL)
				throw SRC_BUG;
			    storage->skip(*ea_offset);
			}
			else // sequential read mode
			{
			    if(!esc->skip_to_next_mark(escape::seqt_ea, false))
				throw Erange("cat_inode::get_ea", string("Error while fetching EA from archive: No escape mark found for that file"));
			    storage->skip(esc->get_position()); // required to eventually reset the compression engine
			    const_cast<cat_inode *>(this)->ea_set_offset(storage->get_position());
			}

			if(ea_get_size() == 0)
			    storage->reset_crc(crc::OLD_CRC_SIZE);
			else
			{
			    storage->reset_crc(tools_file_size_to_crc_size(ea_get_size()));
			    storage->read_ahead(ea_get_size());
			}

			try
			{
			    try
			    {
				if(edit <= 1)
				    throw SRC_BUG;   // EA do not exist in that archive format
				const_cast<ea_attributs *&>(ea) = new (get_pool()) ea_attributs(*storage, edit);
				if(ea == NULL)
				    throw Ememory("cat_inode::get_ea");
			    }
			    catch(Euser_abort & e)
			    {
				throw;
			    }
			    catch(Ebug & e)
			    {
				throw;
			    }
			    catch(Ethread_cancel & e)
			    {
				throw;
			    }
			    catch(Egeneric & e)
			    {
				throw Erange("cat_inode::get_ea", string("Error while reading EA from archive: ") + e.get_message());
			    }
			}
			catch(...)
			{
			    val = storage->get_crc(); // keeps storage in coherent status
			    throw;
			}
			val = storage->get_crc();
			if(val == NULL)
			    throw SRC_BUG;

			ea_get_crc(my_crc); // ea_get_crc() will eventually fetch the CRC for EA from the archive (sequential reading)
			if(my_crc == NULL)
			    throw SRC_BUG;

			if(typeid(*val) != typeid(*my_crc) || *val != *my_crc)
			    throw Erange("cat_inode::get_ea", gettext("CRC error detected while reading EA"));
		    }
		    catch(...)
		    {
			if(val != NULL)
			    delete val;
			throw;
		    }
		    if(val != NULL)
			delete val;
                    return ea;
                }
                else
                    throw SRC_BUG;
                // no need of break here
            throw SRC_BUG; // but ... instead of break we use some more radical precaution.
        case ea_removed:
            return &empty_ea;
                // no need of break here
        default:
            throw SRC_BUG;
        }
    }

    void cat_inode::ea_detach() const
    {
        if(ea != NULL)
        {
            delete ea;
            const_cast<ea_attributs *&>(ea) = NULL;
        }
    }

    infinint cat_inode::ea_get_size() const
    {
        if(ea_saved == ea_full)
        {
            if(ea_size == NULL) // reading an old archive
            {
                if(ea != NULL)
		{
                    const_cast<cat_inode *>(this)->ea_size = new (get_pool()) infinint (ea->space_used());
		    if(ea_size == NULL)
			throw Ememory("cat_inode::ea_get_size");
		}
		else // else we stick with value 0, meaning that we read an old archive
		    return 0;
            }
            return *ea_size;
        }
        else
            throw SRC_BUG;
    }

    void cat_inode::ea_set_offset(const infinint & pos)
    {
	if(ea_offset == NULL)
	{
	    ea_offset = new (get_pool()) infinint(pos);
	    if(ea_offset == NULL)
		throw Ememory("cat_inode::ea_set_offset");
	}
	else
	    *ea_offset = pos;
    }

    bool cat_inode::ea_get_offset(infinint & val) const
    {
	if(ea_offset != NULL)
	{
	    val = *ea_offset;
	    return true;
	}
	else
	    return false;
    }

    void cat_inode::ea_set_crc(const crc & val)
    {
	if(ea_crc != NULL)
	{
	    delete ea_crc;
	    ea_crc = NULL;
	}
	ea_crc = val.clone();
	if(ea_crc == NULL)
	    throw Ememory("cat_inode::ea_set_crc");
    }

    void cat_inode::ea_get_crc(const crc * & ptr) const
    {
	if(ea_get_saved_status() != ea_full)
	    throw SRC_BUG;

        if(esc != NULL && ea_crc == NULL)
        {
            if(esc->skip_to_next_mark(escape::seqt_ea_crc, false))
            {
                crc *tmp = NULL;

                try
                {
                    if(edit >= 8)
                        tmp = create_crc_from_file(*esc, get_pool(), false);
                    else // archive format <= 7
                        tmp = create_crc_from_file(*esc, get_pool(), true);
		    if(tmp == NULL)
			throw SRC_BUG;
                    const_cast<cat_inode *>(this)->ea_crc = tmp;
		    tmp = NULL; // the object is now owned by "this"
                }
                catch(...)
                {
		    if(tmp != NULL)
			delete tmp;
                    throw;
                }
            }
            else
            {
                crc *tmp = new (get_pool()) crc_n(1); // creating a default CRC
                if(tmp == NULL)
                    throw Ememory("cat_inode::ea_get_crc");
                try
                {
                    tmp->clear();
                    const_cast<cat_inode *>(this)->ea_crc = tmp;
                        // this is to avoid trying to fetch the CRC a new time if decision
                        // has been taken to continue the operation after the exception
                        // thrown below has been caught.
		    tmp = NULL;  // the object is now owned by "this"
                }
                catch(...)
                {
                    delete tmp;
                    throw;
                }
                throw Erange("cat_inode::ea_get_crc", gettext("Error while reading CRC for EA from the archive: No escape mark found for that file"));
            }
        }

        if(ea_crc == NULL)
            throw SRC_BUG;
        else
            ptr = ea_crc;
    }

    bool cat_inode::ea_get_crc_size(infinint & val) const
    {
        if(ea_crc != NULL)
        {
            val = ea_crc->get_size();
            return true;
        }
        else
            return false;
    }

    datetime cat_inode::get_last_change() const
    {
        if(last_cha == NULL)
            throw SRC_BUG;
        else
            return *last_cha;
    }

    void cat_inode::set_last_change(const datetime & x_time)
    {
        if(last_cha == NULL)
            throw SRC_BUG;
        else
            *last_cha = x_time;
    }

    void cat_inode::fsa_set_saved_status(fsa_status status)
    {
	if(status == fsa_saved)
	    return;
	switch(status)
	{
	case fsa_none:
	case fsa_partial:
	    if(fsal != NULL)
	    {
		delete fsal;
		fsal = NULL;
	    }
	    if(fsa_offset != NULL)
	    {
		delete fsa_offset;
		fsa_offset = NULL;
	    }
	    break;
	case fsa_full:
	    if(fsal != NULL)
		throw SRC_BUG;
	    if(fsa_offset != NULL)
		throw SRC_BUG;
	    break;
	default:
	    throw SRC_BUG;
	}

	fsa_saved = status;
    }

    void cat_inode::fsa_attach(filesystem_specific_attribute_list *ref)
    {
        if(fsa_saved != fsa_full)
            throw SRC_BUG;

        if(ref != NULL && fsal == NULL)
        {
	    if(fsa_size != NULL)
	    {
		delete fsa_size;
		fsa_size = NULL;
	    }
	    if(fsa_families != NULL)
	    {
		delete fsa_families;
		fsa_families = NULL;
	    }
	    try
	    {
		fsa_size = new (get_pool()) infinint (ref->storage_size());
		fsa_families = new(get_pool()) infinint(fsa_scope_to_infinint(ref->get_fsa_families()));
		if(fsa_size == NULL || fsa_families == NULL)
		    throw Ememory("cat_inode::fsa_attach");
	    }
	    catch(...)
	    {
		if(fsa_size != NULL)
		{
		    delete fsa_size;
		    fsa_size = NULL;
		}
		if(fsa_families != NULL)
		{
		    delete fsa_families;
		    fsa_families = NULL;
		}
		throw;
	    }
	    fsal = ref;
        }
        else
            throw SRC_BUG;
    }

    void cat_inode::fsa_detach() const
    {
        if(fsal != NULL)
        {
            delete fsal;
            const_cast<cat_inode *>(this)->fsal = NULL;
        }
    }

    const filesystem_specific_attribute_list *cat_inode::get_fsa() const
    {
        switch(fsa_saved)
        {
        case fsa_full:
            if(fsal != NULL)
                return fsal;
            else
                if(storage != NULL) // lsa_offset may be equal to zero if the first cat_inode saved had only EA modified since archive of reference
                {
		    crc *val = NULL;
		    const crc *my_crc = NULL;

		    try
		    {
			if(esc == NULL)
			{
			    if(fsa_offset == NULL)
				throw SRC_BUG;
			    storage->skip(*fsa_offset);
			}
			else
			{
			    if(!esc->skip_to_next_mark(escape::seqt_fsa, false))
				throw Erange("cat_inode::get_fsa", string("Error while fetching FSA from archive: No escape mark found for that file"));
			    storage->skip(esc->get_position()); // required to eventually reset the compression engine
			    const_cast<cat_inode *>(this)->fsa_set_offset(storage->get_position());
			}

			storage->suspend_compression();
			try
			{
			    storage->reset_crc(tools_file_size_to_crc_size(fsa_get_size()));

			    try
			    {
				try
				{
				    const_cast<cat_inode *>(this)->fsal = new (get_pool()) filesystem_specific_attribute_list();
				    if(fsal == NULL)
					throw Ememory("cat_inode::get_fsa");
				    try
				    {
					storage->read_ahead(fsa_get_size());
					const_cast<cat_inode *>(this)->fsal->read(*storage, edit);
				    }
				    catch(...)
				    {
					delete fsal;
					const_cast<cat_inode *>(this)->fsal = NULL;
					throw;
				    }
				}
				catch(Euser_abort & e)
				{
				    throw;
				}
				catch(Ebug & e)
				{
				    throw;
				}
				catch(Ethread_cancel & e)
				{
				    throw;
				}
				catch(Egeneric & e)
				{
				    throw Erange("cat_inode::get_fda", string("Error while reading FSA from archive: ") + e.get_message());
				}
			    }
			    catch(...)
			    {
				val = storage->get_crc(); // keeps storage in coherent status
				throw;
			    }

			    val = storage->get_crc();
			    if(val == NULL)
				throw SRC_BUG;
			}
			catch(...)
			{
			    storage->resume_compression();
			    throw;
			}
			storage->resume_compression();

			fsa_get_crc(my_crc); // fsa_get_crc() will eventually fetch the CRC for FSA from the archive (sequential reading)
			if(my_crc == NULL)
			    throw SRC_BUG;

			if(typeid(*val) != typeid(*my_crc) || *val != *my_crc)
			    throw Erange("cat_inode::get_fsa", gettext("CRC error detected while reading FSA"));
		    }
		    catch(...)
		    {
			if(val != NULL)
			    delete val;
			throw;
		    }
		    if(val != NULL)
			delete val;
                    return fsal;
                }
                else
                    throw SRC_BUG;
                // no need of break here
            throw SRC_BUG; // but ... instead of break we use some more radical precaution.
        default:
            throw SRC_BUG;
        }
    }

    infinint cat_inode::fsa_get_size() const
    {
        if(fsa_saved == fsa_full)
	    if(fsa_size != NULL)
		return *fsa_size;
	    else
		throw SRC_BUG;
        else
            throw SRC_BUG;
    }

    void cat_inode::fsa_set_offset(const infinint & pos)
    {
	if(fsa_offset == NULL)
	{
	    fsa_offset = new (get_pool()) infinint(pos);
	    if(fsa_offset == NULL)
		throw Ememory("cat_inode::fsa_set_offset");
	}
	else
	    *fsa_offset = pos;
    }

    bool cat_inode::fsa_get_offset(infinint & pos) const
    {
	if(fsa_offset != NULL)
	{
	    pos = *fsa_offset;
	    return true;
	}
	else
	    return false;
    }


    void cat_inode::fsa_set_crc(const crc & val)
    {
	if(fsa_crc != NULL)
	{
	    delete fsa_crc;
	    fsa_crc = NULL;
	}
	fsa_crc = val.clone();
	if(fsa_crc == NULL)
	    throw Ememory("cat_inode::fsa_set_crc");
    }

    void cat_inode::fsa_get_crc(const crc * & ptr) const
    {
	if(fsa_get_saved_status() != fsa_full)
	    throw SRC_BUG;

        if(esc != NULL && fsa_crc == NULL)
        {
            if(esc->skip_to_next_mark(escape::seqt_fsa_crc, false))
            {
                crc *tmp = NULL;

                try
                {
		    tmp = create_crc_from_file(*esc, get_pool(), false);
		    if(tmp == NULL)
			throw SRC_BUG;
                    const_cast<cat_inode *>(this)->fsa_crc = tmp;
		    tmp = NULL; // the object is now owned by "this"
                }
                catch(...)
                {
		    if(tmp != NULL)
			delete tmp;
                    throw;
                }
            }
            else // fsa_crc mark not found
            {
                crc *tmp = new (get_pool()) crc_n(1); // creating a default CRC
                if(tmp == NULL)
                    throw Ememory("cat_inode::fsa_get_crc");
                try
                {
                    tmp->clear();
                    const_cast<cat_inode *>(this)->fsa_crc = tmp;
                        // this is to avoid trying to fetch the CRC a new time if decision
                        // has been taken to continue the operation after the exception
                        // thrown below has been caught.
		    tmp = NULL;  // the object is now owned by "this"
                }
                catch(...)
                {
                    delete tmp;
                    throw;
                }
                throw Erange("cat_inode::fsa_get_crc", gettext("Error while reading CRC for FSA from the archive: No escape mark found for that file"));
            }
        }

        if(fsa_crc == NULL)
            throw SRC_BUG;
        else
            ptr = fsa_crc;
    }

    bool cat_inode::fsa_get_crc_size(infinint & val) const
    {
        if(fsa_crc != NULL)
        {
            val = fsa_crc->get_size();
            return true;
        }
        else
            return false;
    }

    void cat_inode::nullifyptr()
    {
        last_cha = NULL;
        ea_offset = NULL;
	ea = NULL;
	ea_size = NULL;
        ea_crc = NULL;
	fsa_families = NULL;
	fsa_offset = NULL;
	fsal = NULL;
	fsa_size = NULL;
	fsa_crc = NULL;
        fs_dev = NULL;
	storage = NULL;
	esc = NULL;
    }

    void cat_inode::destroy()
    {
        if(last_cha != NULL)
        {
            delete last_cha;
            last_cha = NULL;
        }
        if(ea_offset != NULL)
        {
            delete ea_offset;
            ea_offset = NULL;
        }
        if(ea != NULL)
        {
            delete ea;
            ea = NULL;
        }
	if(ea_size != NULL)
	{
	    delete ea_size;
	    ea_size = NULL;
	}
        if(ea_crc != NULL)
        {
            delete ea_crc;
            ea_crc = NULL;
        }
	if(fsa_families != NULL)
	{
	    delete fsa_families;
	    fsa_families = NULL;
	}
	if(fsa_offset != NULL)
	{
	    delete fsa_offset;
	    fsa_offset = NULL;
	}
	if(fsal != NULL)
	{
	    delete fsal;
	    fsal = NULL;
	}
	if(fsa_size != NULL)
	{
	    delete fsa_size;
	    fsa_size = NULL;
	}
	if(fsa_crc != NULL)
	{
	    delete fsa_crc;
	    fsa_crc = NULL;
	}
        if(fs_dev != NULL)
        {
            delete fs_dev;
            fs_dev = NULL;
        }

            // we must not release memory pointed to by storage, we do not own that object

            // we must not release memory pointed to by esc, we do not own that object
    }

    template <class T> void copy_ptr(const T *src, T * & dst, memory_pool *p)
    {
	if(src == NULL)
	    dst = NULL;
	else
	{
	    dst = new (p) T(*src);
	    if(dst == NULL)
		throw Ememory("copy_ptr template");
	}
    }

    void cat_inode::copy_from(const cat_inode & ref)
    {
	try
	{
	    uid = ref.uid;
	    gid = ref.gid;
	    perm = ref.perm;
	    last_acc = ref.last_acc;
	    last_mod = ref.last_mod;
	    copy_ptr(ref.last_cha, last_cha, get_pool());
	    xsaved = ref.xsaved;
	    ea_saved = ref.ea_saved;
	    fsa_saved = ref.fsa_saved;
	    copy_ptr(ref.ea_offset, ea_offset, get_pool());
	    copy_ptr(ref.ea, ea, get_pool());
	    copy_ptr(ref.ea_size, ea_size, get_pool());
	    if(ref.ea_crc != NULL)
	    {
		ea_crc = (ref.ea_crc)->clone();
		if(ea_crc == NULL)
		    throw Ememory("cat_inode::copy_from");
	    }
	    else
		ea_crc = NULL;
	    copy_ptr(ref.fsa_families, fsa_families, get_pool());
	    copy_ptr(ref.fsa_offset, fsa_offset, get_pool());
	    copy_ptr(ref.fsal, fsal, get_pool());
	    copy_ptr(ref.fsa_size, fsa_size, get_pool());
	    if(ref.fsa_crc != NULL)
	    {
		fsa_crc = (ref.fsa_crc)->clone();
		if(fsa_crc == NULL)
		    throw Ememory("cat_inode::copy_from");
	    }
	    else
		fsa_crc = NULL;
	    copy_ptr(ref.fs_dev, fs_dev, get_pool());
	    storage = ref.storage; // yes copying the value of the pointer
	    edit = ref.edit;
	    esc = ref.esc; // yes copying the value of the pointer
	}
	catch(...)
	{
	    destroy();
	    throw;
	}
    }


} // end of namespace

