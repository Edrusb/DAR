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

    cat_entree *cat_entree::read(const shared_ptr<user_interaction> & dialog,
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
	    try
	    {
		cat_signature cat_sig(*ptr, reading_ver);

		if(!cat_sig.get_base_and_status((unsigned char &)type, saved))
		{
		    if(!lax)
			throw Erange("cat_entree::read", gettext("corrupted file"));
		    else
			type = ' '; // used to by-pass object construction and return nullptr as value of this method
		}
	    }
	    catch(Erange & e)
	    {
		type = ' '; // used to by-pass object construction and return nullptr as value of this method
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
                if(saved != saved_status::saved)
                {
                    if(!lax)
                        throw Erange("cat_entree::read", gettext("corrupted file"));
                    else
                        dialog->message(gettext("LAX MODE: Unexpected saved status for end of directory entry, assuming data corruption occurred, ignoring and continuing"));
                }
                ret = new (nothrow) cat_eod(pdesc, small);
                break;
            case 'x':
                if(saved != saved_status::saved)
                {
                    if(!lax)
                        throw Erange("cat_entree::read", gettext("corrupted file"));
                    else
                        dialog->message(gettext("LAX MODE: Unexpected saved status for class \"cat_detruit\" object, assuming data corruption occurred, ignoring and continuing"));
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
                    dialog->message(gettext("LAX MODE: found unknown catalogue entry, assuming data corruption occurred, cannot read further the catalogue as I do not know the length of this type of entry"));
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
				dialog->pause(tools_printf(gettext("Entry information CRC failure for %S. Ignore the failure?"), &nom));
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

    cat_entree::cat_entree(const smart_pointer<pile_descriptor> & x_pdesc, bool small, saved_status val): xsaved(val)
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
        cat_signature s(signature(), get_saved_status());

	pdesc.check(small);
	if(small)
	    s.write(*(pdesc.esc));
	else
	    s.write(*(pdesc.stack));
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
