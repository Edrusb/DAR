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
#include "tools.hpp"
#include "generic_file_overlay_for_gpgme.hpp"

namespace libdar
{

#ifdef GPGME_SUPPORT

    static ssize_t gpgme_data_read_cb(void *handle, void *buffer, size_t size);
    static ssize_t gpgme_data_write_cb(void *handle, const void *buffer, size_t size);
    static off_t gpgme_data_seek_cb(void *handle, off_t offset, int whence);
    static void gpgme_data_release_cb(void *handle);

#endif

    generic_file_overlay_for_gpgme::generic_file_overlay_for_gpgme(generic_file *f)
    {

#ifdef GPGME_SUPPORT
	gpgme_error_t err;

	if(f == nullptr)
	    throw SRC_BUG;
	below = f;
	cbs.read = &gpgme_data_read_cb;
	cbs.write = &gpgme_data_write_cb;
	cbs.seek = &gpgme_data_seek_cb;
	cbs.release = &gpgme_data_release_cb;

	err = gpgme_data_new_from_cbs(&handle, &cbs, this);
	if(gpgme_err_code(err) != GPG_ERR_NO_ERROR)
	{
	    throw Erange("generic_file_overlay_for_gpgme::generi_file_overlay_for_gpgme", tools_printf(gettext("Error creating data buffer overlay for GPGME: %s"), tools_gpgme_strerror_r(err).c_str()));
	}
#else
	throw Efeature("Asymetric Strong encryption algorithms using GPGME");
#endif
    }

#if GPGME_SUPPORT
    static ssize_t gpgme_data_read_cb(void *handle, void *buffer, size_t size)
    {
	generic_file_overlay_for_gpgme *obj = (generic_file_overlay_for_gpgme *)(handle);

	return obj->get_below()->read((char*)buffer, size);
    }

    static ssize_t gpgme_data_write_cb(void *handle, const void *buffer, size_t size)
    {
	generic_file_overlay_for_gpgme *obj = (generic_file_overlay_for_gpgme *)(handle);

	obj->get_below()->write((char*)buffer, size);
	return size;
    }

    static off_t gpgme_data_seek_cb(void *handle, off_t offset, int whence)
    {
	generic_file_overlay_for_gpgme *obj = (generic_file_overlay_for_gpgme *)(handle);
	off_t ret;

	switch(whence)
	{
	case SEEK_SET:
	    obj->get_below()->skip(infinint(offset));
	    break;
	case SEEK_CUR:
	    obj->get_below()->skip_relative(offset);
	    break;
	case SEEK_END:
	    obj->get_below()->skip_to_eof();
	    obj->get_below()->skip_relative(offset);
	default:
	    throw SRC_BUG;
	}

	if(whence == SEEK_SET)
	    ret = offset;
	else
	{
	    infinint tmp = obj->get_below()->get_position();

	    ret = 0;
	    tmp.unstack(ret);
	    if(!tmp.is_zero())
		throw Erange("gpgme_data_seek_cb", gettext("File offset too large to be stored in off_t type"));
	}

	return ret;
    }


    static void gpgme_data_release_cb(void *handle)
    {
	    // nothing to do
	    // the generic_file_overlay_for_gpgme object
	    // has no internal data to be released by gpgme
    }

#endif

} // end of namespace
