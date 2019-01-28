/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2019 Denis Corbin
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
#include "compile_time_features.hpp"
#include "tools.hpp"

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
        chemin = (che.append(src)).display();
        status = from_path;
        set_saved_status(saved_status::saved);
        offset = nullptr;
        size = nullptr;
        storage_size = nullptr;
        algo_read = compression::none; // field not used for backup
        algo_write = compression::none; // may be set later by change_compression_algo_write()
        furtive_read_mode = x_furtive_read_mode;
        file_data_status_read = 0;
        file_data_status_write = 0;
        check = nullptr;
        dirty = false;
	delta_sig = nullptr;
	delta_sig_read = false;

        try
        {
            offset = new (nothrow) infinint(0);
            size = new (nothrow) infinint(taille);
            storage_size = new (nothrow) infinint(0);
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

    cat_file::cat_file(const shared_ptr<user_interaction> & dialog,
		       const smart_pointer<pile_descriptor> & pdesc,
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
	delta_sig = nullptr;
	delta_sig_read = false;

	generic_file *ptr = nullptr;

	pdesc->check(small);
	if(small)
	    ptr = pdesc->esc;
	else
	    ptr = pdesc->stack;

        try
        {
            size = new (nothrow) infinint(*ptr);
            if(size == nullptr)
                throw Ememory("cat_file::cat_file(generic_file)");

            if(!small) // inode not partially dumped
            {
                if(saved == saved_status::saved
		    || saved == saved_status::delta)
                {
                    offset = new (nothrow) infinint(*ptr);
                    if(offset == nullptr)
                        throw Ememory("cat_file::cat_file(generic_file)");
                    if(reading_ver > 1)
                    {
                        storage_size = new (nothrow) infinint(*ptr);
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

			    if((file_data_status_read & FILE_DATA_HAS_DELTA_SIG) != 0)
				will_have_delta_signature_structure();
			    file_data_status_read &= ~FILE_DATA_HAS_DELTA_SIG;

                            file_data_status_write = file_data_status_read;

                            ptr->read(&tmp, sizeof(tmp));
                            algo_read = char2compression(tmp);
			    algo_write = algo_read;
                        }
                        else
                            if(storage_size->is_zero()) // in older archive storage_size was set to zero if data was not compressed
                            {
                                *storage_size = *size;
                                algo_read = compression::none;
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
                        storage_size = new (nothrow) infinint(*size);
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
			check = create_crc_from_file(*ptr);
                        if(check == nullptr)
                            throw Ememory("cat_file::cat_file");
                    }
                        // before version 8, crc was dump in any case, not only when data was saved
			// general case treated below
                }
                else // not saved
                {
			// since format 10 the flag is present in any case
		    if(reading_ver >= 10)
		    {
			ptr->read(&file_data_status_read, sizeof(file_data_status_read));

			if((file_data_status_read & FILE_DATA_HAS_DELTA_SIG) != 0)
			    will_have_delta_signature_structure();
			file_data_status_read &= ~FILE_DATA_HAS_DELTA_SIG;
		    }

                    offset = new (nothrow) infinint(0);
                    storage_size = new (nothrow) infinint(0);
                    if(offset == nullptr || storage_size == nullptr)
                        throw Ememory("cat_file::cat_file(generic_file)");
                }

		    // treating case of version below 8 where CRC
		    // was saved in any case (s_saved, s_not_saved,...)

                if(reading_ver >= 2)
                {
                    if(reading_ver < 8)
                    {
                            // fixed length CRC inversion from archive format "02" to "07"
                            // present in any case, even when data is not saved
                            // for archive version >= 8, the crc is only present
                            // if the archive contains file data

                        check = create_crc_from_file(*ptr, true);
                        if(check == nullptr)
                            throw Ememory("cat_file::cat_file");
                    }
                        // archive version >= 8, crc only present if  saved == s_saved/s_delta or when delta_sig (treated above)
                }
                else // no CRC in version "1"
                    check = nullptr;

		if(delta_sig != nullptr)
		{
		    delta_sig->read(false);
		    delta_sig_read = true;
		}
            }
            else // partial dump has been done
            {
                if(saved == saved_status::saved
		   || saved == saved_status::delta)
                {
                    char tmp;

                    ptr->read(&file_data_status_read, sizeof(file_data_status_read));
		    if((file_data_status_read & FILE_DATA_HAS_DELTA_SIG) != 0)
		    {
			file_data_status_read &= ~FILE_DATA_HAS_DELTA_SIG;
			will_have_delta_signature_structure();
		    }
                    file_data_status_write = file_data_status_read;
                    ptr->read(&tmp, sizeof(tmp));
                    algo_read = char2compression(tmp);
		    algo_write = algo_read;
                }
		else // not saved
		{
			// since format 10 the flag is present in any case
		    if(reading_ver >= 10)
		    {
			ptr->read(&file_data_status_read, sizeof(file_data_status_read));
			if((file_data_status_read & FILE_DATA_HAS_DELTA_SIG) != 0)
			{
			    file_data_status_read &= ~FILE_DATA_HAS_DELTA_SIG;
			    will_have_delta_signature_structure();
			}
			file_data_status_write = file_data_status_read;
		    }
		}

                    // Now that all data has been read, setting default value for the undumped ones:

		offset = new (nothrow) infinint(0); // can only be set from post_constructor
                if(offset == nullptr)
                    throw Ememory("cat_file::cat_file(generic_file)");

                storage_size = new (nothrow) infinint(0); // cannot know the storage_size at that time
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
	delta_sig = nullptr;
	delta_sig_read = ref.delta_sig_read;

        try
        {
            if(ref.check != nullptr || (ref.get_escape_layer() != nullptr
					&& (ref.get_saved_status() == saved_status::saved
					    || ref.get_saved_status() == saved_status::delta)))
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
            offset = new (nothrow) infinint(*ref.offset);
            size = new (nothrow) infinint(*ref.size);
            storage_size = new (nothrow) infinint(*ref.storage_size);
            if(offset == nullptr || size == nullptr || storage_size == nullptr)
                throw Ememory("cat_file::cat_file(cat_file)");

	    if(ref.delta_sig != nullptr)
	    {
		delta_sig = new (nothrow) cat_delta_signature(*ref.delta_sig);
		if(delta_sig == nullptr)
		    throw Ememory("cat_file::cat_file(cat_file)");
	    }
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
	char flags = has_delta_signature_structure() ? FILE_DATA_HAS_DELTA_SIG : 0;

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
            if(get_saved_status() == saved_status::saved
	       || get_saved_status() == saved_status::delta)
            {
                char tmp = compression2char(algo_write);

                offset->dump(*ptr);
                storage_size->dump(*ptr);
                ptr->write(&flags, sizeof(flags));
                ptr->write(&tmp, sizeof(tmp));
            }
	    else // since format 9 flag is present in any case to know whether object has delta signature
                ptr->write(&flags, sizeof(flags));

	    if(get_saved_status() == saved_status::saved
	       || get_saved_status() == saved_status::delta)
	    {
                    // since archive version 8, crc is only present for saved inode
                if(check == nullptr)
                    throw SRC_BUG; // no CRC to dump!
                else
                    check->dump(*ptr);
	    }

	    if(has_delta_signature_structure())
		delta_sig->dump_metadata(*ptr);
        }
        else // we only know whether the file will be compressed or using sparse_file data structure
        {
            if(get_saved_status() == saved_status::saved
	       || get_saved_status() == saved_status::delta)
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

    bool cat_file::has_changed_since(const cat_inode & ref, const infinint & hourshift, comparison_fields what_to_check) const
    {
        const cat_file *tmp = dynamic_cast<const cat_file *>(&ref);
        if(tmp != nullptr)
            return cat_inode::has_changed_since(*tmp, hourshift, what_to_check) || *size != *(tmp->size);
        else
            throw SRC_BUG;
    }

    generic_file *cat_file::get_data(get_data_mode mode,
				     shared_ptr<memory_file> delta_sig_mem,
				     shared_ptr<memory_file> delta_ref,
				     const crc **checksum) const
    {
        generic_file *ret = nullptr;

	try
	{
		// sanity checks

	    if(!can_get_data())
		throw Erange("cat_file::get_data", gettext("cannot provide data from a \"not saved\" file object"));

	    if(delta_ref
	       && get_saved_status() != saved_status::delta)
		throw SRC_BUG;

	    if(delta_ref
	       && status != from_path)
		throw Efeature("building an binary difference (rsync) for a file located in an archive");

	    if(status == empty)
		throw Erange("cat_file::get_data", gettext("data has been cleaned, object is now empty"));

	    if(delta_sig_mem)
	    {
		if(delta_sig_mem->get_mode() == gf_read_only)
		    throw SRC_BUG;
		else
		    delta_sig_mem->reset();

		switch(mode)
		{
		case keep_compressed:
		    throw SRC_BUG;
		case keep_hole:
		    throw SRC_BUG;
		case normal:
		    if(get_sparse_file_detection_read())
			throw SRC_BUG; // sparse_file detection is not compatible with delta signature calculation
			// for merging operation, this is the object where we write to that can perform hole detection
			// while the object from we read is able to perform delta signature calculation
		    break;
		case plain:
		    break;
		default:
		    throw SRC_BUG;
		}
	    }

	    if(delta_ref)
	    {
		switch(mode)
		{
		case keep_compressed:
		    throw SRC_BUG;
		case keep_hole:
		    throw SRC_BUG;
		case normal:
		    if(get_sparse_file_detection_read())
			throw SRC_BUG;
		    break;
		case plain:
		    break;
		default:
		    throw SRC_BUG;
		}
	    }

		//

	    if(status == from_path)
	    {
		fichier_local *tmp = nullptr;
		if(mode != normal && mode != plain)
		    throw SRC_BUG; // keep compressed/keep_hole is not possible on an inode take from a filesystem
		ret = tmp = new (nothrow) fichier_local(chemin, furtive_read_mode);
		if(tmp != nullptr)
			// telling *tmp to flush the data from the cache as soon as possible
		    tmp->fadvise(fichier_global::advise_dontneed);

		if(delta_sig_mem || delta_ref)
		{
		    pile *data = new (nothrow) pile();
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

		    if(delta_sig_mem)
		    {
			generic_rsync *delta = new (nothrow) generic_rsync(delta_sig_mem.get(), data->top());
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

		    if(delta_ref)
		    {
			generic_rsync *diff = new (nothrow) generic_rsync(delta_ref.get(),
									  data->top(),
									  tools_file_size_to_crc_size(get_size()),
									  checksum);
			if(diff == nullptr)
			    throw Ememory("cat_file::get_data");
			try
			{
			    data->push(diff);
			}
			catch(...)
			{
			    delete diff;
			    throw;
			}
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
			pile *data = new (nothrow) pile();
			if(data == nullptr)
			    throw Ememory("cat_file::get_data");

			try
			{

				// changing the compression algo of the archive stack

			    if(get_compression_algo_read() != compression::none && mode != keep_compressed)
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
				if(get_compressor_layer()->get_algo() != compression::none)
				{
				    get_pile()->flush_read_above(get_compressor_layer());
				    get_compressor_layer()->suspend_compression();
				}
				    // else nothing to do, de-compression is already disabled
			    }

				// adding a tronc object in the local stack object when no compression is used

			    if(!get_small_read())
			    {
				if(get_compression_algo_read() == compression::none)
				{
				    generic_file *tmp = new (nothrow) tronc(get_pile(), *offset, *storage_size, gf_read_only);
				    if(tmp == nullptr)
					throw Ememory("cat_file::get_data");
				    try
				    {
					data->push(tmp);
				    }
				    catch(...)
				    {
					delete tmp;
					throw;
				    }
				    data->skip(0);
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
				sparse_file *stmp = new (nothrow) sparse_file(parent);
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
				// detection (archive reading, restoration, testing, diffing, ...),
				// but while merging we may have both sparse_file to provide plain file without hole
				// and generic_rsync to compute the delta signature of the plain file

			    parent = data->is_empty() ? get_pile() : data->top();

			    if(delta_sig_mem)
			    {
				generic_rsync *delta = new (nothrow) generic_rsync(delta_sig_mem.get(), parent);
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
				generic_file *tmp = tronc_tmp = new (nothrow) tronc(get_pile(), *offset, gf_read_only);
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
	*offset = r;
    }

    const infinint & cat_file::get_offset() const
    {
	if(get_saved_status() != saved_status::saved
	   && get_saved_status() != saved_status::delta)
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
	    if(get_saved_status() == saved_status::saved
	       || get_saved_status() == saved_status::delta)
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

			    tmp = create_crc_from_file(*(get_escape_layer()));
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
			    const_cast<cat_file *>(this)->check = new (nothrow) crc_n(1);
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

    bool cat_file::get_patch_base_crc(const crc * & c) const
    {
	if(delta_sig == nullptr)
	    return false;
	else
	{
	    if(delta_sig->has_patch_base_crc())
	    {
		delta_sig->get_patch_base_crc(c);
		return true;
	    }
	    else
		return false;
	}
    }

    void cat_file::set_patch_base_crc(const crc & c)
    {
	if(delta_sig == nullptr)
	    throw SRC_BUG;

	delta_sig->set_patch_base_crc(c);
    }

    bool cat_file::get_patch_result_crc(const crc * & c) const
    {
	if(delta_sig != nullptr)
	{
	    if(delta_sig->has_patch_result_crc())
	    {
		delta_sig->get_patch_result_crc(c);
		return true;
	    }
	    else
		throw SRC_BUG;
	}
	else if(check != nullptr && get_saved_status() == saved_status::saved)
	{
	    c = check;
	    return true;
	}
	else
	    return false;
    }

    void cat_file::set_patch_result_crc(const crc & c)
    {
	if(!has_delta_signature_structure())
	    throw SRC_BUG; // the patch_result_crc is only
	    // here to record the real CRC a file has once
	    // restored. When a file is a delta patch, the "check"
	    // field is only the checksum of the patch, not of
	    // the resulting patched file.
	    // If we want to do a new delta patch on a existing patch
	    // we will need the real CRC of the data not the CRC
	    // of the current patch at restoration time to check
	    // this second patch is applied to the correct file.
	    //
	    // This field is thus used only when delta_signature
	    // is present and when the file's data is already a
	    // patch (saved_status::delta) or the saved_status::not_saved status of a
	    // unchanged patch.

	delta_sig->set_patch_result_crc(c);
    }

    void cat_file::will_have_delta_signature_structure()
    {
	generic_file *ptr = nullptr;
	compressor *zip = nullptr;

	if(delta_sig == nullptr)
	{
	    switch(status)
	    {
	    case empty:
		throw SRC_BUG;
	    case from_path:
		delta_sig = new (nothrow) cat_delta_signature();
		break;
	    case from_cat:
		ptr = get_read_cat_layer(get_small_read());
		if(ptr == nullptr)
		    throw SRC_BUG;
		zip = get_compressor_layer();
		if(zip == nullptr)
		    throw SRC_BUG;
		delta_sig = new (nothrow) cat_delta_signature(ptr, zip);
		break;
	    default:
		throw SRC_BUG;
	    }

	    if(delta_sig == nullptr)
		throw Ememory("cat_file::will_have_delta_signature()");
	}
    }

    void cat_file::will_have_delta_signature_available()
    {
	will_have_delta_signature_structure();
	if(delta_sig == nullptr)
	    throw SRC_BUG;

	delta_sig->will_have_signature();
    }

    void cat_file::dump_delta_signature(shared_ptr<memory_file> & sig, generic_file & where, bool small) const
    {
	infinint crc_size;

	if(delta_sig == nullptr)
	    throw SRC_BUG;

	const_cast<cat_delta_signature *>(delta_sig)->set_sig(sig);
	delta_sig->dump_data(where, small);
    }

    void cat_file::dump_delta_signature(generic_file & where, bool small) const
    {
	infinint crc_size;

	if(delta_sig == nullptr)
	    throw SRC_BUG;

	const_cast<cat_delta_signature *>(delta_sig)->set_sig();
	delta_sig->dump_data(where, small);
    }

    void cat_file::read_delta_signature_metadata() const
    {
	compressor *from = nullptr;
	escape *esc = nullptr;
	bool small = get_small_read();

	if(delta_sig == nullptr)
	    throw SRC_BUG;

	if(!delta_sig_read)
	{
	    try
	    {
		switch(status)
		{
		case empty:
		    throw SRC_BUG;
		case from_path:
		    throw SRC_BUG; // signature is calculated while reading the data (get_data()) and kept by the caller
		case from_cat:
		    from = get_compressor_layer();
		    if(from == nullptr)
			throw SRC_BUG;
		    else
			from->suspend_compression();
		    esc = get_escape_layer();
		    if(small && esc == nullptr)
			throw SRC_BUG;
		    break;
		default:
		    throw SRC_BUG;
		}

		if(small)
		{
		    if(!esc->skip_to_next_mark(escape::seqt_delta_sig, true))
			throw Erange("cat_file::read_delta_signature", gettext("can't find mark for delta signature"));
		}

		delta_sig->read(small);
		delta_sig_read = true;
	    }
	    catch(Egeneric & e)
	    {
		if(delta_sig != nullptr)
		{
		    cat_file *me = const_cast<cat_file *>(this);
		    if(me == nullptr)
			throw SRC_BUG;

		    delete delta_sig;
		    me->delta_sig = nullptr;
		}
		e.prepend_message(gettext("Error while retrieving delta signature from the archive: "));
		throw;
	    }
	}
    }

    void cat_file::read_delta_signature(shared_ptr<memory_file> & delta_sig_ret) const
    {
	read_delta_signature_metadata();

	if(delta_sig->can_obtain_sig())
	    delta_sig_ret = delta_sig->obtain_sig();
	else
	    delta_sig_ret.reset();
    }

    void cat_file::drop_delta_signature_data() const
    {
	if(delta_sig != nullptr)
	{
	    cat_file *me = const_cast<cat_file *>(this);

	    if(me == nullptr)
		throw SRC_BUG;
	    me->delta_sig->drop_sig();
	}
    }

    bool cat_file::has_same_delta_signature(const cat_file & ref) const
    {
	shared_ptr<memory_file> sig_me;
	shared_ptr<memory_file> sig_you;

	read_delta_signature(sig_me);
	ref.read_delta_signature(sig_you);

	if(!sig_me)
	    throw SRC_BUG;
	if(!sig_you)
	    throw SRC_BUG;

	infinint size_me = sig_me->size();
	infinint size_you = sig_you->size();

	if(sig_me->size() != sig_you->size())
	    return false;
	else
	    return *sig_me == *sig_you;
    }

    void cat_file::clear_delta_signature_only()
    {
	if(delta_sig != nullptr)
	{
	    if(get_saved_status() == saved_status::delta)
		delta_sig->drop_sig();
	    else
		clear_delta_signature_structure();
	}
    }

    void cat_file::clear_delta_signature_structure()
    {
	if(delta_sig != nullptr)
	{
	    delete delta_sig;
	    delta_sig = nullptr;
	}
    }

    bool cat_file::same_data_as(const cat_file & other, bool check_data, const infinint & hourshift)
    {
	bool ret = true;
	try
	{
	    sub_compare_internal(other, false, check_data, hourshift);
	}
	catch(Erange & e)
	{
	    ret = false;
	}
	return ret;
    }

    void cat_file::sub_compare(const cat_inode & other, bool isolated_mode) const
    {
	sub_compare_internal(other, !isolated_mode, true, 0);
    }

    void cat_file::sub_compare_internal(const cat_inode & other,
					bool can_read_my_data,
					bool can_read_other_data,
					const infinint & hourshift) const
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

	if(!tools_is_equal_with_hourshift(hourshift, get_last_modif(), other.get_last_modif()))
	{
	    string s1 = tools_display_date(get_last_modif());
	    string s2 = tools_display_date(other.get_last_modif());
	    throw Erange("cat_file::sub_compare_internal", tools_printf(gettext("difference of last modification date: %S <--> %S"), &s1, &s2));
	}

	if(!can_read_other_data) // we cannot compare data, nor CRC, no signature, so we just rely on mtime
	{
		// not that hourshift (field from cat_inode) is only used in that context
		// the mtime comparison is also done at cat_inode level when calling compare()
		// but in that contaxt can_read_other_data is true, this we do not compare twice mtimes
	    return; //<<< we stop the method here in that case
	}


	if(f_other->get_saved_status() != saved_status::saved)
	    throw SRC_BUG; // we should compare with a plain object provided by a filesystem object

	if(get_saved_status() == saved_status::saved && can_read_my_data)
	{
		// compare file content and CRC

	    generic_file *me = get_data(normal, nullptr, nullptr);
	    if(me == nullptr)
		throw SRC_BUG;
	    try
	    {
		generic_file *you = f_other->get_data(normal, nullptr, nullptr);
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
	else if(has_delta_signature_available()
		&& (compile_time::librsync() || f_other->has_delta_signature_available()))
	{
		// calculate delta signature of file and comparing them

	    if(f_other->has_delta_signature_available())
		    // this should never be the case, but well it does not hurt taking this case into account
	    {
		    // both only have delta signature

		if(!has_same_delta_signature(*f_other))
		    throw Erange("cat_file::sub_compare", gettext("Delta signature do not match"));
	    }
	    else
	    {
		    // we only have signature and you only have data

		shared_ptr<memory_file> sig_me;
		shared_ptr<memory_file> sig_you(new (nothrow) memory_file());
		null_file trash = gf_write_only;
		generic_file *data;

		if(!sig_you)
		    throw Ememory("cat_file::sub_compare_internal");

		data = f_other->get_data(normal, sig_you, shared_ptr<memory_file>());

		if(data == nullptr)
		    throw SRC_BUG;

		try
		{
		    data->copy_to(trash);
		}
		catch(...)
		{
		    delete data;
		    throw;
		}
		delete data;
		data = nullptr;

		read_delta_signature(sig_me);
		if(!sig_me)
		    throw SRC_BUG;

		try
		{
		    infinint size_me = sig_me->size();
		    infinint size_you = sig_you->size();

		    if(size_me != size_you)
			throw Erange("cat_file::sub_compare", tools_printf(gettext("Delta signature do not have the same size: %i <--> %i"), &size_me, &size_you));
		    if(*sig_me != *sig_you) // comparing file's content
			throw Erange("cat_file::sub_compare", gettext("Delta signature have the same size but do not match"));
		}
		catch(...)
		{
		    drop_delta_signature_data();
		    throw;
		}
		drop_delta_signature_data();
	    }
	}
	else // isolated_mode and no signature or no data
	{
	    const crc *my_crc = nullptr;

	    if(get_saved_status() == saved_status::delta && !has_patch_result_crc())
		throw SRC_BUG;

		// if we have a CRC available for data
	    if((get_saved_status() != saved_status::delta && get_crc(my_crc))
	       ||
	       (get_saved_status() == saved_status::delta && get_patch_result_crc(my_crc)))
	    {
		    // just compare CRC (as for isolated_mode)

		if(my_crc == nullptr)
		    throw SRC_BUG;

		generic_file *you = f_other->get_data(normal, nullptr, nullptr);
		if(you == nullptr)
		    throw SRC_BUG;

		try
		{
		    crc *other_crc = nullptr;

		    try
		    {
			null_file ignore = gf_write_only;

			you->copy_to(ignore, my_crc->get_size(), other_crc);

			if(my_crc->get_size() != other_crc->get_size()
			   || *my_crc != *other_crc)
			    throw Erange("cat_file::compare", tools_printf(gettext("CRC difference concerning file's data")));
		    }
		    catch(...)
		    {
			if(other_crc != nullptr)
			    delete other_crc;
			throw;
		    }
		    if(other_crc != nullptr)
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
	if(delta_sig != nullptr)
	{
	    delete delta_sig;
	    delta_sig = nullptr;
	}
    }


} // end of namespace
