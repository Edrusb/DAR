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

    /// \file archive.hpp
    /// \brief the archive class is defined in this module
    /// \ingroup API


#ifndef ARCHIVE5_HPP
#define ARCHIVE5_HPP

#include "../my_config.h"

#include "archive.hpp"
#include "user_interaction5.hpp"

namespace libdar5
{
	// from archive_options.hpp
    using libdar::archive_options_read;
    using libdar::archive_options_create;
    using libdar::archive_options_isolate;
    using libdar::archive_options_merge;
    using libdar::archive_options_read;
    using libdar::archive_options_extract;
    using libdar::archive_options_listing;
    using libdar::archive_options_diff;
    using libdar::archive_options_test;
    using libdar::archive_options_repair;

	/// the archive class realizes the most general operations on archives

	/// the operations corresponds to the one the final user expects, these
	/// are the same abstraction level as the operation realized by the DAR
	/// command line tool.
	/// \ingroup API
    class archive: public libdar::archive
    {
    public:
	    // using original constructors

	using libdar::archive::archive;

	    /// overwriting op_listing to use the user_interaction as callback
	void op_listing(user_interaction & dialog,
			const archive_options_listing & options);


	    /// overloading get_children_of to use the user_interaction object as callback
	bool get_children_of(user_interaction & dialog,
			     const std::string & dir);

    private:
	static void listing_callback(void *context,
				     const std::string & flag,
				     const std::string & perm,
				     const std::string & uid,
				     const std::string & gid,
				     const std::string & size,
				     const std::string & date,
				     const std::string & filename,
				     bool is_dir,
				     bool has_children);

    };

} // end of namespace

#endif
