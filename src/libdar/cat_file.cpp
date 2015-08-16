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
#include "generic_rsync.hpp"

using namespace std;

namespace libdar
{

    cat_file::cat_file(const infinint & xuid,
		       const infinint & xgid,
		       U_16 xperm,
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
        offset = nullptr;
        size = nullptr;
        storage_size = nullptr;
        algo_read = none; // field not used for backup
        algo_write = none; // may be set later by change_compression_algo_write()
        furtive_read_mode = x_furtive_read_mode;
        file_data_status_read = 0;
        file_data_status_write = 0;
        check = nullptr;
        dirty = false;
	delta_sig_offset = 0;
	delta_sig_size = 0;
	delta_sig_crc = nullptr;
	will_have_delta_sig = false;

        try
        {
            offset = new (get_pool()) infinint(0);
            size = new (get_pool()) infinint(taille);
            storage_size = new (get_pool()) infinint(0);
            if(offset == nullptr || size == nullptr || storage_size == nullptr)
                throw Ememory("cat_file::cat_file");
        }
        catch(...)
        {
            if(offset != nullptr)
            {
                delete offset;
                offset = nullptr;
            }
            if(size != nullptr)
            {
                delete size;
                size = nullptr;
            }
            if(storage_size != nullptr)
            {
                delete storage_size;
                storage_size = nullptr;
            }
            throw;
        }
    }

    cat_file::cat_file(user_interaction & dialog,
		       const pile_descriptor & pdesc,
		       const archive_version & reading_ver,
		       saved_status saved,
		       compression default_algo,
		       bool small) : cat_inode(dialog, pdesc, reading_ver, saved, small)
    {
        chemin = "";
        status = from_cat;
        size = nullptr;
        offset = nullptr;
        storage_size = nullptr;
        check = nullptr;
        algo_read = default_algo;  // only used for archive format "03" and older
        algo_write = default_algo; // may be changed later using change_compression_algo_write()
        furtive_read_mode = false; // no used in that "status" mode
        file_data_status_read = 0;
        file_data_status_write = 0; // may be changed later using set_sparse_file_detection_write()
        dirty = false;
	delta_sig_offset = 0;
	delta_sig_size = 0;
	delta_sig_crc = nullptr;
	will_have_delta_sig = false;

	generic_file *ptr = nullptr;

	pdesc.check(small);
	if(small)
	    ptr = pdesc.esc;
	else
	    ptr = pdesc.stack;

        try
        {
            size = new (get_pool()) infinint(*ptr);
            if(size == nullptr)
                throw Ememory("cat_file::cat_file(generic_file)");

            if(!small) // inode not partially dumped
            {
                if(saved == s_saved)
                {
                    offset = new (get_pool()) infinint(*ptr);
                    if(offset == nullptr)
                        throw Ememory("cat_file::cat_file(generic_file)");
                    if(reading_ver > 1)
                    {
                        storage_size = new (get_pool()) infinint(*ptr);
                        if(storage_size == nullptr)
                            throw Ememory("cat_file::cat_file(generic_file)");
                        if(reading_ver > 7)
                        {
                            char tmp;

                            ptr->read(&file_data_status_read, sizeof(file_data_status_read));
                            if((file_data_status_read & FILE_DATA_IS_DIRTY) != 0)
                            {
                                dirty = true;
                                file_data_status_read &= ~FILE_DATA_IS_DIRTY; // removing the flag DIRTY
                            }
                            file_data_status_write = file_data_status_read;
                            ptr->read(&tmp, sizeof(tmp));
                            algo_read = char2compression(tmp);
			    algo_write= algo_read;
                        }
                        else
                            if(storage_size->is_zero()) // in older archive storage_size was set to zero if data was not compressed
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
                    else // archive format version is "1"
                    {
                        storage_size = new (get_pool()) infinint(*size);
                        if(storage_size == nullptr)
                            throw Ememory("cat_file::cat_file(generic_file)");
                        *storage_size *= 2;
                            // compressed file should be less larger than twice
                            // the original file
                            // (in case the compression is very bad
                            // and takes more place than no compression)
                    }

                    if(reading_ver >= 8)
                    {
			check = create_crc_from_file(*ptr, get_pool());
                        if(check == nullptr)
                            throw Ememory("cat_file::cat_file");
                    }
                        // before version 8, crc was dump in any case, not only when data was saved
                }
                else // not saved
                {
			// since format 9 the flag is present in any case
		    if(reading_ver >= 9)
			ptr->read(&file_data_status_read, sizeof(file_data_status_read));

                    offset = new (get_pool()) infinint(0);
                    storage_size = new (get_pool()) infinint(0);
                    if(offset == nullptr || storage_size == nullptr)
                        throw Ememory("cat_file::cat_file(generic_file)");
                }

		if((file_data_status_read & FILE_DATA_HAS_DELTA_SIG) != 0)
		{
		    will_have_delta_sig = true;
		    delta_sig_offset.read(*ptr);
		    delta_sig_size.read(*ptr);
		    delta_sig_crc = create_crc_from_file(*ptr, get_pool());
		    if(delta_sig_crc == nullptr)
			throw Ememory("cat_file::cat_file(generic_file)");
		    file_data_status_read &= ~FILE_DATA_HAS_DELTA_SIG; // removing the flag DELTA_SIG
		    file_data_status_write &= ~FILE_DATA_HAS_DELTA_SIG; // removing the flag DELTA_SIG
		}
		else
		{
		    delta_sig_offset = 0;
		    delta_sig_crc = nullptr;
		    delta_sig_size = 0;
		}

                if(reading_ver >= 2)
                {
                    if(reading_ver < 8)
                    {
                            // fixed length CRC inversion from archive format "02" to "07"
                            // present in any case, even when data is not saved
                            // for archive version >= 8, the crc is only present
                            // if the archive contains file data

                        check = create_crc_from_file(*ptr, get_pool(), true);
                        if(check == nullptr)
                            throw Ememory("cat_file::cat_file");
                    }
                        // archive version >= 8, crc only present if  saved == s_saved (seen above)
                }
                else // no CRC in version "1"
                    check = nullptr;
            }
            else // partial dump has been done
            {
                if(saved == s_saved)
                {
                    char tmp;

                    ptr->read(&file_data_status_read, sizeof(file_data_status_read));
		    if((file_data_status_read & FILE_DATA_HAS_DELTA_SIG) != 0)
		    {
			file_data_status_read &= ~FILE_DATA_HAS_DELTA_SIG;
			will_have_delta_sig = true;
		    }
                    file_data_status_write = file_data_status_read;
                    ptr->read(&tmp, sizeof(tmp));
                    algo_read = char2compression(tmp);
		    algo_write = algo_read;
                }
		else // not saved
		{
			// since format 9 the flag is present in any case
		    if(reading_ver >= 9)
		    {
			ptr->read(&file_data_status_read, sizeof(file_data_status_read));
			if((file_data_status_read & FILE_DATA_HAS_DELTA_SIG) != 0)
			{
			    file_data_status_read &= ~FILE_DATA_HAS_DELTA_SIG;
			    will_have_delta_sig = true;
			}
			file_data_status_write = file_data_status_read;
		    }
		}

                    // Now that all data has been read, setting default value for the undumped ones:

                if(saved == s_saved)
                    offset = new (get_pool()) infinint(0); // can only be set from post_constructor
                else
                    offset = new (get_pool()) infinint(0);
                if(offset == nullptr)
                    throw Ememory("cat_file::cat_file(generic_file)");

                storage_size = new (get_pool()) infinint(0); // cannot known the storage_size at that time
                if(storage_size == nullptr)
                    throw Ememory("cat_file::cat_file(generic_file)");

                check = nullptr;
            }
        }
        catch(...)
        {
            detruit();
            throw;
        }
    }

    void cat_file::post_constructor(const pile_descriptor & pdesc)
    {
	cat_inode::post_constructor(pdesc);

	pdesc.check(true);

        if(offset == nullptr)
            throw SRC_BUG;
        else
	    *offset = pdesc.esc->get_position(); // data follows right after the inode+file information+CRC
    }

    cat_file::cat_file(const cat_file & ref) : cat_inode(ref)
    {
        status = ref.status;
        chemin = ref.chemin;
        offset = nullptr;
        size = nullptr;
        storage_size = nullptr;
        check = nullptr;
        dirty = ref.dirty;
        algo_read = ref.algo_read;
        algo_write = ref.algo_write;
        furtive_read_mode = ref.furtive_read_mode;
        file_data_status_read = ref.file_data_status_read;
        file_data_status_write = ref.file_data_status_write;
	delta_sig_crc = nullptr;

        try
        {
            if(ref.check != nullptr || (ref.get_escape_layer() != nullptr && ref.get_saved_status() == s_saved))
            {
		if(ref.check == nullptr)
		{
		    const crc *tmp = nullptr;
		    ref.get_crc(tmp);
		    if(ref.check == nullptr) // failed to read the crc from escape layer
			throw SRC_BUG;
		}
		check = ref.check->clone();
                if(check == nullptr)
                    throw Ememory("cat_file::cat_file(cat_file)");
            }
            else
                check = nullptr;
            offset = new (get_pool()) infinint(*ref.offset);
            size = new (get_pool()) infinint(*ref.size);
            storage_size = new (get_pool()) infinint(*ref.storage_size);
            if(offset == nullptr || size == nullptr || storage_size == nullptr)
                throw Ememory("cat_file::cat_file(cat_file)");

	    delta_sig_offset = ref.delta_sig_offset;
	    delta_sig_size = ref.delta_sig_size;

	    if(ref.delta_sig_crc != nullptr)
	    {
		delta_sig_crc = ref.delta_sig_crc->clone();
		if(delta_sig_crc == nullptr)
		    throw Ememory("cat_file::cat_file(cat_file)");
	    }
	    else
		delta_sig_crc = nullptr;

	    will_have_delta_sig = ref.will_have_delta_sig;

        }
        catch(...)
        {
            detruit();
            throw;
        }
    }


    void cat_file::inherited_dump(const pile_descriptor & pdesc, bool small) const
    {
	generic_file *ptr = nullptr;
	char flags = has_delta_signature() ? FILE_DATA_HAS_DELTA_SIG : 0;

	    // setting ptr

	pdesc.check(small);
	if(small)
	    ptr = pdesc.esc;
	else
	    ptr = pdesc.stack;

	    // setting flag

	flags |= file_data_status_write;
	if(dirty)
	    flags |= FILE_DATA_IS_DIRTY;

	    // dumping the cat_inode part of the object

        cat_inode::inherited_dump(pdesc, small);

	    // dumping th cat_file part of the inode

        size->dump(*ptr);
        if(!small)
        {
            if(get_saved_status() == s_saved)
            {
                char tmp = compression2char(algo_write);

                offset->dump(*ptr);
                storage_size->dump(*ptr);
                ptr->write(&flags, sizeof(flags));
                ptr->write(&tmp, sizeof(tmp));

                    // since archive version 8, crc is only present for saved inode
                if(check == nullptr)
                    throw SRC_BUG; // no CRC to dump!
                else
                    check->dump(*ptr);
            }
	    else // since format 9 flag is present in any case to know whether object has delta signature
                ptr->write(&flags, sizeof(flags));

	    if(has_delta_signature())
	    {
		delta_sig_offset.dump(*ptr);
		delta_sig_size.dump(*ptr);
		if(delta_sig_crc == nullptr)
		    throw SRC_BUG;
		delta_sig_crc->dump(*ptr);
	    }
        }
        else // we only know whether the file will be compressed or using sparse_file data structure
        {
            if(get_saved_status() == s_saved)
            {
                char tmp = compression2char(algo_write);

                (void)ptr->write(&flags, sizeof(flags));
                (void)ptr->write(&tmp, sizeof(tmp));
            }
	    else
	    {
		    // since format 9 the flag is present in any case
		(void)ptr->write(&flags, sizeof(flags));
	    }
        }
    }

    bool cat_file::has_changed_since(const cat_inode & ref, const infinint & hourshift, cat_inode::comparison_fields what_to_check) const
    {
        const cat_file *tmp = dynamic_cast<const cat_file *>(&ref);
        if(tmp != nullptr)
            return cat_inode::has_changed_since(*tmp, hourshift, what_to_check) || *size != *(tmp->size);
        else
            throw SRC_BUG;
    }

    generic_file *cat_file::get_data(get_data_mode mode, memory_file *delta_sig) const
    {
        generic_file *ret = nullptr;

	try
	{
		// sanity checks

	    if(get_saved_status() != s_saved)
		throw Erange("cat_file::get_data", gettext("cannot provide data from a \"not saved\" file object"));

	    if(status == empty)
		throw Erange("cat_file::get_data", gettext("data has been cleaned, object is now empty"));

	    if(delta_sig != nullptr)
	    {
		if(delta_sig->get_mode() == gf_read_only)
		    throw SRC_BUG;
		switch(mode)
		{
		case keep_compressed:
		    throw SRC_BUG;
		case keep_hole:
		    throw SRC_BUG;
		case normal:
		case plain:
		    break;
		default:
		    throw SRC_BUG;
		}
		if(get_sparse_file_detection_read())
		    throw SRC_BUG; // sparse_file detection is not compatible with detla signature calculation
		    // for merging operation, this is the object where we write to that can perform hole detection
		    // while the object from we read is able to perform delta signature calculation
	    }

		//

	    if(status == from_path)
	    {
		fichier_local *tmp = nullptr;
		if(mode != normal && mode != plain)
		    throw SRC_BUG; // keep compressed/keep_hole is not possible on an inode take from a filesystem
		ret = tmp = new (get_pool()) fichier_local(chemin, furtive_read_mode);
		if(tmp != nullptr)
			// telling *tmp to flush the data from the cache as soon as possible
		    tmp->fadvise(fichier_global::advise_dontneed);

		if(delta_sig != nullptr)
		{
		    pile *data = new (get_pool()) pile();
		    if(data == nullptr)
			throw Ememory("cat_file::get_data");
		    try
		    {
			data->push(tmp);
		    }
		    catch(...)
		    {
			delete data;
			throw;
		    }
		    ret = data;

		    generic_rsync *delta = new (get_pool()) generic_rsync(delta_sig, data->top());
		    if(delta == nullptr)
			throw Ememory("cat_file::get_data");
		    try
		    {
			data->push(delta);
		    }
		    catch(...)
		    {
			delete delta;
			throw;
		    }
		}
	    }
	    else // inode from archive
		if(get_pile() == nullptr)
		    throw SRC_BUG; // set_archive_localisation never called or with a bad argument
		else
		    if(get_pile()->get_mode() == gf_write_only)
			throw SRC_BUG; // cannot get data from a write-only file !!!
		    else
		    {
			    // we will return a small stack of generic_file over the catalogue stack
			pile *data = new (get_pool()) pile();
			if(data == nullptr)
			    throw Ememory("cat_file::get_data");

			try
			{

				// changing the compression algo of the archive stack

			    if(get_compression_algo_read() != none && mode != keep_compressed)
			    {
				if(get_compression_algo_read() != get_compressor_layer()->get_algo())
				{
				    get_pile()->flush_read_above(get_compressor_layer());
				    get_compressor_layer()->resume_compression();
				    if(get_compression_algo_read() != get_compressor_layer()->get_algo())
					throw SRC_BUG;
				}
				    // else nothing to do, compressor is already properly configured
			    }
			    else  // disabling de-compression
			    {
				if(get_compressor_layer()->get_algo() != none)
				{
				    get_pile()->flush_read_above(get_compressor_layer());
				    get_compressor_layer()->suspend_compression();
				}
				    // else nothing to do, de-compression is already disabled
			    }

				// adding a tronc object in the local stack object when no compression is used

			    if(!get_small_read())
			    {
				if(get_compression_algo_read() == none)
				{
				    generic_file *tmp = new (get_pool()) tronc(get_pile(), *offset, *storage_size, gf_read_only);
				    if(tmp == nullptr)
					throw Ememory("cat_file::get_data");
				    try
				    {
					data->push(tmp);
					data->skip(0);
				    }
				    catch(...)
				    {
					delete tmp;
					throw;
				    }
				}
				else
				    get_pile()->skip(*offset);
			    }

				// determining on which layer to rely on for the next to come sparse file

			    generic_file *parent = data->is_empty() ? get_pile() : data->top();


				// adding a sparse_file object in top of the local stack
				//
				// if a sparse_file object is to be used, it must be placed on top of the
				// returned stack, in order to benefit from the sparse_file::copy_to() specific methods
				// that can restore holes

			    if(get_sparse_file_detection_read() && mode != keep_compressed && mode != keep_hole)
			    {
				sparse_file *stmp = new (get_pool()) sparse_file(parent);
				if(stmp == nullptr)
				    throw Ememory("cat_file::get_data");
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

				// normally having both delta_sig and sparse_file is excluded by sanity checks
				// so we do not add delta_sig above sparse_file which would deserve sparse file
				// detection

			    if(delta_sig != nullptr)
			    {
				generic_rsync *delta = new (get_pool()) generic_rsync(delta_sig, data->top());
				if(delta == nullptr)
				    throw Ememory("cat_file::get_data");
				try
				{
				    data->push(delta);
				}
				catch(...)
				{
				    delete delta;
				    throw;
				}
			    }

				// if the stack to return is empty adding a tronc
				// to have the proper offset zero at the beginning of the data
				//
				// but we must not check the offset coherence between current read
				// position and underlying position when compression is used below
				// because it would lead the tronc to ask the compressor to seek
				// in the compressed data at the current position of uncompressed data

			    if(data->is_empty())
			    {
				tronc *tronc_tmp;
				generic_file *tmp = tronc_tmp = new (get_pool()) tronc(get_pile(), *offset, gf_read_only);
				if(tmp == nullptr)
				    throw Ememory("cat_file::get_data");
				if(tronc_tmp == nullptr)
				    throw SRC_BUG;

				try
				{
				    tronc_tmp->check_underlying_position_while_reading_or_writing(false);
				    data->push(tmp);
				}
				catch(...)
				{
				    delete tmp;
				    throw;
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
	    if(ret != nullptr)
		delete ret;
	    throw;
	}

	if(ret == nullptr)
	    throw Ememory("cat_file::get_data");
	else
	    return ret;
    }

    void cat_file::clean_data()
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

    void cat_file::set_offset(const infinint & r)
    {
	if(status == empty)
	    throw SRC_BUG;
	set_saved_status(s_saved);
	*offset = r;
    }

    const infinint & cat_file::get_offset() const
    {
	if(get_saved_status() != s_saved)
	    throw SRC_BUG;
	if(offset == nullptr)
	    throw SRC_BUG;
	return *offset;
    }

    void cat_file::set_crc(const crc &c)
    {
	if(check != nullptr)
	{
	    delete check;
	    check = nullptr;
	}
	check = c.clone();
	if(check == nullptr)
	    throw Ememory("cat_file::set_crc");
    }

    bool cat_file::get_crc(const crc * & c) const
    {
	if(get_escape_layer() == nullptr)
	    if(check != nullptr)
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
		if(check == nullptr)
		{
		    try
		    {
			get_pile()->flush_read_above(get_escape_layer());
			if(get_escape_layer()->skip_to_next_mark(escape::seqt_file_crc, false))
			{
			    crc *tmp = nullptr;

				// first, recording storage_size (needed when isolating a catalogue in sequential read mode)
			    if(storage_size->is_zero())
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
			    if(tmp == nullptr)
				throw SRC_BUG;
			    else
			    {
				const_cast<cat_file *>(this)->check = tmp;
				tmp = nullptr; // object now owned by "this"
			    }
			}
			else
			    throw Erange("cat_file::cat_file", gettext("can't read data CRC: No escape mark found for that file"));
		    }
		    catch(...)
		    {
			    // we assign a default crc to the object
			    // to avoid trying reading it again later on
			if(check == nullptr)
			{
			    const_cast<cat_file *>(this)->check = new (get_pool()) crc_n(1);
			    if(check == nullptr)
				throw Ememory("cat_file::cat_file");
			}
			throw;
		    }
		}

		if(check == nullptr)
		    throw SRC_BUG; // should not be nullptr now!
		else
		    c = check;
		return true;
	    }
	    else
		return false;
	}
    }

    bool cat_file::get_crc_size(infinint & val) const
    {
	if(check != nullptr)
	{
	    val = check->get_size();
	    return true;
	}
	else
	    return false;
    }

    void cat_file::dump_delta_signature(memory_file &sig, generic_file & where)
    {
	infinint crc_size;

	delta_sig_size = sig.size();
	crc_size = tools_file_size_to_crc_size(delta_sig_size);

	if(delta_sig_crc != nullptr)
	{
	    delete delta_sig_crc;
	    delta_sig_crc = nullptr;
	}

	delta_sig_size.dump(where);
	delta_sig_offset = where.get_position();
	sig.skip(0);
	sig.copy_to(where, crc_size, delta_sig_crc);
	if(delta_sig_crc == nullptr)
	    throw SRC_BUG;
	else
	    delta_sig_crc->dump(where);
    }

    void cat_file::read_delta_signature(memory_file & sig) const
    {
	crc *calculated = nullptr;
	infinint crc_size;
	generic_file *from = nullptr;
	escape *esc = nullptr;
	bool small = get_small_read();
	cat_file *me = const_cast<cat_file *>(this);

	try
	{

	    if(me == nullptr)
		throw SRC_BUG;

	    switch(status)
	    {
	    case empty:
		throw SRC_BUG;
	    case from_path:
		throw SRC_BUG; // signature is caluculated while reading the data (get_data()) and kept by the caller
	    case from_cat:
		from = get_compressor_layer();
		if(from == nullptr)
		    throw SRC_BUG;
		esc = get_escape_layer();
		if(small && esc == nullptr)
		    throw SRC_BUG;
		break;
	    default:
		throw SRC_BUG;
	    }

	    if(small)
	    {
		if(!esc->skip_to_next_mark(escape::seqt_delta_sig, false))
		    throw Erange("cat_file::read_delta_signature", gettext("can't find mark for delta signature"));
		me->delta_sig_size.read(*from);
		me->delta_sig_offset = from->get_position();

		if(delta_sig_crc != nullptr)
		{
		    delete delta_sig_crc;
		    me->delta_sig_crc = nullptr;
		}
		crc_size = tools_file_size_to_crc_size(delta_sig_size);
	    }
	    else // direct access mode
	    {
		if(delta_sig_crc == nullptr)
		    throw SRC_BUG;
		    // we should know the CRC of the delta signature

		crc_size = delta_sig_crc->get_size();
	    }

	    sig.reset(); // makes a brand-new empty memory_file object
	    try
	    {
		tronc bounded = tronc(from, delta_sig_offset, delta_sig_size, false);

		bounded.skip(0);
		bounded.copy_to(sig, crc_size, calculated);
		if(calculated == nullptr)
		    throw SRC_BUG;

		if(small)
		{
		    me->delta_sig_crc = create_crc_from_file(*from, get_pool());
		    if(delta_sig_crc == nullptr)
			throw SRC_BUG;
		}

		if(*delta_sig_crc != *calculated)
		    throw Erange("cat_file::read_delta_signature", gettext("CRC error: data corruption."));
	    }
	    catch(...)
	    {
		if(calculated != nullptr)
		    delete calculated;
		throw;
	    }
	    if(calculated != nullptr)
		delete calculated;

	    sig.skip(0);
	}
	catch(Egeneric & e)
	{
	    e.prepend_message(gettext("Error while retrieving delta signature from the archive: "));
	    throw;
	}
    }

    void cat_file::sub_compare(const cat_inode & other, bool isolated_mode) const
    {
	const cat_file *f_other = dynamic_cast<const cat_file *>(&other);
	if(f_other == nullptr)
	    throw SRC_BUG; // cat_inode::compare should have called us with a correct argument

	if(get_size() != f_other->get_size())
	{
	    infinint s1 = get_size();
	    infinint s2 = f_other->get_size();
	    throw Erange("cat_file::sub_compare", tools_printf(gettext("not same size: %i <--> %i"), &s1, &s2));
	}

	if(get_saved_status() == s_saved && f_other->get_saved_status() == s_saved)
	{
	    if(!isolated_mode)
	    {
		generic_file *me = get_data(normal, nullptr);
		if(me == nullptr)
		    throw SRC_BUG;
		try
		{
		    generic_file *you = f_other->get_data(normal, nullptr);
		    if(you == nullptr)
			throw SRC_BUG;
			// requesting read_ahead for the peer object
			// if the object is found on filesystem its
			// storage_size is zero, which lead a endless
			// read_ahead request, suitable for the current
			// context:
		    try
		    {
			crc *value = nullptr;
			const crc *original = nullptr;
			infinint crc_size;

			if(has_crc())
			{
			    if(get_crc(original))
			    {
				if(original == nullptr)
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
			    if(me->diff(*you,
					get_storage_size(),
					f_other->get_storage_size(),
					crc_size,
					value,
					err_offset))
				throw Erange("cat_file::sub_compare", tools_printf(gettext("different file data, offset of first difference is: %i"), &err_offset));
				// data is the same, comparing the CRC values

			    if(get_crc(original))
			    {
				if(value == nullptr)
				    throw SRC_BUG;
				if(original->get_size() != value->get_size())
				    throw Erange("cat_file::sub_compare", gettext("Same data but CRC value could not be verified because we did not guessed properly its width (sequential read restriction)"));
				if(*original != *value)
				    throw Erange("cat_file::sub_compare", gettext("Same data but stored CRC does not match the data!?!"));
			    }

				// else old archive without CRC

			}
			catch(...)
			{
			    if(value != nullptr)
				delete value;
			    throw;
			}
			if(value != nullptr)
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
		if(check == nullptr)
		    throw SRC_BUG;

		generic_file *you = f_other->get_data(normal, nullptr);
		if(you == nullptr)
		    throw SRC_BUG;

		try
		{
		    crc *other_crc = create_crc_from_size(check->get_size(), get_pool());
		    if(other_crc == nullptr)
			throw SRC_BUG;

		    try
		    {
			null_file ignore = gf_write_only;

			you->copy_to(ignore, check->get_size(), other_crc);

			if(check->get_size() != other_crc->get_size()
			   || *check != *other_crc)
			    throw Erange("cat_file::compare", tools_printf(gettext("CRC difference concerning file's data (comparing with an isolated catalogue)")));
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

    void cat_file::detruit()
    {
        if(offset != nullptr)
        {
            delete offset;
            offset = nullptr;
        }
        if(size != nullptr)
        {
            delete size;
            size = nullptr;
        }
        if(storage_size != nullptr)
        {
            delete storage_size;
            storage_size = nullptr;
        }
        if(check != nullptr)
        {
            delete check;
            check = nullptr;
        }
	if(delta_sig_crc != nullptr)
	{
	    delete delta_sig_crc;
	    delta_sig_crc = nullptr;
	}
    }


} // end of namespace
