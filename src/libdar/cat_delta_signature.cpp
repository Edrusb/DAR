/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2018 Denis Corbin
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

#include "tronc.hpp"
#include "cat_delta_signature.hpp"
#include "tools.hpp"

using namespace std;

namespace libdar
{

    void cat_delta_signature::read(bool sequential_read)
    {
	if(src == nullptr)
	    throw SRC_BUG;

	try
	{
	    patch_base_check = create_crc_from_file(*src);
	    delta_sig_size.read(*src);

	    if(!delta_sig_size.is_zero())
	    {
		if(sequential_read)
		{
		    delta_sig_offset = src->get_position();
		    fetch_data(*src);
		}
		else
		    delta_sig_offset.read(*src);
	    }

	    patch_result_check = create_crc_from_file(*src);
	}
	catch(...)
	{
	    clear();
	    throw;
	}
    }


    std::shared_ptr<memory_file> cat_delta_signature::obtain_sig() const
    {
	if(delta_sig_size.is_zero())
	    throw SRC_BUG;

	if(!sig)
	{
	    if(src == nullptr)
		throw SRC_BUG;
	    fetch_data(*src);
	    if(!sig)
		throw SRC_BUG; // fetch_data() failed but did not raised any exception
	}

	return sig;
    }

    void cat_delta_signature::set_sig(const std::shared_ptr<memory_file> & ptr)
    {
	if(!ptr)
	    throw SRC_BUG;
	sig = ptr;
	delta_sig_size = sig->size();
	if(delta_sig_size.is_zero())
	    throw SRC_BUG;
    }

    void cat_delta_signature::dump_data(generic_file & f, bool sequential_mode) const
    {

	    // fetching the data if it is missing

	if(!delta_sig_size.is_zero())
	{
	    if(!sig)
	    {
		if(src != nullptr)
		    fetch_data(*src);
		else
		    throw SRC_BUG; // sig should be dumped but is not present and cannot be obtained
	    }
	}
	    // dumping data

	if(sequential_mode)
	{
	    if(!has_patch_base_crc())
		throw SRC_BUG;
	    patch_base_check->dump(f);
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
	if(!has_patch_base_crc())
	    throw SRC_BUG;
	patch_base_check->dump(f);
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
	if(patch_base_check != nullptr)
	{
	    delete patch_base_check;
	    patch_base_check = nullptr;
	}
	patch_base_check = c.clone();
	if(patch_base_check == nullptr)
	    throw Ememory("cat_delta_signature::set_crc");
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
    }

    void cat_delta_signature::fetch_data(compressor & f) const
    {
	if(!delta_sig_size.is_zero() && delta_sig_offset.is_zero())
	    throw SRC_BUG;

	if(delta_sig_size.is_zero())
	    return;

	if(delta_sig_size.is_zero())
	    throw SRC_BUG;

	if(sig == nullptr) // we have to fetch the data
	{
	    crc *calculated = nullptr;
	    crc *delta_sig_crc = nullptr;
	    f.suspend_compression();

	    try
	    {
		tronc bounded(&f, delta_sig_offset, delta_sig_size, false);
		infinint crc_size = tools_file_size_to_crc_size(delta_sig_size);

		sig.reset(new (nothrow) memory_file());
		if(!sig)
		    throw Ememory("cat_delta_signature::read");

		bounded.skip(0);
		bounded.copy_to(*sig, crc_size, calculated);
		if(calculated == nullptr)
		    throw SRC_BUG;
		sig->skip(0);

		delta_sig_crc = create_crc_from_file(f);
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
