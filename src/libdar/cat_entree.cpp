/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2024 Denis Corbin
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
} // end extern "C"

#include "cat_entree.hpp"
#include "cat_all_entrees.hpp"
#include "macro_tools.hpp"
#include "cat_signature.hpp"
#include "tools.hpp"

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
		crc *crc_read = create_crc_from_file(*ptr);
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

	if(ret != nullptr)
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

    void cat_entree::set_list_entry(const slice_layout *sly,
				    bool fetch_ea,
				    list_entry & ent) const
    {
	const cat_nomme *tmp_nom = dynamic_cast<const cat_nomme *>(this);
	const cat_inode *tmp_inode = dynamic_cast<const cat_inode *>(this);
	const cat_file *tmp_file = dynamic_cast<const cat_file *>(this);
	const cat_lien *tmp_lien = dynamic_cast<const cat_lien *>(this);
	const cat_device *tmp_device = dynamic_cast<const cat_device *>(this);
	const cat_mirage *tmp_mir = dynamic_cast<const cat_mirage *>(this);
	const cat_directory *tmp_dir = dynamic_cast<const cat_directory *>(this);
	const cat_detruit *tmp_det = dynamic_cast<const cat_detruit *>(this);

	ent.clear();

	if(tmp_mir == nullptr)
	{
	    ent.set_hard_link(false);
	    ent.set_type(signature());
	}
	else
	{
	    ent.set_hard_link(true);
	    ent.set_type(tmp_mir->get_inode()->signature());
	    tmp_inode = tmp_mir->get_inode();
	    tmp_file = dynamic_cast<const cat_file *>(tmp_inode);
	    tmp_lien = dynamic_cast<const cat_lien *>(tmp_inode);
	    tmp_device = dynamic_cast<const cat_device *>(tmp_inode);
	    ent.set_etiquette(tmp_mir->get_etiquette());
	}

	if(tmp_nom != nullptr)
	    ent.set_name(tmp_nom->get_name());

	if(tmp_det != nullptr)
	{
	    ent.set_removed_type(tmp_det->get_signature());
	    ent.set_removal_date(tmp_det->get_date());
	}

	if(tmp_inode != nullptr)
	{
	    ent.set_uid(tmp_inode->get_uid());
	    ent.set_gid(tmp_inode->get_gid());
	    ent.set_perm(tmp_inode->get_perm());
	    ent.set_last_access(tmp_inode->get_last_access());
	    ent.set_last_modif(tmp_inode->get_last_modif());
	    ent.set_saved_status(tmp_inode->get_saved_status());
	    ent.set_ea_status(tmp_inode->ea_get_saved_status());
	    if(tmp_inode->has_last_change())
		ent.set_last_change(tmp_inode->get_last_change());
	    if(tmp_inode->ea_get_saved_status() == ea_saved_status::full)
	    {
		infinint tmp;

		if(tmp_inode->ea_get_offset(tmp))
		    ent.set_archive_offset_for_EA(tmp);
		ent.set_storage_size_for_EA(tmp_inode->ea_get_size());
		if(fetch_ea)
		{
		    const ea_attributs *not_owned = tmp_inode->get_ea();

		    if(not_owned == nullptr)
			throw SRC_BUG;
		    ent.set_ea(*not_owned);
		}
	    }
	    ent.set_fsa_status(tmp_inode->fsa_get_saved_status());
	    if(tmp_inode->fsa_get_saved_status() == fsa_saved_status::full
	       || tmp_inode->fsa_get_saved_status() == fsa_saved_status::partial)
	    {
		ent.set_fsa_scope(tmp_inode->fsa_get_families());

		if(tmp_inode->fsa_get_saved_status() == fsa_saved_status::full)
		{
		    infinint tmp;

		    if(tmp_inode->fsa_get_offset(tmp))
			ent.set_archive_offset_for_FSA(tmp);
		    ent.set_storage_size_for_FSA(tmp_inode->fsa_get_size());
		}
	    }
	}

	if(tmp_file != nullptr)
	{
	    const crc *crc_tmp = nullptr;

	    ent.set_file_size(tmp_file->get_size());
	    ent.set_is_sparse_file(tmp_file->get_sparse_file_detection_read());
	    ent.set_compression_algo(tmp_file->get_compression_algo_read());
	    ent.set_dirtiness(tmp_file->is_dirty());
	    if(tmp_file->get_saved_status() == saved_status::saved
	       || tmp_file->get_saved_status() == saved_status::delta)
	    {
		ent.set_archive_offset_for_data(tmp_file->get_offset());
		ent.set_storage_size_for_data(tmp_file->get_storage_size());
		if(tmp_file->get_crc(crc_tmp) && crc_tmp != nullptr)
		    ent.set_data_crc(*crc_tmp);
	    }

	    if(tmp_file->has_delta_signature_structure())
	    {
		tmp_file->read_delta_signature_metadata();
		tmp_file->drop_delta_signature_data();
	    }
	    ent.set_delta_sig(tmp_file->has_delta_signature_available());
	    if(tmp_file->has_patch_base_crc() && tmp_file->get_patch_base_crc(crc_tmp) && crc_tmp != nullptr)
		ent.set_delta_patch_base_crc(*crc_tmp);
	    if(tmp_file->has_patch_result_crc() && tmp_file->get_patch_result_crc(crc_tmp) && crc_tmp != nullptr)
		ent.set_delta_patch_result_crc(*crc_tmp);
	}

	if(tmp_dir != nullptr)
	{
	    ent.set_file_size(tmp_dir->get_size());
	    ent.set_storage_size_for_data(tmp_dir->get_storage_size());
	    ent.set_empty_dir(tmp_dir->is_empty());
	}

	if(tmp_lien != nullptr && tmp_lien->get_saved_status() == saved_status::saved)
	    ent.set_link_target(tmp_lien->get_target());

	if(tmp_device != nullptr && tmp_device->get_saved_status() == saved_status::saved)
	{
	    ent.set_major(tmp_device->get_major());
	    ent.set_minor(tmp_device->get_minor());
	}

	if(sly != nullptr && tmp_nom != nullptr)
	    ent.set_slices(macro_tools_get_slices(tmp_nom, *sly));
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


    const char *cat_entree_signature2string(unsigned char sign)
    {
	unsigned char normalized_sig = toupper(sign);

	switch(normalized_sig)
	{
	case 'D':
	    return gettext("directory");
	case 'Z':
	    throw SRC_BUG; // EOD should never be considered by overwriting policy
	case 'M':
	    return gettext("hard linked inode");
	case 'F':
	    return gettext("plain file");
	case 'L':
	    return gettext("soft link");
	case 'C':
	    return gettext("char device");
	case 'B':
	    return gettext("block device");
	case 'P':
	    return gettext("named pipe");
	case 'S':
	    return gettext("unix socket");
	case 'X':
	    return gettext("deleted entry");
	case 'O':
	    return gettext("door inode");
	case 'I':
	    throw SRC_BUG; // ignored entry should never be found in an archive
	case 'J':
	    throw SRC_BUG; // ignored directory entry should never be found in an archive
	default:
	    throw SRC_BUG; // unknown entry type
	}
    }

} // end of namespace
