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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

    /// \file archive_options_listing_shell.hpp
    /// \brief this file contains shell_interaction options class for listing
    /// \ingroup API

#ifndef ARCHIVE_OPTIONS_LISTING_SHELL_HPP
#define ARCHIVE_OPTIONS_LISTING_SHELL_HPP

#include "../my_config.h"
#include "archive_options.hpp"

namespace libdar
{

	/// \addtogroup API
        /// @{


	// ///////////////////////////////////////////////////////
	// //// OPTIONS FOR LISTING AN ARCHIVE AS IN SHELL ///////
	// ///////////////////////////////////////////////////////

	/// class holding optional shell specific parameters used to list the contents of an existing archive

	/// \note this class is both used for historical reason and user convenience, but to simplify/clarity the API
	/// the original archive_options_listing only retain the shell independant parameters
    class archive_options_listing_shell: public archive_options_listing
    {
    public:
	archive_options_listing_shell() { clear(); };
	archive_options_listing_shell(const archive_options_listing_shell & ref) = default;
	archive_options_listing_shell(archive_options_listing_shell && ref) noexcept = default;
	archive_options_listing_shell & operator = (const archive_options_listing_shell & ref) = default;
	archive_options_listing_shell & operator = (archive_options_listing_shell && ref) noexcept = default;
	~archive_options_listing_shell() = default;

	virtual void clear() override;

	    /// defines the way archive listing is done:

	enum listformat
	{
	    normal,   ///< the tar-like listing (this is the default)
	    tree,     ///< the original dar's tree listing (for those that like forest)
	    xml,      ///< the xml catalogue output
	    slicing   ///< the slicing output (give info about where files are located)
	};


	    /////////////////////////////////////////////////////////////////////
	    // setting methods

	void set_list_mode(listformat list_mode) { x_list_mode = list_mode; set_slicing_location(list_mode == slicing); };
	void set_sizes_in_bytes(bool arg) { x_sizes_in_bytes = arg; };

	    /////////////////////////////////////////////////////////////////////
	    // getting methods

	listformat get_list_mode() const { return x_list_mode; };
	bool get_sizes_in_bytes() const { return x_sizes_in_bytes; };

    private:
	listformat x_list_mode;
	bool x_sizes_in_bytes;
    };

	/// @}

} // end of namespace

#endif
