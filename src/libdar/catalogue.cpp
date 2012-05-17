//*********************************************************************/
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
// $Id: catalogue.cpp,v 1.47.2.7 2007/07/27 11:27:31 edrusb Rel $
//
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
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#if HAVE_LIMITS_H
#include <limits.h>
#endif
} // end extern "C"

#include <algorithm>
#include <map>
#include "catalogue.hpp"
#include "tools.hpp"
#include "tronc.hpp"
#include "user_interaction.hpp"
#include "deci.hpp"
#include "header.hpp"
#include "cache.hpp"

#define INODE_FLAG_EA_MASK  0x07
#define INODE_FLAG_EA_FULL  0x01
#define INODE_FLAG_EA_PART  0x02
#define INODE_FLAG_EA_NONE  0x03
#define INODE_FLAG_EA_FAKE  0x04

#define REMOVE_TAG gettext("[     REMOVED       ]")

#define SAVED_FAKE_BIT 0x80

#define BUFFER_SIZE 102400
#ifdef SSIZE_MAX
#if SSIZE_MAX < BUFFER_SIZE
#undef BUFFER_SIZE
#define BUFFER_SIZE SSIZE_MAX
#endif
#endif

using namespace std;

namespace libdar
{
    static string local_perm(const inode & ref);
    static string local_uid(const inode & ref);
    static string local_gid(const inode & ref);
    static string local_size(const inode & ref);
    static string local_storage_size(const inode & ref);
    static string local_date(const inode & ref);
    static string local_flag(const inode & ref);
    static void xml_listing_attributes(user_interaction & dialog,      //< for user interaction
				       const string & beginning,       //< character string to use as margin
				       const string & data,            //< ("saved" | "referenced" | "deleted")
				       const string & metadata,        //< ("saved" | "referenced" | "absent")
				       const string & user = "",       //< user name (empty string if not available)
				       const string & group = "",      //< group name (empty string if not available)
				       const string & permissions = "",//< permission (empty string if not available)
				       const string & atime = "",      //< last access time (empty string if not available)
				       const string & mtime = "",      //< last modification time (empty string if not available)
				       const string & ctime = "");     //< last inode change time (empty string if not available)
    static bool extract_base_and_status(unsigned char signature, unsigned char & base, saved_status & saved);

    unsigned char mk_signature(unsigned char base, saved_status state)
    {
        if(! islower(base))
            throw SRC_BUG;
        switch(state)
        {
        case s_saved:
            return base;
        case s_fake:
            return base | SAVED_FAKE_BIT;
        case s_not_saved:
            return toupper(base);
        default:
            throw SRC_BUG;
        }
    }

    void unmk_signature(unsigned char sig, unsigned char & base, saved_status & state)
    {
	if(sig & SAVED_FAKE_BIT == 0)
	    if(islower(sig))
		state = s_saved;
	    else
		state = s_not_saved;
	else
	    state = s_fake;

	base = tolower(sig & ~SAVED_FAKE_BIT);
    }

    void entree_stats::add(const entree *ref)
    {
        if(dynamic_cast<const eod *>(ref) == NULL) // we ignore eod
        {
            const inode *ino = dynamic_cast<const inode *>(ref);
            const hard_link *h = dynamic_cast<const hard_link *>(ref);
            const detruit *x = dynamic_cast<const detruit *>(ref);

            if(ino != NULL && h == NULL) // won't count twice the same inode if it is referenced with hard_link
            {
                if(ino->get_saved_status() == s_saved)
                    saved++;
                total++;
            }

            if(x != NULL)
                num_x++;
            else
            {
                const directory *d = dynamic_cast<const directory*>(ref);
                if(d != NULL)
                    num_d++;
                else
                {
                    const chardev *c = dynamic_cast<const chardev *>(ref);
                    if(c != NULL)
                        num_c++;
                    else
                    {
                        const blockdev *b = dynamic_cast<const blockdev *>(ref);
                        if(b != NULL)
                            num_b++;
                        else
                        {
                            const tube *p = dynamic_cast<const tube *>(ref);
                            if(p != NULL)
                                num_p++;
                            else
                            {
                                const prise *s = dynamic_cast<const prise *>(ref);
                                if(s != NULL)
                                    num_s++;
                                else
                                {
                                    const lien *l = dynamic_cast<const lien *>(ref);
                                    if(l != NULL)
                                        num_l++;
                                    else
                                    {
                                        const file *f = dynamic_cast<const file *>(ref);
                                        const file_etiquette *e = dynamic_cast<const file_etiquette *>(ref);

                                        if(f != NULL)
                                            num_f++;
                                            // no else, because a "file_etiquette" is also a "file"
                                        if(e != NULL)
                                            num_hard_linked_inodes++;
                                        if(h != NULL)
                                            num_hard_link_entries++;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    void entree_stats::listing(user_interaction & dialog) const
    {
        dialog.printf(gettext("\nCATALOGUE CONTENTS :\n\n"));
        dialog.printf(gettext("total number of inode : %i\n"), &total);
        dialog.printf(gettext("saved inode           : %i\n"), &saved);
        dialog.printf(gettext("distribution of inodes\n"));
        dialog.printf(gettext(" - directories        : %i\n"), &num_d);
        dialog.printf(gettext(" - plain files        : %i\n"), &num_f);
        dialog.printf(gettext(" - symbolic links     : %i\n"), &num_l);
        dialog.printf(gettext(" - named pipes        : %i\n"), &num_p);
        dialog.printf(gettext(" - unix sockets       : %i\n"), &num_s);
        dialog.printf(gettext(" - character devices  : %i\n"), &num_c);
        dialog.printf(gettext(" - block devices      : %i\n"), &num_b);
        dialog.printf(gettext("hard links informations\n"));
        dialog.printf(gettext("   number of file inode with hard links : %i\n"), &num_hard_linked_inodes);
        dialog.printf(gettext("   total number of hard links           : %i\n"), &num_hard_link_entries);
        dialog.printf(gettext("destroyed entries informations\n"));
        dialog.printf(gettext("   %i file(s) have been record as destroyed since backup of reference\n\n"), &num_x);
    }

    entree *entree::read(user_interaction & dialog,
			 generic_file & f,
			 const dar_version & reading_ver,
			 entree_stats & stats,
			 std::map <infinint, file_etiquette *> & corres,
			 compression default_algo,
			 generic_file *data_loc,
			 generic_file *ea_loc)
    {
        char type;
        saved_status saved;
        entree *ret = NULL;
        map <infinint, file_etiquette *>::iterator it;
        infinint tmp;
        hard_link *ptr_l = NULL;
        file_etiquette *ptr_e = NULL;

        S_I lu = f.read(&type, 1);

        if(lu == 0)
            return ret;

        if(!extract_base_and_status((unsigned char)type, (unsigned char &)type, saved))
            throw Erange("entree::read", gettext("corrupted file"));

        switch(type)
        {
        case 'f':
            ret = new file(dialog, f, reading_ver, saved, default_algo, data_loc, ea_loc);
            break;
        case 'l':
            ret = new lien(dialog, f, reading_ver, saved, ea_loc);
            break;
        case 'c':
            ret = new chardev(dialog, f, reading_ver, saved, ea_loc);
            break;
        case 'b':
            ret = new blockdev(dialog, f, reading_ver, saved, ea_loc);
            break;
        case 'p':
            ret = new tube(dialog, f, reading_ver, saved, ea_loc);
            break;
        case 's':
            ret = new prise(dialog, f, reading_ver, saved, ea_loc);
            break;
        case 'd':
            ret = new directory(dialog, f, reading_ver, saved, stats, corres, default_algo, data_loc, ea_loc);
            break;
        case 'z':
            if(saved != s_saved)
                throw Erange("entree::read", gettext("corrupted file"));
            ret = new eod(f);
            break;
        case 'x':
            if(saved != s_saved)
                throw Erange("entree::read", gettext("corrupted file"));
            ret = new detruit(f);
            break;
        case 'h':
            ret = ptr_l = new hard_link(f, tmp);
            if(ptr_l == NULL)
                throw Ememory("entree::read");
            it = corres.find(tmp);
            if(it != corres.end())
                ptr_l->set_reference(it->second);
            else
            {
                delete ptr_l;
                throw Erange("entree::read", gettext("corrupted file"));
            }
            break;
        case 'e':
            ret = ptr_e = new file_etiquette(dialog, f, reading_ver, saved, default_algo, data_loc, ea_loc);
            if(ret == NULL)
                throw Ememory("entree::read");
            if(corres.find(ptr_e->get_etiquette()) != corres.end())
                throw SRC_BUG;
            corres[ptr_e->get_etiquette()] = ptr_e;
            break;
        default :
            throw Erange("entree::read", gettext("unknown type of data in catalogue"));
        }

        stats.add(ret);
        return ret;
    }

    void entree::dump(user_interaction & dialog, generic_file & f) const
    {
        char s = signature();
        f.write(&s, 1);
    }

    bool compatible_signature(unsigned char a, unsigned char b)
    {
        a = tolower(a & ~SAVED_FAKE_BIT);
        b = tolower(b & ~SAVED_FAKE_BIT);

        switch(a)
        {
        case 'e':
        case 'f':
            return b == 'e' || b == 'f';
        default:
            return b == a;
        }
    }

    nomme::nomme(generic_file & f)
    {
        tools_read_string(f, xname);
    }

    void nomme::dump(user_interaction & dialog, generic_file & f) const
    {
        entree::dump(dialog, f);
        tools_write_string(f, xname);
    }

    inode::inode(U_16 xuid, U_16 xgid, U_16 xperm,
                 const infinint & last_access,
                 const infinint & last_modif,
                 const string & xname,
		 const infinint & fs_device) : nomme(xname)
    {
        uid = xuid;
        gid = xgid;
        perm = xperm;
        xsaved = s_not_saved;
        ea_saved = ea_none;
        ea_offset = NULL;
        ea = NULL;
        clear(ea_crc);
        last_acc = NULL;
        last_mod = NULL;
        ea_offset = NULL;
        last_cha = NULL;
	fs_dev = NULL;
	storage = NULL;
	version_copy(edit, "00");

        try
        {
            last_acc = new infinint(last_access);
            last_mod = new infinint(last_modif);
            ea_offset = new infinint(0);
            last_cha = new infinint(0);
            if(last_acc == NULL || last_mod == NULL || ea_offset == NULL || last_cha == NULL)
                throw Ememory("inde::inode");
	    fs_dev = new infinint(fs_device);
        }
        catch(...)
        {
            if(last_acc != NULL)
                delete last_acc;
            if(last_mod != NULL)
                delete last_mod;
            if(ea_offset != NULL)
                delete ea_offset;
            if(last_cha != NULL)
                delete last_cha;
	    if(fs_dev != NULL)
		delete fs_dev;
            throw;
        }
    }

    inode::inode(user_interaction & dialog,
		 generic_file & f,
		 const dar_version & reading_ver,
		 saved_status saved,
		 generic_file *ea_loc) : nomme(f)
    {
        U_16 tmp;
        unsigned char flag;

        xsaved = saved;
	version_copy(edit, reading_ver);

        if(version_greater(reading_ver, "01"))
        {
            f.read((char *)(&flag), 1);
            flag &= INODE_FLAG_EA_MASK;
            switch(flag)
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
            default:
                throw Erange("inode::inode", gettext("badly structured inode: unknown inode flag"));
            }
        }
        else
            ea_saved = ea_none;

        if(f.read((char *)&tmp, sizeof(tmp)) != sizeof(tmp))
            throw Erange("inode::inode", gettext("missing data to build an inode"));
        uid = ntohs(tmp);
        if(f.read((char *)&tmp, sizeof(tmp)) != sizeof(tmp))
            throw Erange("inode::inode", gettext("missing data to build an inode"));
        gid = ntohs(tmp);
        if(f.read((char *)&tmp, sizeof(tmp)) != sizeof(tmp))
            throw Erange("inode::inode", gettext("missing data to build an inode"));
        perm = ntohs(tmp);

        last_acc = NULL;
        last_mod = NULL;
        last_cha = NULL;
        ea_offset = NULL;
	fs_dev = NULL;
        try
        {
	    fs_dev = new infinint(0); // the filesystemID is not saved in archive
            last_acc = new infinint(dialog, NULL, &f);
            last_mod = new infinint(dialog, NULL, &f);
            if(last_acc == NULL || last_mod == NULL)
                throw Ememory("inode::inode(file)");

            switch(ea_saved)
            {
            case ea_full:
                ea_offset = new infinint(dialog, NULL, &f);
                if(ea_offset == NULL)
                    throw Ememory("inode::inode(file)");
                f.read((char *)ea_crc, CRC_SIZE);
                last_cha = new infinint(dialog, NULL, &f);
                if(last_cha == NULL)
                    throw Ememory("inode::inode(file)");
                break;
            case ea_partial:
	    case ea_fake:
                ea_offset = new infinint(0);
                if(ea_offset == NULL)
                    throw Ememory("inode::inode(file)");
                clear(ea_crc);
                last_cha = new infinint(dialog, NULL, &f);
                if(last_cha == NULL)
                    throw Ememory("inode::inode(file)");
                break;
            case ea_none:
                ea_offset = new infinint(0);
                if(ea_offset == NULL)
                    throw Ememory("inode::inode(file)");
                clear(ea_crc);
                last_cha = new infinint(0);
                if(last_cha == NULL)
                    throw Ememory("inode::inode(file)");
                break;
            default:
                throw SRC_BUG;
            }
            ea = NULL; // in any case

                // to be able later to read EA from archive file
	    if(ea_loc == NULL)
		throw SRC_BUG;
            storage = ea_loc;
        }
        catch(...)
        {
            if(last_acc != NULL)
                delete last_acc;
            if(last_mod != NULL)
                delete last_mod;
            if(last_cha != NULL)
                delete last_cha;
            if(ea_offset != NULL)
                delete ea_offset;
	    if(fs_dev != NULL)
		delete fs_dev;
	    throw;
        }
    }

    inode::inode(const inode & ref) : nomme(ref)
    {
        uid = ref.uid;
        gid = ref.gid;
        perm = ref.perm;
        xsaved = ref.xsaved;
        ea_saved = ref.ea_saved;
	storage = ref.storage;

        last_acc = NULL;
        last_mod = NULL;
        last_cha = NULL;
        ea_offset = NULL;
        ea = NULL;
	fs_dev = NULL;
        try
        {
	    version_copy(edit, ref.edit);
            last_acc = new infinint(*ref.last_acc);
            last_mod = new infinint(*ref.last_mod);
	    fs_dev = new infinint(*ref.fs_dev);
            if(last_acc == NULL || last_mod == NULL || fs_dev == NULL)
                throw Ememory("inode::inode(inode)");

            switch(ea_saved)
            {
            case ea_full:
                ea_offset = new infinint(*ref.ea_offset);
                if(ea_offset == NULL)
                    throw Ememory("inode::inode(inode)");
                copy_crc(ea_crc, ref.ea_crc);
                if(ref.ea != NULL) // might be NULL if build from a file
                {
                    ea = new ea_attributs(*ref.ea);
                    if(ea == NULL)
                        throw Ememory("inode::inode(const inode &)");
                }
                else
                    ea = NULL;
                last_cha = new infinint(*ref.last_cha);
                if(last_cha == NULL)
                    throw Ememory("inode::inode(inode)");
                break;
            case ea_partial:
	    case ea_fake:
                last_cha = new infinint(*ref.last_cha);
                if(last_cha == NULL)
                    throw Ememory("inode::inode(inode)");
                ea_offset = new infinint(0);
                if(ea_offset == NULL)
                    throw Ememory("inode::inode(inode)");
                ea = NULL;
                break;
            case ea_none:
                ea_offset = new infinint(0);
                last_cha = new infinint(0);
                if(ea_offset == NULL || last_cha == NULL)
                    throw Ememory("inode::inode(inode)");
                ea = NULL;
                break;
            default:
                throw SRC_BUG;
            }
        }
        catch(...)
        {
            if(last_acc != NULL)
                delete last_acc;
            if(last_mod != NULL)
                delete last_mod;
            if(ea != NULL)
                delete ea;
            if(ea_offset != NULL)
                delete ea_offset;
            if(last_cha != NULL)
                delete last_cha;
	    if(fs_dev != NULL)
		delete fs_dev;
            throw;
        }
    }

    inode::~inode()
    {
        if(last_acc != NULL)
            delete last_acc;
        if(last_mod != NULL)
            delete last_mod;
        if(ea != NULL)
            delete ea;
        if(ea_offset != NULL)
            delete ea_offset;
        if(last_cha != NULL)
            delete last_cha;
	if(fs_dev != NULL)
	    delete fs_dev;
    }

    bool inode::same_as(const inode & ref) const
    {
        return nomme::same_as(ref) && compatible_signature(ref.signature(), signature());
    }

    bool inode::is_more_recent_than(const inode & ref, const infinint & hourshift) const
    {
        return *ref.last_mod < *last_mod && !is_equal_with_hourshift(hourshift, *ref.last_mod, *last_mod);
    }

    bool inode::has_changed_since(const inode & ref, const infinint & hourshift, comparison_fields what_to_check) const
    {
        return (what_to_check != cf_inode_type && (hourshift > 0 ? ! is_equal_with_hourshift(hourshift, *ref.last_mod, *last_mod) : *ref.last_mod != *last_mod))
            || (what_to_check == cf_all && uid != ref.uid)
            || (what_to_check == cf_all && gid != ref.gid)
            || (what_to_check != cf_mtime && what_to_check != cf_inode_type && perm != ref.perm);
    }

    void inode::compare(user_interaction & dialog, const inode &other, const mask & ea_mask, comparison_fields what_to_check) const
    {
        if(!same_as(other))
            throw Erange("inode::compare",gettext("different file type"));
        if(what_to_check == cf_all && get_uid() != other.get_uid())
            throw Erange("inode.compare", gettext("different owner"));
        if(what_to_check == cf_all && get_gid() != other.get_gid())
            throw Erange("inode.compare", gettext("different owner group"));
        if((what_to_check == cf_all || what_to_check == cf_ignore_owner) && get_perm() != other.get_perm())
            throw Erange("inode.compare", gettext("different permission"));
        sub_compare(dialog, other);

	switch(ea_get_saved_status())
	{
	case ea_full:
	    if(other.ea_get_saved_status() == ea_full)
	    {
		const ea_attributs *me = get_ea(dialog); // this pointer must not be freed
		const ea_attributs *you = other.get_ea(dialog); // this pointer must not be freed too
		if(me->diff(*you, ea_mask))
		    throw Erange("inode::compare", gettext("different Extended Attributes"));
	    }
	    else
	    {
#ifdef EA_SUPPORT
		throw Erange("inode::compare", gettext("no Extended Attribute to compare with"));
#else
		throw Efeature(gettext("Cannot compare EA: EA support has not been activated at compilation time"));
#endif
	    }
		// else we ignore the EA present in the argument,
		// this is not a symetrical comparison
		// we check that all data in current object are the same in the argument
		// but additional data can reside in the argument
	    break;
	case ea_partial:
	case ea_fake:
	    if(other.ea_get_saved_status() != ea_none)
	    {
		if(get_last_change() < other.get_last_change())
		    throw Erange("inode::compare", gettext("inode last change date (ctime) greater, EA might be different"));
	    }
	    else
	    {
#ifdef EA_SUPPORT
		throw Erange("inode::compare", gettext("no Extended Attributes to compare with"));
#else
	        throw Efeature(gettext("Cannot compare EA: EA support has not been activated at compilation time"));
#endif
	    }
	    break;
	case ea_none:
	    break;
	default:
	    throw SRC_BUG;
	}
    }

    void inode::dump(user_interaction & dialog, generic_file & r) const
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
        default:
            throw SRC_BUG; // unknown value for ea_saved
        }
        nomme::dump(dialog, r);

        r.write((char *)(&flag), 1);
        tmp = htons(uid);
        r.write((char *)&tmp, sizeof(tmp));
        tmp = htons(gid);
        r.write((char *)&tmp, sizeof(tmp));
        tmp = htons(perm);
        r.write((char *)&tmp, sizeof(tmp));
        if(last_acc == NULL)
            throw SRC_BUG;
        last_acc->dump(r);
        if(last_mod == NULL)
            throw SRC_BUG;
        last_mod->dump(r);
        switch(ea_saved)
        {
        case ea_full:
            ea_offset->dump(r);
            r.write((char *)ea_crc, CRC_SIZE);
                // no break
        case ea_partial:
	case ea_fake:
            last_cha->dump(r);
            break;
        case ea_none:
            break;
        default:
            throw SRC_BUG;
        }
    }

    void inode::ea_set_saved_status(ea_status status)
    {
        if(status == ea_saved)
            return;
        switch(status)
        {
        case ea_none:
            if(ea != NULL)
            {
                delete ea;
                ea = NULL;
            }
            break;
        case ea_full:
            if(ea != NULL)
                throw SRC_BUG;
            *ea_offset = 0;
            *last_cha = 0;
            break;
        case ea_partial:
	case ea_fake:
            if(ea != NULL)
            {
                delete ea;
                ea = NULL;
            }
            break;
        default:
            throw SRC_BUG;
        }
        ea_saved = status;
    }

    void inode::ea_attach(ea_attributs *ref)
    {
        if(ref != NULL && ea == NULL)
            ea = ref;
        else
            throw SRC_BUG;
        if(ea_saved != ea_full)
            throw SRC_BUG;
    }

    const ea_attributs *inode::get_ea(user_interaction & dialog) const
    {
        if(ea_saved == ea_full)
            if(ea != NULL)
                return ea;
            else
                if(*ea_offset != 0 && storage != NULL)
                {
                    crc val;
                    storage->skip(*ea_offset);
                    storage->reset_crc();
                    try
                    {
			try
			{
			    if(edit[0] == '0' && edit[1] == '0')
				throw SRC_BUG;
			    const_cast<ea_attributs *&>(ea) = new ea_attributs(dialog, *storage, edit);
			    if(ea == NULL)
				throw Ememory("inode::get_ea");
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
			    throw Erange("inode::get_ea", string("Error while reading EA from archive: ") + e.get_message());
			}
                    }
                    catch(...)
                    {
                        storage->get_crc(val); // keeps storage in coherent status
                        throw;
                    }
                    storage->get_crc(val);
                    if(!same_crc(val, ea_crc))
                        throw Erange("inode::get_ea", gettext("CRC error detected while reading EA"));
                    return ea;
                }
                else
                    throw SRC_BUG;
        else
            throw SRC_BUG;
    }

    void inode::ea_detach() const
    {
        if(ea != NULL)
        {
            delete ea;
            const_cast<ea_attributs *&>(ea) = NULL;
        }
    }

    infinint inode::get_last_change() const
    {
        if(ea_saved != ea_none)
            return *last_cha;
        else
            throw SRC_BUG;
    }

    void inode::set_last_change(const infinint & x_time)
    {
        if(ea_saved != ea_none)
            *last_cha = x_time;
        else
            throw SRC_BUG;
    }

    file::file(U_16 xuid, U_16 xgid, U_16 xperm,
               const infinint & last_access,
               const infinint & last_modif,
               const string & src,
               const path & che,
               const infinint & taille,
	       const infinint & fs_device) : inode(xuid, xgid, xperm, last_access, last_modif, src, fs_device), chemin(che + src)
    {
        status = from_path;
        set_saved_status(s_saved);
        offset = NULL;
        size = NULL;
        storage_size = NULL;
	loc = NULL; // field not used for backup
	algo = none; // field not used for backup
        try
        {
            offset = new infinint(0);
            size = new infinint(taille);
            storage_size = new infinint(0);
            if(offset == NULL || size == NULL || storage_size == NULL)
                throw Ememory("file::file");
        }
        catch(...)
        {
            if(offset != NULL)
                delete offset;
            if(size != NULL)
                delete size;
            if(storage_size != NULL)
                delete storage_size;
            throw;
        }
    }

    file::file(user_interaction & dialog,
	       generic_file & f,
	       const dar_version & reading_ver,
	       saved_status saved,
	       compression default_algo,
	       generic_file *data_loc,
	       generic_file *ea_loc) : inode(dialog, f, reading_ver, saved, ea_loc), chemin("vide")
    {
        status = from_cat;
        size = NULL;
        offset = NULL;
        storage_size = NULL;
	algo = default_algo; // only used for archive format "03" and older
	loc = data_loc;
        try
        {
            size = new infinint(dialog, NULL, &f);
            if(size == NULL)
                throw Ememory("file::file(generic_file)");

            if(saved == s_saved)
            {
                offset = new infinint(dialog, NULL, &f);
                if(offset == NULL)
                    throw Ememory("file::file(generic_file)");
                if(version_greater(reading_ver, "01"))
                {
                    storage_size = new infinint(dialog, NULL, &f);
                    if(storage_size == NULL)
                        throw Ememory("file::file(generic_file)");
                }
                else // version is "01"
                {
                    storage_size = new infinint(*size);
                    if(storage_size == NULL)
                        throw Ememory("file::file(generic_file)");
                    *storage_size *= 2;
			// compressed file should be less than twice
			// larger than original file
			// (in case the compression is very bad
			// and takes more place than no compression !)
                }
            }
            else // not saved
            {
                offset = new infinint(0);
                storage_size = new infinint(0);
                if(offset == NULL || storage_size == NULL)
                    throw Ememory("file::file(generic_file)");
            }

            if(version_greater(reading_ver, "01"))
	    {
                if(f.read(check, CRC_SIZE) != CRC_SIZE)
                    throw Erange("file::file", gettext("can't read CRC data"));
		available_crc = true;
	    }
	    else
		available_crc = false;
        }
        catch(...)
        {
            detruit();
            throw;
        }
    }

    file::file(const file & ref) : inode(ref), chemin(ref.chemin)
    {
        status = ref.status;
	available_crc = ref.available_crc;
	loc = ref.loc;
	algo = ref.algo;
	if(available_crc)
	    copy_crc(check, ref.check);
        try
        {
            offset = new infinint(*ref.offset);
            size = new infinint(*ref.size);
            storage_size = new infinint(*ref.storage_size);
            if(offset == NULL || size == NULL || storage_size == NULL)
                throw Ememory("file::file(file)");
        }
        catch(...)
        {
            detruit();
            throw;
        }
    }

    void file::detruit()
    {
        if(offset != NULL)
            delete offset;
        if(size != NULL)
            delete size;
        if(storage_size != NULL)
            delete storage_size;
    }

    void file::dump(user_interaction & dialog, generic_file & f) const
    {
        inode::dump(dialog, f);
        size->dump(f);
        if(get_saved_status() == s_saved)
        {
            offset->dump(f);
            storage_size->dump(f);
        }
        if(f.write((char *)check, CRC_SIZE) != CRC_SIZE)
            throw Erange("file::dump", gettext("cannot dump CRC data to file"));
    }

    bool file::has_changed_since(const inode & ref, const infinint & hourshift, inode::comparison_fields what_to_check) const
    {
        const file *tmp = dynamic_cast<const file *>(&ref);
        if(tmp != NULL)
            return inode::has_changed_since(*tmp, hourshift, what_to_check) || *size != *(tmp->size);
        else
            throw SRC_BUG;
    }

    generic_file *file::get_data(user_interaction & dialog, bool keep_compressed) const
    {
        generic_file *ret;

        if(get_saved_status() != s_saved)
            throw Erange("file::get_data", gettext("cannot provide data from a \"not saved\" file object"));

        if(status == empty)
            throw Erange("file::get_data", gettext("data has been cleaned, object is now empty"));

        if(status == from_path)
	{
	    if(keep_compressed)
		throw SRC_BUG; // keep compressed is not possible on an inode take from a filesystem
            ret = new fichier(dialog, chemin, gf_read_only);
	}
        else // inode from archive
            if(loc == NULL)
                throw SRC_BUG; // set_archive_localisation never called or with a bad argument
            else
                if(loc->get_mode() == gf_write_only)
                    throw SRC_BUG; // cannot get data from a write-only file !!!
                else
                {
                    tronc *tmp = new tronc(dialog, loc, *offset, *storage_size == 0 ? *size : *storage_size, gf_read_only);
		    if(tmp == NULL)
			throw Ememory("file::get_data");
		    if(*size > 0 && *storage_size != 0 && ! keep_compressed)
		    {
			ret = new compressor(dialog, get_compression_algo_used(), tmp);
			if(ret == NULL)
			    delete tmp;
		    }
		    else
			ret = tmp;
                }

        if(ret == NULL)
            throw Ememory("file::get_data");
        else
            return ret;
    }

    void file::clean_data()
    {
        switch(status)
        {
        case from_path:
            chemin = "/"; // smallest possible memory allocation
            break;
        case from_cat:
            *offset = 0; // smallest possible memory allocation
                // warning, cannot change "size", as it is dump() in catalogue later
            break;
        case empty:
                // nothing to do
            break;
        default:
            throw SRC_BUG;
        }
        status = empty;
    }

    void file::set_offset(const infinint & r)
    {
        if(status == empty)
            throw SRC_BUG;
        set_saved_status(s_saved);
        *offset = r;
    }

    bool file::get_crc(crc & c) const
    {
        if(available_crc)
        {
            copy_crc(c, check);
            return true;
        }
        else
            return false;
    }

    void file::sub_compare(user_interaction & dialog, const inode & other) const
    {
        const file *f_other = dynamic_cast<const file *>(&other);
        if(f_other == NULL)
            throw SRC_BUG; // inode::compare should have called us with a correct argument

        if(get_size() != f_other->get_size())
            throw Erange("file::sub_compare", gettext("not same size"));
        if(get_saved_status() == s_saved && f_other->get_saved_status() == s_saved)
        {
            generic_file *me = get_data(dialog);
            if(me == NULL)
                throw SRC_BUG;
            try
            {
                generic_file *you = f_other->get_data(dialog);
                if(you == NULL)
                    throw SRC_BUG;
                try
                {
                    if(me->diff(*you))
                        throw Erange("file::sub_compare", gettext("different file data"));
                }
                catch(...)
                {
                    delete you;
                    throw;
                }
                delete you;
            }
            catch(...)
            {
                delete me;
                throw;
            }
            delete me;
        }
    }

    file_etiquette::file_etiquette(U_16 xuid, U_16 xgid, U_16 xperm,
                                   const infinint & last_access,
                                   const infinint & last_modif,
                                   const string & src,
                                   const path & che,
                                   const infinint & taille,
				   const infinint & fs_device,
				   const infinint & etiquette_number) : file(xuid, xgid, xperm, last_access, last_modif, src, che, taille, fs_device), etiquette(etiquette_number)
    {
	    // nothing to do more (all is done just above).
    }

    file_etiquette::file_etiquette(const file_etiquette & ref) : file(ref), etiquette(ref.etiquette)
    {
	    // nothing to do more (all is done just above).
    }

    file_etiquette::file_etiquette(user_interaction & dialog,
				   generic_file &f,
				   const dar_version & reading_ver,
				   saved_status saved,
				   compression default_algo,
				   generic_file *data_loc,
				   generic_file *ea_loc)
	: file(dialog, f, reading_ver, saved, default_algo, data_loc, ea_loc)
    {
        etiquette = infinint(dialog, NULL, &f);
    }

    void file_etiquette::dump(user_interaction & dialog, generic_file &f) const
    {
        file::dump(dialog, f);
        etiquette.dump(f);
    }

    hard_link::hard_link(const string & name, file_etiquette *ref) : nomme(name)
    {
        if(ref == NULL)
            throw SRC_BUG;
        set_reference(ref);
    }

    hard_link::hard_link(generic_file & f, infinint & etiquette) : nomme(f)
    {
        etiquette.read(f);
    }

    void hard_link::dump(user_interaction & dialog, generic_file &f) const
    {
        nomme::dump(dialog, f);
        get_etiquette().dump(f);
    }

    void hard_link::set_reference(file_etiquette *ref)
    {
        if(ref == NULL)
            throw SRC_BUG;
        x_ref = ref;
    }

    infinint hard_link::get_etiquette() const
    {
	if(x_ref == NULL)
	    throw SRC_BUG;
	if(dynamic_cast<file_etiquette *>(x_ref) == NULL)
	    throw SRC_BUG;

	return x_ref->get_etiquette();
    }

    lien::lien(U_16 uid, U_16 gid, U_16 perm,
               const infinint & last_access,
               const infinint & last_modif,
               const string & name,
               const string & target,
	       const infinint & fs_device) : inode(uid, gid, perm, last_access, last_modif, name, fs_device)
    {
        points_to = target;
        set_saved_status(s_saved);
    }

    lien::lien(user_interaction & dialog,
	       generic_file & f,
	       const dar_version & reading_ver,
	       saved_status saved,
	       generic_file *ea_loc) : inode(dialog, f, reading_ver, saved, ea_loc)
    {
        if(saved == s_saved)
            tools_read_string(f, points_to);
    }

    string lien::get_target() const
    {
        if(get_saved_status() != s_saved)
            throw SRC_BUG;
        return points_to;
    }

    void lien::set_target(string x)
    {
        set_saved_status(s_saved);
        points_to = x;
    }

    void lien::sub_compare(user_interaction & dialog, const inode & other) const
    {
        const lien *l_other = dynamic_cast<const lien *>(&other);
        if(l_other == NULL)
            throw SRC_BUG; // bad argument inode::compare has a bug

        if(get_saved_status() == s_saved && l_other->get_saved_status() == s_saved)
            if(get_target() != l_other->get_target())
                throw Erange("lien:sub_compare", gettext("symbolic link does not point to the same target"));
    }

    void lien::dump(user_interaction & dialog, generic_file & f) const
    {
        inode::dump(dialog, f);
        if(get_saved_status() == s_saved)
            tools_write_string(f, points_to);
    }

    directory::directory(U_16 xuid, U_16 xgid, U_16 xperm,
                         const infinint & last_access,
                         const infinint & last_modif,
                         const string & xname,
			 const infinint & fs_device) : inode(xuid, xgid, xperm, last_access, last_modif, xname, fs_device)
    {
        parent = NULL;
        fils.clear();
        it = fils.begin();
        set_saved_status(s_saved);
	recursive_has_changed = true;
    }

    directory::directory(const directory &ref) : inode(ref)
    {
        parent = NULL;
        fils.clear();
        it = fils.begin();
	recursive_has_changed = ref.recursive_has_changed;
    }

    directory::directory(user_interaction & dialog,
			 generic_file & f, const dar_version & reading_ver, saved_status saved,
			 entree_stats & stats,
			 std::map <infinint, file_etiquette *> & corres,
			 compression default_algo,
			 generic_file *data_loc,
			 generic_file *ea_loc) : inode(dialog, f, reading_ver, saved, ea_loc)
    {
        entree *p;
        nomme *t;
        directory *d;
        eod *fin = NULL;

        parent = NULL;
        fils.clear();
        it = fils.begin();
	recursive_has_changed = true; // need to call recursive_has_changed_update() first if this fields has to be used

        try
        {
            while(fin == NULL)
            {
                p = entree::read(dialog, f, reading_ver, stats, corres, default_algo, data_loc, ea_loc);
                if(p != NULL)
                {
                    d = dynamic_cast<directory *>(p);
                    fin = dynamic_cast<eod *>(p);
                    t = dynamic_cast<nomme *>(p);

                    if(t != NULL) // p is a "nomme"
                        fils.push_back(t);
                    if(d != NULL) // p is a directory
                        d->parent = this;
                    if(t == NULL && fin == NULL)
                        throw SRC_BUG; // neither an eod nor a nomme ! what's that ???
                }
                else
                    throw Erange("directory::directory", gettext("missing data to build a directory"));
            }
            delete fin; // no nead to keep it
        }
        catch(Egeneric & e)
        {
            clear();
            throw;
        }
    }

    directory::~directory()
    {
        clear();
    }

    void directory::dump(user_interaction & dialog, generic_file & f) const
    {
        vector<nomme *>::const_iterator x = fils.begin();
        inode::dump(dialog, f);
        eod fin;

        while(x != fils.end())
            if(dynamic_cast<ignored *>(*x) != NULL)
                x++; // "ignored" need not to be saved, they are only useful when updating_destroyed
            else
                (*x++)->dump(dialog, f);

        fin.dump(dialog, f); // end of "this" directory
    }

    void directory::add_children(nomme *r)
    {
        directory *d = dynamic_cast<directory *>(r);
        nomme *ancien;

        if(search_children(r->get_name(), ancien))
        {
            directory *a_dir = const_cast<directory *>(dynamic_cast<const directory *>(ancien));
            vector<nomme *>::iterator pos = find(fils.begin(), fils.end(), ancien);
            if(pos == fils.end())
                throw SRC_BUG; // ancien not found in fils !!!?

            if(a_dir != NULL && d != NULL)
            {
                vector<nomme *>::iterator it = a_dir->fils.begin();
                while(it != a_dir->fils.end())
                    d->add_children(*it++);
                a_dir->fils.clear();
                delete a_dir;
                *pos = r;
            }
            else
            {
                delete ancien;
                *pos = r;
            }
        }
        else
            fils.push_back(r);
        if(d != NULL)
            d->parent = this;
    }

    void directory::reset_read_children() const
    {
        directory *moi = const_cast<directory *>(this);
        moi->it = moi->fils.begin();
    }

    bool directory::read_children(const nomme *&r) const
    {
        directory *moi = const_cast<directory *>(this);
        if(moi->it != moi->fils.end())
        {
            r = *(moi->it)++;
            return true;
        }
        else
            return false;
    }

    void directory::clear()
    {
        it = fils.begin();
        while(it != fils.end())
        {
            delete *it;
            it++;
        }
        fils.clear();
        it = fils.begin();
    }

    void directory::listing(user_interaction & dialog,
			    const mask &m, bool filter_unsaved, string marge) const
    {
        vector<nomme *>::const_iterator it = fils.begin();
	thread_cancellation thr;

	thr.check_self_cancellation();
        while(it != fils.end())
        {
            const directory *dir = dynamic_cast<directory *>(*it);
            const detruit *det = dynamic_cast<detruit *>(*it);
            const inode *ino = dynamic_cast<inode *>(*it);
            const hard_link *hard = dynamic_cast<hard_link *>(*it);

            if(*it == NULL)
                throw SRC_BUG; // NULL entry ! should not be
            if(m.is_covered((*it)->get_name()) || dir != NULL)
            {
                if(det != NULL)
                {
                    string tmp = (*it)->get_name();
                    dialog.printf(gettext("%S[ REMOVED ]    %S\n"), &marge, &tmp);
                }
                else
                {
                    if(hard != NULL)
                        ino = hard->get_inode();

                    if(ino == NULL)
                        throw SRC_BUG;
                    else
			if(!filter_unsaved
			   || ino->get_saved_status() != s_not_saved
			   || (ino->ea_get_saved_status() != ea_none && ino->ea_get_saved_status() != ea_partial)
			   || (dir != NULL && dir->get_recursive_has_changed()))
			{
			    string a = local_perm(*ino);
			    string b = local_uid(*ino);
			    string c = local_gid(*ino);
			    string d = local_size(*ino);
			    string e = local_date(*ino);
			    string f = local_flag(*ino);
			    string g = (*it)->get_name();

			    dialog.printf("%S%S\t%S\t%S\t%S\t%S\t%S\t%S\n", &marge, &a, &b, &c, &d, &e, &f, &g);

			    if(dir != NULL)
			    {
				dir->listing(dialog, m, filter_unsaved, marge + "|  ");
				dialog.printf("%S+---\n", &marge);
			    }
			}
                }
            }

            it++;
        }
    }

    void directory::tar_listing(user_interaction & dialog,
				const mask &m, bool filter_unsaved, const string & beginning) const
    {
        vector<nomme *>::const_iterator it = fils.begin();
        string sep = beginning == "" ? "" : "/";
	thread_cancellation thr;

	thr.check_self_cancellation();
        while(it != fils.end())
        {
            const directory *dir = dynamic_cast<directory *>(*it);
            const detruit *det = dynamic_cast<detruit *>(*it);
            const inode *ino = dynamic_cast<inode *>(*it);
            const hard_link *hard = dynamic_cast<hard_link *>(*it);

            if(*it == NULL)
                throw SRC_BUG; // NULL entry ! should not be
            if(m.is_covered((*it)->get_name()) || dir != NULL)
            {
                if(det != NULL)
                {
                    string tmp = (*it)->get_name();
		    if(dialog.get_use_listing())
 			dialog.listing(REMOVE_TAG, "xxxxxxxxxx", "", "", "", "", beginning+sep+tmp, false, false);
		    else
			dialog.printf("%s %S%S%S\n", REMOVE_TAG, &beginning, &sep, &tmp);
                }
                else
                {
                    if(hard != NULL)
                        ino = hard->get_inode();

                    if(ino == NULL)
                        throw SRC_BUG;
                    else
			if(!filter_unsaved
			   || ino->get_saved_status() != s_not_saved
			   || (ino->ea_get_saved_status() != ea_none && ino->ea_get_saved_status() != ea_partial)
			   || (dir != NULL && dir->get_recursive_has_changed()))
			{
			    string a = local_perm(*ino);
			    string b = local_uid(*ino);
			    string c = local_gid(*ino);
			    string d = local_size(*ino);
			    string e = local_date(*ino);
			    string f = local_flag(*ino);
			    string g = (*it)->get_name();

			    if(dialog.get_use_listing())
				dialog.listing(f, a, b, c, d, e, beginning+sep+g, dir != NULL, dir != NULL && dir->has_children());
			    else
				dialog.printf("%S   %S   %S\t%S\t%S\t%S\t%S%S%S\n", &f, &a, &b, &c, &d, &e, &beginning, &sep, &g);

			    if(dir != NULL)
				dir->tar_listing(dialog, m, filter_unsaved, beginning + sep + (*it)->get_name());
			}
			// else do nothing
		}
	    }

	    it++;
	}
    }

    void directory::xml_listing(user_interaction & dialog,
				const mask &m,
				bool filter_unsaved,
				const string & beginning) const
    {
    	vector<nomme *>::const_iterator it = fils.begin();
	thread_cancellation thr;

	thr.check_self_cancellation();
	while(it != fils.end())
	{
	    const directory *dir = dynamic_cast<directory *>(*it);
	    const detruit *det = dynamic_cast<detruit *>(*it);
	    const inode *ino = dynamic_cast<inode *>(*it);
	    const hard_link *hard = dynamic_cast<hard_link *>(*it);
	    const lien *sym = dynamic_cast<lien *>(*it);
	    const device *dev = dynamic_cast<device *>(*it);

	    if(*it == NULL)
		throw SRC_BUG; // NULL entry ! should not be

	    if(m.is_covered((*it)->get_name()) || dir != NULL)
	    {
		string name = tools_output2xml((*it)->get_name());

		if(det != NULL)
		{
		    unsigned char sig;
		    saved_status state;
		    string data = "deleted";
		    string metadata = "absent";

		    unmk_signature(det->get_signature(), sig, state);
		    switch(sig)
		    {
		    case 'd':
			dialog.printf("%S<Directory name=\"%S\">\n", &beginning, &name);
			xml_listing_attributes(dialog, beginning, data, metadata);
			dialog.printf("%S</Directory>\n", &beginning);
			break;
		    case 'f':
		    case 'h':
		    case 'e':
			dialog.printf("%S<File name=\"%S\">\n", &beginning, &name);
			xml_listing_attributes(dialog, beginning, data, metadata);
			dialog.printf("%S</File>\n", &beginning);
			break;
		    case 'l':
			dialog.printf("%S<Symlink name=\"%S\">\n", &beginning, &name);
			xml_listing_attributes(dialog, beginning, data, metadata);
			dialog.printf("%S</Symlink>\n", &beginning);
			break;
		    case 'c':
			dialog.printf("%S<Device name=\"%S\" type=\"character\">\n", &beginning, &name);
			xml_listing_attributes(dialog, beginning, data, metadata);
			dialog.printf("%S</Device>\n", &beginning);
			break;
		    case 'b':
			dialog.printf("%S<Device name=\"%S\" type=\"block\">\n", &beginning, &name);
			xml_listing_attributes(dialog, beginning, data, metadata);
			dialog.printf("%S</Device>\n", &beginning);
			break;
		    case 'p':
			dialog.printf("%S<Pipe name=\"%S\">\n", &beginning, &name);
			xml_listing_attributes(dialog, beginning, data, metadata);
			dialog.printf("%S</Pipe>\n", &beginning);
			break;
		    case 's':
			dialog.printf("%S<Socket name=\"%S\">\n", &beginning, &name);
			xml_listing_attributes(dialog, beginning, data, metadata);
			dialog.printf("%S</Socket>\n", &beginning);
			break;
		    default:
			throw SRC_BUG;
		    }
		}
		else
		{
		    if(hard != NULL)
			ino = hard->get_inode();
		    if(ino == NULL)
			throw SRC_BUG; // this is a nomme which is neither a detruit nor an inode

		    if(!filter_unsaved
		       || ino->get_saved_status() != s_not_saved
		       || (ino->ea_get_saved_status() != ea_none && ino->ea_get_saved_status() != ea_partial)
		       || (dir != NULL && dir->get_recursive_has_changed()))
		    {
			string perm = local_perm(*ino);
			string uid = local_uid(*ino);
			string gid = local_gid(*ino);
			string size = local_size(*ino);
			string stored = local_storage_size(*ino);
			string mtime = deci(ino->get_last_modif()).human();
			string atime = deci(ino->get_last_access()).human();
			string ctime = "";
			string data, metadata, maj, min, chksum, target;
			const file *reg = dynamic_cast<const file *>(ino); // ino is no more *it (if *it was a hard_link)
			saved_status data_st;
			ea_status ea_st = ino->ea_get_saved_status();
			unsigned char sig;
			crc tmp;
			unmk_signature(ino->signature(), sig, data_st);
			data_st = ino->get_saved_status(); // the trusted source for inode status is get_saved_status, not the signature (may change in future, who knows)
			if(stored == "0")
			    stored = size;

			    // defining "data" string

			switch(data_st)
			{
			case s_saved:
			    data = "saved";
			    break;
			case s_fake:
			case s_not_saved:
			    data = "referenced";
			    break;
			default:
			    throw SRC_BUG;
			}

			    // defining "metadata" string

			switch(ea_st)
			{
			case ea_full:
			    metadata = "saved";
			    ctime = deci(ino->get_last_change()).human();
			    break;
			case ea_partial:
			case ea_fake:
			    metadata = "referenced";
			    ctime = deci(ino->get_last_change()).human();
			    break;
			case ea_none:
			    metadata = "absent";
			    ctime = "";
			    break;
			default:
			    throw SRC_BUG;
			}

			    // building entry for each type of inode

			switch(sig)
			{
			case 'd': // directories
			    dialog.printf("%S<Directory name=\"%S\">\n", &beginning, &name);
			    xml_listing_attributes(dialog, beginning, data, metadata, uid, gid, perm, atime, mtime, ctime);
			    dir->xml_listing(dialog, m, filter_unsaved, beginning + "\t");
			    dialog.printf("%S</Directory>\n", &beginning);
			    break;
			case 'f': // plain files
			case 'h': // hard linked files
			case 'e': // hard linked files
			    if(data_st == s_saved)
			    {
				if(reg == NULL)
				    throw SRC_BUG; // f, e is signature for plain files, h has been
				if(!reg->get_crc(tmp))
				    chksum = "";
				else
				    chksum = crc2str(tmp);
			    }
			    else
			    {
				stored = "";
				chksum = "";
			    }
			    dialog.printf("%S<File name=\"%S\" size=\"%S\" stored=\"%S\" crc=\"%S\">\n",
					  &beginning, &name, &size, &stored, &chksum);
			    xml_listing_attributes(dialog, beginning, data, metadata, uid, gid, perm, atime, mtime, ctime);
			    dialog.printf("%S</File>\n", &beginning);
			    break;
			case 'l': // soft link
			    if(data_st == s_saved)
				target = tools_output2xml(sym->get_target());
			    else
				target = "";
			    dialog.printf("%S<Symlink name=\"%S\" target=\"%S\">\n",
					  &beginning, &name, &target);
			    xml_listing_attributes(dialog, beginning, data, metadata, uid, gid, perm, atime, mtime, ctime);
			    dialog.printf("%S</Symlink>\n", &beginning);
			    break;
			case 'c':
			case 'b':
				// this is maybe less performant, to have both 'c' and 'b' here and
				// make an additional test, but this has the advantage to not duplicate
				// very similar code, which would obviously evoluate the same way.
				// Experience shows that two identical codes even when driven by the same need
				// are an important source of bugs, as one will forget to update both of them, the
				// same way...

			    if(sig == 'c')
				target = "character";
			    else
				target = "block";
				// we re-used target variable which is not used for the current inode

			    if(data_st == s_saved)
			    {
				maj = tools_uword2str(dev->get_major());
				min = tools_uword2str(dev->get_minor());
			    }
			    else
				maj = min = "";

			    dialog.printf("%S<Device name=\"%S\" type=\"%S\" major=\"%S\" minor=\"%S\">\n",
					  &beginning, &name, &target, &maj, &min);
			    xml_listing_attributes(dialog, beginning, data, metadata, uid, gid, perm, atime, mtime, ctime);
			    dialog.printf("%S</Device>\n", &beginning);
			    break;
			case 'p':
			    dialog.printf("%S<Pipe name=\"%S\">\n", &beginning, &name);
			    xml_listing_attributes(dialog, beginning, data, metadata, uid, gid, perm, atime, mtime, ctime);
			    dialog.printf("%S</Pipe>\n", &beginning);
			    break;
			case 's':
			    dialog.printf("%S<Socket name=\"%S\">\n", &beginning, &name);
			    xml_listing_attributes(dialog, beginning, data, metadata, uid, gid, perm, atime, mtime, ctime);
			    dialog.printf("%S</Socket>\n", &beginning);
			    break;
			default:
			    throw SRC_BUG;
			}
		    }
		}  // end of filter check

		it++;
	    }  // end of loop
	}
    }

    bool directory::search_children(const string &name, nomme *&ref)
    {
	vector<nomme *>::iterator ut = fils.begin();

	while(ut != fils.end() && (*ut)->get_name() != name)
	    ut++;

	if(ut != fils.end())
	{
	    ref = *ut;
	    return true;
	}
	else
	    return false;
    }

    bool directory::callback_for_children_of(user_interaction & dialog, const string & sdir) const
    {
	const directory *current = this;
	const nomme *next_nom = NULL;
	const directory *next_dir = NULL;
	const inode *next_ino = NULL;
	const detruit *next_detruit = NULL;
	string segment;
	bool loop = true;
	nomme *tmp_nom;

	if(!dialog.get_use_listing())
	    throw Erange("directory::callback_for_children_of", gettext("listing() method must be given"));

	if(sdir != "")
	{
	    path dir = sdir;

	    if(!dir.is_relative())
		throw Erange("directory::callback_for_children_of", gettext("argument must be a relative path"));

		///////////////////////////
		// looking for the inner most directory (basename of given path)
		//

	    do
	    {
		if(!dir.pop_front(segment))
		{
		    segment = dir.display();
		    loop = false;
		}

		if(const_cast<directory *>(current)->search_children(segment, tmp_nom))
		{
		    next_nom = const_cast<const nomme *>(tmp_nom);
		    next_dir = dynamic_cast<const directory *>(next_nom);
		    if(next_dir != NULL)
			current = next_dir;
		    else
			return false;
		}
		else
		    return false;
	    }
	    while(loop);
	}

	    ///////////////////////////
	    // calling listing() for each element of the "current" directory
	    //

	if(current == NULL)
	    throw SRC_BUG;

	loop = false; // loop now serves as returned value

	current->reset_read_children();
	while(current->read_children(next_nom))
	{
	    next_ino = dynamic_cast<const inode *>(next_nom);
	    next_detruit = dynamic_cast<const detruit *>(next_nom);
	    next_dir = dynamic_cast<const directory *>(next_nom);
	    if(next_ino != NULL)
	    {
		string a = local_perm(*next_ino);
		string b = local_uid(*next_ino);
		string c = local_gid(*next_ino);
		string d = local_size(*next_ino);
		string e = local_date(*next_ino);
		string f = local_flag(*next_ino);
		string g = next_ino->get_name();
		dialog.listing(f,a,b,c,d,e,g, next_dir != NULL, next_dir != NULL && next_dir->has_children());
		loop = true;
	    }
	    else
		if(next_detruit != NULL)
		{
		    string a = next_detruit->get_name();
		    dialog.listing(REMOVE_TAG, "xxxxxxxxxx", "", "", "", "", a, false, false);
		    loop = true;
		}
		else
		    throw SRC_BUG; // unknown class
	}

	return loop;
    }

    void directory::recursive_has_changed_update() const
    {
	vector<nomme *>::const_iterator it = fils.begin();

	const_cast<directory *>(this)->recursive_has_changed = false;
	while(it != fils.end())
	{
	    const directory *d = dynamic_cast<directory *>(*it);
	    const inode *ino = dynamic_cast<inode *>(*it);
	    if(d != NULL)
	    {
		d->recursive_has_changed_update();
		const_cast<directory *>(this)->recursive_has_changed |= d->get_recursive_has_changed();
	    }
	    if(ino != NULL && !recursive_has_changed)
		const_cast<directory *>(this)->recursive_has_changed |=
		    ino->get_saved_status() != s_not_saved
		    || ino->ea_get_saved_status() != ea_none;
	    it++;
	}
    }

    device::device(U_16 uid, U_16 gid, U_16 perm,
		   const infinint & last_access,
		   const infinint & last_modif,
		   const string & name,
		   U_16 major,
		   U_16 minor,
		   const infinint & fs_dev) : inode(uid, gid, perm, last_access, last_modif, name, fs_dev)
    {
	xmajor = major;
	xminor = minor;
	set_saved_status(s_saved);
    }

    device::device(user_interaction & dialog,
		   generic_file & f,
		   const dar_version & reading_ver,
		   saved_status saved,
		   generic_file *ea_loc) : inode(dialog, f, reading_ver, saved, ea_loc)
    {
	U_16 tmp;

	if(saved == s_saved)
	{
	    if(f.read((char *)&tmp, (size_t)sizeof(tmp)) != sizeof(tmp))
		throw Erange("special::special", gettext("missing data to build a special device"));
	    xmajor = ntohs(tmp);
	    if(f.read((char *)&tmp, (size_t)sizeof(tmp)) != sizeof(tmp))
		throw Erange("special::special", gettext("missing data to build a special device"));
	    xminor = ntohs(tmp);
	}
    }

    void device::dump(user_interaction & dialog, generic_file & f) const
    {
	U_16 tmp;

	inode::dump(dialog, f);
	if(get_saved_status() == s_saved)
	{
	    tmp = htons(xmajor);
	    f.write((char *)&tmp, (size_t)sizeof(tmp));
	    tmp = htons(xminor);
	    f.write((char *)&tmp, (size_t)sizeof(tmp));
	}
    }

    void device::sub_compare(user_interaction & dialog, const inode & other) const
    {
	const device *d_other = dynamic_cast<const device *>(&other);
	if(d_other == NULL)
	    throw SRC_BUG; // bug in inode::compare
	if(get_saved_status() == s_saved && d_other->get_saved_status() == s_saved)
	{
	    if(get_major() != d_other->get_major())
		throw Erange("device::sub_compare", gettext("devices have not the same major number"));
	    if(get_minor() != d_other->get_minor())
		throw Erange("device::sub_compare", gettext("devices have not the same minor number"));
	}
    }

    void ignored_dir::dump(user_interaction & dialog, generic_file & f) const
    {
	directory tmp = directory(get_uid(), get_gid(), get_perm(), get_last_access(), get_last_modif(), get_name(), 0);
	tmp.set_saved_status(get_saved_status());
	tmp.dump(dialog, f); // dump an empty directory
    }

    catalogue::catalogue(user_interaction & dialog) : out_compare("/")
    {
	contenu = NULL;
	cat_ui = NULL;

	try
	{
	    contenu = new directory(0,0,0,0,0,"root",0);
	    if(contenu == NULL)
		throw Ememory("catalogue::catalogue(path)");
	    current_compare = contenu;
	    current_add = contenu;
	    current_read = contenu;
	    cat_ui = dialog.clone();
	    sub_tree = NULL;
	}
	catch(...)
	{
	    if(contenu != NULL)
		delete contenu;
	    if(cat_ui != NULL)
		delete cat_ui;
	    throw;
	}

	stats.clear();
    }

    catalogue::catalogue(user_interaction & dialog,
			 generic_file & f, const dar_version & reading_ver,
			 compression default_algo,
			 generic_file *data_loc,
			 generic_file *ea_loc) : out_compare("/")
    {
	string tmp;
	unsigned char a;
	saved_status st;
	unsigned char base;
	cache over_f = cache(dialog, f, BUFFER_SIZE, 1, 100, 20, 1, 100, 20);
	std::map <infinint, file_etiquette *> corres;
	contenu = NULL;
	cat_ui = NULL;

	try
	{
	    over_f.read((char *)&a, 1); // need to read the signature before constructing "contenu"
	    if(! extract_base_and_status(a, base, st))
		throw Erange("catalogue::catalogue(generic_file &)", gettext("incoherent catalogue structure"));
	    if(base != 'd')
		throw Erange("catalogue::catalogue(generic_file &)", gettext("incoherent catalogue structure"));

	    stats.clear();
	    contenu = new directory(dialog, over_f, reading_ver, st, stats, corres, default_algo, data_loc, ea_loc);
	    if(contenu == NULL)
		throw Ememory("catalogue::catalogue(path)");
	    current_compare = contenu;
	    current_add = contenu;
	    current_read = contenu;
	    sub_tree = NULL;
	    cat_ui = dialog.clone();
	}
	catch(...)
	{
	    if(contenu == NULL)
		delete contenu;
	    if(cat_ui == NULL)
		delete cat_ui;
	    throw;
	}
    }

    catalogue & catalogue::operator = (const catalogue &ref)
    {
	detruire();
	out_compare = ref.out_compare;
	partial_copy_from(ref);
	return *this;
    }

    void catalogue::reset_read()
    {
	current_read = contenu;
	contenu->reset_read_children();
    }

    void catalogue::skip_read_to_parent_dir()
    {
	directory *tmp = current_read->get_parent();
	if(tmp == NULL)
	    throw Erange("catalogue::skip_read_to_parent_dir", gettext("root does not have a parent directory"));
	current_read = tmp;
    }

    bool catalogue::read(const entree * & ref)
    {
	const nomme *tmp;

	if(current_read->read_children(tmp))
	{
	    const directory *dir = dynamic_cast<const directory *>(tmp);
	    if(dir != NULL)
	    {
		current_read = const_cast<directory *>(dir);
		dir->reset_read_children();
	    }
	    ref = tmp;
	    return true;
	}
	else
	{
	    directory *papa = current_read->get_parent();
	    ref = & r_eod;
	    if(papa == NULL)
		return false; // we reached end of root, no eod generation
	    else
	    {
		current_read = papa;
		return true;
	    }
	}
    }

    bool catalogue::read_if_present(string *name, const nomme * & ref)
    {
	nomme *tmp;

	if(current_read == NULL)
	    throw Erange("catalogue::read_if_present", gettext("no current directory defined"));
	if(name == NULL) // we have to go to parent directory
	{
	    if(current_read->get_parent() == NULL)
		throw Erange("catalogue::read_if_present", gettext("root directory has no parent directory"));
	    else
		current_read = current_read->get_parent();
	    ref = NULL;
	    return true;
	}
	else // looking for a real filename
	    if(current_read->search_children(*name, tmp))
	    {
		directory *d = dynamic_cast<directory *>(tmp);
		if(d != NULL) // this is a directory need to chdir to it
		    current_read = d;
		ref = tmp;
		return true;
	    }
	    else // filename not present in current dir
		return false;
    }

    void catalogue::reset_sub_read(const path &sub)
    {
	if(! sub.is_relative())
	    throw SRC_BUG;

	if(sub_tree != NULL)
	    delete sub_tree;
	sub_tree = new path(sub);
	if(sub_tree == NULL)
	    throw Ememory("catalogue::reset_sub_read");
	sub_count = -1; // must provide the path to subtree;
	reset_read();
    }

    bool catalogue::sub_read(const entree * &ref)
    {
	string tmp;

	if(sub_tree == NULL)
	    throw SRC_BUG; // reset_sub_read

	switch(sub_count)
	{
	case 0 : // sending oed to go back to the root
	    if(sub_tree->pop(tmp))
	    {
		ref = &r_eod;
		return true;
	    }
	    else
	    {
		ref = NULL;
		delete sub_tree;
		sub_tree = NULL;
		sub_count = -2;
		return false;
	    }
	case -2: // reading is finished
	    return false;
	case -1: // providing path to sub_tree
	    if(sub_tree->read_subdir(tmp))
	    {
		nomme *xtmp;

		if(current_read->search_children(tmp, xtmp))
		{
		    ref = xtmp;
		    directory *dir = dynamic_cast<directory *>(xtmp);

		    if(dir != NULL)
		    {
			current_read = dir;
			return true;
		    }
		    else
			if(sub_tree->read_subdir(tmp))
			{
			    cat_ui->warning(sub_tree->display() + gettext(" is not present in the archive"));
			    delete sub_tree;
			    sub_tree = NULL;
			    sub_count = -2;
			    return false;
			}
			else // subdir is a single file (no tree))
			{
			    sub_count = 0;
			    return true;
			}
		}
		else
		{
		    cat_ui->warning(sub_tree->display() + gettext(" is not present in the archive"));
		    delete sub_tree;
		    sub_tree = NULL;
		    sub_count = -2;
		    return false;
		}
	    }
	    else
	    {
		sub_count = 1;
		current_read->reset_read_children();
		    // now reading the sub_tree
		    // no break !
	    }

	default:
	    if(read(ref) && sub_count > 0)
	    {
		const directory *dir = dynamic_cast<const directory *>(ref);
		const eod *fin = dynamic_cast<const eod *>(ref);

		if(dir != NULL)
		    sub_count++;
		if(fin != NULL)
		    sub_count--;

		return true;
	    }
	    else
		throw SRC_BUG;
	}
    }

    void catalogue::reset_add()
    {
	current_add = contenu;
    }

    void catalogue::add(entree *ref)
    {
	if(current_add == NULL)
	    throw SRC_BUG;

	eod *f = dynamic_cast<eod *>(ref);

	if(f == NULL) // ref is not eod
	{
	    nomme *n = dynamic_cast<nomme *>(ref);
	    directory *t = dynamic_cast<directory *>(ref);

	    if(n == NULL)
		throw SRC_BUG; // unknown type neither "eod" nor "nomme"
	    current_add->add_children(n);
	    if(t != NULL) // ref is a directory
		current_add = t;
	    stats.add(ref);
	}
	else // ref is an eod
	{
	    directory *parent = current_add->get_parent();
	    delete ref; // all data given throw add becomes owned by the catalogue object
	    if(parent == NULL)
		throw Erange("catalogue::add_file", gettext("root has no parent directory, cannot change to it"));
	    else
		current_add = parent;
	}
    }

    void catalogue::add_in_current_read(nomme *ref)
    {
	if(current_read == NULL)
	    throw SRC_BUG; // current read directory does not exists
	current_read->add_children(ref);
    }

    void catalogue::reset_compare()
    {
	current_compare = contenu;
	out_compare = "/";
    }

    bool catalogue::compare(const entree * target, const entree * & extracted)
    {
	const directory *dir = dynamic_cast<const directory *>(target);
	const eod *fin = dynamic_cast<const eod *>(target);
	const nomme *nom = dynamic_cast<const nomme *>(target);

	if(out_compare.degre() > 1) // actually scanning a inexisting directory
	{
	    if(dir != NULL)
		out_compare += dir->get_name();
	    else
		if(fin != NULL)
		{
		    string tmp_s;

		    if(!out_compare.pop(tmp_s))
			if(out_compare.is_relative())
			    throw SRC_BUG; // should not be a relative path !!!
			else // both cases are bugs, but need to know which case is generating a bug
			    throw SRC_BUG; // out_compare.degre() > 0 but cannot pop !
		}

	    return false;
	}
	else // scanning an existing directory
	{
	    nomme *found;

	    if(fin != NULL)
	    {
		directory *tmp = current_compare->get_parent();
		if(tmp == NULL)
		    throw Erange("catalogue::compare", gettext("root has no parent directory"));
		current_compare = tmp;
		extracted = target;
		return true;
	    }

	    if(nom == NULL)
		throw SRC_BUG; // ref, is neither a eod nor a nomme ! what's that ???

	    if(current_compare->search_children(nom->get_name(), found))
	    {
		const detruit *src_det = dynamic_cast<const detruit *>(nom);
		const detruit *dst_det = dynamic_cast<const detruit *>(found);
		const inode *src_ino = dynamic_cast<const inode *>(nom);
		const inode *dst_ino = dynamic_cast<const inode *>(found);
		const etiquette *src_eti = dynamic_cast<const etiquette *>(nom);
		const etiquette *dst_eti = dynamic_cast<const etiquette *>(found);

		    // extracting inode from hard links
		if(src_eti != NULL)
		    src_ino = src_eti->get_inode();
		if(dst_eti != NULL)
		    dst_ino = dst_eti->get_inode();

		    // updating internal structure to follow directory tree :
		if(dir != NULL)
		{
		    directory *d_ext = dynamic_cast<directory *>(found);
		    if(d_ext != NULL)
			current_compare = d_ext;
		    else
			out_compare += dir->get_name();
		}

		    // now comparing the objects :
		if(src_ino != NULL)
		    if(dst_ino != NULL)
		    {
			if(!src_ino->same_as(*dst_ino))
			    return false;
		    }
		    else
			return false;
		else
		    if(src_det != NULL)
			if(dst_det != NULL)
			{
			    if(!dst_det->same_as(*dst_det))
				return false;
			}
			else
			    return false;
		    else
			throw SRC_BUG; // src_det == NULL && src_ino == NULL, thus a nomme which is neither detruit nor inode !

		if(dst_eti != NULL)
		    extracted = dst_eti->get_inode();
		else
		    extracted = found;
		return true;
	    }
	    else
	    {
		if(dir != NULL)
		    out_compare += dir->get_name();
		return false;
	    }
	}
    }

    infinint catalogue::update_destroyed_with(catalogue & ref)
    {
	directory *current = contenu;
	nomme *ici;
	const entree *projo;
	const eod *pro_eod;
	const directory *pro_dir;
	const detruit *pro_det;
	const nomme *pro_nom;
	infinint count = 0;

	ref.reset_read();
	while(ref.read(projo))
	{
	    pro_eod = dynamic_cast<const eod *>(projo);
	    pro_dir = dynamic_cast<const directory *>(projo);
	    pro_det = dynamic_cast<const detruit *>(projo);
	    pro_nom = dynamic_cast<const nomme *>(projo);

	    if(pro_eod != NULL)
	    {
		directory *tmp = current->get_parent();
		if(tmp == NULL)
		    throw SRC_BUG; // reached root for "contenu", and not yet for "ref";
		current = tmp;
		continue;
	    }

	    if(pro_det != NULL)
		continue;

	    if(pro_nom == NULL)
		throw SRC_BUG; // neither an eod nor a nomme ! what's that ?

	    if(!current->search_children(pro_nom->get_name(), ici))
	    {
		current->add_children(new detruit(pro_nom->get_name(), pro_nom->signature()));
		count++;
		if(pro_dir != NULL)
		    ref.skip_read_to_parent_dir();
	    }
	    else
		if(pro_dir != NULL)
		{
		    directory *ici_dir = dynamic_cast<directory *>(ici);

		    if(ici_dir != NULL)
			current = ici_dir;
		    else
			ref.skip_read_to_parent_dir();
		}
	}

	return count;
    }

    void catalogue::update_absent_with(catalogue & ref)
    {
       	directory *current = contenu;
	nomme *ici;
	const entree *projo;
	const eod *pro_eod;
	const directory *pro_dir;
	const detruit *pro_det;
	const nomme *pro_nom;
	const inode *pro_ino;

	ref.reset_read();
	while(ref.read(projo))
	{
	    pro_eod = dynamic_cast<const eod *>(projo);
	    pro_dir = dynamic_cast<const directory *>(projo);
	    pro_det = dynamic_cast<const detruit *>(projo);
	    pro_nom = dynamic_cast<const nomme *>(projo);
	    pro_ino = dynamic_cast<const inode *>(projo);

	    if(pro_eod != NULL)
	    {
		directory *tmp = current->get_parent();
		if(tmp == NULL)
		    throw SRC_BUG; // reached root for "contenu", and not yet for "ref";
		current = tmp;
		continue;
	    }

	    if(pro_det != NULL)
		continue;

	    if(pro_nom == NULL)
		throw SRC_BUG; // neither an eod nor a nomme! what's that?

	    if(pro_ino == NULL)
		throw SRC_BUG; // a nome that is not an inode nor a detruit!? What's that?

	    if(!current->search_children(pro_ino->get_name(), ici))
	    {
		entree *clo_ent = NULL;
		inode *clo_ino = NULL;
		directory *clo_dir = NULL;
		try
		{
		    clo_ent = pro_ino->clone();
		    clo_ino = dynamic_cast<inode *>(clo_ent);
		    clo_dir = dynamic_cast<directory *>(clo_ent);

			// sanity checks

		    if(clo_ino == NULL)
			throw SRC_BUG; // clone of an inode is not an inode???
		    if(clo_dir != NULL ^ pro_dir != NULL)
			throw SRC_BUG; // both must be NULL or both must be non NULL

			// converting inode to unsaved entry

		    clo_ino->set_saved_status(s_not_saved);
		    if(clo_ino->ea_get_saved_status() != inode::ea_none)
			clo_ino->ea_set_saved_status(inode::ea_partial);

			// adding it to the catalogue

		    current->add_children(clo_ino);

			// recusing in case of directory

		    if(clo_dir != NULL)
			if(current->search_children(pro_ino->get_name(), ici))
			{
			    if((void *)ici != (void *)clo_ent)
				throw SRC_BUG;  // we have just added the entry we were looking for, but could found another one!?!
			    current = clo_dir;
			}
			else
			    throw SRC_BUG; // cannot find the entry we have just added!!!
		}
		catch(...)
		{
		    if(clo_ent != NULL)
			delete clo_ent;
		    throw;
		}
	    }
	    else
		if(pro_dir != NULL)
		{
		    directory *ici_dir = dynamic_cast<directory *>(ici);

		    if(ici_dir != NULL)
			current = ici_dir;
		    else
			ref.skip_read_to_parent_dir();
		}
	}
    }


    void catalogue::listing(const mask &m, bool filter_unsaved, string marge) const
    {
	cat_ui->printf(gettext("access mode    | user | group | size  |          date                 | [data ][ EA  ][compr] |   filename\n"));
	cat_ui->printf("---------------+------+-------+-------+-------------------------------+-----------------------+-----------\n");
	if(filter_unsaved)
	    contenu->recursive_has_changed_update();
	contenu->listing(*cat_ui, m, filter_unsaved, marge);
    }

    void catalogue::tar_listing(const mask &m, bool filter_unsaved, const string & beginning) const
    {
	if(!cat_ui->get_use_listing())
	{
	    cat_ui->printf(gettext("[data ][ EA  ][compr] | permission | user  | group | size  |          date                 |    filename\n"));
	    cat_ui->printf("----------------------+------------+-------+-------+-------+-------------------------------+------------\n");
	}
	if(filter_unsaved)
	    contenu->recursive_has_changed_update();
	contenu->tar_listing(*cat_ui, m, filter_unsaved, beginning);
    }

    void catalogue::xml_listing(const mask &m, bool filter_unsaved, const string & beginning) const
    {
	cat_ui->warning("<?xml version=\"1.0\" ?>");
	cat_ui->warning("<!DOCTYPE Catalog SYSTEM \"dar-catalog-1.0.dtd\">\n");
	cat_ui->warning("<Catalog format=\"1.0\">");
	if(filter_unsaved)
	    contenu->recursive_has_changed_update();
	contenu->xml_listing(*cat_ui, m, filter_unsaved, beginning);
	cat_ui->warning("</Catalog>");
    }

    static void dummy_call(char *x)
    {
	static char id[]="$Id: catalogue.cpp,v 1.47.2.7 2007/07/27 11:27:31 edrusb Rel $";
	dummy_call(id);
    }

    void catalogue::dump(generic_file & ref) const
    {
	cache over_ref = cache(*cat_ui, ref, BUFFER_SIZE, 1, 100, 20, 1, 100, 20);

	contenu->dump(*cat_ui, over_ref);
    }

    void catalogue::partial_copy_from(const catalogue & ref)
    {
	contenu = NULL;
	cat_ui = NULL;
	sub_tree = NULL;

	try
	{
	    if(ref.contenu == NULL)
		throw SRC_BUG;
	    contenu = new directory(*ref.contenu);
	    if(contenu == NULL)
		throw Ememory("catalogue::catalogue(const catalogue &)");
	    current_compare = contenu;
	    current_add = contenu;
	    current_read = contenu;
	    if(ref.sub_tree != NULL)
		sub_tree = new path(*ref.sub_tree);
	    else
		sub_tree = NULL;
	    sub_count = ref.sub_count;
	    stats = ref.stats;
	    cat_ui = ref.cat_ui->clone();
	}
	catch(...)
	{
	    if(contenu != NULL)
		delete contenu;
	    if(cat_ui != NULL)
		delete cat_ui;
	    if(sub_tree != NULL)
		delete sub_tree;
	    throw;
	}
    }

    void catalogue::detruire()
    {
	if(contenu != NULL)
	    delete contenu;
	if(sub_tree != NULL)
	    delete sub_tree;
	if(cat_ui != NULL)
	    delete cat_ui;
    }


    const eod catalogue::r_eod;


    static string local_perm(const inode &ref)
    {
	string ret = "";
	saved_status st;
	char type;

	U_32 perm = ref.get_perm();
	if(!extract_base_and_status(ref.signature(), (unsigned char &)type, st))
	    throw SRC_BUG;

	if(type == 'f')
	    type = '-';
	if(type == 'e')
	    type = 'h';
	ret += type;
	if((perm & 0400) != 0)
	    ret += 'r';
	else
	    ret += '-';
	if((perm & 0200) != 0)
	    ret += 'w';
	else
	    ret += '-';
	if((perm & 0100) != 0)
	    if((perm & 04000) != 0)
		ret += 's';
	    else
		ret += 'x';
	else
	    if((perm & 04000) != 0)
		ret += 'S';
	    else
		ret += '-';
	if((perm & 040) != 0)
	    ret += 'r';
	else
	    ret += '-';
	if((perm & 020) != 0)
	    ret += 'w';
	else
	    ret += '-';
	if((perm & 010) != 0)
	    if((perm & 02000) != 0)
		ret += 's';
	    else
		ret += 'x';
	else
	    if((perm & 02000) != 0)
		ret += 'S';
	    else
		ret += '-';
	if((perm & 04) != 0)
	    ret += 'r';
	else
	    ret += '-';
	if((perm & 02) != 0)
	    ret += 'w';
	else
	    ret += '-';
	if((perm & 01) != 0)
	    if((perm & 01000) != 0)
		ret += 't';
	    else
		ret += 'x';
	else
	    if((perm & 01000) != 0)
		ret += 'T';
	    else
		ret += '-';

	return ret;
    }

    static string local_uid(const inode & ref)
    {
	return tools_name_of_uid(ref.get_uid());
    }

    static string local_gid(const inode & ref)
    {
	return tools_name_of_gid(ref.get_gid());
    }

    static string local_size(const inode & ref)
    {
	string ret;

	const file *fic = dynamic_cast<const file *>(&ref);
	if(fic != NULL)
	{
	    deci d = fic->get_size();
	    ret = d.human();
	}
	else
	    ret = "0";

	return ret;
    }

    static string local_storage_size(const inode & ref)
    {
	string ret;

	const file *fic = dynamic_cast<const file*>(&ref);
	if(fic != NULL)
	{
	    deci d = fic->get_storage_size();
	    ret = d.human();
	}
	else
	    ret = "0";

	return ret;
    }

    static string local_date(const inode & ref)
    {
	return tools_display_date(ref.get_last_modif());
    }

    static string local_flag(const inode & ref)
    {
	string ret;

	switch(ref.get_saved_status())
	{
	case s_saved:
	    ret = gettext("[Saved]");
	    break;
	case s_fake:
	    ret = gettext("[InRef]");
	    break;
	case s_not_saved:
	    ret = "[     ]";
	    break;
	default:
	    throw SRC_BUG;
	}

	switch(ref.ea_get_saved_status())
	{
	case inode::ea_full:
	    ret += gettext("[Saved]");
	    break;
	case inode::ea_fake:
	    ret += gettext("[InRef]");
	    break;
	case inode::ea_partial:
	    ret += "[     ]";
	    break;
	case inode::ea_none:
	    ret += "       ";
	    break;
	default:
	    throw SRC_BUG;
	}

	const file *fic = dynamic_cast<const file *>(&ref);
	if(fic != NULL && fic->get_saved_status() == s_saved)
	    if(fic->get_storage_size() == 0)
		ret += "[     ]";
	    else
		if(fic->get_size() >= fic->get_storage_size())
		    ret += "[" + tools_addspacebefore(deci(((fic->get_size() - fic->get_storage_size())*100)/fic->get_size()).human(), 4) +"%]";
		else
		    ret += gettext("[Worse]");
	else
	    ret += "[-----]";

	return ret;
    }

    static void xml_listing_attributes(user_interaction & dialog,
				       const string & beginning,
				       const string & data,
				       const string & metadata,
				       const string & user,
				       const string & group,
				       const string & permissions,
				       const string & atime,
				       const string & mtime,
				       const string & ctime)
    {
	dialog.printf("%S<Attributes data=\"%S\" metadata=\"%S\" user=\"%S\" group=\"%S\" permissions=\"%S\" atime=\"%S\" mtime=\"%S\" ctime=\"%S\" />\n",
		      &beginning, &data, &metadata, &user, &group, &permissions, &atime, &mtime, &ctime);
    }


    static bool extract_base_and_status(unsigned char signature, unsigned char & base, saved_status & saved)
    {
	bool fake = (signature & SAVED_FAKE_BIT) != 0;

	signature &= ~SAVED_FAKE_BIT;
	if(!isalpha(signature))
	    return false;
	base = tolower(signature);

	if(fake)
	    if(base == signature)
		saved = s_fake;
	    else
		return false;
	else
	    if(signature == base)
		saved = s_saved;
	    else
		saved = s_not_saved;
	return true;
    }

} // end of namespace
