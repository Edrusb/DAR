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

#include "tronc.hpp"
#include "cat_delta_signature.hpp"

using namespace std;

namespace libdar
{

    cat_delta_signature::cat_delta_signature(generic_file & f, bool sequential_read)
    {
	init();
	read_metadata(f, sequential_read);
    }

    void cat_delta_signature::read_metadata(generic_file & f, bool sequential_read)
    {
	clear();

	patch_base_check = create_crc_from_file(f, nullptr);
	delta_sig_size.read(f);

	if(delta_sig_size.is_zero())
	    just_crc = true;
	else
	{
	    just_crc = false;
	    if(sequential_read)
	    {
		delta_sig_offset = f.get_position();
		read_data(f);
	    }
	    else
		delta_sig_offset.read(f);
	}

	patch_result_check = create_crc_from_file(f, nullptr);
    }

    void cat_delta_signature::read_data(generic_file & f)
    {
	if(!delta_sig_size.is_zero() && delta_sig_offset.is_zero())
	    throw SRC_BUG;

	if(!delta_sig_size.is_zero() && just_crc)
	    throw SRC_BUG;

	if(sig == nullptr && !delta_sig_size.is_zero())
	{
	    crc *calculated = nullptr;
	    crc *delta_sig_crc = nullptr;

	    try
	    {
		tronc bounded(&f, delta_sig_offset, delta_sig_size, false);
		infinint crc_size = tools_file_size_to_crc_size(delta_sig_size);

		sig = new memory_file();
		if(sig == nullptr)
		    throw Ememory("cat_delta_signature::read");
		sig_is_ours = true;

		bounded.skip(0);
		bounded.copy_to(*sig, crc_size, calculated);
		if(calculated == nullptr)
		    throw SRC_BUG;
		sig->skip(0);

		delta_sig_crc = create_crc_from_file(f, nullptr);
		if(delta_sig_crc == nullptr)
		    throw SRC_BUG;
		if(*delta_sig_crc != *calculated)
		    throw Erange("cat_delta_signature::read_delta_signature", gettext("CRC error met while reading delta signature: data corruption."));
	    }
	    catch(...)
	    {
		if(calculated != nullptr)
		    delete calculated;
		if(delta_sig_crc != nullptr)
		    delete delta_sig_crc;
		throw;
	    }
	    if(calculated != nullptr)
		delete calculated;
	    if(delta_sig_crc != nullptr)
		delete delta_sig_crc;
	}
    }

    memory_file *cat_delta_signature::obtain_sig()
    {
	memory_file *ret = sig;

	if(just_crc)
	    throw SRC_BUG;

	if(sig == nullptr)
	    throw SRC_BUG;
	if(!sig_is_ours)
	    throw SRC_BUG;

	sig_is_ours = false;
	sig = nullptr;

	return ret;
    }

    void cat_delta_signature::set_sig_ref(memory_file *ptr)
    {
	if(ptr == nullptr)
	    throw SRC_BUG;

	if(sig != nullptr)
	{
	    if(sig_is_ours)
		delete sig;
	}

	sig_is_ours = false;
	sig = ptr;
	just_crc = false;
	delta_sig_size = ptr->size();
    }

    void cat_delta_signature::dump_data(generic_file & f, bool sequential_mode) const
    {
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

	    if(sig == nullptr)
		throw SRC_BUG;

	    try
	    {
		me->delta_sig_offset = f.get_position();
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

	    if(sig_is_ours)
		delete sig;
	    me->sig = nullptr;
	    me->sig_is_ours = false;
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
	delta_sig_offset = 0;
	delta_sig_size = 0;
	sig = nullptr;
	sig_is_ours = false;
	patch_base_check = nullptr;
	patch_result_check = nullptr;
	just_crc = true;
    }

    void cat_delta_signature::copy_from(const cat_delta_signature & ref)
    {
	delta_sig_offset = ref.delta_sig_offset;
	delta_sig_size = ref.delta_sig_size;
	sig_is_ours = ref.sig_is_ours;
	if(sig_is_ours)
	    if(ref.sig != nullptr)
	    {
		sig = new (nothrow) memory_file(*ref.sig);
		if(sig == nullptr)
		    throw Ememory("cat_delta_signature::copy_from");
	    }
	    else
		sig = nullptr;
	else
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
	just_crc = ref.just_crc;
    }

    void cat_delta_signature::move_from(cat_delta_signature && ref) noexcept
    {
	delta_sig_offset = move(ref.delta_sig_offset);
	delta_sig_size = move(ref.delta_sig_size);

	    // for sig_is_ours field, we cannot assume bool's move operator
	    // swaps the values (implementation dependant),
	    // we need to be sure values are swapped:
	swap(sig_is_ours, ref.sig_is_ours);

	    // we can swap the memory file, because sig_is_ours is swapped
	    // too and we will known when destroying ref whether we own
	    // the object pointed to by sig or not
	swap(sig, ref.sig);
	swap(patch_base_check, ref.patch_base_check);
	swap(patch_result_check, ref.patch_result_check);
	just_crc = move(ref.just_crc);
    }

    void cat_delta_signature::clear_sig() noexcept
    {
	if(sig != nullptr)
	{
	    if(sig_is_ours)
		delete sig;
	    sig = nullptr;
	}
    }

    void cat_delta_signature::destroy() noexcept
    {
	clear_sig();
	if(patch_base_check != nullptr)
	{
	    delete patch_base_check;
	    patch_base_check = nullptr;
	}
	if(patch_result_check != nullptr)
	{
	    delete patch_result_check;
	    patch_result_check = nullptr;
	}
    }

} // end of namespace
