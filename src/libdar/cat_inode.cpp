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

#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

#if HAVE_TIME_H
# include <time.h>
#endif

#if HAVE_SYS_TIME_H
# include <sys/time.h>
#endif

#if HAVE_LIMITS_H
#include <limits.h>
#endif
} // end extern "C"

#include "cat_inode.hpp"
#include "cat_lien.hpp"
#include "tools.hpp"
#include "cat_signature.hpp"

    /// all values of FLAG for EA are covered by the EA_MASK:
#define INODE_FLAG_EA_MASK  0x07

    // FLAG values for EA
#define INODE_FLAG_EA_FULL  0x01
#define INODE_FLAG_EA_PART  0x02
#define INODE_FLAG_EA_NONE  0x03
#define INODE_FLAG_EA_FAKE  0x04
#define INODE_FLAG_EA_REMO  0x05

    /// all values of FLAG for FSA are covered by the FSA_MASK:
#define INODE_FLAG_FSA_MASK 0x18

    // FLAG values for FSA
#define INODE_FLAG_FSA_NONE 0x00
#define INODE_FLAG_FSA_PART 0x08
#define INODE_FLAG_FSA_FULL 0x10

using namespace std;

namespace libdar
{

    const ea_attributs cat_inode::empty_ea;

    cat_inode::cat_inode(const infinint & xuid,
			 const infinint & xgid,
			 U_16 xperm,
			 const datetime & last_access,
			 const datetime & last_modif,
			 const datetime & last_change,
			 const string & xname,
			 const infinint & fs_device) : cat_nomme(xname, saved_status::saved)
    {
	nullifyptr();
        uid = xuid;
        gid = xgid;
        perm = xperm;
        ea_saved = ea_saved_status::none;
	fsa_saved =  fsa_saved_status::none;
        edit = 0;
	small_read = false;

        try
        {
            last_acc = last_access;
            last_mod = last_modif;
	    last_cha = last_change;
            fs_dev = new (nothrow) infinint(fs_device);
            if(fs_dev == nullptr)
                throw Ememory("cat_inode::cat_inode");
        }
        catch(...)
        {
	    destroy();
	    throw;
        }
    }

    cat_inode::cat_inode(const shared_ptr<user_interaction> & dialog,
			 const smart_pointer<pile_descriptor> & pdesc,
			 const archive_version & reading_ver,
			 saved_status saved,
			 bool small) : cat_nomme(pdesc, small, saved)
    {
        U_16 tmp;
        unsigned char flag;
	generic_file *ptr = nullptr;

	pdesc->check(small);
	if(small)
	    ptr = pdesc->esc;
	else
	    ptr = pdesc->stack;

	nullifyptr();
	try
	{
	    edit = reading_ver;
	    small_read = small;

	    if(reading_ver > 1)
	    {
		ptr->read((char *)(&flag), 1);
		unsigned char ea_flag = flag & INODE_FLAG_EA_MASK;
		switch(ea_flag)
		{
		case INODE_FLAG_EA_FULL:
		    ea_saved = ea_saved_status::full;
		    break;
		case INODE_FLAG_EA_PART:
		    ea_saved = ea_saved_status::partial;
		    break;
		case INODE_FLAG_EA_NONE:
		    ea_saved = ea_saved_status::none;
		    break;
		case INODE_FLAG_EA_FAKE:
		    ea_saved = ea_saved_status::fake;
		    break;
		case INODE_FLAG_EA_REMO:
		    ea_saved = ea_saved_status::removed;
		    break;
		default:
		    throw Erange("cat_inode::cat_inode", gettext("badly structured inode: unknown inode flag"));
		}
	    }
	    else
		ea_saved = ea_saved_status::none;

	    if(reading_ver <= 7)
	    {
		    // UID and GID were stored on 16 bits each

		if(ptr->read((char *)&tmp, sizeof(tmp)) != sizeof(tmp))
		    throw Erange("cat_inode::cat_inode", gettext("missing data to build an inode"));
		uid = ntohs(tmp);
		if(ptr->read((char *)&tmp, sizeof(tmp)) != sizeof(tmp))
		    throw Erange("cat_inode::cat_inode", gettext("missing data to build an inode"));
		gid = ntohs(tmp);
	    }
	    else // archive format >= "08"
	    {
		uid = infinint(*ptr);
		gid = infinint(*ptr);
	    }

	    if(ptr->read((char *)&tmp, sizeof(tmp)) != sizeof(tmp))
		throw Erange("cat_inode::cat_inode", gettext("missing data to build an inode"));
	    perm = ntohs(tmp);

	    last_acc.read(*ptr, reading_ver);
	    last_mod.read(*ptr, reading_ver);

	    if(reading_ver >= 8)
	    {
		last_cha.read(*ptr, reading_ver);

		if(ea_saved == ea_saved_status::full)
		{
		    ea_size = new (nothrow) infinint(*ptr);
		    if(ea_size == nullptr)
			throw Ememory("cat_inode::cat_inode(file)");
		}
	    }
	    else // archive format <= 7
	    {
		    // ea_size stays nullptr meaning EA size unknown (old format)
		last_cha.nullify();
	    }

	    if(!small) // reading a full entry from catalogue
	    {
		switch(ea_saved)
		{
		case ea_saved_status::full:
		    ea_offset = new (nothrow) infinint(*ptr);
		    if(ea_offset == nullptr)
			throw Ememory("cat_inode::cat_inode(file)");

		    if(reading_ver <= 7)
		    {
			ea_crc = create_crc_from_file(*ptr, true);
			if(ea_crc == nullptr)
			    throw SRC_BUG;

			last_cha.read(*ptr, reading_ver);
		    }
		    else // archive format >= 8
		    {
			ea_crc = create_crc_from_file(*ptr, false);
			if(ea_crc == nullptr)
			    throw SRC_BUG;
		    }
		    break;
		case ea_saved_status::partial:
		case ea_saved_status::fake:
		    if(reading_ver <= 7)
		    {
			last_cha.read(*ptr, reading_ver);
		    }
		    break;
		case ea_saved_status::none:
		case ea_saved_status::removed:
		    if(reading_ver <= 7)
		    {
			last_cha.nullify();
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
	    ea = nullptr; // in any case

	    if(reading_ver >= 9)
	    {
		unsigned char fsa_flag = flag & INODE_FLAG_FSA_MASK;

		switch(fsa_flag)
		{
		case INODE_FLAG_FSA_NONE:
		    fsa_saved = fsa_saved_status::none;
		    break;
		case INODE_FLAG_FSA_PART:
		    fsa_saved = fsa_saved_status::partial;
		    break;
		case INODE_FLAG_FSA_FULL:
		    fsa_saved = fsa_saved_status::full;
		    break;
		default:
		    throw Erange("cat_inode::cat_inode", gettext("badly structured inode: unknown inode flag for FSA"));
		}

		if(fsa_saved !=  fsa_saved_status::none)
		{
		    fsa_families = new (nothrow) infinint(*ptr);
		    if(fsa_families == nullptr)
			throw Ememory("cat_inode::cat_inode(file)");
		}

		if(fsa_saved ==  fsa_saved_status::full)
		{
		    fsa_size = new (nothrow) infinint(*ptr);
		    if(fsa_size == nullptr)
			throw Ememory("cat_inode::cat_inode(file)");
		}

		if(!small)
		{
		    switch(fsa_saved)
		    {
		    case  fsa_saved_status::full:
			fsa_offset = new (nothrow) infinint(*ptr);
			fsa_crc = create_crc_from_file(*ptr);
			if(fsa_offset == nullptr || fsa_crc == nullptr)
			    throw Ememory("cat_inode::cat_inode(file)");
			break;
		    case fsa_saved_status::partial:
		    case  fsa_saved_status::none:
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
		fsa_saved =  fsa_saved_status::none;
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

    cat_inode & cat_inode::operator = (const cat_inode & ref)
    {
        cat_nomme *me = this;
        const cat_nomme *nref = &ref;

        *me = *nref; // copying the "cat_nomme" part of the object

	destroy();
	copy_from(ref);

        return *this;
    }

    cat_inode::~cat_inode() noexcept(false)
    {
	destroy();
    }

    bool cat_inode::same_as(const cat_inode & ref) const
    {
        return cat_nomme::same_as(ref) && cat_signature::compatible_signature(signature(), ref.signature());
    }

    bool cat_inode::is_more_recent_than(const cat_inode & ref, const infinint & hourshift) const
    {
        return ref.last_mod < last_mod && !tools_is_equal_with_hourshift(hourshift, ref.last_mod, last_mod);
    }

    bool cat_inode::has_changed_since(const cat_inode & ref, const infinint & hourshift, comparison_fields what_to_check) const
    {
        return (what_to_check != comparison_fields::inode_type && (!hourshift.is_zero() ? !tools_is_equal_with_hourshift(hourshift, ref.last_mod, last_mod) : !ref.last_mod.loose_equal(last_mod)))
            || (what_to_check == comparison_fields::all && uid != ref.uid)
            || (what_to_check == comparison_fields::all && gid != ref.gid)
            || (what_to_check != comparison_fields::mtime && what_to_check != comparison_fields::inode_type && perm != ref.perm);
    }

    void cat_inode::compare(const cat_inode &other,
			    const mask & ea_mask,
			    comparison_fields what_to_check,
			    const infinint & hourshift,
			    bool symlink_date,
			    const fsa_scope & scope,
			    bool isolated_mode,
			    bool seq_read_mode) const
    {
	bool do_mtime_test = dynamic_cast<const cat_lien *>(&other) == nullptr || symlink_date;

        if(!same_as(other))
            throw Erange("cat_inode::compare",gettext("different file type"));
        if(what_to_check == comparison_fields::all && get_uid() != other.get_uid())
	{
	    infinint u1 = get_uid();
	    infinint u2 = other.get_uid();
            throw Erange("cat_inode.compare", tools_printf(gettext("different owner (uid): %i <--> %i"), &u1, &u2));
	}
        if(what_to_check == comparison_fields::all && get_gid() != other.get_gid())
	{
	    infinint g1 = get_gid();
	    infinint g2 = other.get_gid();
            throw Erange("cat_inode.compare", tools_printf(gettext("different owner group (gid): %i <--> %i"), &g1, &g2));
	}
        if((what_to_check == comparison_fields::all || what_to_check == comparison_fields::ignore_owner) && get_perm() != other.get_perm())
	{
	    string p1 = tools_int2octal(get_perm());
	    string p2 = tools_int2octal(other.get_perm());
            throw Erange("cat_inode.compare", tools_printf(gettext("different permission: %S <--> %S"), &p1, &p2));
	}
        if(do_mtime_test
	   && (what_to_check == comparison_fields::all || what_to_check == comparison_fields::ignore_owner || what_to_check == comparison_fields::mtime)
           && !tools_is_equal_with_hourshift(hourshift, get_last_modif(), other.get_last_modif()))
	{
	    string s1 = tools_display_date(get_last_modif());
	    string s2 = tools_display_date(other.get_last_modif());
            throw Erange("cat_inode.compare", tools_printf(gettext("difference of last modification date: %S <--> %S"), &s1, &s2));
	}

        sub_compare(other, isolated_mode, seq_read_mode);

        switch(ea_get_saved_status())
        {
        case ea_saved_status::full:
            if(other.ea_get_saved_status() == ea_saved_status::full)
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
        case ea_saved_status::partial:
        case ea_saved_status::fake:
            if(other.ea_get_saved_status() != ea_saved_status::none && other.ea_get_saved_status() != ea_saved_status::removed)
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
        case ea_saved_status::none:
        case ea_saved_status::removed:
            break;
        default:
            throw SRC_BUG;
        }

	switch(fsa_get_saved_status())
	{
	case  fsa_saved_status::full:
	    if(other.fsa_get_saved_status() == fsa_saved_status::full)
	    {
		if(!isolated_mode)
		{
		    const filesystem_specific_attribute_list *me = get_fsa();
		    const filesystem_specific_attribute_list *you = other.get_fsa();

		    if(me == nullptr)
			throw SRC_BUG;
		    if(you == nullptr)
			throw SRC_BUG;

		    if(!me->is_included_in(*you, scope))
			throw Erange("cat_inode::compare", gettext("different Filesystem Specific Attributes"));
		}
	    }
	    else
	    {
		if(scope.size() > 0)
		    throw Erange("cat_inode::compare", gettext("No Filesystem Specific Attribute to compare with"));

		    // this is not perfect, the "other" could have no FSA due to the non empty scope excluding only
		    // the valid FSAs. A more specific comparison would worth it...
	    }
	    break;
	case fsa_saved_status::partial:
	    if(other.fsa_get_saved_status() != fsa_saved_status::none)
	    {
		if(!tools_is_equal_with_hourshift(hourshift, get_last_change(), other.get_last_change())
                   && get_last_change() < other.get_last_change())
                    throw Erange("cat_inode::compare", gettext("inode last change date (ctime) greater, FSA might be different"));
	    }
	    else
	    {
		if(scope.size() > 0)
		    throw Erange("cat_inode::compare", gettext("Filesystem Specific Attribute are missing"));

		    // this is not perfect, the "other" could have no FSA due to the non empty scope excluding only
		    // the valid FSAs. A more specific comparison would worth it...
	    }
	    break;
	case fsa_saved_status::none:
	    break; // nothing to check
	default:
	    throw SRC_BUG;
	}
    }

    void cat_inode::inherited_dump(const pile_descriptor & pdesc, bool small) const
    {
        U_16 tmp;
        unsigned char flag = 0;
	generic_file *ptr = nullptr;

	pdesc.check(small);
	if(small)
	    ptr = pdesc.esc;
	else
	    ptr = pdesc.stack;

	    // setting up the flag field

        switch(ea_saved)
        {
        case ea_saved_status::none:
            flag |= INODE_FLAG_EA_NONE;
            break;
        case ea_saved_status::partial:
            flag |= INODE_FLAG_EA_PART;
            break;
        case ea_saved_status::fake:
            flag |= INODE_FLAG_EA_FAKE;
            break;
        case ea_saved_status::full:
            flag |= INODE_FLAG_EA_FULL;
            break;
        case ea_saved_status::removed:
            flag |= INODE_FLAG_EA_REMO;
            break;
        default:
            throw SRC_BUG; // unknown value for ea_saved
        }

	switch(fsa_saved)
	{
	case fsa_saved_status::none:
	    flag |= INODE_FLAG_FSA_NONE;
	    break;
	case fsa_saved_status::partial:
	    flag |= INODE_FLAG_FSA_PART;
	    break;
	case fsa_saved_status::full:
	    flag |= INODE_FLAG_FSA_FULL;
	    break;
	default:
	    throw SRC_BUG; // unknown value for fsa_saved
	}

	    // saving parent class data

        cat_nomme::inherited_dump(pdesc, small);

	    // saving unix inode specific part

        ptr->write((char *)(&flag), 1);
        uid.dump(*ptr);
        gid.dump(*ptr);
        tmp = htons(perm);
        ptr->write((char *)&tmp, sizeof(tmp));
        last_acc.dump(*ptr);
        last_mod.dump(*ptr);
        last_cha.dump(*ptr);

	    // EA part

        if(ea_saved == ea_saved_status::full)
            ea_get_size().dump(*ptr);

        if(!small)
        {
            switch(ea_saved)
            {
            case ea_saved_status::full:
		if(ea_offset == nullptr)
		    throw SRC_BUG;
                ea_offset->dump(*ptr);
                if(ea_crc == nullptr)
                    throw SRC_BUG;
                ea_crc->dump(*ptr);
                break;
            case ea_saved_status::partial:
            case ea_saved_status::fake:
            case ea_saved_status::none:
            case ea_saved_status::removed:
                break;
            default:
                throw SRC_BUG;
            }
        }

	    // FSA part

	if(fsa_saved != fsa_saved_status::none)
	{
	    if(fsa_families == nullptr)
		throw SRC_BUG;
	    fsa_families->dump(*ptr);
	}
	if(fsa_saved == fsa_saved_status::full)
	{
	    if(fsa_size == nullptr)
		throw SRC_BUG;
	    fsa_size->dump(*ptr);
	}

	if(!small)
	{
	    switch(fsa_saved)
	    {
	    case fsa_saved_status::full:
		if(fsa_offset == nullptr)
		    throw SRC_BUG;
		fsa_offset->dump(*ptr);
		if(fsa_crc == nullptr)
		    throw SRC_BUG;
		fsa_crc->dump(*ptr);
		break;
	    case fsa_saved_status::partial:
	    case fsa_saved_status::none:
		break;
	    default:
		throw SRC_BUG;
	    }
	}
    }

    void cat_inode::ea_set_saved_status(ea_saved_status status)
    {
        if(status == ea_saved)
            return;
        switch(status)
        {
        case ea_saved_status::none:
        case ea_saved_status::removed:
        case ea_saved_status::partial:
        case ea_saved_status::fake:
            if(ea != nullptr)
            {
                delete ea;
                ea = nullptr;
            }
	    if(ea_offset != nullptr)
	    {
		delete ea_offset;
		ea_offset = nullptr;
	    }
            break;
        case ea_saved_status::full:
            if(ea != nullptr)
                throw SRC_BUG;
	    if(ea_offset != nullptr)
		throw SRC_BUG;
            break;
        default:
            throw SRC_BUG;
        }
        ea_saved = status;
    }

    void cat_inode::ea_attach(ea_attributs *ref)
    {
        if(ea_saved != ea_saved_status::full)
            throw SRC_BUG;

        if(ref != nullptr && ea == nullptr)
        {
	    if(ea_size != nullptr)
	    {
		delete ea_size;
		ea_size = nullptr;
	    }
            ea_size = new (nothrow) infinint(ref->space_used());
	    if(ea_size == nullptr)
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
        case ea_saved_status::full:
            if(ea != nullptr)
                return ea;
            else
                if(get_pile() != nullptr) // reading from archive
                {
		    crc *val = nullptr;
		    const crc *my_crc = nullptr;

		    try
		    {
			if(!small_read) // direct read mode
			{
			    if(ea_offset == nullptr)
				throw SRC_BUG;
			    get_pile()->flush_read_above(get_compressor_layer());
			    get_compressor_layer()->resume_compression();
			    get_pile()->skip(*ea_offset);
			}
			else // sequential read mode
			{
			    if(get_escape_layer() == nullptr)
				throw SRC_BUG;

				// passing over the delta signature mark if any
			    (void)get_escape_layer()->skip_to_next_mark(escape::seqt_delta_sig, false);

				// warning this section calls *esc directly while it may be managed by another thread
				// we are reading from the stack the possible thread is not in read_ahead operation
				// so it is pending for read request or other orders
			    if(!get_escape_layer()->skip_to_next_mark(escape::seqt_ea, false))
				throw Erange("cat_inode::get_ea", string("Error while fetching EA from archive: No escape mark found for that file"));
				// resuming compression (EA are always stored compressed)
			    get_pile()->flush_read_above(get_compressor_layer());
			    get_compressor_layer()->resume_compression();

				// we shall reset layers above esc for they do not assume nothing has changed below them
			    get_pile()->flush_read_above(get_escape_layer());
				// now we can continue normally using get_pile()

			    const_cast<cat_inode *>(this)->ea_set_offset(get_pile()->get_position());
			}

			if(ea_get_size().is_zero())
			    get_pile()->reset_crc(crc::OLD_CRC_SIZE);
			else
			{
			    get_pile()->reset_crc(tools_file_size_to_crc_size(ea_get_size()));
			    get_pile()->read_ahead(ea_get_size());
			}

			try
			{
			    try
			    {
				if(edit <= 1)
				    throw SRC_BUG;   // EA do not exist in that archive format
				const_cast<ea_attributs *&>(ea) = new (nothrow) ea_attributs(*get_pile(), edit);
				if(ea == nullptr)
				    throw Ememory("cat_inode::get_ea");
				get_compressor_layer()->suspend_compression();
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
			    val = get_pile()->get_crc(); // keeps storage in coherent status
			    throw;
			}
			val = get_pile()->get_crc();
			if(val == nullptr)
			    throw SRC_BUG;

			ea_get_crc(my_crc); // ea_get_crc() will eventually fetch the CRC for EA from the archive (sequential reading)
			if(my_crc == nullptr)
			    throw SRC_BUG;

			if(typeid(*val) != typeid(*my_crc) || *val != *my_crc)
			    throw Erange("cat_inode::get_ea", gettext("CRC error detected while reading EA"));
		    }
		    catch(...)
		    {
			if(val != nullptr)
			    delete val;
			throw;
		    }
		    if(val != nullptr)
			delete val;
                    return ea;
                }
                else
                    throw SRC_BUG;
                // no need of break here
            throw SRC_BUG; // but ... instead of break we use some more radical precaution.
        case ea_saved_status::removed:
            return &empty_ea;
                // no need of break here
        default:
            throw SRC_BUG;
        }
    }

    void cat_inode::ea_detach() const
    {
        if(ea != nullptr)
        {
            delete ea;
            const_cast<ea_attributs *&>(ea) = nullptr;
        }
    }

    infinint cat_inode::ea_get_size() const
    {
        if(ea_saved == ea_saved_status::full)
        {
            if(ea_size == nullptr) // reading an old archive
            {
                if(ea != nullptr)
		{
                    const_cast<cat_inode *>(this)->ea_size = new (nothrow) infinint (ea->space_used());
		    if(ea_size == nullptr)
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
	if(ea_offset == nullptr)
	{
	    ea_offset = new (nothrow) infinint(pos);
	    if(ea_offset == nullptr)
		throw Ememory("cat_inode::ea_set_offset");
	}
	else
	    *ea_offset = pos;
    }

    bool cat_inode::ea_get_offset(infinint & val) const
    {
	if(ea_offset != nullptr)
	{
	    val = *ea_offset;
	    return true;
	}
	else
	    return false;
    }

    void cat_inode::ea_set_crc(const crc & val)
    {
	if(ea_crc != nullptr)
	{
	    delete ea_crc;
	    ea_crc = nullptr;
	}
	ea_crc = val.clone();
	if(ea_crc == nullptr)
	    throw Ememory("cat_inode::ea_set_crc");
    }

    void cat_inode::ea_get_crc(const crc * & ptr) const
    {
	if(ea_get_saved_status() != ea_saved_status::full)
	    throw SRC_BUG;

        if(small_read && ea_crc == nullptr)
        {
	    if(get_escape_layer() == nullptr)
		throw SRC_BUG;

            if(get_escape_layer()->skip_to_next_mark(escape::seqt_ea_crc, false))
            {
                crc *tmp = nullptr;

                try
                {
                    if(edit >= 8)
                        tmp = create_crc_from_file(*get_escape_layer(), false);
                    else // archive format <= 7
                        tmp = create_crc_from_file(*get_escape_layer(), true);
		    if(tmp == nullptr)
			throw SRC_BUG;
                    const_cast<cat_inode *>(this)->ea_crc = tmp;
		    tmp = nullptr; // the object is now owned by "this"
                }
                catch(...)
                {
		    get_pile()->flush_read_above(get_escape_layer());
		    if(tmp != nullptr)
			delete tmp;
                    throw;
                }
		get_pile()->flush_read_above(get_escape_layer());
            }
            else // skip failed on the escape layer
            {
                crc *tmp = new (nothrow) crc_n(1); // creating a default CRC
                if(tmp == nullptr)
                    throw Ememory("cat_inode::ea_get_crc");

		get_pile()->flush_read_above(get_escape_layer());
                try
                {
                    tmp->clear();
                    const_cast<cat_inode *>(this)->ea_crc = tmp;
                        // this is to avoid trying to fetch the CRC a new time if decision
                        // has been taken to continue the operation after the exception
                        // thrown below has been caught.
		    tmp = nullptr;  // the object is now owned by "this"
                }
                catch(...)
                {
                    delete tmp;
                    throw;
                }
                throw Erange("cat_inode::ea_get_crc", gettext("Error while reading CRC for EA from the archive: No escape mark found for that file"));
            }
        }

        if(ea_crc == nullptr)
            throw SRC_BUG;
        else
            ptr = ea_crc;
    }

    bool cat_inode::ea_get_crc_size(infinint & val) const
    {
        if(ea_crc != nullptr)
        {
            val = ea_crc->get_size();
            return true;
        }
        else
            return false;
    }

    void cat_inode::fsa_set_saved_status(fsa_saved_status status)
    {
	if(status == fsa_saved)
	    return;
	switch(status)
	{
	case fsa_saved_status::none:
	case fsa_saved_status::partial:
	    if(fsal != nullptr)
	    {
		delete fsal;
		fsal = nullptr;
	    }
	    if(fsa_offset != nullptr)
	    {
		delete fsa_offset;
		fsa_offset = nullptr;
	    }
	    break;
	case fsa_saved_status::full:
	    if(fsal != nullptr)
		throw SRC_BUG;
	    if(fsa_offset != nullptr)
		throw SRC_BUG;
	    break;
	default:
	    throw SRC_BUG;
	}

	fsa_saved = status;
    }

    void cat_inode::fsa_partial_attach(const fsa_scope & val)
    {
	if(fsa_saved != fsa_saved_status::partial)
	    throw SRC_BUG;

	if(fsa_families == nullptr)
	    fsa_families = new(nothrow) infinint(fsa_scope_to_infinint(val));
	else
	    *fsa_families = fsa_scope_to_infinint(val);
    }

    void cat_inode::fsa_attach(filesystem_specific_attribute_list *ref)
    {
        if(fsa_saved != fsa_saved_status::full)
            throw SRC_BUG;

        if(ref != nullptr && fsal == nullptr)
        {
	    if(fsa_size != nullptr)
	    {
		delete fsa_size;
		fsa_size = nullptr;
	    }
	    if(fsa_families != nullptr)
	    {
		delete fsa_families;
		fsa_families = nullptr;
	    }
	    try
	    {
		fsa_size = new (nothrow) infinint (ref->storage_size());
		fsa_families = new(nothrow) infinint(fsa_scope_to_infinint(ref->get_fsa_families()));
		if(fsa_size == nullptr || fsa_families == nullptr)
		    throw Ememory("cat_inode::fsa_attach");
	    }
	    catch(...)
	    {
		if(fsa_size != nullptr)
		{
		    delete fsa_size;
		    fsa_size = nullptr;
		}
		if(fsa_families != nullptr)
		{
		    delete fsa_families;
		    fsa_families = nullptr;
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
        if(fsal != nullptr)
        {
            delete fsal;
            const_cast<cat_inode *>(this)->fsal = nullptr;
        }
    }

    const filesystem_specific_attribute_list *cat_inode::get_fsa() const
    {
        switch(fsa_saved)
        {
        case fsa_saved_status::full:
            if(fsal != nullptr)
                return fsal;
            else
                if(get_pile() != nullptr)  // reading from archive
                {
		    crc *val = nullptr;
		    const crc *my_crc = nullptr;

		    try
		    {
			generic_file *reader = nullptr;

			if(get_escape_layer() == nullptr)
			    reader = get_compressor_layer();
			else
			    reader = get_escape_layer();
			if(reader == nullptr)
			    throw SRC_BUG;

			    // we shall reset layers above reader
			get_pile()->flush_read_above(reader);

			if(!small_read) // direct reading mode
			{
			    if(fsa_offset == nullptr)
				throw SRC_BUG;
			    reader->skip(*fsa_offset);
			}
			else
			{
			    if(get_escape_layer() == nullptr)
				throw SRC_BUG;

				// passing over the delta signature mark if any
			    (void)get_escape_layer()->skip_to_next_mark(escape::seqt_delta_sig, false);

				// warning this section calls *get_escape_layer() directly while it may be managed by another thread
				// we are reading from the get_pile() the possible thread is not in read_ahead operation
				// so it is pending for read request or other orders
			    if(!get_escape_layer()->skip_to_next_mark(escape::seqt_fsa, false))
				throw Erange("cat_inode::get_fsa", string("Error while fetching FSA from archive: No escape mark found for that file"));
			    const_cast<cat_inode *>(this)->fsa_set_offset(get_escape_layer()->get_position());
			}

			if(get_escape_layer() == nullptr)
			{
				// FSA is never stored compressed, we must change the compression algo
				// but only if necessary
			    if(get_compressor_layer()->get_algo() != compression::none)
				get_compressor_layer()->suspend_compression();
			}

			reader->reset_crc(tools_file_size_to_crc_size(fsa_get_size()));

			try
			{
			    try
			    {
				const_cast<cat_inode *>(this)->fsal = new (nothrow) filesystem_specific_attribute_list();
				if(fsal == nullptr)
				    throw Ememory("cat_inode::get_fsa");
				try
				{
				    reader->read_ahead(fsa_get_size());
				    const_cast<cat_inode *>(this)->fsal->read(*reader, edit);
				}
				catch(...)
				{
				    delete fsal;
				    const_cast<cat_inode *>(this)->fsal = nullptr;
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
			    val = reader->get_crc(); // keeps storage in coherent status
			    throw;
			}

			val = reader->get_crc();
			if(val == nullptr)
			    throw SRC_BUG;

			fsa_get_crc(my_crc); // fsa_get_crc() will eventually fetch the CRC for FSA from the archive (sequential reading)
			if(my_crc == nullptr)
			    throw SRC_BUG;

			if(typeid(*val) != typeid(*my_crc) || *val != *my_crc)
			    throw Erange("cat_inode::get_fsa", gettext("CRC error detected while reading FSA"));
		    }
		    catch(...)
		    {
			if(val != nullptr)
			    delete val;
			throw;
		    }
		    if(val != nullptr)
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
        if(fsa_saved == fsa_saved_status::full)
	    if(fsa_size != nullptr)
		return *fsa_size;
	    else
		throw SRC_BUG;
        else
            throw SRC_BUG;
    }

    void cat_inode::fsa_set_offset(const infinint & pos)
    {
	if(fsa_offset == nullptr)
	{
	    fsa_offset = new (nothrow) infinint(pos);
	    if(fsa_offset == nullptr)
		throw Ememory("cat_inode::fsa_set_offset");
	}
	else
	    *fsa_offset = pos;
    }

    bool cat_inode::fsa_get_offset(infinint & pos) const
    {
	if(fsa_offset != nullptr)
	{
	    pos = *fsa_offset;
	    return true;
	}
	else
	    return false;
    }


    void cat_inode::fsa_set_crc(const crc & val)
    {
	if(fsa_crc != nullptr)
	{
	    delete fsa_crc;
	    fsa_crc = nullptr;
	}
	fsa_crc = val.clone();
	if(fsa_crc == nullptr)
	    throw Ememory("cat_inode::fsa_set_crc");
    }

    void cat_inode::fsa_get_crc(const crc * & ptr) const
    {
	if(fsa_get_saved_status() != fsa_saved_status::full)
	    throw SRC_BUG;

        if(small_read && fsa_crc == nullptr)
        {
	    if(get_escape_layer() == nullptr)
		throw SRC_BUG;

	    if(get_pile() == nullptr)
		throw SRC_BUG;

            if(get_escape_layer()->skip_to_next_mark(escape::seqt_fsa_crc, false))
            {
                crc *tmp = nullptr;

                try
                {
		    tmp = create_crc_from_file(*get_escape_layer(), false);
		    if(tmp == nullptr)
			throw SRC_BUG;
                    const_cast<cat_inode *>(this)->fsa_crc = tmp;
		    tmp = nullptr; // the object is now owned by "this"
                }
                catch(...)
                {
		    get_pile()->flush_read_above(get_escape_layer());
		    if(tmp != nullptr)
			delete tmp;
                    throw;
                }

		get_pile()->flush_read_above(get_escape_layer());
            }
            else // fsa_crc mark not found
            {
                crc *tmp = new (nothrow) crc_n(1); // creating a default CRC
                if(tmp == nullptr)
                    throw Ememory("cat_inode::fsa_get_crc");

		get_pile()->flush_read_above(get_escape_layer());
                try
                {
                    tmp->clear();
                    const_cast<cat_inode *>(this)->fsa_crc = tmp;
                        // this is to avoid trying to fetch the CRC a new time if decision
                        // has been taken to continue the operation after the exception
                        // thrown below has been caught.
		    tmp = nullptr;  // the object is now owned by "this"
                }
                catch(...)
                {
                    delete tmp;
                    throw;
                }
                throw Erange("cat_inode::fsa_get_crc", gettext("Error while reading CRC for FSA from the archive: No escape mark found for that file"));
            }
        }

        if(fsa_crc == nullptr)
            throw SRC_BUG;
        else
            ptr = fsa_crc;
    }

    bool cat_inode::fsa_get_crc_size(infinint & val) const
    {
        if(fsa_crc != nullptr)
        {
            val = fsa_crc->get_size();
            return true;
        }
        else
            return false;
    }

    void cat_inode::nullifyptr() noexcept
    {
        ea_offset = nullptr;
	ea = nullptr;
	ea_size = nullptr;
        ea_crc = nullptr;
	fsa_families = nullptr;
	fsa_offset = nullptr;
	fsal = nullptr;
	fsa_size = nullptr;
	fsa_crc = nullptr;
        fs_dev = nullptr;
    }

    void cat_inode::destroy() noexcept
    {
        if(ea_offset != nullptr)
        {
            delete ea_offset;
            ea_offset = nullptr;
        }
        if(ea != nullptr)
        {
            delete ea;
            ea = nullptr;
        }
	if(ea_size != nullptr)
	{
	    delete ea_size;
	    ea_size = nullptr;
	}
        if(ea_crc != nullptr)
        {
            delete ea_crc;
            ea_crc = nullptr;
        }
	if(fsa_families != nullptr)
	{
	    delete fsa_families;
	    fsa_families = nullptr;
	}
	if(fsa_offset != nullptr)
	{
	    delete fsa_offset;
	    fsa_offset = nullptr;
	}
	if(fsal != nullptr)
	{
	    delete fsal;
	    fsal = nullptr;
	}
	if(fsa_size != nullptr)
	{
	    delete fsa_size;
	    fsa_size = nullptr;
	}
	if(fsa_crc != nullptr)
	{
	    delete fsa_crc;
	    fsa_crc = nullptr;
	}
        if(fs_dev != nullptr)
        {
            delete fs_dev;
            fs_dev = nullptr;
        }
    }

    template <class T> void copy_ptr(const T *src, T * & dst)
    {
	if(src == nullptr)
	    dst = nullptr;
	else
	{
	    dst = new (nothrow) T(*src);
	    if(dst == nullptr)
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
	    last_cha = ref.last_cha;
	    ea_saved = ref.ea_saved;
	    fsa_saved = ref.fsa_saved;
	    small_read = ref.small_read;
	    copy_ptr(ref.ea_offset, ea_offset);
	    copy_ptr(ref.ea, ea);
	    copy_ptr(ref.ea_size, ea_size);
	    if(ref.ea_crc != nullptr)
	    {
		ea_crc = (ref.ea_crc)->clone();
		if(ea_crc == nullptr)
		    throw Ememory("cat_inode::copy_from");
	    }
	    else
		ea_crc = nullptr;
	    copy_ptr(ref.fsa_families, fsa_families);
	    copy_ptr(ref.fsa_offset, fsa_offset);
	    copy_ptr(ref.fsal, fsal);
	    copy_ptr(ref.fsa_size, fsa_size);
	    if(ref.fsa_crc != nullptr)
	    {
		fsa_crc = (ref.fsa_crc)->clone();
		if(fsa_crc == nullptr)
		    throw Ememory("cat_inode::copy_from");
	    }
	    else
		fsa_crc = nullptr;
	    copy_ptr(ref.fs_dev, fs_dev);
	    edit = ref.edit;
	}
	catch(...)
	{
	    destroy();
	    throw;
	}
    }

    void cat_inode::move_from(cat_inode && ref) noexcept
    {
	uid = std::move(ref.uid);
	gid = std::move(ref.gid);
	perm = std::move(ref.perm);
	last_acc = std::move(ref.last_acc);
	last_mod = std::move(ref.last_mod);
	last_cha = std::move(ref.last_cha);
	ea_saved = std::move(ref.ea_saved);
	fsa_saved = std::move(ref.fsa_saved);
	small_read = std::move(ref.small_read);
	swap(ref.ea_offset, ea_offset);
	swap(ref.ea, ea);
	swap(ref.ea_size, ea_size);
	swap(ref.ea_crc, ea_crc);
	swap(ref.fsa_families, fsa_families);
	swap(ref.fsa_offset, fsa_offset);
	swap(ref.fsal, fsal);
	swap(ref.fsa_size, fsa_size);
	swap(ref.fsa_crc, fsa_crc);
	swap(ref.fs_dev, fs_dev);
	edit = std::move(ref.edit);
    }


} // end of namespace
