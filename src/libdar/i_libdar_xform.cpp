/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2021 Denis Corbin
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
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
} // end extern "C"

#include <string>
#include <new>

#include "i_libdar_xform.hpp"
#include "sar.hpp"
#include "trivial_sar.hpp"
#include "macro_tools.hpp"
#include "tools.hpp"

using namespace std;

namespace libdar
{
    libdar_xform::i_libdar_xform::i_libdar_xform(const shared_ptr<user_interaction> & ui,
						 const string & chem,
						 const string & basename,
						 const string & extension,
						 const infinint & min_digits,
						 const string & execute): mem_ui(ui)
    {
	sar *tmp_sar = nullptr;

	can_xform = true;
	init_entrep();
	src_path.reset(new (nothrow) path(chem));
	if(!src_path)
	    throw Ememory("i_libdar_xform::lidar_xform");

	entrep_src->set_location(*src_path);

	tmp_sar = new (nothrow) libdar::sar(get_pointer(),
					    basename,
					    extension,
					    entrep_src,
					    false,
					    min_digits,
					    false,
					    execute);
	source.reset(tmp_sar);
	if(!source)
	    throw Ememory("i_libdar_xform::lidar_xform");
	if(tmp_sar == nullptr)
	    throw SRC_BUG;
	else
	{
		// yes we modify directly the object
		// we assigned to "source", only for
		// short time and simplicity not to cast
		// back to libdar::sar type
	    tmp_sar->set_info_status(CONTEXT_OP);
	    format_07_compatible = tmp_sar->is_an_old_start_end_archive();
	    dataname = tmp_sar->get_data_name();
	}
    }

    libdar_xform::i_libdar_xform::i_libdar_xform(const shared_ptr<user_interaction> & ui,
						 const std::string & pipename) : mem_ui(ui)
    {
	trivial_sar *tmp_sar = nullptr;

	can_xform = true;
	init_entrep();
	tmp_sar = new (nothrow) libdar::trivial_sar(get_pointer(), pipename, false);
	source.reset(tmp_sar);
	if(!source)
	    throw Ememory("i_libdar_xform::i_libdar_xform");
	if(tmp_sar == nullptr)
	    throw SRC_BUG;
	else
	{
	    format_07_compatible = tmp_sar->is_an_old_start_end_archive();
	    dataname = tmp_sar->get_data_name();
	}
    }

    libdar_xform::i_libdar_xform::i_libdar_xform(const shared_ptr<user_interaction> & ui,
						 int filedescriptor) : mem_ui(ui)
    {
	trivial_sar *tmp_sar = nullptr;

	can_xform = true;
	init_entrep();
	tmp_sar = new (nothrow) libdar::trivial_sar(get_pointer(), filedescriptor, false);
	source.reset(tmp_sar);
	if(!source)
	    throw Ememory("i_libdar_xform::i_libdar_xform");
	if(tmp_sar == nullptr)
	    throw SRC_BUG;
	else
	{
	    format_07_compatible = tmp_sar->is_an_old_start_end_archive();
	    dataname = tmp_sar->get_data_name();
	}
    }

    void libdar_xform::i_libdar_xform::xform_to(const string & chem,
						const string & basename,
						const string & extension,
						bool allow_over,
						bool warn_over,
						const infinint & pause,
						const infinint & first_slice_size,
						const infinint & slice_size,
						const string & slice_perm,
						const string & slice_user,
						const string & slice_group,
						hash_algo hash,
						const infinint & min_digits,
						const string & execute)
    {
	unique_ptr<path> dst_path(new (nothrow) path(chem));
	label internal_name;
	thread_cancellation thr;
        unique_ptr<generic_file> destination;
	bool force_perm = slice_perm != "";
	U_I perm = force_perm ? tools_octal2int(slice_perm) : 0;

	if(!dst_path)
	    throw Ememory("i_libdar_xform::xform_to");
	entrep_dst->set_location(*dst_path);
	entrep_dst->set_user_ownership(slice_user);
	entrep_dst->set_group_ownership(slice_group);
	tools_avoid_slice_overwriting_regex(get_ui(),
					    *entrep_dst,
					    basename,
					    extension,
					    false,
					    allow_over,
					    warn_over,
					    false);
	internal_name.generate_internal_filename();

	thr.check_self_cancellation();
	if(slice_size.is_zero()) // generating a single-sliced archive
	{
	    destination.reset(new (nothrow) libdar::trivial_sar(get_pointer(),
								gf_write_only,
								basename,
								extension,
								*entrep_dst,
								internal_name,
								dataname,
								execute,
								allow_over,
								warn_over,
								force_perm,
								perm,
								hash,
								min_digits,
								format_07_compatible));
	}
	else // generating multi-sliced archive
	{
	    destination.reset(new (nothrow) libdar::sar(get_pointer(),
							gf_write_only,
							basename,
							extension,
							slice_size,
							first_slice_size,
							warn_over,
							allow_over,
							pause,
							entrep_dst,
							internal_name,
							dataname,
							force_perm,
							perm,
							hash,
							min_digits,
							format_07_compatible,
							execute));
	}
	if(!destination)
	    throw Ememory("i_libdar_xform::xform_to");

	xform_to(destination.get());
    }

    void libdar_xform::i_libdar_xform::xform_to(int filedescriptor,
						const string & execute)
    {
	label internal_name;
	unique_ptr<generic_file> destination;

	internal_name.generate_internal_filename();
	destination.reset(macro_tools_open_archive_tuyau(get_pointer(),
							 filedescriptor,
							 gf_write_only,
							 internal_name,
							 dataname,
							 format_07_compatible,
							 execute));

	if(!destination)
	    throw Ememory("i_libdar_xform::xform_to");

	xform_to(destination.get());
    }

    void libdar_xform::i_libdar_xform::init_entrep()
    {
	entrep_src.reset(new (nothrow) entrepot_local("", "", false));
	if(!entrep_src)
	    throw Ememory("i_libdar_xform::lidar_xform");
	entrep_dst.reset(new (nothrow) entrepot_local("", "", false));
	if(!entrep_dst)
	    throw Ememory("i_libdar_xform::lidar_xform");
    }

    void libdar_xform::i_libdar_xform::xform_to(generic_file *dst)
    {
	if(dst == nullptr)
	    throw SRC_BUG;
	try
	{
	    source->copy_to(*dst);
	}
	catch(Escript & e)
	{
	    throw;
	}
	catch(Euser_abort & e)
	{
	    throw;
	}
	catch(Ebug & e)
	{
	    throw;
	}
	catch(Ethread_cancel & e)
	{
	    throw;
	}
	catch(Egeneric & e)
	{
	    string msg = string(gettext("Error transforming the archive :"))+e.get_message();
	    throw Edata(msg);
	}
    }

} // end of namespace
