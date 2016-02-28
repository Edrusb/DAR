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
#include "delta_signature.hpp"

using namespace std;

namespace libdar
{

    delta_signature::delta_signature(generic_file & f, bool sequential_read)
    {
	init();
	read(f, sequential_read);
    }


    void delta_signature::read(generic_file & f, bool sequential_read)
    {
	destroy();

	    patch_base_check = create_crc_from_file(f, get_pool());
	    delta_sig_size.read(f);
	    delta_sig_offset = f.get_position();

	    if(delta_sig_size > 0)
	    {
		if(sequential_read)
		    fetch_signature_data(f);
		else
		    delta_sig_offset.read(f);
	    }

	    patch_result_check = create_crc_from_file(f, get_pool());
    }

    void delta_signature::dump(generic_file & f, bool sequential_read) const
    {
	crc *calculated = nullptr;
	delta_signature *me = const_cast<delta_signature *>(this);

	if(read_from != nullptr)
	    me->fetch_signature_data(*read_from);
	if(! is_completed(sequential_read))
	    throw SRC_BUG;
	try
	{
	    patch_base_check->dump(f);
	    delta_sig_size.dump(f);

	    if(delta_sig_size > 0)
	    {
		if(sequential_read)
		{
		    infinint crc_size = tools_file_size_to_crc_size(delta_sig_size);
		    me->delta_sig_offset = f.get_position();
		    sig->skip(0);
		    sig->copy_to(f, crc_size, calculated);
		    if(calculated == nullptr)
			throw SRC_BUG;
		    calculated->dump(f);
		}
		else
		    delta_sig_offset.dump(f);
	    }
	    patch_result_check->dump(f);
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

    void delta_signature::attach_sig(memory_file *delta_sig)
    {
	detach_sig();
	sig = delta_sig;
    }

    const memory_file & delta_signature::get_sig() const
    {
	if(sig == nullptr && read_from != nullptr)
	{
	    delta_signature *me = const_cast<delta_signature *>(this);
	    me->fetch_signature_data(*read_from);
	}

	return *sig;
    }

    void delta_signature::detach_sig()
    {
	if(sig != nullptr)
	{
	    delete sig;
	    sig = nullptr;
	}
    }

    bool delta_signature::get_patch_base_crc(const crc * & c) const
    {
	if(patch_base_check != nullptr)
	{
	    c = patch_base_check;
	    return true;
	}
	else
	    return false;
    }

    void delta_signature::set_patch_base_crc(const crc & c)
    {
	if(patch_base_check != nullptr)
	{
	    delete patch_base_check;
	    patch_base_check = nullptr;
	}
	patch_base_check = c.clone();
	if(patch_base_check == nullptr)
	    throw Ememory("delta_signature::set_crc");
    }

    bool delta_signature::get_patch_result_crc(const crc * & c) const
    {
	if(patch_result_check != nullptr)
	{
	    c = patch_result_check;
	    return true;
	}
	else
	    return false;
    }

    void delta_signature::set_patch_result_crc(const crc & c)
    {
	if(patch_result_check != nullptr)
	{
	    delete patch_result_check;
	    patch_result_check = nullptr;
	}
	patch_result_check = c.clone();
	if(patch_result_check == nullptr)
	    throw Ememory("delta_signature::set_crc");
    }

    void delta_signature::init()
    {
	delta_sig_offset = 0;
	delta_sig_size = 0;
	sig = nullptr;
	patch_base_check = nullptr;
	patch_result_check = nullptr;
    }

    void delta_signature::destroy()
    {
	delta_sig_offset = 0;
	delta_sig_size = 0;
	if(sig != nullptr)
	{
	    delete sig;
	    sig = nullptr;
	}
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

    bool delta_signature::is_completed(bool sequential_mode) const
    {
	if(sequential_mode)
	{
	    if(sig == nullptr && delta_sig_size > 0)
		return false;
	}
	else
	{
	    if(read_from == nullptr && sig == nullptr && delta_sig_size > 0)
		return false;
	}

	if(patch_base_check == nullptr)
	    return false;
	if(patch_result_check == nullptr)
	    return false;

	return true;
    }

    void delta_signature::fetch_signature_data(generic_file &fic)
    {
	if(sig == nullptr && delta_sig_size > 0)
	{
	    crc *calculated = nullptr;
	    crc *delta_sig_crc = nullptr;

	    try
	    {
		tronc bounded = tronc(&fic, delta_sig_offset, delta_sig_size, false);
		infinint crc_size = tools_file_size_to_crc_size(delta_sig_size);

		sig = new memory_file();
		if(sig == nullptr)
		    throw Ememory("delta_signature::read");

		bounded.skip(0);
		bounded.copy_to(*sig, crc_size, calculated);
		if(calculated == nullptr)
		    throw SRC_BUG;
		sig->skip(0);

		delta_sig_crc = create_crc_from_file(fic, get_pool());
		if(delta_sig_crc == nullptr)
		    throw SRC_BUG;
		if(*delta_sig_crc != *calculated)
		    throw Erange("delta_signature::read_delta_signature", gettext("CRC error met while reading delta signature: data corruption."));
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

} // end of namespace
