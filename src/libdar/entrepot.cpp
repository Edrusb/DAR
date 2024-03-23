//*********************************************************************/
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

#include <typeinfo>

extern "C"
{
#if HAVE_ERRNO_H
#include <errno.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif
} // end extern "C"

#include "entrepot.hpp"
#include "cygwin_adapt.hpp"
#include "tools.hpp"
#include "hash_fichier.hpp"
#include "cache_global.hpp"
#include "tuyau_global.hpp"

using namespace std;

namespace libdar
{

    entrepot::entrepot(): where("/"), root("/")
    {
	user = "";
	group = "";
    }

    void entrepot::set_location(const path & chemin)
    {
	if(where != chemin)
	{
	    read_dir_flush();
	    where = chemin;
	}
    }

    void entrepot::set_root(const path & p_root)
    {
	if(p_root.is_relative())
	    throw Erange("entrepot::set_root", std::string(gettext("root's entrepot must be an absolute path: ")) + p_root.display());
	root = p_root;
    }

    path entrepot::get_full_path() const
    {
	if(get_location().is_relative())
	    return get_root() + get_location();
	else
	    return get_location();
    }

    fichier_global *entrepot::open(const shared_ptr<user_interaction> & dialog,
				   const std::string & filename,
				   gf_mode mode,
				   bool force_permission,
				   U_I permission,
				   bool fail_if_exists,
				   bool erase,
				   hash_algo algo,
				   bool provide_a_plain_file) const
    {
	fichier_global *ret = nullptr;
	tuyau_global *pipe_g = nullptr;

	    // sanity check
	if(algo != hash_algo::none && (mode != gf_write_only || (!erase && !fail_if_exists)))
	    throw SRC_BUG; // if hashing is asked, we cannot accept to open an existing file without erasing its contents

	try
	{
		// creating the file to read from or write to data
	    ret = inherited_open(dialog,
				 filename,
				 mode,
				 force_permission,
				 permission,
				 fail_if_exists,
				 erase);

	    if(ret == nullptr)
		throw SRC_BUG;

	    if(!provide_a_plain_file)
	    {
		pipe_g = new (nothrow) tuyau_global(dialog,
						    ret);

		if(pipe_g == nullptr)
		    throw Ememory("entrepot::open");
		else
		    ret = nullptr; // now managed by pipe_g

		ret = pipe_g;
		pipe_g = nullptr;
	    }

	    if(algo != hash_algo::none)
	    {
		fichier_global *hash_file = nullptr;
		fichier_global *tmp = nullptr;

		    // creating the file to write hash to

		try
		{
		    hash_file = inherited_open(dialog,
					       filename+"."+hash_algo_to_string(algo),
					       gf_write_only,
					       force_permission,
					       permission,
					       fail_if_exists,
					       erase);

		    if(hash_file == nullptr)
			throw SRC_BUG;

		    try
		    {
			tmp = new (nothrow) hash_fichier(dialog,
							 ret,
							 filename,
							 hash_file,
							 algo);
			if(tmp == nullptr)
			    throw Ememory("entrepot::entrepot");
			else
			{
			    ret = tmp;
			    hash_file = nullptr;
			}
		    }
		    catch(...)
		    {
			if(hash_file != nullptr)
			    delete hash_file;
			throw;
		    }

		}
		catch(Egeneric & e)
		{
		    e.prepend_message(gettext("Error met while creating the hash file: "));
		    throw;
		}
	    }
	}
	catch(...)
	{
	    if(ret != nullptr)
	    {
		delete ret;
		ret = nullptr;
	    }
	    if(pipe_g != nullptr)
	    {
		delete pipe_g;
		pipe_g = nullptr;
	    }
	    throw;
	}

	return ret;
    }


} // end of namespace
