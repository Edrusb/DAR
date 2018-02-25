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
} // end extern "C"

#include "cat_entree.hpp"
#include "cat_all_entrees.hpp"
#include "cat_tools.hpp"

using namespace std;


namespace libdar
{

    const U_I cat_entree::ENTREE_CRC_SIZE = 2;

    void entree_stats::add(const cat_entree *ref)
    {
        if(dynamic_cast<const cat_eod *>(ref) == nullptr // we ignore cat_eod
           && dynamic_cast<const cat_ignored *>(ref) == nullptr // as well we ignore "cat_ignored"
           && dynamic_cast<const cat_ignored_dir *>(ref) == nullptr) // and "cat_ignored_dir"
        {
            const cat_inode *ino = dynamic_cast<const cat_inode *>(ref);
            const cat_mirage *h = dynamic_cast<const cat_mirage *>(ref);
            const cat_detruit *x = dynamic_cast<const cat_detruit *>(ref);


            if(h != nullptr) // won't count twice the same inode if it is referenced with hard_link
            {
                ++num_hard_link_entries;
                if(!h->is_inode_counted())
                {
                    ++num_hard_linked_inodes;
                    h->set_inode_counted(true);
                    ino = h->get_inode();
                }
            }

            if(ino != nullptr)
            {
                ++total;
                if(ino->get_saved_status() == s_saved
		   || ino->get_saved_status() == s_delta)
                    ++saved;
            }

            if(x != nullptr)
                ++num_x;
            else
            {
                const cat_directory *d = dynamic_cast<const cat_directory*>(ino);
                if(d != nullptr)
                    ++num_d;
                else
                {
                    const cat_chardev *c = dynamic_cast<const cat_chardev *>(ino);
                    if(c != nullptr)
                        ++num_c;
                    else
                    {
                        const cat_blockdev *b = dynamic_cast<const cat_blockdev *>(ino);
                        if(b != nullptr)
                            ++num_b;
                        else
                        {
                            const cat_tube *p = dynamic_cast<const cat_tube *>(ino);
                            if(p != nullptr)
                                ++num_p;
                            else
                            {
                                const cat_prise *s = dynamic_cast<const cat_prise *>(ino);
                                if(s != nullptr)
                                    ++num_s;
                                else
                                {
                                    const cat_lien *l = dynamic_cast<const cat_lien *>(ino);
                                    if(l != nullptr)
                                        ++num_l;
                                    else
                                    {
					const cat_door *D = dynamic_cast<const cat_door *>(ino);
					if(D != nullptr)
					    ++num_D;
					else
					{
					    const cat_file *f = dynamic_cast<const cat_file *>(ino);
					    if(f != nullptr)
						++num_f;
					    else
						if(h == nullptr)
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
	dialog.printf("");
        dialog.printf(gettext("CATALOGUE CONTENTS :"));
        dialog.printf(gettext("total number of inode : %i"), &total);
        dialog.printf(gettext("saved inode           : %i"), &saved);
        dialog.printf(gettext("distribution of inode(s)"));
        dialog.printf(gettext(" - directories        : %i"), &num_d);
        dialog.printf(gettext(" - plain files        : %i"), &num_f);
        dialog.printf(gettext(" - symbolic links     : %i"), &num_l);
        dialog.printf(gettext(" - named pipes        : %i"), &num_p);
        dialog.printf(gettext(" - unix sockets       : %i"), &num_s);
        dialog.printf(gettext(" - character devices  : %i"), &num_c);
        dialog.printf(gettext(" - block devices      : %i"), &num_b);
	dialog.printf(gettext(" - Door entries       : %i"), &num_D);
        dialog.printf(gettext("hard links information"));
        dialog.printf(gettext(" - number of inode with hard link           : %i"), &num_hard_linked_inodes);
        dialog.printf(gettext(" - number of reference to hard linked inodes: %i"), &num_hard_link_entries);
        dialog.printf(gettext("destroyed entries information"));
        dialog.printf(gettext("   %i file(s) have been record as destroyed since backup of reference"), &num_x);
	dialog.printf("");
    }

    cat_entree *cat_entree::read(user_interaction & dialog,
				 const smart_pointer<pile_descriptor> & pdesc,
				 const archive_version & reading_ver,
				 entree_stats & stats,
				 std::map <infinint, cat_etoile *> & corres,
				 compression default_algo,
				 bool lax,
				 bool only_detruit,
				 bool small)
    {
        char type;
        saved_status saved;
        cat_entree *ret = nullptr;
        map <infinint, cat_etoile *>::iterator it;
        infinint tmp;
	bool read_crc;
	generic_file *ptr = nullptr;

	pdesc->check(small);
	if(small)
	{
	    ptr = pdesc->esc;
	    pdesc->stack->flush_read_above(pdesc->esc);
	}
	else
	    ptr = pdesc->stack;

        read_crc = small && !ptr->crc_status();
	    // crc may be activated when reading a hard link (mirage read, now reading the inode)

        if(read_crc)
            ptr->reset_crc(ENTREE_CRC_SIZE);

        try
        {
            S_I lu = ptr->read(&type, 1);

            if(lu == 0)
		type = ' '; // used to by-pass object construction and return nullptr as value of this method
	    else
	    {
		if(!extract_base_and_status((unsigned char)type, (unsigned char &)type, saved))
		{
		    if(!lax)
			throw Erange("cat_entree::read", gettext("corrupted file"));
		    else
			type = ' '; // used to by-pass object construction and return nullptr as value of this method
		}
	    }

            switch(type)
            {
	    case ' ':
		break; // returned value will be nullptr
            case 'f':
                ret = new (nothrow) cat_file(dialog, pdesc, reading_ver, saved, default_algo, small);
                break;
            case 'l':
                ret = new (nothrow) cat_lien(dialog, pdesc, reading_ver, saved, small);
                break;
            case 'c':
                ret = new (nothrow) cat_chardev(dialog, pdesc, reading_ver, saved, small);
                break;
            case 'b':
                ret = new (nothrow) cat_blockdev(dialog, pdesc, reading_ver, saved, small);
                break;
            case 'p':
                ret = new (nothrow) cat_tube(dialog, pdesc, reading_ver, saved, small);
                break;
            case 's':
                ret = new (nothrow) cat_prise(dialog, pdesc, reading_ver, saved, small);
                break;
            case 'd':
                ret = new (nothrow) cat_directory(dialog, pdesc, reading_ver, saved, stats, corres, default_algo, lax, only_detruit, small);
                break;
            case 'm':
                ret = new (nothrow) cat_mirage(dialog, pdesc, reading_ver, saved, stats, corres, default_algo, cat_mirage::fmt_mirage, lax, small);
                break;
            case 'h': // old hard-link object
                ret = new (nothrow) cat_mirage(dialog, pdesc, reading_ver, saved, stats, corres, default_algo, cat_mirage::fmt_hard_link, lax, small);
                break;
            case 'e': // old etiquette object
                ret = new (nothrow) cat_mirage(dialog, pdesc, reading_ver, saved, stats, corres, default_algo, lax, small);
                break;
            case 'z':
                if(saved != s_saved)
                {
                    if(!lax)
                        throw Erange("cat_entree::read", gettext("corrupted file"));
                    else
                        dialog.message(gettext("LAX MODE: Unexpected saved status for end of directory entry, assuming data corruption occurred, ignoring and continuing"));
                }
                ret = new (nothrow) cat_eod(pdesc, small);
                break;
            case 'x':
                if(saved != s_saved)
                {
                    if(!lax)
                        throw Erange("cat_entree::read", gettext("corrupted file"));
                    else
                        dialog.message(gettext("LAX MODE: Unexpected saved status for class \"cat_detruit\" object, assuming data corruption occurred, ignoring and continuing"));
                }
                ret = new (nothrow) cat_detruit(pdesc, reading_ver, small);
                break;
	    case 'o':
		ret = new (nothrow) cat_door(dialog, pdesc, reading_ver, saved, default_algo, small);
		break;
            default :
                if(!lax)
                    throw Erange("cat_entree::read", gettext("unknown type of data in catalogue"));
                else
                {
                    dialog.message(gettext("LAX MODE: found unknown catalogue entry, assuming data corruption occurred, cannot read further the catalogue as I do not know the length of this type of entry"));
                    return ret;  // nullptr
                }
            }
	    if(ret == nullptr && type != ' ')
		throw Ememory("cat_entree::read");
        }
        catch(...)
        {
            if(read_crc)
            {
		crc * tmp = ptr->get_crc(); // keep pdesc in a coherent status
		if(tmp != nullptr)
		    delete tmp;
            }
            throw;
        }

        if(read_crc)
        {
            crc *crc_calc = ptr->get_crc();

	    if(crc_calc == nullptr)
		throw SRC_BUG;

	    if(type == ' ') // corrupted data while in lax mode
	    {
		delete crc_calc;
		return ret; // returning nullptr
	    }

	    try
	    {
		crc *crc_read = create_crc_from_file(*ptr, nullptr);
		if(crc_read == nullptr)
		    throw SRC_BUG;

		try
		{
		    if(*crc_read != *crc_calc)
		    {
			cat_nomme * ret_nom = dynamic_cast<cat_nomme *>(ret);
			string nom = ret_nom != nullptr ? ret_nom->get_name() : "";

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
				throw Erange("cat_entree::read", tools_printf(gettext("Entry information CRC failure for %S"), &nom));
			    else
				throw Erange("cat_entree::read", gettext(gettext("Entry information CRC failure")));
			}
		    }
		    ret->post_constructor(*pdesc);
		}
		catch(...)
		{
		    if(crc_read != nullptr)
			delete crc_read;
		    throw;
		}
		if(crc_read != nullptr)
		    delete crc_read;
	    }
	    catch(...)
	    {
		if(crc_calc != nullptr)
		    delete crc_calc;
		throw;
	    }
	    if(crc_calc != nullptr)
		delete crc_calc;
	}

        stats.add(ret);
        return ret;
    }

    cat_entree::cat_entree(const smart_pointer<pile_descriptor> & x_pdesc, bool small)
    {
	if(small && x_pdesc->esc == nullptr)
	    throw SRC_BUG;

	change_location(x_pdesc);
    }

    void cat_entree::change_location(const smart_pointer<pile_descriptor> & x_pdesc)
    {
	if(x_pdesc->stack == nullptr)
	    throw SRC_BUG;

	if(x_pdesc->compr == nullptr)
	    throw SRC_BUG;

	pdesc = x_pdesc;
    }

    void cat_entree::dump(const pile_descriptor & pdesc, bool small) const
    {
	pdesc.check(small);
        if(small)
        {
	    crc *tmp = nullptr;

	    try
	    {
		pdesc.stack->sync_write_above(pdesc.esc);
		pdesc.esc->reset_crc(ENTREE_CRC_SIZE);
		try
		{
		    inherited_dump(pdesc, small);
		}
		catch(...)
		{
		    tmp = pdesc.esc->get_crc(); // keep f in a coherent status
		    throw;
		}

		tmp = pdesc.esc->get_crc();
		if(tmp == nullptr)
		    throw SRC_BUG;

		tmp->dump(*pdesc.esc);
	    }
	    catch(...)
	    {
		if(tmp != nullptr)
		    delete tmp;
		throw;
	    }
	    if(tmp != nullptr)
		delete tmp;
        }
        else
            inherited_dump(pdesc, small);
    }

    void cat_entree::inherited_dump(const pile_descriptor & pdesc, bool small) const
    {
        char s = signature();

	pdesc.check(small);
	if(small)
	    pdesc.esc->write(&s, 1);
	else
	    pdesc.stack->write(&s, 1);
    }

    generic_file *cat_entree::get_read_cat_layer(bool small) const
    {
	generic_file *ret = nullptr;

	pdesc->check(small);

	if(small)
	{
	    pdesc->stack->flush_read_above(pdesc->esc);
	    ret = pdesc->esc;
	}
	else
	    ret = pdesc->stack;

	return ret;
    }

} // end of namespace
