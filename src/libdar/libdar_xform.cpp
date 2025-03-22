/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2025 Denis Corbin
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

#include <string>
#include <new>

#include "libdar_xform.hpp"
#include "sar.hpp"
#include "trivial_sar.hpp"
#include "macro_tools.hpp"
#include "i_libdar_xform.hpp"
#include "nls_swap.hpp"

using namespace std;

namespace libdar
{
    libdar_xform::libdar_xform(const shared_ptr<user_interaction> & ui,
			       const string & chem,
			       const string & basename,
			       const string & extension,
			       const infinint & min_digits,
			       const string & execute)
    {
	NLS_SWAP_IN;
        try
        {
	    pimpl.reset(new (nothrow) i_libdar_xform(ui,
						     chem,
						     basename,
						     extension,
						     min_digits,
						     execute));
	    if(!pimpl)
		throw Ememory("libdar_xform::libdar_xform");
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    libdar_xform::libdar_xform(const shared_ptr<user_interaction> & ui,
			       const std::string & pipename)
    {
	NLS_SWAP_IN;
        try
        {
	    pimpl.reset(new (nothrow) i_libdar_xform(ui,
						     pipename));
	    if(!pimpl)
		throw Ememory("libdar_xform::libdar_xform");
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    libdar_xform::libdar_xform(libdar_xform && ref) noexcept = default;

    libdar_xform & libdar_xform::operator = (libdar_xform && ref) noexcept = default;

    libdar_xform::~libdar_xform() = default;

    libdar_xform::libdar_xform(const shared_ptr<user_interaction> & ui,
			       int filedescriptor)
    {
	NLS_SWAP_IN;
        try
        {
	    pimpl.reset(new (nothrow) i_libdar_xform(ui,
						     filedescriptor));
	    if(!pimpl)
		throw Ememory("libdar_xform::libdar_xform");
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void libdar_xform::xform_to(const string & chem,
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
	NLS_SWAP_IN;
        try
        {
	    pimpl->xform_to(chem,
			    basename,
			    extension,
			    allow_over,
			    warn_over,
			    pause,
			    first_slice_size,
			    slice_size,
			    slice_perm,
			    slice_user,
			    slice_group,
			    hash,
			    min_digits,
			    execute);
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void libdar_xform::xform_to(int filedescriptor,
				const string & execute)
    {
        NLS_SWAP_IN;
        try
        {
	    pimpl->xform_to(filedescriptor,
			    execute);
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

} // end of namespace
