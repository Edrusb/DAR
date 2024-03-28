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

    /// \file generic_file_overlay_for_gpgme.hpp
    /// \brief adaptation class from gpgme data buffer to libdar generic_file interface
    /// \ingroup Private

#ifndef GENERIC_FILE_OVERLAY_FOR_GPGME_HPP
#define GENERIC_FILE_OVERLAY_FOR_GPGME_HPP

extern "C"
{
#if HAVE_GPGME_H
#include <gpgme.h>
#endif
}

#include "../my_config.h"
#include "generic_file.hpp"

namespace libdar
{
	/// \addtogroup Private
	/// @{

	/// generic_file interface for for gpgme

    class generic_file_overlay_for_gpgme
    {
    public:
	    /// create a gpgme data buffer of the given generic_file

	    /// \param[in] f is a pointer to an existing generic_file that must exist during the whole live of this overlay object
	generic_file_overlay_for_gpgme(generic_file *f);

	    /// no copy constructor allowed
	generic_file_overlay_for_gpgme(const generic_file_overlay_for_gpgme & ref) = delete;

	    /// no move constructor allowed
	generic_file_overlay_for_gpgme(generic_file_overlay_for_gpgme && re) noexcept = delete;

	    /// no asignment operator
	generic_file_overlay_for_gpgme & operator = (const generic_file_overlay_for_gpgme & ref) = delete;

	    /// no move operator
	generic_file_overlay_for_gpgme & operator = (generic_file_overlay_for_gpgme && ref) noexcept = delete;

#ifdef GPGME_SUPPORT
	    /// destructor
	~generic_file_overlay_for_gpgme() { gpgme_data_release(handle); };

	    /// return the handle to be used by gpgme to drive the generic_file given at constructor time
	gpgme_data_t get_gpgme_handle() const { return handle; };

	generic_file *get_below() { return below; };

    private:
	generic_file *below;
	gpgme_data_t handle;
	gpgme_data_cbs cbs;

#endif
    };

	/// @}

} // end of namespace

#endif
