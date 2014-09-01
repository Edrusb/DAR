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

#include "cat_file.hpp"
#include "fichier_local.hpp"
#include "pile.hpp"
#include "tronc.hpp"
#include "compressor.hpp"
#include "sparse_file.hpp"
#include "null_file.hpp"

using namespace std;

namespace libdar
{

    file::file(const infinint & xuid, const infinint & xgid, U_16 xperm,
               const datetime & last_access,
               const datetime & last_modif,
               const datetime & last_change,
               const string & src,
               const path & che,
               const infinint & taille,
               const infinint & fs_device,
               bool x_furtive_read_mode) : cat_inode(xuid, xgid, xperm, last_access, last_modif, last_change, src, fs_device)
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
            offset = new (get_pool()) infinint(0);
            size = new (get_pool()) infinint(taille);
            storage_size = new (get_pool()) infinint(0);
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
               compressor *efsa_loc,
               escape *ptr) : cat_inode(dialog, f, reading_ver, saved, efsa_loc, ptr)
    {
        chemin = "";
        status = from_cat;
        size = NULL;
        offset = NULL;
        storage_size = NULL;
        check = NULL;
        algo_read = default_algo;  // only used for archive format "03" and older
        algo_write = default_algo; // may be changed later using change_compression_algo_write()
        furtive_read_mode = false; // no used in that "status" mode
        file_data_status_read = 0;
        file_data_status_write = 0; // may be changed later using set_sparse_file_detection_write()
        loc = data_loc;
        dirty = false;
        try
        {
            size = new (get_pool()) infinint(f);
            if(size == NULL)
                throw Ememory("file::file(generic_file)");

            if(ptr == NULL) // inode not partially dumped
            {
                if(saved == s_saved)
                {
                    offset = new (get_pool()) infinint(f);
                    if(offset == NULL)
                        throw Ememory("file::file(generic_file)");
                    if(reading_ver > 1)
                    {
                        storage_size = new (get_pool()) infinint(f);
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
				algo_write = algo_read;
			    }
                    }
                    else // version is "01"
                    {
                        storage_size = new (get_pool()) infinint(*size);
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
			check = create_crc_from_file(f, get_pool());
                        if(check == NULL)
                            throw Ememory("file::file");
                    }
                        // before version 8, crc was dump in any case, not only when data was saved
                }
                else // not saved
                {
                    offset = new (get_pool()) infinint(0);
                    storage_size = new (get_pool()) infinint(0);
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

                        check = create_crc_from_file(f, get_pool(), true);
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
                    offset = new (get_pool()) infinint(0); // can only be set from post_constructor
                else
                    offset = new (get_pool()) infinint(0);
                if(offset == NULL)
                    throw Ememory("file::file(generic_file)");

                storage_size = new (get_pool()) infinint(0); // cannot known the storage_size at that time
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

    file::file(const file & ref) : cat_inode(ref)
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
            if(ref.check != NULL || (get_escape_layer() != NULL && ref.get_saved_status() == s_saved))
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
            offset = new (get_pool()) infinint(*ref.offset);
            size = new (get_pool()) infinint(*ref.size);
            storage_size = new (get_pool()) infinint(*ref.storage_size);
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
        cat_inode::inherited_dump(f, small);
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

    bool file::has_changed_since(const cat_inode & ref, const infinint & hourshift, cat_inode::comparison_fields what_to_check) const
    {
        const file *tmp = dynamic_cast<const file *>(&ref);
        if(tmp != NULL)
            return cat_inode::has_changed_since(*tmp, hourshift, what_to_check) || *size != *(tmp->size);
        else
            throw SRC_BUG;
    }

    generic_file *file::get_data(get_data_mode mode) const
    {
        generic_file *ret = NULL;

	try
	{
	    if(get_saved_status() != s_saved)
		throw Erange("file::get_data", gettext("cannot provide data from a \"not saved\" file object"));

	    if(status == empty)
		throw Erange("file::get_data", gettext("data has been cleaned, object is now empty"));

	    if(status == from_path)
	    {
		fichier_local *tmp = NULL;
		if(mode != normal && mode != plain)
		    throw SRC_BUG; // keep compressed/keep_hole is not possible on an inode take from a filesystem
		ret = tmp = new (get_pool()) fichier_local(chemin, furtive_read_mode);
		if(tmp != NULL)
			// telling *tmp to flush the data from the cache as soon as possible
		    tmp->fadvise(fichier_global::advise_dontneed);
	    }
	    else // inode from archive
		if(loc == NULL)
		    throw SRC_BUG; // set_archive_localisation never called or with a bad argument
		else
		    if(loc->get_mode() == gf_write_only)
			throw SRC_BUG; // cannot get data from a write-only file !!!
		    else
		    {
			pile *data = new (get_pool()) pile();
			if(data == NULL)
			    throw Ememory("file::get_data");

			try
			{
			    generic_file *tmp;

			    if(get_escape_layer() == NULL)
				tmp = new (get_pool()) tronc(loc, *offset, *storage_size, gf_read_only);
			    else
				tmp = new (get_pool()) tronc(get_escape_layer(), *offset, gf_read_only);
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
				tmp = new (get_pool()) compressor(get_compression_algo_read(), *data->top());
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
				sparse_file *stmp = new (get_pool()) sparse_file(data->top());
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
	if(get_saved_status() != s_saved)
	    throw SRC_BUG;
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

			    tmp = create_crc_from_file(*(get_escape_layer()), get_pool());
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
			    const_cast<file *>(this)->check = new (get_pool()) crc_n(1);
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

    void file::sub_compare(const cat_inode & other, bool isolated_mode) const
    {
	const file *f_other = dynamic_cast<const file *>(&other);
	if(f_other == NULL)
	    throw SRC_BUG; // cat_inode::compare should have called us with a correct argument

	if(get_size() != f_other->get_size())
	{
	    infinint s1 = get_size();
	    infinint s2 = f_other->get_size();
	    throw Erange("file::sub_compare", tools_printf(gettext("not same size: %i <--> %i"), &s1, &s2));
	}

	if(get_saved_status() == s_saved && f_other->get_saved_status() == s_saved)
	{
	    if(!isolated_mode)
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
			    // data is the same, comparing the CRC values

			    if(get_crc(original))
			    {
				if(value == NULL)
				    throw SRC_BUG;
				if(original->get_size() != value->get_size())
				    throw Erange("file::sub_compare", gettext("Same data but CRC value could not be verified because we did not guessed properly its width (sequential read restriction)"));
				if(*original != *value)
				    throw Erange("file::sub_compare", gettext("Same data but stored CRC does not match the data!?!"));
			    }

			    // else old archive without CRC

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
	    else // isolated mode
	    {
		if(check == NULL)
		    throw SRC_BUG;

		generic_file *you = f_other->get_data(normal);
		if(you == NULL)
		    throw SRC_BUG;

		try
		{
		    crc *other_crc = create_crc_from_size(check->get_size(), get_pool());
		    if(other_crc == NULL)
			throw SRC_BUG;

		    try
		    {
			null_file ignore = gf_write_only;

			you->copy_to(ignore, check->get_size(), other_crc);

			if(check->get_size() != other_crc->get_size()
			   || *check != *other_crc)
			    throw Erange("file::compare", tools_printf(gettext("CRC difference concerning file's data (comparing with an isolated catalogue)")));
		    }
		    catch(...)
		    {
			delete other_crc;
			throw;
		    }
		    delete other_crc;
		}
		catch(...)
		{
		    delete you;
		    throw;
		}
		delete you;
	    }
	}
    }


} // end of namespace

