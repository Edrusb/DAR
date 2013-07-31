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
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#if HAVE_LIMITS_H
#include <limits.h>
#endif
} // end extern "C"

#include <typeinfo>
#include <algorithm>
#include <map>
#include <new>
#include "catalogue.hpp"
#include "tools.hpp"
#include "tronc.hpp"
#include "user_interaction.hpp"
#include "deci.hpp"
#include "header.hpp"
#include "defile.hpp"
#include "pile.hpp"
#include "sparse_file.hpp"
#include "fichier.hpp"
#include "macro_tools.hpp"
#include "null_file.hpp"

#define INODE_FLAG_EA_MASK  0x07
#define INODE_FLAG_EA_FULL  0x01
#define INODE_FLAG_EA_PART  0x02
#define INODE_FLAG_EA_NONE  0x03
#define INODE_FLAG_EA_FAKE  0x04
#define INODE_FLAG_EA_REMO  0x05

#define MIRAGE_ALONE 'X'
#define MIRAGE_WITH_INODE '>'

#define REMOVE_TAG gettext("[--- REMOVED ENTRY ----]")

#define SAVED_FAKE_BIT 0x80

#define SMALL_ROBUST_BUFFER_SIZE 10
#ifdef SSIZE_MAX
#if SSIZE_MAX < SMALL_ROBUST_BUFFER_SIZE
#undef SMALL_ROBUST_BUFFER_SIZE
#define SMALL_ROBUST_BUFFER_SIZE SSIZE_MAX
#endif
#endif

using namespace std;

namespace libdar
{
    static string local_perm(const inode & ref, bool hard);
    static string local_uid(const inode & ref);
    static string local_gid(const inode & ref);
    static string local_size(const inode & ref);
    static string local_storage_size(const inode & ref);
    static string local_date(const inode & ref);
    static string local_flag(const inode & ref, bool isolated, bool dirty_seq);
    static void xml_listing_attributes(user_interaction & dialog, //< for user interaction
                                       const string & beginning,  //< character string to use as margin
                                       const string & data,       //< ("saved" | "referenced" | "deleted")
                                       const string & metadata,   //< ("saved" | "referenced" | "absent")
                                       const entree * obj = NULL, //< the object to display inode information about
                                       bool list_ea = false);     //< whether to list Extended Attributes

    static bool extract_base_and_status(unsigned char signature, unsigned char & base, saved_status & saved);
    static void unmk_signature(unsigned char sig, unsigned char & base, saved_status & state, bool isolated);
    static bool local_check_dirty_seq(escape *ptr);
    static void local_display_ea(user_interaction & dialog, const inode * ino, const string &prefix, const string &suffix, bool xml_output = false);

    static inline string yes_no(bool val) { return (val ? "yes" : "no"); }


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

    static void unmk_signature(unsigned char sig, unsigned char & base, saved_status & state, bool isolated)
    {
        if((sig & SAVED_FAKE_BIT) == 0 && !isolated)
            if(islower(sig))
                state = s_saved;
            else
                state = s_not_saved;
        else
            state = s_fake;

        base = tolower(sig & ~SAVED_FAKE_BIT);
    }

    const U_I entree::ENTREE_CRC_SIZE = 2;

    void entree_stats::add(const entree *ref)
    {
        if(dynamic_cast<const eod *>(ref) == NULL // we ignore eod
           && dynamic_cast<const ignored *>(ref) == NULL // as well we ignore "ignored"
           && dynamic_cast<const ignored_dir *>(ref) == NULL) // and "ignored_dir"
        {
            const inode *ino = dynamic_cast<const inode *>(ref);
            const mirage *h = dynamic_cast<const mirage *>(ref);
            const detruit *x = dynamic_cast<const detruit *>(ref);


            if(h != NULL) // won't count twice the same inode if it is referenced with hard_link
            {
                ++num_hard_link_entries;
                if(!h->is_inode_counted())
                {
                    ++num_hard_linked_inodes;
                    h->set_inode_counted(true);
                    ino = h->get_inode();
                }
            }

            if(ino != NULL)
            {
                ++total;
                if(ino->get_saved_status() == s_saved)
                    ++saved;
            }

            if(x != NULL)
                ++num_x;
            else
            {
                const directory *d = dynamic_cast<const directory*>(ino);
                if(d != NULL)
                    ++num_d;
                else
                {
                    const chardev *c = dynamic_cast<const chardev *>(ino);
                    if(c != NULL)
                        ++num_c;
                    else
                    {
                        const blockdev *b = dynamic_cast<const blockdev *>(ino);
                        if(b != NULL)
                            ++num_b;
                        else
                        {
                            const tube *p = dynamic_cast<const tube *>(ino);
                            if(p != NULL)
                                ++num_p;
                            else
                            {
                                const prise *s = dynamic_cast<const prise *>(ino);
                                if(s != NULL)
                                    ++num_s;
                                else
                                {
                                    const lien *l = dynamic_cast<const lien *>(ino);
                                    if(l != NULL)
                                        ++num_l;
                                    else
                                    {
					const door *D = dynamic_cast<const door *>(ino);
					if(D != NULL)
					    ++num_D;
					else
					{
					    const file *f = dynamic_cast<const file *>(ino);
					    if(f != NULL)
						++num_f;
					    else
						if(h == NULL)
						    throw SRC_BUG; // unknown entry
					}
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
        dialog.printf(gettext("distribution of inode(s)\n"));
        dialog.printf(gettext(" - directories        : %i\n"), &num_d);
        dialog.printf(gettext(" - plain files        : %i\n"), &num_f);
        dialog.printf(gettext(" - symbolic links     : %i\n"), &num_l);
        dialog.printf(gettext(" - named pipes        : %i\n"), &num_p);
        dialog.printf(gettext(" - unix sockets       : %i\n"), &num_s);
        dialog.printf(gettext(" - character devices  : %i\n"), &num_c);
        dialog.printf(gettext(" - block devices      : %i\n"), &num_b);
	dialog.printf(gettext(" - Door entries       : %i\n"), &num_D);
        dialog.printf(gettext("hard links information\n"));
        dialog.printf(gettext(" - number of inode with hard link           : %i\n"), &num_hard_linked_inodes);
        dialog.printf(gettext(" - number of reference to hard linked inodes: %i\n"), &num_hard_link_entries);
        dialog.printf(gettext("destroyed entries information\n"));
        dialog.printf(gettext("   %i file(s) have been record as destroyed since backup of reference\n\n"), &num_x);
    }

    entree *entree::read(user_interaction & dialog,
                         generic_file & f,
                         const archive_version & reading_ver,
                         entree_stats & stats,
                         std::map <infinint, etoile *> & corres,
                         compression default_algo,
                         generic_file *data_loc,
                         generic_file *ea_loc,
                         bool lax,
                         bool only_detruit,
                         escape *ptr)
    {
        char type;
        saved_status saved;
        entree *ret = NULL;
        map <infinint, etoile *>::iterator it;
        infinint tmp;
        bool read_crc = (ptr != NULL) && !f.crc_status();

        if(read_crc)
            f.reset_crc(ENTREE_CRC_SIZE);

        try
        {
            S_I lu = f.read(&type, 1);

            if(lu == 0)
                return ret;

            if(!extract_base_and_status((unsigned char)type, (unsigned char &)type, saved))
            {
                if(!lax)
                    throw Erange("entree::read", gettext("corrupted file"));
                else
                    return ret;
            }

            switch(type)
            {
            case 'f':
                ret = new (nothrow) file(dialog, f, reading_ver, saved, default_algo, data_loc, ea_loc, ptr);
                break;
            case 'l':
                ret = new (nothrow) lien(dialog, f, reading_ver, saved, ea_loc, ptr);
                break;
            case 'c':
                ret = new (nothrow) chardev(dialog, f, reading_ver, saved, ea_loc, ptr);
                break;
            case 'b':
                ret = new (nothrow) blockdev(dialog, f, reading_ver, saved, ea_loc, ptr);
                break;
            case 'p':
                ret = new (nothrow) tube(dialog, f, reading_ver, saved, ea_loc, ptr);
                break;
            case 's':
                ret = new (nothrow) prise(dialog, f, reading_ver, saved, ea_loc, ptr);
                break;
            case 'd':
                ret = new (nothrow) directory(dialog, f, reading_ver, saved, stats, corres, default_algo, data_loc, ea_loc, lax, only_detruit, ptr);
                break;
            case 'm':
                ret = new (nothrow) mirage(dialog, f, reading_ver, saved, stats, corres, default_algo, data_loc, ea_loc, mirage::fmt_mirage, lax, ptr);
                break;
            case 'h': // old hard-link object
                ret = new (nothrow) mirage(dialog, f, reading_ver, saved, stats, corres, default_algo, data_loc, ea_loc, mirage::fmt_hard_link, lax, ptr);
                break;
            case 'e': // old etiquette object
                ret = new (nothrow) mirage(dialog, f, reading_ver, saved, stats, corres, default_algo, data_loc, ea_loc, lax, ptr);
                break;
            case 'z':
                if(saved != s_saved)
                {
                    if(!lax)
                        throw Erange("entree::read", gettext("corrupted file"));
                    else
                        dialog.warning(gettext("LAX MODE: Unexpected saved status for end of directory entry, assuming data corruption occurred, ignoring and continuing"));
                }
                ret = new (nothrow) eod(f);
                break;
            case 'x':
                if(saved != s_saved)
                {
                    if(!lax)
                        throw Erange("entree::read", gettext("corrupted file"));
                    else
                        dialog.warning(gettext("LAX MODE: Unexpected saved status for class \"detruit\" object, assuming data corruption occurred, ignoring and continuing"));
                }
                ret = new (nothrow) detruit(f, reading_ver);
                break;
	    case 'o':
		ret = new (nothrow) door(dialog, f, reading_ver, saved, default_algo, data_loc, ea_loc, ptr);
		break;
            default :
                if(!lax)
                    throw Erange("entree::read", gettext("unknown type of data in catalogue"));
                else
                {
                    dialog.warning(gettext("LAX MODE: found unknown catalogue entry, assuming data corruption occurred, cannot read further the catalogue as I do not know the length of this type of entry"));
                    return ret;  // NULL
                }
            }
	    if(ret == NULL)
		throw Ememory("entree::read");
        }
        catch(...)
        {
            if(read_crc)
            {
		crc * tmp = f.get_crc(); // keep f in a coherent status
		if(tmp != NULL)
		    delete tmp;
            }
            throw;
        }

        if(read_crc)
        {
            crc *crc_calc = f.get_crc();

	    if(crc_calc == NULL)
		throw SRC_BUG;

	    try
	    {
		crc *crc_read = create_crc_from_file(f);
		if(crc_read == NULL)
		    throw SRC_BUG;

		try
		{
		    if(*crc_read != *crc_calc)
		    {
			nomme * ret_nom = dynamic_cast<nomme *>(ret);
			string nom = ret_nom != NULL ? ret_nom->get_name() : "";

			try
			{
			    if(!lax)
				throw Erange("", "temporary exception");
			    else
			    {
				if(nom == "")
				    nom = gettext("unknown entry");
				dialog.pause(tools_printf(gettext("Entry information CRC failure for %S. Ignore the failure?"), &nom));
			    }
			}
			catch(Egeneric & e) // we catch here the temporary exception and the Euser_abort thrown by dialog.pause()
			{
			    if(nom != "")
				throw Erange("entree::read", tools_printf(gettext("Entry information CRC failure for %S"), &nom));
			    else
				throw Erange("entree::read", gettext(gettext("Entry information CRC failure")));
			}
		    }
		    ret->post_constructor(*ptr);
		}
		catch(...)
		{
		    if(crc_read != NULL)
			delete crc_read;
		    throw;
		}
		if(crc_read != NULL)
		    delete crc_read;
	    }
	    catch(...)
	    {
		if(crc_calc != NULL)
		    delete crc_calc;
		throw;
	    }
	    if(crc_calc != NULL)
		delete crc_calc;
	}

        stats.add(ret);
        return ret;
    }

    void entree::dump(generic_file & f, bool small) const
    {
        if(small)
        {
	    crc *tmp = NULL;

	    try
	    {
		f.reset_crc(ENTREE_CRC_SIZE);
		try
		{
		    inherited_dump(f, small);
		}
		catch(...)
		{
		    tmp = f.get_crc(); // keep f in a coherent status
		    throw;
		}

		tmp = f.get_crc();
		if(tmp == NULL)
		    throw SRC_BUG;

		tmp->dump(f);
	    }
	    catch(...)
	    {
		if(tmp != NULL)
		    delete tmp;
		throw;
	    }
	    if(tmp != NULL)
		delete tmp;
        }
        else
            inherited_dump(f, small);
    }

    void entree::inherited_dump(generic_file & f, bool small) const
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

    void nomme::inherited_dump(generic_file & f, bool small) const
    {
        entree::inherited_dump(f, small);
        tools_write_string(f, xname);
    }

    const ea_attributs inode::empty_ea;

    inode::inode(const infinint & xuid, const infinint & xgid, U_16 xperm,
                 const infinint & last_access,
                 const infinint & last_modif,
                 const infinint & last_change,
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
        ea_crc = NULL;
        last_acc = NULL;
        last_mod = NULL;
        ea_offset = NULL;
        ea_size = 0;
        last_cha = NULL;
        fs_dev = NULL;
        storage = NULL;
        esc = NULL;
        edit = 0;

        try
        {
            last_acc = new (nothrow) infinint(last_access);
            last_mod = new (nothrow) infinint(last_modif);
            last_cha = new (nothrow) infinint(last_change);
            ea_offset = new (nothrow) infinint(0);
            fs_dev = new (nothrow) infinint(fs_device);
            if(last_acc == NULL || last_mod == NULL || ea_offset == NULL || last_cha == NULL || fs_dev == NULL)
                throw Ememory("inde::inode");
        }
        catch(...)
        {
            if(last_acc != NULL)
            {
                delete last_acc;
                last_acc = NULL;
            }
            if(last_mod != NULL)
            {
                delete last_mod;
                last_mod = NULL;
            }
            if(ea_offset != NULL)
            {
                delete ea_offset;
                ea_offset = NULL;
            }
            if(last_cha != NULL)
            {
                delete last_cha;
                last_cha = NULL;
            }
            if(fs_dev != NULL)
            {
                delete fs_dev;
                fs_dev = NULL;
            }
            throw;
        }
    }

    inode::inode(user_interaction & dialog,
                 generic_file & f,
                 const archive_version & reading_ver,
                 saved_status saved,
                 generic_file *ea_loc,
                 escape *ptr) : nomme(f)
    {
        U_16 tmp;
        unsigned char flag;

        xsaved = saved;
        edit = reading_ver;
        esc = ptr;
        last_acc = NULL;
        last_mod = NULL;
        last_cha = NULL;
        ea_offset = NULL;
        fs_dev = NULL;
        ea_crc = NULL;

        if(reading_ver > 1)
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
            case INODE_FLAG_EA_REMO:
                ea_saved = ea_removed;
                break;
            default:
                throw Erange("inode::inode", gettext("badly structured inode: unknown inode flag"));
            }
        }
        else
            ea_saved = ea_none;

        if(reading_ver <= 7)
        {
            if(f.read((char *)&tmp, sizeof(tmp)) != sizeof(tmp))
                throw Erange("inode::inode", gettext("missing data to build an inode"));
            uid = ntohs(tmp);
            if(f.read((char *)&tmp, sizeof(tmp)) != sizeof(tmp))
                throw Erange("inode::inode", gettext("missing data to build an inode"));
            gid = ntohs(tmp);
        }
        else // archive format >= "08"
        {
            uid = infinint(f);
            gid = infinint(f);
        }

        if(f.read((char *)&tmp, sizeof(tmp)) != sizeof(tmp))
            throw Erange("inode::inode", gettext("missing data to build an inode"));
        perm = ntohs(tmp);

        try
        {
            fs_dev = new (nothrow) infinint(0); // the filesystemID is not saved in archive
            last_acc = new (nothrow) infinint(f);
            last_mod = new (nothrow) infinint(f);
            if(reading_ver >= 8)
            {
                last_cha = new (nothrow) infinint(f);
                if(last_acc == NULL || last_mod == NULL || last_cha == NULL)
                    throw Ememory("inode::inode(file)");

                if(ea_saved == ea_full)
                    ea_size = infinint(f);
            }
            else // archive format <= 7
            {
                if(last_acc == NULL || last_mod == NULL)
                    throw Ememory("inode::inode(file)");
                ea_size = 0; // meaning EA size unknown (old format)
            }

            if(esc == NULL) // reading a full entry from catalogue
            {
                switch(ea_saved)
                {
                case ea_full:
                    ea_offset = new (nothrow) infinint(f);
                    if(ea_offset == NULL)
                        throw Ememory("inode::inode(file)");

		    if(reading_ver <= 7)
		    {
			ea_crc = create_crc_from_file(f, true);
			if(ea_crc == NULL)
			    throw SRC_BUG;

			last_cha = new (nothrow) infinint(f);
			if(last_cha == NULL)
			    throw Ememory("inode::inode(file)");
		    }
		    else // archive format >= 8
		    {
			ea_crc = create_crc_from_file(f, false);
			if(ea_crc == NULL)
			    throw SRC_BUG;
		    }
                    break;
                case ea_partial:
                case ea_fake:
                    ea_offset = new (nothrow) infinint(0);
                    if(ea_offset == NULL)
                        throw Ememory("inode::inode(file)");
                    if(reading_ver <= 7)
                    {
                        last_cha = new (nothrow) infinint(f);
                        if(last_cha == NULL)
                            throw Ememory("inode::inode(file)");
                    }
                    break;
                case ea_none:
                case ea_removed:
                    ea_offset = new (nothrow) infinint(0);
                    if(ea_offset == NULL)
                        throw Ememory("inode::inode(file)");
                    if(reading_ver <= 7)
                    {
                        last_cha = new (nothrow) infinint(0);
                        if(last_cha == NULL)
                            throw Ememory("inode::inode(file)");
                    }
                    break;
                default:
                    throw SRC_BUG;
                }
            }
            else // reading a small dump using escape sequence marks
            {
                    // header version is greater than or equal to "08" (small dump appeared at
                    // this version of archive format) ea_offset ea_CRC have been dumped a bit
                    // further in that case, for now we set them manually to a default value,
                    // and will fetch their real value upon request by get_ea() or ea_get_crc()
                    // methods

                ea_offset = new (nothrow) infinint(0);
                if(ea_offset == NULL)
                    throw Ememory("inode::inode(file)");
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
            {
                delete last_acc;
                last_acc = NULL;
            }
            if(last_mod != NULL)
            {
                delete last_mod;
                last_mod = NULL;
            }
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
            if(fs_dev != NULL)
            {
                delete fs_dev;
                fs_dev = NULL;
            }
	    if(ea_crc != NULL)
	    {
		delete ea_crc;
		ea_crc = NULL;
	    }
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
        esc = ref.esc;
        ea_size = ref.ea_size;

        last_acc = NULL;
        last_mod = NULL;
        last_cha = NULL;
        ea_offset = NULL;
        ea = NULL;
        ea_crc = NULL;
        fs_dev = NULL;
        try
        {
            edit = ref.edit;
            last_acc = new (nothrow) infinint(*ref.last_acc);
            last_mod = new (nothrow) infinint(*ref.last_mod);
            last_cha = new (nothrow) infinint(*ref.last_cha);
            fs_dev = new (nothrow) infinint(*ref.fs_dev);
            if(last_acc == NULL || last_mod == NULL || last_cha == NULL || fs_dev == NULL)
                throw Ememory("inode::inode(inode)");

            switch(ea_saved)
            {
            case ea_full:
                ea_offset = new (nothrow) infinint(*ref.ea_offset);
                if(ea_offset == NULL)
                    throw Ememory("inode::inode(inode)");
                if(ref.ea_crc != NULL)
                {
                    ea_crc = ref.ea_crc->clone();
                    if(ea_crc == NULL)
                        throw Ememory("inode::inode(inode)");
                }

                if(ref.ea != NULL) // might be NULL if build from a file
                {
                    ea = new (nothrow) ea_attributs(*ref.ea);
                    if(ea == NULL)
                        throw Ememory("inode::inode(const inode &)");
                }
                else
                    ea = NULL;
                break;
            case ea_partial:
            case ea_fake:
            case ea_none:
            case ea_removed:
                ea_offset = new (nothrow) infinint(0);
                if(ea_offset == NULL)
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
            {
                delete last_acc;
                last_acc = NULL;
            }
            if(last_mod != NULL)
            {
                delete last_mod;
                last_mod = NULL;
            }
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
            if(last_cha != NULL)
            {
                delete last_cha;
                last_cha = NULL;
            }
            if(ea_crc != NULL)
            {
                delete ea_crc;
                ea_crc = NULL;
            }
            if(fs_dev != NULL)
            {
                delete fs_dev;
                fs_dev = NULL;
            }
            throw;
        }
    }

    const inode & inode::operator = (const inode & ref)
    {
        nomme *me = this;
        const nomme *nref = &ref;

        *me = *nref; // copying the "nomme" part of the object

            // some sanity checks

        if(last_acc == NULL)
            throw SRC_BUG;
        if(last_mod == NULL)
            throw SRC_BUG;
        if(ea_offset == NULL)
            throw SRC_BUG;
        if(last_cha == NULL)
            throw SRC_BUG;
        if(fs_dev == NULL)
            throw SRC_BUG;
        if(ref.last_acc == NULL)
            throw SRC_BUG;
        if(ref.last_mod == NULL)
            throw SRC_BUG;
        if(ref.ea_offset == NULL)
            throw SRC_BUG;
        if(ref.last_cha == NULL)
            throw SRC_BUG;
        if(ref.fs_dev == NULL)
            throw SRC_BUG;
            // storage is a pointer to an independant object it may be NULL, thus we do not check its value.

            // now copying the inode part
        esc = ref.esc; // may be NULL, no control needed, we just copy the possible address to an external escape object (which must exist).
        uid = ref.uid;
        gid = ref.gid;
        perm = ref.perm;
        *last_acc = *ref.last_acc;
        *last_mod = *ref.last_mod;
        xsaved = ref.xsaved;
        ea_saved = ref.ea_saved;
        ea_size = ref.ea_size;
        *ea_offset = *ref.ea_offset;
        if(ea != NULL)
            if(ref.ea == NULL)
            {
                delete ea;
                ea = NULL;
            }
            else
                *ea = *ref.ea;
        else // ea == NULL
            if(ref.ea != NULL)
            {
                ea = new (nothrow) ea_attributs(*ref.ea);
                if(ea == NULL)
                    throw Ememory("inode::operator =");
            }
            // else nothing to do (both ea and ref.ea are NULL)
        *last_cha = *ref.last_cha;
        if(ea_crc != NULL)
            if(ref.ea_crc == NULL)
            {
                delete ea_crc;
                ea_crc = NULL;
            }
            else // both ea_crc and ref.ea_crc not NULL
		if(ea_crc->get_size() == ref.ea_crc->get_size())
		    *ea_crc = *(ref.ea_crc);
		else
		{
		    delete ea_crc;
		    ea_crc = ref.ea_crc->clone();
		}
        else // ea_crc == NULL
            if(ref.ea_crc != NULL)
            {
                ea_crc = ref.ea_crc->clone();
                if(ea_crc == NULL)
                    throw Ememory("inode::operator =");
            }
            // else both ea_crc and ref.ea_crc are NULL, nothing to do.

        *fs_dev = *ref.fs_dev;
        storage = ref.storage; // yes, we copy the address not the object
        edit = ref.edit;

        return *this;
    }

    inode::~inode()
    {
            // we must not release memory pointed to by esc, we do not own that object
        if(last_acc != NULL)
        {
            delete last_acc;
            last_acc = NULL;
        }
        if(last_mod != NULL)
        {
            delete last_mod;
            last_mod = NULL;
        }
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
        if(last_cha != NULL)
        {
            delete last_cha;
            last_cha = NULL;
        }
        if(ea_crc != NULL)
        {
            delete ea_crc;
            ea_crc = NULL;
        }
        if(fs_dev != NULL)
        {
            delete fs_dev;
            fs_dev = NULL;
        }
    }

    bool inode::same_as(const inode & ref) const
    {
        return nomme::same_as(ref) && compatible_signature(ref.signature(), signature());
    }

    bool inode::is_more_recent_than(const inode & ref, const infinint & hourshift) const
    {
        return *ref.last_mod < *last_mod && !tools_is_equal_with_hourshift(hourshift, *ref.last_mod, *last_mod);
    }

    bool inode::has_changed_since(const inode & ref, const infinint & hourshift, comparison_fields what_to_check) const
    {
        return (what_to_check != cf_inode_type && (hourshift > 0 ? ! tools_is_equal_with_hourshift(hourshift, *ref.last_mod, *last_mod) : *ref.last_mod != *last_mod))
            || (what_to_check == cf_all && uid != ref.uid)
            || (what_to_check == cf_all && gid != ref.gid)
            || (what_to_check != cf_mtime && what_to_check != cf_inode_type && perm != ref.perm);
    }

    void inode::compare(const inode &other,
                        const mask & ea_mask,
                        comparison_fields what_to_check,
                        const infinint & hourshift,
			bool symlink_date) const
    {
	bool do_mtime_test = dynamic_cast<const lien *>(&other) == NULL || symlink_date;

        if(!same_as(other))
            throw Erange("inode::compare",gettext("different file type"));
        if(what_to_check == cf_all && get_uid() != other.get_uid())
	{
	    infinint u1 = get_uid();
	    infinint u2 = other.get_uid();
            throw Erange("inode.compare", tools_printf(gettext("different owner (uid): %i <--> %i"), &u1, &u2));
	}
        if(what_to_check == cf_all && get_gid() != other.get_gid())
	{
	    infinint g1 = get_gid();
	    infinint g2 = other.get_gid();
            throw Erange("inode.compare", tools_printf(gettext("different owner group (gid): %i <--> %i"), &g1, &g2));
	}
        if((what_to_check == cf_all || what_to_check == cf_ignore_owner) && get_perm() != other.get_perm())
	{
	    string p1 = tools_int2octal(get_perm());
	    string p2 = tools_int2octal(other.get_perm());
            throw Erange("inode.compare", tools_printf(gettext("different permission: %S <--> %S"), &p1, &p2));
	}
        if(do_mtime_test
	   && (what_to_check == cf_all || what_to_check == cf_ignore_owner || what_to_check == cf_mtime)
           && !tools_is_equal_with_hourshift(hourshift, get_last_modif(), other.get_last_modif()))
	{
	    string s1 = tools_display_date(get_last_modif());
	    string s2 = tools_display_date(other.get_last_modif());
            throw Erange("inode.compare", tools_printf(gettext("difference of last modification date: %S <--> %S"), &s1, &s2));
	}

        sub_compare(other);

        switch(ea_get_saved_status())
        {
        case ea_full:
            if(other.ea_get_saved_status() == ea_full)
            {
                const ea_attributs *me = get_ea(); // this pointer must not be freed
                const ea_attributs *you = other.get_ea(); // this pointer must not be freed neither
                if(me->diff(*you, ea_mask))
                    throw Erange("inode::compare", gettext("different Extended Attributes"));
            }
            else
            {
#ifdef EA_SUPPORT
                throw Erange("inode::compare", gettext("no Extended Attribute to compare with"));
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
                if(!tools_is_equal_with_hourshift(get_last_change(), other.get_last_change(), hourshift)
                   && get_last_change() < other.get_last_change())
                    throw Erange("inode::compare", gettext("inode last change date (ctime) greater, EA might be different"));
            }
            else
            {
#ifdef EA_SUPPORT
                throw Erange("inode::compare", gettext("no Extended Attributes to compare with"));
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
    }

    void inode::inherited_dump(generic_file & r, bool small) const
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
        nomme::inherited_dump(r, small);

        r.write((char *)(&flag), 1);
        uid.dump(r);
        gid.dump(r);
        tmp = htons(perm);
        r.write((char *)&tmp, sizeof(tmp));
        if(last_acc == NULL)
            throw SRC_BUG;
        last_acc->dump(r);
        if(last_mod == NULL)
            throw SRC_BUG;
        last_mod->dump(r);
        if(last_cha == NULL)
            throw SRC_BUG;
        last_cha->dump(r);
        if(ea_saved == ea_full)
            ea_get_size().dump(r);

        if(!small)
        {
            switch(ea_saved)
            {
            case ea_full:
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
    }

    void inode::ea_set_saved_status(ea_status status)
    {
        if(status == ea_saved)
            return;
        switch(status)
        {
        case ea_none:
        case ea_removed:
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
        {
            ea = ref;
            ea_size = ea->space_used();
        }
        else
            throw SRC_BUG;
        if(ea_saved != ea_full)
            throw SRC_BUG;
    }

    const ea_attributs *inode::get_ea() const
    {
        switch(ea_saved)
        {
        case ea_full:
            if(ea != NULL)
                return ea;
            else
                if(storage != NULL) // ea_offset may be equal to zero if the first inode saved had only EA modified since archive of reference
                {
		    crc *val = NULL;
		    const crc *my_crc = NULL;

		    try
		    {
			if(esc == NULL)
			    storage->skip(*ea_offset);
			else
			{
			    if(!esc->skip_to_next_mark(escape::seqt_ea, false))
				throw Erange("inode::get_ea", string("Error while fetching EA from archive: No escape mark found for that file"));
			    storage->skip(esc->get_position()); // required to eventually reset the compression engine
			}

			if(ea_get_size() == 0)
			    storage->reset_crc(crc::OLD_CRC_SIZE);
			else
			    storage->reset_crc(tools_file_size_to_crc_size(ea_get_size()));

			try
			{
			    try
			    {
				if(edit <= 1)
				    throw SRC_BUG;   // EA do not exist in that archive format
				const_cast<ea_attributs *&>(ea) = new (nothrow) ea_attributs(*storage, edit);
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
			    throw Erange("inode::get_ea", gettext("CRC error detected while reading EA"));
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

    void inode::ea_detach() const
    {
        if(ea != NULL)
        {
            delete ea;
            const_cast<ea_attributs *&>(ea) = NULL;
        }
    }

    infinint inode::ea_get_size() const
    {
        if(ea_saved == ea_full)
        {
            if(ea_size == 0) // reading an old archive
            {
                if(ea != NULL)
                    const_cast<inode *>(this)->ea_size = ea->space_used();
                    // else we stick with value 0
            }
            return ea_size;
        }
        else
            throw SRC_BUG;
    }

    void inode::ea_set_crc(const crc & val)
    {
	if(ea_crc != NULL)
	{
	    delete ea_crc;
	    ea_crc = NULL;
	}
	ea_crc = val.clone();
	if(ea_crc == NULL)
	    throw Ememory("inode::ea_set_crc");
    }

    void inode::ea_get_crc(const crc * & ptr) const
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
                        tmp = create_crc_from_file(*esc, false);
                    else // archive format <= 7
                        tmp = create_crc_from_file(*esc, true);
		    if(tmp == NULL)
			throw SRC_BUG;
                    const_cast<inode *>(this)->ea_crc = tmp;
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
                crc *tmp = new (nothrow) crc_n(1); // creating a default CRC
                if(tmp == NULL)
                    throw Ememory("inode::ea_get_crc");
                try
                {
                    tmp->clear();
                    const_cast<inode *>(this)->ea_crc = tmp;
                        // this is to avoid trying to fetch the CRC a new time if decision
                        // has been taken to continue the operation after the exception
                        // thrown below has been catched.
		    tmp = NULL;  // the object is now owned by "this"
                }
                catch(...)
                {
                    delete tmp;
                    throw;
                }
                throw Erange("inode::ea_get_crc", gettext("Error while reading CRC for EA from the archive: No escape mark found for that file"));
            }
        }

        if(ea_crc == NULL)
            throw SRC_BUG;
        else
            ptr = ea_crc;
    }

    bool inode::ea_get_crc_size(infinint & val) const
    {
        if(ea_crc != NULL)
        {
            val = ea_crc->get_size();
            return true;
        }
        else
            return false;
    }

    infinint inode::get_last_change() const
    {
        if(last_cha == NULL)
            throw SRC_BUG;
        else
            return *last_cha;
    }

    void inode::set_last_change(const infinint & x_time)
    {
        if(last_cha == NULL)
            throw SRC_BUG;
        else
            *last_cha = x_time;
    }

    etoile::etoile(inode *host, const infinint & etiquette_number)
    {
        if(host == NULL)
            throw SRC_BUG;
        if(dynamic_cast<directory *>(host) != NULL)
            throw Erange("etoile::etoile", gettext("Hard links of directories are not supported"));
        hosted = host;
        etiquette = etiquette_number;
	refs.clear();
    }

    void etoile::add_ref(void *ref)
    {
	if(find(refs.begin(), refs.end(), ref) != refs.end())
	    throw SRC_BUG; // this reference is already known

	refs.push_back(ref);
    }

    void etoile::drop_ref(void *ref)
    {
	list<void *>::iterator it = find(refs.begin(), refs.end(), ref);

	if(it == refs.end())
	    throw SRC_BUG; // cannot drop a reference that does not exist

	refs.erase(it);
	if(refs.size() == 0)
	    delete this;
    }

    mirage::mirage(user_interaction & dialog,
                   generic_file & f,
                   const archive_version & reading_ver,
                   saved_status saved,
                   entree_stats & stats,
                   std::map <infinint, etoile *> & corres,
                   compression default_algo,
                   generic_file *data_loc,
                   generic_file *ea_loc,
                   mirage_format fmt,
                   bool lax,
                   escape *ptr) : nomme(f)
    {
        init(dialog,
             f,
             reading_ver,
             saved,
             stats,
             corres,
             default_algo,
             data_loc,
             ea_loc,
             fmt,
             lax,
             ptr);
    }

    mirage::mirage(user_interaction & dialog,
                   generic_file & f,
                   const archive_version & reading_ver,
                   saved_status saved,
                   entree_stats & stats,
                   std::map <infinint, etoile *> & corres,
                   compression default_algo,
                   generic_file *data_loc,
                   generic_file *ea_loc,
                   bool lax,
                   escape *ptr) : nomme("TEMP")
    {
        init(dialog,
             f,
             reading_ver,
             saved,
             stats,
             corres,
             default_algo,
             data_loc,
             ea_loc,
             fmt_file_etiquette,
             lax,
             ptr);
    }

    void mirage::init(user_interaction & dialog,
                      generic_file & f,
                      const archive_version & reading_ver,
                      saved_status saved,
                      entree_stats & stats,
                      std::map <infinint, etoile *> & corres,
                      compression default_algo,
                      generic_file *data_loc,
                      generic_file *ea_loc,
                      mirage_format fmt,
                      bool lax,
                      escape *ptr)
    {
        infinint tmp_tiquette;
        char tmp_flag;
        map<infinint, etoile *>::iterator etl;
        inode *ino_ptr = NULL;
        entree *entree_ptr = NULL;
        entree_stats fake_stats; // the call to entree_read will increment counters with the inode we will read
            // but this inode will also be counted from the entree_read we are call from.
            // thus we must not increment the real entree_stats structure here


        if(fmt != fmt_file_etiquette)
            tmp_tiquette = infinint(f);

        switch(fmt)
        {
        case fmt_mirage:
            f.read(&tmp_flag, 1);
            break;
        case fmt_hard_link:
            tmp_flag = MIRAGE_ALONE;
            break;
        case fmt_file_etiquette:
            tmp_flag = MIRAGE_WITH_INODE;
            break;
        default:
            throw SRC_BUG;
        }

        switch(tmp_flag)
        {
        case MIRAGE_ALONE:

                // we must link with the already existing etoile

            etl = corres.find(tmp_tiquette);
            if(etl == corres.end())
                throw Erange("mirage::mirage", gettext("Incoherent catalogue structure: hard linked inode's data not found"));
            else
            {
                if(etl->second == NULL)
                    throw SRC_BUG;
                star_ref = etl->second;
                star_ref->add_ref(this);
            }
            break;
        case MIRAGE_WITH_INODE:

                // we first read the attached inode

            if(fmt == fmt_file_etiquette)
            {
                nomme *tmp_ptr = new (nothrow) file(dialog, f, reading_ver, saved, default_algo, data_loc, ea_loc, ptr);
                entree_ptr = tmp_ptr;
                if(tmp_ptr != NULL)
                {
                    change_name(tmp_ptr->get_name());
                    tmp_ptr->change_name("");
                    tmp_tiquette = infinint(f);
                }
                else
                    throw Ememory("mirage::init");
            }
            else
                entree_ptr = entree::read(dialog, f, reading_ver, fake_stats, corres, default_algo, data_loc, ea_loc, lax, false, ptr);

            ino_ptr = dynamic_cast<inode *>(entree_ptr);
            if(ino_ptr == NULL || dynamic_cast<directory *>(entree_ptr) != NULL)
            {
                if(entree_ptr != NULL)
                {
                    delete entree_ptr;
                    entree_ptr = NULL;
                }
                throw Erange("mirage::mirage", gettext("Incoherent catalogue structure: hard linked data is not an inode"));
            }

                // then we can bind the inode to the next to be create etoile object

            try
            {
                    // we must check that an already exiting etoile is not present

                etl = corres.find(tmp_tiquette);
                if(etl == corres.end())
                {
                        // we can now create the etoile and add it in the corres map;

                    star_ref = new (nothrow) etoile(ino_ptr, tmp_tiquette);
                    try
                    {
                        if(star_ref == NULL)
                            throw Ememory("mirage::mirage");
                        ino_ptr = NULL; // the object pointed to by ino_ptr is now managed by star_ref
                        star_ref->add_ref(this);
                        corres[tmp_tiquette] = star_ref;
                    }
                    catch(...)
                    {
                        if(star_ref != NULL)
                        {
                            delete star_ref;
                            star_ref = NULL;
                        }
                        etl = corres.find(tmp_tiquette);
                        if(etl != corres.end())
                            corres.erase(etl);
                        throw;
                    }
                }
                else
                    throw Erange("mirage::mirage", gettext("Incoherent catalogue structure: duplicated hard linked inode's data"));
            }
            catch(...)
            {
                if(ino_ptr != NULL)
                {
                    delete ino_ptr;
                    ino_ptr = NULL;
                }
                throw;
            }

            break;
        default:
            throw Erange("mirage::mirage", gettext("Incoherent catalogue structure: unknown status flag for hard linked inode"));
        }
    }

    const mirage & mirage::operator = (const mirage & ref)
    {
        etoile *tmp_ref;
        const nomme * ref_nom = & ref;
        nomme * this_nom = this;
        *this_nom = *ref_nom; // copying the nomme part of these objects

        if(ref.star_ref == NULL)
            throw SRC_BUG;
        tmp_ref = star_ref;
        star_ref = ref.star_ref;
        star_ref->add_ref(this);
        tmp_ref->drop_ref(this);

        return *this;
    }

    void mirage::post_constructor(generic_file & f)
    {
        if(star_ref == NULL)
            throw SRC_BUG;

        if(star_ref->get_ref_count() == 1) // first time this inode is seen
            star_ref->get_inode()->post_constructor(f);
    }

    void mirage::inherited_dump(generic_file & f, bool small) const
    {
        if(star_ref->get_ref_count() > 1)
        {
            char buffer[] = { MIRAGE_ALONE, MIRAGE_WITH_INODE };
            nomme::inherited_dump(f, small);
            star_ref->get_etiquette().dump(f);
            if((small && !is_inode_wrote())
               || (!small && !is_inode_dumped()))
            {
                f.write(buffer+1, 1); // writing one char MIRAGE_WITH_INODE
                star_ref->get_inode()->specific_dump(f, small);
                if(!small)
                    set_inode_dumped(true);
            }
            else
                f.write(buffer, 1); // writing one char MIRAGE_ALONE
        }
        else // no need to record this inode with the hard link overhead
        {
            inode *real = star_ref->get_inode();
            real->change_name(get_name()); // set the name of the mirage object to the inode
            real->specific_dump(f, small);
        }
    }


    file::file(const infinint & xuid, const infinint & xgid, U_16 xperm,
               const infinint & last_access,
               const infinint & last_modif,
               const infinint & last_change,
               const string & src,
               const path & che,
               const infinint & taille,
               const infinint & fs_device,
               bool x_furtive_read_mode) : inode(xuid, xgid, xperm, last_access, last_modif, last_change, src, fs_device)
    {
        chemin = (che + src).display();
        status = from_path;
        set_saved_status(s_saved);
        offset = NULL;
        size = NULL;
        storage_size = NULL;
        loc = NULL; // field not used for backup
        algo_read = none; // field not used for backup
        algo_write = none; // may be set later by change_compression_algo_write()
        furtive_read_mode = x_furtive_read_mode;
        file_data_status_read = 0;
        file_data_status_write = 0;
        check = NULL;
        dirty = false;

        try
        {
            offset = new (nothrow) infinint(0);
            size = new (nothrow) infinint(taille);
            storage_size = new (nothrow) infinint(0);
            if(offset == NULL || size == NULL || storage_size == NULL)
                throw Ememory("file::file");
        }
        catch(...)
        {
            if(offset != NULL)
            {
                delete offset;
                offset = NULL;
            }
            if(size != NULL)
            {
                delete size;
                size = NULL;
            }
            if(storage_size != NULL)
            {
                delete storage_size;
                storage_size = NULL;
            }
            throw;
        }
    }

    file::file(user_interaction & dialog,
               generic_file & f,
               const archive_version & reading_ver,
               saved_status saved,
               compression default_algo,
               generic_file *data_loc,
               generic_file *ea_loc,
               escape *ptr) : inode(dialog, f, reading_ver, saved, ea_loc, ptr)
    {
        chemin = "";
        status = from_cat;
        size = NULL;
        offset = NULL;
        storage_size = NULL;
        check = NULL;
        algo_read = default_algo; // only used for archive format "03" and older
        algo_write = default_algo; // may be changed later using change_compression_algo_write()
        furtive_read_mode = false; // no used in that "status" mode
        file_data_status_read = 0;
        file_data_status_write = 0; // may be changed later using set_sparse_file_detection_write()
        loc = data_loc;
        dirty = false;
        try
        {
            size = new (nothrow) infinint(f);
            if(size == NULL)
                throw Ememory("file::file(generic_file)");

            if(ptr == NULL) // inode not partially dumped
            {
                if(saved == s_saved)
                {
                    offset = new (nothrow) infinint(f);
                    if(offset == NULL)
                        throw Ememory("file::file(generic_file)");
                    if(reading_ver > 1)
                    {
                        storage_size = new (nothrow) infinint(f);
                        if(storage_size == NULL)
                            throw Ememory("file::file(generic_file)");
                        if(reading_ver > 7)
                        {
                            char tmp;

                            f.read(&file_data_status_read, sizeof(file_data_status_read));
                            if((file_data_status_read & FILE_DATA_IS_DIRTY) != 0)
                            {
                                dirty = true;
                                file_data_status_read &= ~FILE_DATA_IS_DIRTY; // removing the flag DIRTY flag
                            }
                            file_data_status_write = file_data_status_read;
                            f.read(&tmp, sizeof(tmp));
                            algo_read = char2compression(tmp);
			    algo_write= algo_read;
                        }
                        else
                            if(*storage_size == 0) // in older archive storage_size was set to zero if data was not compressed
                            {
                                *storage_size = *size;
                                algo_read = none;
				algo_write = algo_read;
                            }
                            else
			    {
                                algo_read = default_algo;
				algo_write= algo_read;
			    }
                    }
                    else // version is "01"
                    {
                        storage_size = new (nothrow) infinint(*size);
                        if(storage_size == NULL)
                            throw Ememory("file::file(generic_file)");
                        *storage_size *= 2;
                            // compressed file should be less larger than twice
                            // the original file
                            // (in case the compression is very bad
                            // and takes more place than no compression)
                    }

                    if(reading_ver >= 8)
                    {
			check = create_crc_from_file(f);
                        if(check == NULL)
                            throw Ememory("file::file");
                    }
                        // before version 8, crc was dump in any case, not only when data was saved
                }
                else // not saved
                {
                    offset = new (nothrow) infinint(0);
                    storage_size = new (nothrow) infinint(0);
                    if(offset == NULL || storage_size == NULL)
                        throw Ememory("file::file(generic_file)");
                }

                if(reading_ver >= 2)
                {
                    if(reading_ver < 8)
                    {
                            // fixed length CRC inversion from archive format "02" to "07"
                            // present in any case, even when data is not saved
                            // for archive version >= 8, the crc is only present
                            // if the archive does contain file data

                        check = create_crc_from_file(f, true);
                        if(check == NULL)
                            throw Ememory("file::file");
                    }
                        // archive version >= 8, crc only present if  saved == s_saved (seen above)
                }
                else // no CRC in version "01"
                    check = NULL;
            }
            else // partial dump has been done
            {
                if(saved == s_saved)
                {
                    char tmp;

                    f.read(&file_data_status_read, sizeof(file_data_status_read));
                    file_data_status_write = file_data_status_read;
                    f.read(&tmp, sizeof(tmp));
                    algo_read = char2compression(tmp);
		    algo_write = algo_read;
                }

                    // Now that all data has been read, setting default value for the undumped ones:

                if(saved == s_saved)
                    offset = new (nothrow) infinint(0); // can only be set from post_constructor
                else
                    offset = new (nothrow) infinint(0);
                if(offset == NULL)
                    throw Ememory("file::file(generic_file)");

                storage_size = new (nothrow) infinint(0); // cannot known the storage_size at that time
                if(storage_size == NULL)
                    throw Ememory("file::file(generic_file)");

                check = NULL;
            }
        }
        catch(...)
        {
            detruit();
            throw;
        }
    }

    void file::post_constructor(generic_file & f)
    {
        if(offset == NULL)
            throw SRC_BUG;
        else
            *offset = f.get_position(); // data follows right after the inode+file information+CRC
    }

    file::file(const file & ref) : inode(ref)
    {
        status = ref.status;
        chemin = ref.chemin;
        offset = NULL;
        size = NULL;
        storage_size = NULL;
        check = NULL;
        dirty = ref.dirty;
        loc = ref.loc;
        algo_read = ref.algo_read;
        algo_write = ref.algo_write;
        furtive_read_mode = ref.furtive_read_mode;
        file_data_status_read = ref.file_data_status_read;
        file_data_status_write = ref.file_data_status_write;

        try
        {
            dirty = ref.dirty;

            if(ref.check != NULL || get_escape_layer() != NULL)
            {
		if(ref.check == NULL)
		{
		    const crc *tmp = NULL;
		    ref.get_crc(tmp);
		    if(ref.check == NULL) // failed to read the crc from escape layer
			throw SRC_BUG;
		}
		check = ref.check->clone();
                if(check == NULL)
                    throw Ememory("file::file(file)");
            }
            else
                check = NULL;
            offset = new (nothrow) infinint(*ref.offset);
            size = new (nothrow) infinint(*ref.size);
            storage_size = new (nothrow) infinint(*ref.storage_size);
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
        {
            delete offset;
            offset = NULL;
        }
        if(size != NULL)
        {
            delete size;
            size = NULL;
        }
        if(storage_size != NULL)
        {
            delete storage_size;
            storage_size = NULL;
        }
        if(check != NULL)
        {
            delete check;
            check = NULL;
        }
    }

    void file::inherited_dump(generic_file & f, bool small) const
    {
        inode::inherited_dump(f, small);
        size->dump(f);
        if(!small)
        {
            if(get_saved_status() == s_saved)
            {
                char tmp = compression2char(algo_write);
                char flags = file_data_status_write;

                offset->dump(f);
                storage_size->dump(f);
                if(dirty)
                    flags |= FILE_DATA_IS_DIRTY;
                (void)f.write(&flags, sizeof(flags));
                (void)f.write(&tmp, sizeof(tmp));

                    // since archive version 8, crc is only present for saved inode
                if(check == NULL)
                    throw SRC_BUG; // no CRC to dump!
                else
                    check->dump(f);
            }
        }
        else // we only know whether the file will be compressed or using sparse_file data structure
        {
            if(get_saved_status() == s_saved)
            {
                char tmp = compression2char(algo_write);

                (void)f.write(&file_data_status_write, sizeof(file_data_status_write));
                (void)f.write(&tmp, sizeof(tmp));
            }
        }
    }

    bool file::has_changed_since(const inode & ref, const infinint & hourshift, inode::comparison_fields what_to_check) const
    {
        const file *tmp = dynamic_cast<const file *>(&ref);
        if(tmp != NULL)
            return inode::has_changed_since(*tmp, hourshift, what_to_check) || *size != *(tmp->size);
        else
            throw SRC_BUG;
    }

    generic_file *file::get_data(get_data_mode mode) const
    {
        generic_file *ret = NULL;

        if(get_saved_status() != s_saved)
            throw Erange("file::get_data", gettext("cannot provide data from a \"not saved\" file object"));

        if(status == empty)
            throw Erange("file::get_data", gettext("data has been cleaned, object is now empty"));

	try
	{
	    if(status == from_path)
	    {
		if(mode != normal && mode != plain)
		    throw SRC_BUG; // keep compressed/keep_hole is not possible on an inode take from a filesystem
		ret = new (nothrow) fichier(chemin, furtive_read_mode);
	    }
	    else // inode from archive
		if(loc == NULL)
		    throw SRC_BUG; // set_archive_localisation never called or with a bad argument
		else
		    if(loc->get_mode() == gf_write_only)
			throw SRC_BUG; // cannot get data from a write-only file !!!
		    else
		    {
			pile *data = new (nothrow) pile();
			if(data == NULL)
			    throw Ememory("file::get_data");

			try
			{
			    generic_file *tmp;

			    if(get_escape_layer() == NULL)
				tmp = new (nothrow) tronc(loc, *offset, *storage_size, gf_read_only);
			    else
				tmp = new (nothrow) tronc(get_escape_layer(), *offset, gf_read_only);
			    if(tmp == NULL)
				throw Ememory("file::get_data");
			    try
			    {
				data->push(tmp);
			    }
			    catch(...)
			    {
				delete tmp;
				throw;
			    }
			    data->skip(0); // set the reading cursor at the beginning

			    if(*size > 0 && get_compression_algo_read() != none && mode != keep_compressed)
			    {
				tmp = new (nothrow) compressor(get_compression_algo_read(), *data->top());
				if(tmp == NULL)
				    throw Ememory("file::get_data");
				try
				{
				    data->push(tmp);
				}
				catch(...)
				{
				    delete tmp;
				    throw;
				}
			    }

			    if(get_sparse_file_detection_read() && mode != keep_compressed && mode != keep_hole)
			    {
				sparse_file *stmp = new (nothrow) sparse_file(data->top());
				if(stmp == NULL)
				    throw Ememory("file::get_data");
				try
				{
				    data->push(stmp);
				}
				catch(...)
				{
				    delete stmp;
				    throw;
				}

				switch(mode)
				{
				case keep_compressed:
				case keep_hole:
				    throw SRC_BUG;
				case normal:
				    break;
				case plain:
				    stmp->copy_to_without_skip(true);
				    break;
				default:
				    throw SRC_BUG;
				}
			    }

			    ret = data;
			}
			catch(...)
			{
			    delete data;
			    throw;
			}
		    }
	}
	catch(...)
	{
	    if(ret != NULL)
		delete ret;
	    ret = NULL;
	    throw;
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
            chemin = ""; // smallest possible memory allocation
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

    const infinint & file::get_offset() const
    {
        if(offset == NULL)
            throw SRC_BUG;
        return *offset;
    }

    void file::set_crc(const crc &c)
    {
	if(check != NULL)
	{
	    delete check;
	    check = NULL;
	}
	check = c.clone();
	if(check == NULL)
	    throw Ememory("file::set_crc");
    }

    bool file::get_crc(const crc * & c) const
    {
        if(get_escape_layer() == NULL)
            if(check != NULL)
            {
                c = check;
                return true;
            }
            else
                return false;
        else
        {
            if(get_saved_status() == s_saved)
            {
                if(check == NULL)
                {
		    try
		    {
			if(get_escape_layer()->skip_to_next_mark(escape::seqt_file_crc, false))
			{
			    crc *tmp = NULL;

				// first, recording storage_size (needed when isolating a catalogue in sequential read mode)
			    if(*storage_size == 0)
			    {
				infinint pos = get_escape_layer()->get_position();
				if(pos < *offset)
				    throw SRC_BUG;
				else
				    *storage_size = pos - *offset;
			    }
			    else
				throw SRC_BUG; // how is this possible ??? it should always be zero in sequential read mode !

			    tmp = create_crc_from_file(*(get_escape_layer()));
			    if(tmp == NULL)
				throw SRC_BUG;
			    else
			    {
				const_cast<file *>(this)->check = tmp;
				tmp = NULL; // object now owned by "this"
			    }
			}
			else
			    throw Erange("file::file", gettext("can't read data CRC: No escape mark found for that file"));
		    }
		    catch(...)
		    {
			    // we assign a default crc to the object
			    // to avoid trying reading it again later on
			if(check == NULL)
			{
			    const_cast<file *>(this)->check = new (nothrow) crc_n(1);
			    if(check == NULL)
				throw Ememory("file::file");
			}
			throw;
		    }
                }

                if(check == NULL)
                    throw SRC_BUG; // should not be NULL now!
                else
                    c = check;
                return true;
            }
            else
                return false;
        }
    }

    bool file::get_crc_size(infinint & val) const
    {
        if(check != NULL)
        {
            val = check->get_size();
            return true;
        }
        else
            return false;
    }

    void file::sub_compare(const inode & other) const
    {
        const file *f_other = dynamic_cast<const file *>(&other);
        if(f_other == NULL)
            throw SRC_BUG; // inode::compare should have called us with a correct argument

        if(get_size() != f_other->get_size())
	{
	    infinint s1 = get_size();
	    infinint s2 = f_other->get_size();
            throw Erange("file::sub_compare", tools_printf(gettext("not same size: %i <--> %i"), &s1, &s2));
	}

        if(get_saved_status() == s_saved && f_other->get_saved_status() == s_saved)
        {
            generic_file *me = get_data(normal);
            if(me == NULL)
                throw SRC_BUG;
            try
            {
                generic_file *you = f_other->get_data(normal);
                if(you == NULL)
                    throw SRC_BUG;
                try
                {
                    crc *value = NULL;
                    const crc *original = NULL;
		    infinint crc_size;

		    if(has_crc())
		    {
			if(get_crc(original))
			{
			    if(original == NULL)
				throw SRC_BUG;
			    crc_size = original->get_size();
			}
			else
			    throw SRC_BUG; // has a crc but cannot get it?!?
		    }
		    else // we must not fetch the crc yet, especially when perfoming a sequential read
			crc_size = tools_file_size_to_crc_size(f_other->get_size());

		    try
		    {
			infinint err_offset;
			if(me->diff(*you, crc_size, value, err_offset))
			    throw Erange("file::sub_compare", tools_printf(gettext("different file data, offset of first difference is: %i"), &err_offset));
			else
			{
			    if(original != NULL)
			    {
				if(value == NULL)
				    throw SRC_BUG;
				if(original->get_size() != value->get_size())
				    throw Erange("file::sub_compare", gettext("Same data but CRC value could not be verified because we did not guessed properly its width (sequential read restriction)"));
				if(*original != *value)
				    throw Erange("file::sub_compare", gettext("Same data but stored CRC does not match the data!?!"));
			    }
			}
		    }
		    catch(...)
		    {
			if(value != NULL)
			    delete value;
			throw;
		    }
		    if(value != NULL)
			delete value;
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

    generic_file *door::get_data(get_data_mode mode) const
    {
	generic_file *ret = NULL;

	if(status == from_path)
	{
	    ret = new (nothrow) null_file(gf_read_only);
	    if(ret == NULL)
		throw Ememory("door::get_data");
	}
	else
	    ret = file::get_data(mode);

	return ret;
    }

    lien::lien(const infinint & uid, const infinint & gid, U_16 perm,
               const infinint & last_access,
               const infinint & last_modif,
               const infinint & last_change,
               const string & name,
               const string & target,
               const infinint & fs_device) : inode(uid, gid, perm, last_access, last_modif, last_change, name, fs_device)
    {
        points_to = target;
        set_saved_status(s_saved);
    }

    lien::lien(user_interaction & dialog,
               generic_file & f,
               const archive_version & reading_ver,
               saved_status saved,
               generic_file *ea_loc,
               escape *ptr) : inode(dialog, f, reading_ver, saved, ea_loc, ptr)
    {
        if(saved == s_saved)
            tools_read_string(f, points_to);
    }

    const string & lien::get_target() const
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

    void lien::sub_compare(const inode & other) const
    {
        const lien *l_other = dynamic_cast<const lien *>(&other);
        if(l_other == NULL)
            throw SRC_BUG; // bad argument inode::compare has a bug

        if(get_saved_status() == s_saved && l_other->get_saved_status() == s_saved)
            if(get_target() != l_other->get_target())
                throw Erange("lien:sub_compare", string(gettext("symbolic link does not point to the same target: ")) + get_target() + " <--> " + l_other->get_target());
    }

    void lien::inherited_dump(generic_file & f, bool small) const
    {
        inode::inherited_dump(f, small);
        if(get_saved_status() == s_saved)
            tools_write_string(f, points_to);
    }

    const eod directory::fin;

    directory::directory(const infinint & xuid, const infinint & xgid, U_16 xperm,
                         const infinint & last_access,
                         const infinint & last_modif,
                         const infinint & last_change,
                         const string & xname,
                         const infinint & fs_device) : inode(xuid, xgid, xperm, last_access, last_modif, last_change, xname, fs_device)
    {
        parent = NULL;
#ifdef LIBDAR_FAST_DIR
        fils.clear();
#endif
        ordered_fils.clear();
        it = ordered_fils.begin();
        set_saved_status(s_saved);
        recursive_has_changed = true;
    }

    directory::directory(const directory &ref) : inode(ref)
    {
        parent = NULL;
#ifdef LIBDAR_FAST_DIR
        fils.clear();
#endif
        ordered_fils.clear();
        it = ordered_fils.begin();
        recursive_has_changed = ref.recursive_has_changed;
    }

    const directory & directory::operator = (const directory & ref)
    {
        const inode *ref_ino = &ref;
        inode * this_ino = this;

        *this_ino = *ref_ino; // this assigns the inode part of the object
            // we don't modify the existing subfiles or subdirectories nor we copy them from the reference directory
        return *this;
    }


    directory::directory(user_interaction & dialog,
                         generic_file & f,
                         const archive_version & reading_ver,
                         saved_status saved,
                         entree_stats & stats,
                         std::map <infinint, etoile *> & corres,
                         compression default_algo,
                         generic_file *data_loc,
                         generic_file *ea_loc,
                         bool lax,
                         bool only_detruit,
                         escape *ptr) : inode(dialog, f, reading_ver, saved, ea_loc, ptr)
    {
        entree *p;
        nomme *t;
        directory *d;
        detruit *x;
        mirage *m;
        eod *fin = NULL;
        bool lax_end = false;

        parent = NULL;
#ifdef LIBDAR_FAST_DIR
        fils.clear();
#endif
        ordered_fils.clear();
        recursive_has_changed = true; // need to call recursive_has_changed_update() first if this fields has to be used

        try
        {
            while(fin == NULL && !lax_end)
            {
                try
                {
                    p = entree::read(dialog, f, reading_ver, stats, corres, default_algo, data_loc, ea_loc, lax, only_detruit, ptr);
                }
                catch(Euser_abort & e)
                {
                    throw;
                }
                catch(Ethread_cancel & e)
                {
                    throw;
                }
                catch(Egeneric & e)
                {
                    if(!lax)
                        throw;
                    else
                    {
                        dialog.warning(string(gettext("LAX MODE: Error met building a catalogue entry, skipping this entry and continuing. Skipped error is: ")) + e.get_message());
                        p = NULL;
                    }
                }

                if(p != NULL)
                {
                    d = dynamic_cast<directory *>(p);
                    fin = dynamic_cast<eod *>(p);
                    t = dynamic_cast<nomme *>(p);
                    x = dynamic_cast<detruit *>(p);
                    m = dynamic_cast<mirage *>(p);

                    if(!only_detruit || d != NULL || x != NULL || fin != NULL || m != NULL)
                    {
                            // we must add the mirage object, else
                            // we will trigger an incoherent catalogue structure
                            // as the mirages without inode cannot link to the mirage with inode
                            // carring the same etiquette if we destroy them right now.
                        if(t != NULL) // p is a "nomme"
                        {
#ifdef LIBDAR_FAST_DIR
                            fils[t->get_name()] = t;
#endif
                            ordered_fils.push_back(t);
                        }
                        if(d != NULL) // p is a directory
                            d->parent = this;
                        if(t == NULL && fin == NULL)
                            throw SRC_BUG; // neither an eod nor a nomme ! what's that ???
                    }
                    else
                    {
                        delete p;
                        p = NULL;
                        d = NULL;
                        fin = NULL;
                        t = NULL;
                        x = NULL;
                    }
                }
                else
                    if(!lax)
                        throw Erange("directory::directory", gettext("missing data to build a directory"));
                    else
                        lax_end = true;
            }
            if(fin != NULL)
            {
                delete fin; // no need to keep it
                fin = NULL;
            }

            it = ordered_fils.begin();
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

    void directory::inherited_dump(generic_file & f, bool small) const
    {
        list<nomme *>::const_iterator x = ordered_fils.begin();
        inode::inherited_dump(f, small);

        if(!small)
        {
            while(x != ordered_fils.end())
            {
                if(*x == NULL)
                    throw SRC_BUG;
                if(dynamic_cast<ignored *>(*x) != NULL)
                    ++x; // "ignored" need not to be saved, they are only useful when updating_destroyed
                else
                {
                    (*x)->specific_dump(f, small);
                    ++x;
                }
            }
        }
            // else in small mode, we do not dump any children
            // an inode may have children while small dump is asked
            // when performing a merging operation

        fin.specific_dump(f, small); // end of "this" directory
            // fin is a static constant variable of class directory,
            // this hack avoids recurrent construction/destruction of a eod object.
    }

    void directory::add_children(nomme *r)
    {
        directory *d = dynamic_cast<directory *>(r);
	nomme *ancien_nomme;

        if(r == NULL)
            throw SRC_BUG;

	if(search_children(r->get_name(), ancien_nomme))  // same entry already present
        {
            directory *a_dir = dynamic_cast<directory *>(ancien_nomme);

            if(a_dir != NULL && d != NULL) // both directories : merging them
            {
                a_dir = d; // updates the inode part, does not touch the directory specific part as defined in the directory::operator =
                list<nomme *>::iterator xit = d->ordered_fils.begin();
                while(xit != d->ordered_fils.end())
                {
                    a_dir->add_children(*xit);
                    ++xit;
                }

		    // need to clear the lists of objects before destroying the directory objects itself
		    // to avoid the destructor destroyed the director children that have been merged to the a_dir directory
#ifdef LIBDAR_FAST_DIR
                d->fils.clear();
#endif
		d->ordered_fils.clear();
                delete r;
                r = NULL;
                d = NULL;
            }
            else // not directories: removing and replacing old entry
            {
                if(ancien_nomme == NULL)
                    throw SRC_BUG;

		    // removing the old object
		remove(ancien_nomme->get_name());
		ancien_nomme = NULL;

                    // adding the new object
#ifdef LIBDAR_FAST_DIR
                fils[r->get_name()] = r;
#endif
                ordered_fils.push_back(r);
            }
        }
        else // no conflict: adding
        {
#ifdef LIBDAR_FAST_DIR
            fils[r->get_name()] = r;
#endif
            ordered_fils.push_back(r);
        }

        if(d != NULL)
            d->parent = this;
    }

    void directory::reset_read_children() const
    {
        directory *moi = const_cast<directory *>(this);
        moi->it = moi->ordered_fils.begin();
    }

    void directory::end_read() const
    {
        directory *moi = const_cast<directory *>(this);
        moi->it = moi->ordered_fils.end();
    }

    bool directory::read_children(const nomme *&r) const
    {
        directory *moi = const_cast<directory *>(this);
        if(moi->it != moi->ordered_fils.end())
        {
            r = *(moi->it);
            ++(moi->it);
            return true;
        }
        else
            return false;
    }

    void directory::tail_to_read_children()
    {
#ifdef LIBDAR_FAST_DIR
        map<string, nomme *>::iterator dest;
	list<nomme *>::iterator ordered_dest = it;

        while(ordered_dest != ordered_fils.end())
        {
	    try
	    {
		if(*ordered_dest == NULL)
		    throw SRC_BUG;
		dest = fils.find((*ordered_dest)->get_name());
		fils.erase(dest);
		delete *ordered_dest;
		*ordered_dest = NULL;
		ordered_dest++;
	    }
	    catch(...)
	    {
		ordered_fils.erase(it, ordered_dest);
		throw;
	    }
        }
#endif
        ordered_fils.erase(it, ordered_fils.end());
        it = ordered_fils.end();
    }

    void directory::remove(const string & name)
    {

                    // localizing old object in ordered_fils
	list<nomme *>::iterator ot = ordered_fils.begin();

	while(ot != ordered_fils.end() && *ot != NULL && (*ot)->get_name() != name)
	    ++ot;

	if(ot == ordered_fils.end())
            throw Erange("directory::remove", tools_printf(gettext("Cannot remove nonexistent entry %S from catalogue"), &name));

	if(*ot == NULL)
	    throw SRC_BUG;


#ifdef LIBDAR_FAST_DIR
	    // localizing old object in fils
	map<string, nomme *>::iterator ut = fils.find(name);
	if(ut == fils.end())
	    throw SRC_BUG;

	    // sanity checks
	if(*ot != ut->second)
	    throw SRC_BUG;

	    // removing reference from fils
	fils.erase(ut);
#endif

	    // recoding the address of the object to remove
	nomme *obj = *ot;

	    // removing its reference from ordered_fils
	ordered_fils.erase(ot);

	    // destroying the object itself
	delete obj;
        reset_read_children();
    }

    void directory::clear()
    {
        it = ordered_fils.begin();
        while(it != ordered_fils.end())
        {
            if(*it == NULL)
                throw SRC_BUG;
            delete *it;
            *it = NULL;
            ++it;
        }
#ifdef LIBDAR_FAST_DIR
        fils.clear();
#endif
        ordered_fils.clear();
        it = ordered_fils.begin();
    }

    bool directory::search_children(const string &name, nomme * & ptr)
    {
#ifdef LIBDAR_FAST_DIR
        map<string, nomme *>::iterator ut = fils.find(name);

        if(ut != fils.end())
        {
            if(ut->second == NULL)
                throw SRC_BUG;
            ptr = ut->second;
	    if(ptr == NULL)
		throw SRC_BUG;
        }
        else
            ptr = NULL;
#else
	list<nomme *>::iterator ot = ordered_fils.begin();

	while(ot != ordered_fils.end() && *ot != NULL && (*ot)->get_name() != name)
	    ++ot;

	if(ot != ordered_fils.end())
	{
	    ptr = *ot;
	    if(ptr == NULL)
		throw SRC_BUG;
	}
	else
	    ptr = NULL;
#endif
        return ptr != NULL;
    }

    bool directory::callback_for_children_of(user_interaction & dialog, const string & sdir, bool isolated) const
    {
        const directory *current = this;
        const nomme *next_nom = NULL;
        const directory *next_dir = NULL;
        const inode *next_ino = NULL;
        const detruit *next_detruit = NULL;
        const mirage *next_mir = NULL;
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
                    next_mir = dynamic_cast<const mirage *>(next_nom);
                    if(next_mir != NULL)
                        next_dir = dynamic_cast<const directory *>(next_mir->get_inode());
                    else
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
            next_mir = dynamic_cast<const mirage *>(next_nom);
            if(next_mir != NULL)
                next_ino = next_mir->get_inode();
            else
                next_ino = dynamic_cast<const inode *>(next_nom);
            next_detruit = dynamic_cast<const detruit *>(next_nom);
            next_dir = dynamic_cast<const directory *>(next_ino);
            if(next_ino != NULL)
            {
                string a = local_perm(*next_ino, next_mir != NULL);
                string b = local_uid(*next_ino);
                string c = local_gid(*next_ino);
                string d = local_size(*next_ino);
                string e = local_date(*next_ino);
                string f = local_flag(*next_ino, isolated, false);
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
        list<nomme *>::const_iterator it = ordered_fils.begin();

        const_cast<directory *>(this)->recursive_has_changed = false;
        while(it != ordered_fils.end())
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
                    || ino->ea_get_saved_status() == ea_full
                    || ino->ea_get_saved_status() == ea_removed;
            ++it;
        }
    }

    infinint directory::get_tree_size() const
    {
        infinint ret = ordered_fils.size();
	const directory *fils_dir = NULL;

        list<nomme *>::const_iterator ot = ordered_fils.begin();
        while(ot != ordered_fils.end())
        {
	    if(*ot == NULL)
		throw SRC_BUG;
            fils_dir = dynamic_cast<const directory *>(*ot);
            if(fils_dir != NULL)
                ret += fils_dir->get_tree_size();

            ++ot;
        }

        return ret;
    }

    infinint directory::get_tree_ea_num() const
    {
        infinint ret = 0;

        list<nomme *>::const_iterator it = ordered_fils.begin();

        while(it != ordered_fils.end())
        {
            const directory *fils_dir = dynamic_cast<const directory *>(*it);
            const inode *fils_ino = dynamic_cast<const inode *>(*it);
            const mirage *fils_mir = dynamic_cast<const mirage *>(*it);

            if(fils_mir != NULL)
                fils_ino = fils_mir->get_inode();

            if(fils_ino != NULL)
                if(fils_ino->ea_get_saved_status() != ea_none && fils_ino->ea_get_saved_status() != ea_removed)
                    ++ret;
            if(fils_dir != NULL)
                ret += fils_dir->get_tree_ea_num();

            ++it;
        }


        return ret;
    }

    infinint directory::get_tree_mirage_num() const
    {
        infinint ret = 0;

        list<nomme *>::const_iterator it = ordered_fils.begin();

        while(it != ordered_fils.end())
        {
            const directory *fils_dir = dynamic_cast<const directory *>(*it);
            const mirage *fils_mir = dynamic_cast<const mirage *>(*it);

            if(fils_mir != NULL)
                ++ret;

            if(fils_dir != NULL)
                ret += fils_dir->get_tree_mirage_num();

            ++it;
        }


        return ret;
    }

    void directory::get_etiquettes_found_in_tree(map<infinint, infinint> & already_found) const
    {
        list<nomme *>::const_iterator it = ordered_fils.begin();

        while(it != ordered_fils.end())
        {
            const mirage *fils_mir = dynamic_cast<const mirage *>(*it);
            const directory *fils_dir = dynamic_cast<const directory *>(*it);

            if(fils_mir != NULL)
            {
                map<infinint, infinint>::iterator tiq = already_found.find(fils_mir->get_etiquette());
                if(tiq == already_found.end())
                    already_found[fils_mir->get_etiquette()] = 1;
                else
                    already_found[fils_mir->get_etiquette()] = tiq->second + 1;
                    // due to st::map implementation, it is not recommanded to modify an entry directly
                    // using a "pair" structure (the one that holds .first and .second fields)
            }

            if(fils_dir != NULL)
                fils_dir->get_etiquettes_found_in_tree(already_found);

            ++it;
        }
    }

    void directory::remove_all_mirages_and_reduce_dirs()
    {
        list<nomme *>::iterator curs = ordered_fils.begin();

        while(curs != ordered_fils.end())
        {
            if(*curs == NULL)
                throw SRC_BUG;
            directory *d = dynamic_cast<directory *>(*curs);
            mirage *m = dynamic_cast<mirage *>(*curs);
            nomme *n = dynamic_cast<nomme *>(*curs);

		// sanity check
            if((m != NULL && n == NULL) || (d != NULL && n == NULL))
                throw SRC_BUG;

		// recursive call
            if(d != NULL)
                d->remove_all_mirages_and_reduce_dirs();

            if(m != NULL || (d != NULL && d->is_empty()))
            {
#ifdef LIBDAR_FAST_DIR
                map<string, nomme *>::iterator monfils = fils.find(n->get_name());

                if(monfils == fils.end())
                    throw SRC_BUG;
                if(monfils->second != *curs)
                    throw SRC_BUG;
                fils.erase(monfils);
#endif
                curs = ordered_fils.erase(curs);
                    // curs now points to the next item
                delete n;
            }
            else
                ++curs;
        }
    }

    device::device(const infinint & uid, const infinint & gid, U_16 perm,
                   const infinint & last_access,
                   const infinint & last_modif,
                   const infinint & last_change,
                   const string & name,
                   U_16 major,
                   U_16 minor,
                   const infinint & fs_dev) : inode(uid, gid, perm, last_access, last_modif, last_change, name, fs_dev)
    {
        xmajor = major;
        xminor = minor;
        set_saved_status(s_saved);
    }

    device::device(user_interaction & dialog,
                   generic_file & f,
                   const archive_version & reading_ver,
                   saved_status saved,
                   generic_file *ea_loc,
                   escape *ptr) : inode(dialog, f, reading_ver, saved, ea_loc, ptr)
    {
        U_16 tmp;

        if(saved == s_saved)
        {
            if(f.read((char *)&tmp, sizeof(tmp)) != sizeof(tmp))
                throw Erange("special::special", gettext("missing data to build a special device"));
            xmajor = ntohs(tmp);
            if(f.read((char *)&tmp, sizeof(tmp)) != sizeof(tmp))
                throw Erange("special::special", gettext("missing data to build a special device"));
            xminor = ntohs(tmp);
        }
    }

    void device::inherited_dump(generic_file & f, bool small) const
    {
        U_16 tmp;

        inode::inherited_dump(f, small);
        if(get_saved_status() == s_saved)
        {
            tmp = htons(xmajor);
            f.write((char *)&tmp, sizeof(tmp));
            tmp = htons(xminor);
            f.write((char *)&tmp, sizeof(tmp));
        }
    }

    void device::sub_compare(const inode & other) const
    {
        const device *d_other = dynamic_cast<const device *>(&other);
        if(d_other == NULL)
            throw SRC_BUG; // bug in inode::compare
        if(get_saved_status() == s_saved && d_other->get_saved_status() == s_saved)
        {
            if(get_major() != d_other->get_major())
                throw Erange("device::sub_compare", tools_printf(gettext("devices have not the same major number: %d <--> %d"), get_major(), d_other->get_major()));
            if(get_minor() != d_other->get_minor())
                throw Erange("device::sub_compare", tools_printf(gettext("devices have not the same minor number: %d <--> %d"), get_minor(), d_other->get_minor()));
        }
    }

    detruit::detruit(generic_file & f, const archive_version & reading_ver) : nomme(f)
    {
        if(f.read((char *)&signe, 1) != 1)
            throw Erange("detruit::detruit", gettext("missing data to build"));

        if(reading_ver > 7)
            del_date.read(f);
        else
            del_date = 0;
    }

    void detruit::inherited_dump(generic_file & f, bool small) const
    {
        nomme::inherited_dump(f, small);
        f.write((char *)&signe, 1);
        del_date.dump(f);
    }

    void ignored_dir::inherited_dump(generic_file & f, bool small) const
    {
        directory tmp = directory(get_uid(), get_gid(), get_perm(), get_last_access(), get_last_modif(), get_last_change(), get_name(), 0);
        tmp.set_saved_status(get_saved_status());
        tmp.specific_dump(f, small); // dump an empty directory
    }

    catalogue::catalogue(user_interaction & dialog, const infinint & root_last_modif, const label & data_name) : mem_ui(dialog), out_compare("/")
    {
        contenu = NULL;

        try
        {
            contenu = new (nothrow) directory(0,0,0,0,root_last_modif,0,"root",0);
            if(contenu == NULL)
                throw Ememory("catalogue::catalogue(path)");
            current_compare = contenu;
            current_add = contenu;
            current_read = contenu;
            sub_tree = NULL;
            ref_data_name = data_name;
        }
        catch(...)
        {
            if(contenu != NULL)
                delete contenu;
            throw;
        }

        stats.clear();
    }

    catalogue::catalogue(user_interaction & dialog,
                         generic_file & ff,
                         const archive_version & reading_ver,
                         compression default_algo,
                         generic_file *data_loc,
                         generic_file *ea_loc,
                         bool lax,
                         const label & lax_layer1_data_name,
                         bool only_detruit) : mem_ui(dialog), out_compare("/")
    {
        string tmp;
        unsigned char a;
        saved_status st;
        unsigned char base;
        map <infinint, etoile *> corres;
        crc *calc_crc = NULL;
	crc *read_crc = NULL;
        contenu = NULL;

        try
        {
            ff.reset_crc(CAT_CRC_SIZE);
            try
            {
                if(reading_ver > 7)
                {
                        // we first need to read the ref_data_name
		    try
		    {
			ref_data_name.read(ff);
		    }
		    catch(Erange & e)
		    {
                        throw Erange("catalogue::catalogue(generic_file &)", gettext("incoherent catalogue structure"));
		    }

                }
                else
                    ref_data_name.clear(); // a cleared data_name is emulated for older archives

                if(lax)
                {
                    if(ref_data_name != lax_layer1_data_name && !lax_layer1_data_name.is_cleared())
                    {
                        dialog.warning(gettext("LAX MODE: catalogue label does not match archive label, as if it was an extracted catalogue, assuming data corruption occurred and fixing the catalogue to be considered an a plain internal catalogue"));
                        ref_data_name = lax_layer1_data_name;
                    }
                }

                ff.read((char *)&a, 1); // need to read the signature before constructing "contenu"
                if(! extract_base_and_status(a, base, st) && !lax)
                    throw Erange("catalogue::catalogue(generic_file &)", gettext("incoherent catalogue structure"));
                if(base != 'd' && !lax)
                    throw Erange("catalogue::catalogue(generic_file &)", gettext("incoherent catalogue structure"));

                stats.clear();
                contenu = new (nothrow) directory(dialog, ff, reading_ver, st, stats, corres, default_algo, data_loc, ea_loc, lax, only_detruit, NULL);
                if(contenu == NULL)
                    throw Ememory("catalogue::catalogue(path)");
                if(only_detruit)
                    contenu->remove_all_mirages_and_reduce_dirs();
                current_compare = contenu;
                current_add = contenu;
                current_read = contenu;
                sub_tree = NULL;
            }
            catch(...)
            {
                calc_crc = ff.get_crc(); // keeping "f" in coherent status
		if(calc_crc != NULL)
		{
		    delete calc_crc;
		    calc_crc = NULL;
		}
                throw;
            }
            calc_crc = ff.get_crc(); // keeping "f" incoherent status in any case
	    if(calc_crc == NULL)
		throw SRC_BUG;

            if(reading_ver > 7)
            {
                bool force_crc_failure = false;

                try
                {
		    read_crc = create_crc_from_file(ff);
                }
                catch(Egeneric & e)
                {
                    force_crc_failure = true;
                }

                if(force_crc_failure || read_crc == NULL || calc_crc == NULL || read_crc->get_size() != calc_crc->get_size() || *read_crc != *calc_crc)
                {
                    if(!lax)
                        throw Erange("catalogue::catalogue(generic_file &)", gettext("CRC failed for table of contents (aka \"catalogue\")"));
                    else
                        dialog.pause(gettext("LAX MODE: CRC failed for catalogue, the archive contents is corrupted. This may even lead dar to see files in the archive that never existed, but this will most probably lead to other failures in restoring files. Shall we proceed anyway?"));
                }
            }
        }
        catch(...)
        {
            if(contenu != NULL)
                delete contenu;
	    if(calc_crc != NULL)
		delete calc_crc;
	    if(read_crc != NULL)
		delete read_crc;
            throw;
        }
	    // "contenu" must not be destroyed under normal terminaison!
	if(calc_crc != NULL)
	    delete calc_crc;
	if(read_crc != NULL)
	    delete read_crc;
    }

    const catalogue & catalogue::operator = (const catalogue &ref)
    {
        mem_ui * me = this;
        const mem_ui & you = ref;

        detruire();
        if(me == NULL)
            throw SRC_BUG;
        *me = you; // copying the mem_ui data

            // now copying the catalogue's data

        out_compare = ref.out_compare;
        partial_copy_from(ref);

        return *this;
    }

    void catalogue::reset_read() const
    {
        catalogue *ceci = const_cast<catalogue *>(this);
        ceci->current_read = ceci->contenu;
        ceci->contenu->reset_read_children();
    }

    void catalogue::end_read() const
    {
        catalogue *ceci = const_cast<catalogue *>(this);
        ceci->current_read = ceci->contenu;
        ceci->contenu->end_read();
    }

    void catalogue::skip_read_to_parent_dir() const
    {
        catalogue *ceci = const_cast<catalogue *>(this);
        directory *tmp = ceci->current_read->get_parent();

        if(tmp == NULL)
            throw Erange("catalogue::skip_read_to_parent_dir", gettext("root does not have a parent directory"));
        ceci->current_read = tmp;
    }

    bool catalogue::read(const entree * & ref) const
    {
        catalogue *ceci = const_cast<catalogue *>(this);
        const nomme *tmp;

        if(ceci->current_read->read_children(tmp))
        {
            const directory *dir = dynamic_cast<const directory *>(tmp);
            if(dir != NULL)
            {
                ceci->current_read = const_cast<directory *>(dir);
                dir->reset_read_children();
            }
            ref = tmp;
            return true;
        }
        else
        {
            directory *papa = ceci->current_read->get_parent();
            ref = & r_eod;
            if(papa == NULL)
                return false; // we reached end of root, no eod generation
            else
            {
                ceci->current_read = papa;
                return true;
            }
        }
    }

    bool catalogue::read_if_present(string *name, const nomme * & ref) const
    {
        catalogue *ceci = const_cast<catalogue *>(this);
        nomme *tmp;

        if(ceci->current_read == NULL)
            throw Erange("catalogue::read_if_present", gettext("no current directory defined"));
        if(name == NULL) // we have to go to parent directory
        {
            if(ceci->current_read->get_parent() == NULL)
                throw Erange("catalogue::read_if_present", gettext("root directory has no parent directory"));
            else
                ceci->current_read = ceci->current_read->get_parent();
            ref = NULL;
            return true;
        }
        else // looking for a real filename
            if(ceci->current_read->search_children(*name, tmp))
            {
                directory *d = dynamic_cast<directory *>(tmp);
                if(d != NULL) // this is a directory need to chdir to it
                    ceci->current_read = d;
                ref = tmp;
                return true;
            }
            else // filename not present in current dir
                return false;
    }

    void catalogue::remove_read_entry(std::string & name)
    {
        if(current_read == NULL)
            throw Erange("catalogue::remove_read_entry", gettext("no current reading directory defined"));
        current_read->remove(name);
    }

    void catalogue::tail_catalogue_to_current_read()
    {
        while(current_read != NULL)
        {
            current_read->tail_to_read_children();
            current_read = current_read->get_parent();
        }

        current_read = contenu;
    }

    void catalogue::reset_sub_read(const path &sub)
    {
        if(! sub.is_relative())
            throw SRC_BUG;

        if(sub_tree != NULL)
            delete sub_tree;
        sub_tree = new (nothrow) path(sub);
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
                            get_ui().warning(sub_tree->display() + gettext(" is not present in the archive"));
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
                    get_ui().warning(sub_tree->display() + gettext(" is not present in the archive"));
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
            if(parent == NULL)
                throw SRC_BUG; // root has no parent directory, cannot change to it
            else
                current_add = parent;
            delete ref; // all data given throw add becomes owned by the catalogue object
        }
    }

    void catalogue::re_add_in(const string &subdirname)
    {
        nomme *sub = NULL;

        if(current_add->search_children(subdirname, sub))
        {
            directory *subdir = dynamic_cast<directory *>(sub);
            if(subdir != NULL)
                current_add = subdir;
            else
                throw Erange("catalogue::re_add_in", gettext("Cannot recurs in a non directory entry"));
        }
        else
            throw Erange("catalogue::re_add_in", gettext("The entry to recurs in does not exist, cannot add further entry to that absent subdirectory"));
    }

    void catalogue::re_add_in_replace(const directory &dir)
    {
        if(dir.has_children())
            throw Erange("catalogue::re_add_in_replace", "Given argument must be an empty dir");
        re_add_in(dir.get_name());
        *current_add = dir; // the directory's 'operator =' method does preverse existing children of the left (assigned) operand
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
        const mirage *mir = dynamic_cast<const mirage *>(target);
        const directory *dir = dynamic_cast<const directory *>(target);
        const eod *fin = dynamic_cast<const eod *>(target);
        const nomme *nom = dynamic_cast<const nomme *>(target);

        if(mir != NULL)
            dir = dynamic_cast<const directory *>(mir->get_inode());

        if(out_compare.degre() > 1) // actually scanning a nonexisting directory
        {
            if(dir != NULL)
                out_compare += dir->get_name();
            else
                if(fin != NULL)
                {
                    string tmp_s;

                    if(!out_compare.pop(tmp_s))
                    {
                        if(out_compare.is_relative())
                            throw SRC_BUG; // should not be a relative path !!!
                        else // both cases are bugs, but need to know which case is generating a bug
                            throw SRC_BUG; // out_compare.degre() > 0 but cannot pop !
                    }
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
                const mirage *src_mir = dynamic_cast<const mirage *>(nom);
                const mirage *dst_mir = dynamic_cast<const mirage *>(found);

                    // extracting inode from hard links
                if(src_mir != NULL)
                    src_ino = src_mir->get_inode();
                if(dst_mir != NULL)
                    dst_ino = dst_mir->get_inode();

                    // updating internal structure to follow directory tree :
                if(dir != NULL)
                {
                    const directory *d_ext = dynamic_cast<const directory *>(dst_ino);
                    if(d_ext != NULL)
                        current_compare = const_cast<directory *>(d_ext);
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

                if(dst_mir != NULL)
                    extracted = dst_mir->get_inode();
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
        const mirage *pro_mir;
        infinint count = 0;

        ref.reset_read();
        while(ref.read(projo))
        {
            pro_eod = dynamic_cast<const eod *>(projo);
            pro_dir = dynamic_cast<const directory *>(projo);
            pro_det = dynamic_cast<const detruit *>(projo);
            pro_nom = dynamic_cast<const nomme *>(projo);
            pro_mir = dynamic_cast<const mirage *>(projo);

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
                unsigned char firm;

                if(pro_mir != NULL)
                    firm = pro_mir->get_inode()->signature();
                else
                    firm = pro_nom->signature();

		detruit *det_tmp = new (nothrow) detruit(pro_nom->get_name(), firm, current->get_last_modif());
		if(det_tmp == NULL)
		    throw Ememory("catalogue::update_destroyed_with");
		try
		{
		    current->add_children(det_tmp);
		}
		catch(...)
		{
		    delete det_tmp;
		    throw;
		}

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

    void catalogue::update_absent_with(catalogue & ref, infinint aborting_next_etoile)
    {
        directory *current = contenu;
        nomme *ici;
        const entree *projo;
        const eod *pro_eod;
        const directory *pro_dir;
        const detruit *pro_det;
        const nomme *pro_nom;
        const inode *pro_ino;
        const mirage *pro_mir;
        map<infinint, etoile *> corres_clone;
            // for each etiquette from the reference catalogue
            // gives an cloned or original etoile object
            // in the current catalogue

        ref.reset_read();
        while(ref.read(projo))
        {
            pro_eod = dynamic_cast<const eod *>(projo);
            pro_dir = dynamic_cast<const directory *>(projo);
            pro_det = dynamic_cast<const detruit *>(projo);
            pro_nom = dynamic_cast<const nomme *>(projo);
            pro_ino = dynamic_cast<const inode *>(projo);
            pro_mir = dynamic_cast<const mirage *>(projo);

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

            if(pro_mir != NULL)
                pro_ino = pro_mir->get_inode();
                // warning: the returned inode's name is undefined
                // one must use the mirage's own name

            if(pro_ino == NULL)
                throw SRC_BUG; // a nome that is not an inode nor a detruit!? What's that?

            if(!current->search_children(pro_nom->get_name(), ici))
            {
                entree *clo_ent = NULL;
                inode *clo_ino = NULL;
                directory *clo_dir = NULL;
                mirage *clo_mir = NULL;
                etoile *clo_eto = NULL;

                try
                {
                    clo_ent = pro_ino->clone();
                    clo_ino = dynamic_cast<inode *>(clo_ent);
                    clo_dir = dynamic_cast<directory *>(clo_ent);

                        // sanity checks

                    if(clo_ino == NULL)
                        throw SRC_BUG; // clone of an inode is not an inode???
                    if((clo_dir != NULL) ^ (pro_dir != NULL))
                        throw SRC_BUG; // both must be NULL or both must be non NULL

                        // converting inode to unsaved entry

                    clo_ino->set_saved_status(s_not_saved);
                    if(clo_ino->ea_get_saved_status() != inode::ea_none)
                    {
                        if(clo_ino->ea_get_saved_status() == inode::ea_removed)
                            clo_ino->ea_set_saved_status(inode::ea_none);
                        else
                            clo_ino->ea_set_saved_status(inode::ea_partial);
                    }

                        // handling hard links

                    if(pro_mir != NULL)
                    {
                        try
                        {
                            map<infinint, etoile *>::iterator it = corres_clone.find(pro_mir->get_etiquette());
                            if(it == corres_clone.end())
                            {
                                clo_eto = new (nothrow) etoile(clo_ino, aborting_next_etoile++);
                                if(clo_eto == NULL)
                                    throw Ememory("catalogue::update_absent_with");
                                else
                                    clo_ent = NULL; // object now managed by clo_eto

                                try
                                {
                                    corres_clone[pro_mir->get_etiquette()] = clo_eto;
                                    clo_mir = new (nothrow) mirage(pro_mir->get_name(), clo_eto);
                                    if(clo_mir == NULL)
                                        throw Ememory("catalogue::update_absent_with");
                                }
                                catch(...)
                                {
                                    if(clo_eto != NULL)
                                        delete clo_eto;
                                    throw;
                                }
                            }
                            else // mapping already exists (a star already shines)
                            {
                                    // we have cloned the inode but we do not need it as
                                    // an hard linked structure already exists
                                delete clo_ent;
                                clo_ent = NULL;

                                    // so we add a new reference to the existing hard linked structure
                                clo_mir = new (nothrow) mirage(pro_mir->get_name(), it->second);
                                if(clo_mir == NULL)
                                    throw Ememory("catalogue::update_absent_with");
                            }

                                // adding it to the catalogue

                            current->add_children(clo_mir);

                        }
                        catch(...)
                        {
                            if(clo_mir != NULL)
                            {
                                delete clo_mir;
                                clo_mir = NULL;
                            }
                            throw;
                        }
                    }
                    else // not a hard link entry
                    {
                            // adding it to the catalogue

                        current->add_children(clo_ino);
                        clo_ent = NULL; // object now managed by the current catalogue
                    }

                        // recusing in case of directory

                    if(clo_dir != NULL)
                    {
                        if(current->search_children(pro_ino->get_name(), ici))
                        {
                            if((void *)ici != (void *)clo_dir)
                                throw SRC_BUG;  // we have just added the entry we were looking for, but could find another one!?!
                            current = clo_dir;
                        }
                        else
                            throw SRC_BUG; // cannot find the entry we have just added!!!
                    }
                }
                catch(...)
                {
                    if(clo_ent != NULL)
                    {
                        delete clo_ent;
                        clo_ent = NULL;
                    }
                    throw;
                }
            }
            else // entry found in the current catalogue
            {
                if(pro_dir != NULL)
                {
                    directory *ici_dir = dynamic_cast<directory *>(ici);

                    if(ici_dir != NULL)
                        current = ici_dir;
                    else
                        ref.skip_read_to_parent_dir();
                }

                if(pro_mir != NULL)
                {
                    mirage *ici_mir = dynamic_cast<mirage *>(ici);

                    if(ici_mir != NULL && corres_clone.find(pro_mir->get_etiquette()) == corres_clone.end())
                    {
                            // no correspondance found
                            // so we add a one to the map

                        corres_clone[pro_mir->get_etiquette()] = ici_mir->get_etoile();
                    }
                }
            }
        }
    }

    void catalogue::listing(bool isolated,
                            const mask &selection,
                            const mask &subtree,
                            bool filter_unsaved,
                            bool list_ea,
                            string marge) const
    {
        const entree *e = NULL;
        thread_cancellation thr;
        const string marge_plus = " |  ";
        const U_I marge_plus_length = marge_plus.size();
        defile juillet = FAKE_ROOT;
        const eod tmp_eod;

        get_ui().printf(gettext("access mode    | user | group | size  |          date                 | [data ][ EA  ][compr][S]|   filename\n"));
        get_ui().printf("---------------+------+-------+-------+-------------------------------+-------------------------+-----------\n");
        if(filter_unsaved)
            contenu->recursive_has_changed_update();

        reset_read();
        while(read(e))
        {
            const eod *e_eod = dynamic_cast<const eod *>(e);
            const directory *e_dir = dynamic_cast<const directory *>(e);
            const detruit *e_det = dynamic_cast<const detruit *>(e);
            const inode *e_ino = dynamic_cast<const inode *>(e);
            const mirage *e_hard = dynamic_cast<const mirage *>(e);
            const nomme *e_nom = dynamic_cast<const nomme *>(e);

            thr.check_self_cancellation();
            juillet.enfile(e);

            if(e_eod != NULL)
            {
                    // descendre la marge
                U_I length = marge.size();
                if(length >= marge_plus_length)
                    marge.erase(length - marge_plus_length, marge_plus_length);
                else
                    throw SRC_BUG;
                get_ui().printf("%S +---\n", &marge);
            }
            else
                if(e_nom == NULL)
                    throw SRC_BUG; // not an eod nor a nomme, what's that?
                else
                    if(subtree.is_covered(juillet.get_path()) && (e_dir != NULL || selection.is_covered(e_nom->get_name())))
                    {
                        if(e_det != NULL)
                        {
                            string tmp = e_nom->get_name();
                            string tmp_date = e_det->get_date() != 0 ? tools_display_date(e_det->get_date()) : "Unknown date";
                            saved_status poub;
                            char type;

                            if(!extract_base_and_status(e_det->get_signature(), (unsigned char &)type, poub))
                                type = '?';
                            if(type == 'f')
                                type = '-';
                            get_ui().printf(gettext("%S [%c] [ REMOVED ENTRY ] (%S)  %S\n"), &marge, type, &tmp_date, &tmp);
                        }
                        else
                        {
                            if(e_hard != NULL)
                                e_ino = e_hard->get_inode();

                            if(e_ino == NULL)
                                throw SRC_BUG;
                            else
                                if(!filter_unsaved
                                   || e_ino->get_saved_status() != s_not_saved
                                   || (e_ino->ea_get_saved_status() == inode::ea_full || e_ino->ea_get_saved_status() == inode::ea_fake)
                                   || (e_dir != NULL && e_dir->get_recursive_has_changed()))
                                {
                                    bool dirty_seq = local_check_dirty_seq(get_escape_layer());
                                    string a = local_perm(*e_ino, e_hard != NULL);
                                    string b = local_uid(*e_ino);
                                    string c = local_gid(*e_ino);
                                    string d = local_size(*e_ino);
                                    string e = local_date(*e_ino);
                                    string f = local_flag(*e_ino, isolated, dirty_seq);
                                    string g = e_nom->get_name();
				    if(list_ea && e_hard != NULL)
				    {
					infinint tiq = e_hard->get_etiquette();
					g += tools_printf(" [%i] ", &tiq);
				    }

                                    get_ui().printf("%S%S\t%S\t%S\t%S\t%S\t%S %S\n", &marge, &a, &b, &c, &d, &e, &f, &g);
                                    if(list_ea)
                                        local_display_ea(get_ui(), e_ino, marge + gettext("      Extended Attribute: ["), "]");

                                    if(e_dir != NULL)
                                        marge += marge_plus;
                                }
				else // not saved, filtered out
				{
				    if(e_dir != NULL)
				    {
					skip_read_to_parent_dir();
					juillet.enfile(&tmp_eod);
				    }
				}
                        }
                    }
                    else // filtered out
                        if(e_dir != NULL)
                        {
                            skip_read_to_parent_dir();
                            juillet.enfile(&tmp_eod);
                        }
                // else what was skipped is not a directory, nothing to do.
        }
    }

    void catalogue::tar_listing(bool isolated,
                                const mask &selection,
                                const mask &subtree,
                                bool filter_unsaved,
                                bool list_ea,
                                string beginning) const
    {
        const entree *e = NULL;
        thread_cancellation thr;
        defile juillet = FAKE_ROOT;
        const eod tmp_eod;

        if(!get_ui().get_use_listing())
        {
            get_ui().printf(gettext("[data ][ EA  ][compr][S]| permission | user  | group | size  |          date                 |    filename\n"));
            get_ui().printf("------------------------+------------+-------+-------+-------+-------------------------------+------------\n");
        }
        if(filter_unsaved)
            contenu->recursive_has_changed_update();

        reset_read();
        while(read(e))
        {
            string sep = beginning == "" ? "" : "/";
            const eod *e_eod = dynamic_cast<const eod *>(e);
            const directory *e_dir = dynamic_cast<const directory *>(e);
            const detruit *e_det = dynamic_cast<const detruit *>(e);
            const inode *e_ino = dynamic_cast<const inode *>(e);
            const mirage *e_hard = dynamic_cast<const mirage *>(e);
            const nomme *e_nom = dynamic_cast<const nomme *>(e);

            thr.check_self_cancellation();
            juillet.enfile(e);

            if(e_eod != NULL)
            {
                string tmp;

                    // removing the last directory of the path contained in beginning
                path p = beginning;
                if(p.pop(tmp))
                    beginning = p.display();
                else
                    if(p.degre() == 1)
                        beginning = "";
                    else
                        throw SRC_BUG;
            }
            else
                if(e_nom == NULL)
                    throw SRC_BUG;
                else
                {
                    if(subtree.is_covered(juillet.get_path()) && (e_dir != NULL || selection.is_covered(e_nom->get_name())))
                    {
                        if(e_det != NULL)
                        {
                            string tmp = e_nom->get_name();
                            string tmp_date = e_det->get_date() != 0 ? tools_display_date(e_det->get_date()) : "Unknown date";
                            if(get_ui().get_use_listing())
                                get_ui().listing(REMOVE_TAG, "xxxxxxxxxx", "", "", "", tmp_date, beginning+sep+tmp, false, false);
                            else
                            {
                                saved_status poub;
                                char type;

                                if(!extract_base_and_status(e_det->get_signature(), (unsigned char &)type, poub))
                                    type = '?';
                                if(type == 'f')
                                    type = '-';
                                get_ui().printf("%s (%S) [%c] %S%S%S\n", REMOVE_TAG, &tmp_date, type,  &beginning, &sep, &tmp);
                            }
                        }
                        else
                        {
                            if(e_hard != NULL)
                                e_ino = e_hard->get_inode();

                            if(e_ino == NULL)
                                throw SRC_BUG;
                            else
			    {
				string nom = e_nom->get_name();

                                if(!filter_unsaved
                                   || e_ino->get_saved_status() != s_not_saved
                                   || (e_ino->ea_get_saved_status() == inode::ea_full || e_ino->ea_get_saved_status() == inode::ea_fake)
                                   || (e_dir != NULL && e_dir->get_recursive_has_changed()))
                                {
                                    bool dirty_seq = local_check_dirty_seq(get_escape_layer());
                                    string a = local_perm(*e_ino, e_hard != NULL);
                                    string b = local_uid(*e_ino);
                                    string c = local_gid(*e_ino);
                                    string d = local_size(*e_ino);
                                    string e = local_date(*e_ino);
                                    string f = local_flag(*e_ino, isolated, dirty_seq);

				    if(list_ea && e_hard != NULL)
				    {
					infinint tiq = e_hard->get_etiquette();
					nom += tools_printf(" [%i] ", &tiq);
				    }

                                    if(get_ui().get_use_listing())
                                        get_ui().listing(f, a, b, c, d, e, beginning+sep+nom, e_dir != NULL, e_dir != NULL && e_dir->has_children());
                                    else
                                        get_ui().printf("%S %S   %S\t%S\t%S\t%S\t%S%S%S\n", &f, &a, &b, &c, &d, &e, &beginning, &sep, &nom);
                                    if(list_ea)
                                        local_display_ea(get_ui(), e_ino, gettext("      Extended Attribute: ["), "]");

                                    if(e_dir != NULL)
                                        beginning += sep + nom;
                                }
				else // not saved, filtered out
				{
				    if(e_dir != NULL)
				    {
					juillet.enfile(&tmp_eod);
					skip_read_to_parent_dir();
				    }
				}
			    }
                        }
                    }
                    else // excluded, filtered out
                        if(e_dir != NULL)
                        {
                            juillet.enfile(&tmp_eod);
                            skip_read_to_parent_dir();
                        }
                        // else what was skipped is not a directory, nothing to do.
                }
        }
    }

    void catalogue::xml_listing(bool isolated,
                                const mask &selection,
                                const mask &subtree,
                                bool filter_unsaved,
                                bool list_ea,
                                string beginning) const
    {
        const entree *e = NULL;
        thread_cancellation thr;
        defile juillet = FAKE_ROOT;

        get_ui().warning("<?xml version=\"1.0\" ?>");
        get_ui().warning("<!DOCTYPE Catalog SYSTEM \"dar-catalog-1.0.dtd\">\n");
        get_ui().warning("<Catalog format=\"1.1\">");
        if(filter_unsaved)
            contenu->recursive_has_changed_update();

        reset_read();
        while(read(e))
        {
            const eod *e_eod = dynamic_cast<const eod *>(e);
            const directory *e_dir = dynamic_cast<const directory *>(e);
            const detruit *e_det = dynamic_cast<const detruit *>(e);
            const inode *e_ino = dynamic_cast<const inode *>(e);
            const mirage *e_hard = dynamic_cast<const mirage *>(e);
            const lien *e_sym = dynamic_cast<const lien *>(e);
            const device *e_dev = dynamic_cast<const device *>(e);
            const nomme *e_nom = dynamic_cast<const nomme *>(e);
            const eod tmp_eod;

            thr.check_self_cancellation();
            juillet.enfile(e);

            if(e_eod != NULL)
            {
                U_I length = beginning.size();

                if(length > 0)
                    beginning.erase(length - 1, 1); // removing the last tab character
                else
                    throw SRC_BUG;
                get_ui().printf("%S</Directory>\n", &beginning);
            }
            else
                if(e_nom == NULL)
                    throw SRC_BUG;
                else
                {
                    if(subtree.is_covered(juillet.get_path()) && (e_dir != NULL || selection.is_covered(e_nom->get_name())))
                    {
                        string name = tools_output2xml(e_nom->get_name());

                        if(e_det != NULL)
                        {
                            unsigned char sig;
                            saved_status state;
                            string data = "deleted";
                            string metadata = "absent";

                            unmk_signature(e_det->get_signature(), sig, state, isolated);
                            switch(sig)
                            {
                            case 'd':
                                get_ui().printf("%S<Directory name=\"%S\">\n", &beginning, &name);
                                xml_listing_attributes(get_ui(), beginning, data, metadata);
                                get_ui().printf("%S</Directory>\n", &beginning);
                                break;
                            case 'f':
                            case 'h':
                            case 'e':
                                get_ui().printf("%S<File name=\"%S\">\n", &beginning, &name);
                            xml_listing_attributes(get_ui(), beginning, data, metadata);
                            get_ui().printf("%S</File>\n", &beginning);
                            break;
                            case 'l':
                                get_ui().printf("%S<Symlink name=\"%S\">\n", &beginning, &name);
                                xml_listing_attributes(get_ui(), beginning, data, metadata);
                                get_ui().printf("%S</Symlink>\n", &beginning);
                                break;
                            case 'c':
                                get_ui().printf("%S<Device name=\"%S\" type=\"character\">\n", &beginning, &name);
                                xml_listing_attributes(get_ui(), beginning, data, metadata);
                                get_ui().printf("%S</Device>\n", &beginning);
                                break;
                            case 'b':
                                get_ui().printf("%S<Device name=\"%S\" type=\"block\">\n", &beginning, &name);
                                xml_listing_attributes(get_ui(), beginning, data, metadata);
                                get_ui().printf("%S</Device>\n", &beginning);
                                break;
                            case 'p':
                                get_ui().printf("%S<Pipe name=\"%S\">\n", &beginning, &name);
                                xml_listing_attributes(get_ui(), beginning, data, metadata);
                                get_ui().printf("%S</Pipe>\n", &beginning);
                                break;
                            case 's':
                                get_ui().printf("%S<Socket name=\"%S\">\n", &beginning, &name);
                                xml_listing_attributes(get_ui(), beginning, data, metadata);
                                get_ui().printf("%S</Socket>\n", &beginning);
                                break;
                            default:
                                throw SRC_BUG;
                            }
                        }
                        else // other than detruit object
                        {
                            if(e_hard != NULL)
                            {
                                e_ino = e_hard->get_inode();
                                e_sym = dynamic_cast<const lien *>(e_ino);
                                e_dev = dynamic_cast<const device *>(e_ino);
                            }

                            if(e_ino == NULL)
                                throw SRC_BUG; // this is a nomme which is neither a detruit nor an inode

                            if(!filter_unsaved
                               || e_ino->get_saved_status() != s_not_saved
                               || (e_ino->ea_get_saved_status() == inode::ea_full || e_ino->ea_get_saved_status() == inode::ea_fake)
                               || (e_dir != NULL && e_dir->get_recursive_has_changed()))
                            {
                                string data, metadata, maj, min, chksum, target;
                                string dirty, sparse;
                                string size = local_size(*e_ino);
                                string stored = local_storage_size(*e_ino);
                                const file *reg = dynamic_cast<const file *>(e_ino); // ino is no more it->second (if it->second was a mirage)

                                saved_status data_st;
                                inode::ea_status ea_st = isolated ? inode::ea_fake : e_ino->ea_get_saved_status();
                                unsigned char sig;

                                unmk_signature(e_ino->signature(), sig, data_st, isolated);
                                data_st = isolated ? s_fake : e_ino->get_saved_status(); // the trusted source for inode status is get_saved_status, not the signature (may change in future, who knows)
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
                                case inode::ea_full:
                                    metadata = "saved";
                                    break;
                                case inode::ea_partial:
                                case inode::ea_fake:
                                    metadata = "referenced";
                                    break;
                                case inode::ea_none:
                                case inode::ea_removed:
                                    metadata = "absent";
                                    break;
                                default:
                                    throw SRC_BUG;
                                }

                                    // building entry for each type of inode

                                switch(sig)
                                {
                                case 'd': // directories
                                    get_ui().printf("%S<Directory name=\"%S\">\n", &beginning, &name);
                                    xml_listing_attributes(get_ui(), beginning, data, metadata, e, list_ea);
                                    beginning += "\t";
                                    break;
                                case 'h': // hard linked files
                                case 'e': // hard linked files
                                    throw SRC_BUG; // no more used in dynamic data
                                case 'f': // plain files
                                    if(data_st == s_saved)
                                    {
                                        const crc *tmp = NULL;
                                        const file * f_ino = dynamic_cast<const file *>(e_ino);

                                        if(f_ino == NULL)
                                            throw SRC_BUG;
                                        dirty = yes_no(f_ino->is_dirty());
                                        sparse= yes_no(f_ino->get_sparse_file_detection_read());

                                        if(reg == NULL)
                                            throw SRC_BUG; // f is signature for plain files
                                        if(reg->get_crc(tmp) && tmp != NULL)
                                            chksum = tmp->crc2str();
					else
                                            chksum = "";
                                    }
                                    else
                                    {
                                        stored = "";
                                        chksum = "";
                                        dirty = "";
                                        sparse = "";
                                    }
                                    get_ui().printf("%S<File name=\"%S\" size=\"%S\" stored=\"%S\" crc=\"%S\" dirty=\"%S\" sparse=\"%S\">\n",
                                                    &beginning, &name, &size, &stored, &chksum, &dirty, &sparse);
                                    xml_listing_attributes(get_ui(), beginning, data, metadata, e, list_ea);
                                    get_ui().printf("%S</File>\n", &beginning);
                                    break;
                                case 'l': // soft link
                                    if(data_st == s_saved)
                                        target = tools_output2xml(e_sym->get_target());
                                    else
                                        target = "";
                                    get_ui().printf("%S<Symlink name=\"%S\" target=\"%S\">\n",
                                                    &beginning, &name, &target);
                                    xml_listing_attributes(get_ui(), beginning, data, metadata, e, list_ea);
                                    get_ui().printf("%S</Symlink>\n", &beginning);
                                    break;
                                case 'c':
                                case 'b':
                                        // this is maybe less performant, to have both 'c' and 'b' here and
                                        // make an additional test, but this has the advantage to not duplicate
                                        // very similar code, which would obviously evoluate the same way.
                                        // Experience shows that two identical codes even when driven by the same need
                                        // are an important source of bugs, as one may forget to update both of them, the
                                        // same way...

                                    if(sig == 'c')
                                        target = "character";
                                    else
                                        target = "block";
                                    // we re-used target variable which is not used for the current inode

                                if(data_st == s_saved)
                                {
                                    maj = tools_uword2str(e_dev->get_major());
                                    min = tools_uword2str(e_dev->get_minor());
                                }
                                else
                                    maj = min = "";

                                get_ui().printf("%S<Device name=\"%S\" type=\"%S\" major=\"%S\" minor=\"%S\">\n",
                                                &beginning, &name, &target, &maj, &min);
                                xml_listing_attributes(get_ui(), beginning, data, metadata, e, list_ea);
                                get_ui().printf("%S</Device>\n", &beginning);
                                break;
                                case 'p':
                                    get_ui().printf("%S<Pipe name=\"%S\">\n", &beginning, &name);
                                    xml_listing_attributes(get_ui(), beginning, data, metadata, e, list_ea);
                                    get_ui().printf("%S</Pipe>\n", &beginning);
                                    break;
                                case 's':
                                    get_ui().printf("%S<Socket name=\"%S\">\n", &beginning, &name);
                                    xml_listing_attributes(get_ui(), beginning, data, metadata, e, list_ea);
                                    get_ui().printf("%S</Socket>\n", &beginning);
                                    break;
                                default:
                                    throw SRC_BUG;
                                }
                            }
			    else // not saved, filtered out
			    {
				if(e_dir != NULL)
				{
				    skip_read_to_parent_dir();
				    juillet.enfile(&tmp_eod);
				}
			    }
                        }
                    }  // end of filter check
		    else // filtered out
			if(e_dir != NULL)
                        {
                            skip_read_to_parent_dir();
                            juillet.enfile(&tmp_eod);
                        }
                        // else what was skipped is not a directory, nothing to do.
                }
        }

        get_ui().warning("</Catalog>");
    }

    void catalogue::dump(generic_file & f) const
    {
	crc *tmp = NULL;

	try
	{
	    f.reset_crc(CAT_CRC_SIZE);
	    try
	    {
		ref_data_name.dump(f);
		contenu->dump(f, false);
	    }
	    catch(...)
	    {
		tmp = f.get_crc();
		throw;
	    }
	    tmp = f.get_crc();
	    if(tmp == NULL)
		throw SRC_BUG;
	    tmp->dump(f);
	}
	catch(...)
	{
	    if(tmp != NULL)
		delete tmp;
	    throw;
	}
	if(tmp != NULL)
	    delete tmp;
    }

    void catalogue::reset_all()
    {
	out_compare = "/";
	current_compare = contenu;
	current_add = contenu;
	current_read = contenu;
	if(sub_tree != NULL)
	{
	    delete sub_tree;
	    sub_tree = NULL;
	}
    }

    void catalogue::copy_detruits_from(const catalogue & ref)
    {
        const entree *ent;

        ref.reset_read();
        reset_add();

        while(ref.read(ent))
        {
            const detruit *ent_det = dynamic_cast<const detruit *>(ent);
            const directory *ent_dir = dynamic_cast<const directory *>(ent);
            const eod *ent_eod = dynamic_cast<const eod *>(ent);

            if(ent_dir != NULL)
                re_add_in(ent_dir->get_name());
            if(ent_eod != NULL)
            {
                eod *tmp = new (nothrow) eod();
                if(tmp == NULL)
                    throw Ememory("catalogue::copy_detruits_from");
                try
                {
                    add(tmp);
                }
                catch(...)
                {
                    delete tmp;
                    throw;
                }
            }
            if(ent_det != NULL)
            {
                detruit *cp = new (nothrow) detruit(*ent_det);
                if(cp == NULL)
                    throw Ememory("catalogue::copy_detruits_from");
                try
                {
                    add(cp);
                }
                catch(...)
                {
                    delete cp;
                    throw;
                }
            }
        }
    }

    void catalogue::swap_stuff(catalogue & ref)
    {
	    // swapping contenu
	directory *tmp = contenu;
	contenu = ref.contenu;
	ref.contenu = tmp;
	tmp = NULL;

	    // swapping stats
	entree_stats tmp_st = stats;
	stats = ref.stats;
	ref.stats = tmp_st;

	    // swapping label
	label tmp_lab;
	tmp_lab = ref_data_name;
	ref_data_name = ref.ref_data_name;
	ref.ref_data_name = tmp_lab;

	    // avoid pointers to point to the now other's object tree
	reset_all();
	ref.reset_all();
    }

    void catalogue::partial_copy_from(const catalogue & ref)
    {
        contenu = NULL;
        sub_tree = NULL;

        try
        {
            if(ref.contenu == NULL)
                throw SRC_BUG;
            contenu = new (nothrow) directory(*ref.contenu);
            if(contenu == NULL)
                throw Ememory("catalogue::catalogue(const catalogue &)");
            current_compare = contenu;
            current_add = contenu;
            current_read = contenu;
            if(ref.sub_tree != NULL)
	    {
                sub_tree = new (nothrow) path(*ref.sub_tree);
		if(sub_tree == NULL)
		    throw Ememory("catalogue::partial_copy_from");
	    }
            else
                sub_tree = NULL;
            sub_count = ref.sub_count;
            stats = ref.stats;
        }
        catch(...)
        {
            if(contenu != NULL)
            {
                delete contenu;
                contenu = NULL;
            }
            if(sub_tree != NULL)
            {
                delete sub_tree;
                sub_tree = NULL;
            }
            throw;
        }
    }

    void catalogue::detruire()
    {
        if(contenu != NULL)
        {
            delete contenu;
            contenu = NULL;
        }
        if(sub_tree != NULL)
        {
            delete sub_tree;
            sub_tree = NULL;
        }
    }


    const eod catalogue::r_eod;
    const U_I catalogue::CAT_CRC_SIZE = 4;


    static string local_perm(const inode &ref, bool hard)
    {
        string ret = hard ? "*" : " ";
        saved_status st;
        char type;

        U_32 perm = ref.get_perm();
        if(!extract_base_and_status(ref.signature(), (unsigned char &)type, st))
            throw SRC_BUG;

        if(type == 'f') // plain files
            type = '-';
	if(type == 'o') // door "files"
	    type = 'D';
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

    static string local_flag(const inode & ref, bool isolated, bool dirty_seq)
    {
        string ret;
        const file *ref_f = dynamic_cast<const file *>(&ref);
        bool dirty = dirty_seq || (ref_f != NULL ? ref_f->is_dirty() : false);
	saved_status st = ref.get_saved_status();
	inode::ea_status ea_st = ref.ea_get_saved_status();

	if(isolated && st == s_saved && !dirty)
	    st = s_fake;

	if(isolated && ea_st == inode::ea_full)
	    ea_st = inode::ea_fake;

        switch(st)
        {
        case s_saved:
            if(dirty)
                ret = gettext("[DIRTY]");
            else
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


        switch(ea_st)
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
        case inode::ea_removed:
            ret += "[Suppr]";
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

        if(fic != NULL && fic->get_sparse_file_detection_read())
            ret += "[X]";
        else
            ret += "[ ]";

        return ret;
    }

    static void xml_listing_attributes(user_interaction & dialog,
                                       const string & beginning,
                                       const string & data,
                                       const string & metadata,
                                       const entree * obj,
                                       bool list_ea)
    {
        string user;
        string group;
        string permissions;
        string atime;
        string mtime;
        string ctime;
        const inode *e_ino = dynamic_cast<const inode *>(obj);
        const mirage *e_hard = dynamic_cast<const mirage *>(obj);

        if(e_ino != NULL)
        {
            user = local_uid(*e_ino);
            group = local_gid(*e_ino);
            permissions = local_perm(*e_ino, e_hard != NULL);
            atime = deci(e_ino->get_last_access()).human();
            mtime = deci(e_ino->get_last_modif()).human();
            if(e_ino->has_last_change())
            {
                ctime = deci(e_ino->get_last_change()).human();
                if(ctime == "0")
                    ctime = "";
            }
            else
                ctime = "";
        }
        else
        {
            user = "";
            group = "";
            permissions = "";
            atime = "";
            mtime = "";
            ctime = "";
        }

        dialog.printf("%S<Attributes data=\"%S\" metadata=\"%S\" user=\"%S\" group=\"%S\" permissions=\"%S\" atime=\"%S\" mtime=\"%S\" ctime=\"%S\" />\n",
                      &beginning, &data, &metadata, &user, &group, &permissions, &atime, &mtime, &ctime);
        if(list_ea && e_ino != NULL && e_ino->ea_get_saved_status() == inode::ea_full)
        {
            string new_begin = beginning + "\t";
            local_display_ea(dialog, e_ino, new_begin + "<EA_entry> ea_name=\"", "\">", true);
            dialog.printf("%S</Attributes>", &beginning);
        }
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

    static bool local_check_dirty_seq(escape *ptr)
    {
        bool ret;

        if(ptr != NULL)
        {
            bool already_set = ptr->is_unjumpable_mark(escape::seqt_file);

            if(!already_set)
                ptr->add_unjumpable_mark(escape::seqt_file);
            ret = ptr != NULL && ptr->skip_to_next_mark(escape::seqt_dirty, true);
            if(!already_set)
                ptr->remove_unjumpable_mark(escape::seqt_file);
        }
        else
            ret = false;

        return ret;
    }

    static void local_display_ea(user_interaction & dialog,
                                 const inode * ino,
                                 const string &prefix,
                                 const string &suffix,
                                 bool xml_output)
    {
        if(ino == NULL)
            return;

        if(ino->ea_get_saved_status() == inode::ea_full)
        {
            const ea_attributs *owned = ino->get_ea();
            string key, val;

            if(owned == NULL)
                throw SRC_BUG;

            owned->reset_read();
            while(owned->read(key, val))
            {
                if(xml_output)
                    key = tools_output2xml(key);
                dialog.warning(prefix + key + suffix);
            }
        }
    }

} // end of namespace

