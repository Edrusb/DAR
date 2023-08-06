/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2023 Denis Corbin
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

#include "tronc.hpp"
#include "cat_delta_signature.hpp"
#include "tools.hpp"

using namespace std;

namespace libdar
{

    cat_delta_signature::cat_delta_signature(generic_file *f, proto_compressor *c)
    {
	init();

	src = f;
	zip = c;

	if(src == nullptr)
	    throw SRC_BUG;
	if(zip == nullptr)
	    throw SRC_BUG;
	pending_read = true;
    }

    void cat_delta_signature::read(bool sequential_read, const archive_version & ver)
    {
	if(src == nullptr)
	    throw SRC_BUG;

	try
	{
	    if(ver < archive_version(11,2))
		patch_base_check = create_crc_from_file(*src);
		// starting format 10.2 the patch base check
		// has been moved before the delta patch,
		// while this cat_delta_structure stays written
		// after the delta patch and its CRC
		// To patch_base_check is since then set
		// calling dump_patch_base_crc()
	    else
	    {
		if(patch_base_check != nullptr)
		{
		    delete patch_base_check;
		    patch_base_check = nullptr;
		}
	    }
	    delta_sig_size.read(*src);

	    if(!delta_sig_size.is_zero())
	    {
		if(sequential_read)
		{
		    delta_sig_offset = src->get_position();
		    fetch_data(ver);
		}
		else
		    delta_sig_offset.read(*src);
	    }

	    patch_result_check = create_crc_from_file(*src);
	    pending_read = false;
	}
	catch(...)
	{
	    clear();
	    throw;
	}
    }

    std::shared_ptr<memory_file> cat_delta_signature::obtain_sig(const archive_version & ver) const
    {
	if(delta_sig_size.is_zero())
	    throw SRC_BUG;

	if(!sig)
	{
	    if(src == nullptr)
		throw SRC_BUG;

	    fetch_data(ver);
	    if(!sig)
		throw SRC_BUG; // fetch_data() failed but did not raised any exception
	}

	return sig;
    }

    void cat_delta_signature::set_sig(const std::shared_ptr<memory_file> & ptr, U_I sig_block_size)
    {
	if(!ptr)
	    throw SRC_BUG;
	sig = ptr;
	delta_sig_size = sig->size();
	if(delta_sig_size.is_zero())
	    throw SRC_BUG;
	sig_block_len = sig_block_size;
	if(sig_block_len == 0)
	    throw SRC_BUG;
    }

    void cat_delta_signature::dump_data(generic_file & f, bool sequential_mode, const archive_version & ver) const
    {

	    // fetching the data if it is missing

	if(!delta_sig_size.is_zero())
	{
	    if(!sig)
		fetch_data(ver);
	}
	    // dumping data

	if(sequential_mode)
	{
		//if(!has_patch_base_crc())
		// throw SRC_BUG;
		// patch_base_check->dump(f);
		///// since format 11.2 we do not save patch_base_crc
		///// has been moved to cat_file
		///// to be saved before the delta patch and
		///// allow patching a file when reading a backup
		///// in sequential mode
		///// the field is still present in this class
		///// for backward compatibility with older format

	    delta_sig_size.dump(f);
	}

	if(!delta_sig_size.is_zero())
	{
	    infinint crc_size = tools_file_size_to_crc_size(delta_sig_size);
	    crc *calculated = nullptr;
	    cat_delta_signature *me = const_cast<cat_delta_signature *>(this);

	    try
	    {
		me->delta_sig_offset = f.get_position();
		infinint(sig_block_len).dump(f);
		if(!sig)
		    throw SRC_BUG;
		sig->skip(0);
		sig->copy_to(f, crc_size, calculated);
		if(calculated == nullptr)
		    throw SRC_BUG;
		calculated->dump(f);
	    }
	    catch(...)
	    {
		if(calculated != nullptr)
		    delete calculated;
		throw;
	    }
	    if(calculated != nullptr)
		delete calculated;
	}

	if(sequential_mode)
	{
	    if(!has_patch_result_crc())
		throw SRC_BUG;
	    patch_result_check->dump(f);
	}
    }

    void cat_delta_signature::dump_metadata(generic_file & f) const
    {
	    // if(!has_patch_base_crc())
	    // throw SRC_BUG;
	    // patch_base_check->dump(f);
	    ///// patch_base_check has moved to cat_file
	    ///// since format 11.2

	delta_sig_size.dump(f);
	if(!delta_sig_size.is_zero())
	    delta_sig_offset.dump(f);
	if(!has_patch_result_crc())
	    throw SRC_BUG;
	patch_result_check->dump(f);
    }

    bool cat_delta_signature::get_patch_base_crc(const crc * & c) const
    {
	if(patch_base_check != nullptr)
	{
	    c = patch_base_check;
	    return true;
	}
	else
	    return false;
    }

    void cat_delta_signature::set_patch_base_crc(const crc & c)
    {
	throw SRC_BUG; // no more used since format 11.2
    }

    bool cat_delta_signature::get_patch_result_crc(const crc * & c) const
    {
	if(patch_result_check != nullptr)
	{
	    c = patch_result_check;
	    return true;
	}
	else
	    return false;
    }

    void cat_delta_signature::set_patch_result_crc(const crc & c)
    {
	if(patch_result_check != nullptr)
	{
	    delete patch_result_check;
	    patch_result_check = nullptr;
	}
	patch_result_check = c.clone();
	if(patch_result_check == nullptr)
	    throw Ememory("cat_delta_signature::set_crc");
    }

    void cat_delta_signature::init() noexcept
    {
	patch_base_check = nullptr;
	delta_sig_size = 0;
	delta_sig_offset = 0;
	sig.reset();
	patch_result_check = nullptr;
	src = nullptr;
	zip = nullptr;
	sig_block_len = 0;
	pending_read = false;
    }

    void cat_delta_signature::copy_from(const cat_delta_signature & ref)
    {
	delta_sig_offset = ref.delta_sig_offset;
	delta_sig_size = ref.delta_sig_size;
	sig = ref.sig;
	if(ref.patch_base_check != nullptr)
	{
	    patch_base_check = ref.patch_base_check->clone();
	    if(patch_base_check == nullptr)
		throw Ememory("cat_delta_signature::copy_from");
	}
	else
	    patch_base_check = nullptr;
	if(ref.patch_result_check != nullptr)
	{
	    patch_result_check = ref.patch_result_check->clone();
	    if(patch_result_check == nullptr)
		throw Ememory("cat_delta_signature::copy_from");
	}
	else
	    patch_result_check = nullptr;
	src = ref.src;
	zip = ref.zip;
	pending_read = ref.pending_read;
    }

    void cat_delta_signature::move_from(cat_delta_signature && ref) noexcept
    {
	delta_sig_offset = move(ref.delta_sig_offset);
	delta_sig_size = move(ref.delta_sig_size);

       	    // we can swap the memory file, because sig_is_ours is swapped
	    // too and we will known when destroying ref whether we own
	    // the object pointed to by sig or not
	sig.swap(ref.sig);
	swap(patch_base_check, ref.patch_base_check);
	swap(patch_result_check, ref.patch_result_check);
	src = move(ref.src);
	zip = move(ref.zip);
	pending_read = move(ref.pending_read);
    }

    void cat_delta_signature::destroy() noexcept
    {
	if(patch_base_check != nullptr)
	{
	    delete patch_base_check;
	    patch_base_check = nullptr;
	}
	sig.reset();
	if(patch_result_check != nullptr)
	{
	    delete patch_result_check;
	    patch_result_check = nullptr;
	}
	src = nullptr;
	zip = nullptr;
    }

    void cat_delta_signature::fetch_data(const archive_version & ver) const
    {
	if(!delta_sig_size.is_zero() && delta_sig_offset.is_zero())
	    throw SRC_BUG;

	if(delta_sig_size.is_zero())
	    return;

	if(sig == nullptr) // we have to fetch the data
	{
	    crc *calculated = nullptr;
	    crc *delta_sig_crc = nullptr;

	    if(src == nullptr)
		throw SRC_BUG;
	    if(zip == nullptr)
		throw SRC_BUG;

		// need to suspend compression before reading the data
	    zip->suspend_compression();

	    try
	    {
		src->skip(delta_sig_offset);

		if(ver >= archive_version(10,1))
		{

			// we need first to read the block len used to build the signature

		    infinint tmp(*src);
		    sig_block_len = 0;
		    tmp.unstack(sig_block_len);
		    if(!tmp.is_zero())
			throw Erange("cat_delta_signature::fetch_data", gettext("data corrupted when attempting to read delta signature block size"));
		}
		else
		    sig_block_len = 2048; // RS_DEFAULT_BLOCK_LEN from librsync. Using value in case this macro would change in the future

		    // now we can read the delta signature itself

		tronc bounded(src, src->get_position(), delta_sig_size, false);
		infinint crc_size = tools_file_size_to_crc_size(delta_sig_size);

		sig.reset(new (nothrow) memory_file());
		if(!sig)
		    throw Ememory("cat_delta_signature::read");

		bounded.skip(0);
		bounded.copy_to(*sig, crc_size, calculated);
		if(calculated == nullptr)
		    throw SRC_BUG;
		sig->skip(0);

		delta_sig_crc = create_crc_from_file(*src);
		if(delta_sig_crc == nullptr)
		    throw Erange("cat_delta_signature::fetch_data", gettext("Error while reading CRC of delta signature data. Data corruption occurred"));
		if(*delta_sig_crc != *calculated)
		    throw Erange("cat_delta_signature::read_data", gettext("CRC error met while reading delta signature: data corruption."));
	    }
	    catch(...)
	    {
		if(calculated != nullptr)
		    delete calculated;
		if(delta_sig_crc != nullptr)
		    delete delta_sig_crc;
		sig.reset();
		throw;
	    }
	    if(calculated != nullptr)
		delete calculated;
	    if(delta_sig_crc != nullptr)
		delete delta_sig_crc;
	}
    }

} // end of namespace
