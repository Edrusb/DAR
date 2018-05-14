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

    /// \file archive5.hpp
    /// \brief API v5 backward compatible class archive
    /// \ingroup API5


#ifndef ARCHIVE5_HPP
#define ARCHIVE5_HPP

#include "../my_config.h"

#include "archive.hpp"
#include "user_interaction5.hpp"
#include "archive_options5.hpp"

namespace libdar5
{

        /// \addtogroup API5
        /// @{


	// from archive_options.hpp
    using libdar::path;
    using libdar::statistics;

	/// the archive class realizes the most general operations on archives

	/// the operations corresponds to the one the final user expects, these
	/// are the same abstraction level as the operation realized by the DAR
	/// command line tool.
	/// \ingroup API5

    class archive: public libdar::archive
    {
    public:
	archive(user_interaction & dialog,
		const path & chem,
		const std::string & basename,
		const std::string & extension,
		const archive_options_read & options):
	    libdar::archive(user_interaction5_clone_to_shared_ptr(dialog),
			    chem,
			    basename,
			    extension,
			    options)
	{}

	archive(user_interaction & dialog,
		const path & fs_root,
		const path & sauv_path,
		const std::string & filename,
		const std::string & extension,
		const archive_options_create & options,
		statistics * progressive_report):
	    libdar::archive(user_interaction5_clone_to_shared_ptr(dialog),
			    fs_root,
			    sauv_path,
			    filename,
			    extension,
			    options,
			    progressive_report)
	{}

	archive(user_interaction & dialog,
		const path & sauv_path,
		archive *ref_arch1,
		const std::string & filename,
		const std::string & extension,
		const archive_options_merge & options,
		statistics * progressive_report):
	    libdar::archive(user_interaction5_clone_to_shared_ptr(dialog),
			    sauv_path,
			    std::shared_ptr<archive>(ref_arch1),
			    filename,
			    extension,
			    options,
			    progressive_report)
	{}

	archive(user_interaction & dialog,
		const path & chem_src,
		const std::string & basename_src,
		const std::string & extension_src,
		const archive_options_read & options_read,
		const path & chem_dst,
		const std::string & basename_dst,
		const std::string & extension_dst,
		const archive_options_repair & options_repair):
	    libdar::archive(user_interaction5_clone_to_shared_ptr(dialog),
			    chem_src,
			    basename_src,
			    extension_src,
			    options_read,
			    chem_dst,
			    basename_dst,
			    extension_dst,
			    options_repair)
	{}

	statistics op_extract(user_interaction & dialog,
			      const path &fs_root,
			      const archive_options_extract & options,
			      statistics *progressive_report)
	{
	    return libdar::archive::op_extract(fs_root,
					       options,
					       progressive_report);
	}

	void summary(user_interaction & dialog)
	{
	    libdar::archive::summary();
	}

	    /// overwriting op_listing to use the user_interaction as callback
	void op_listing(user_interaction & dialog,
			const archive_options_listing & options);

	statistics op_diff(user_interaction & dialog,
			   const path & fs_root,
			   const archive_options_diff & options,
			   statistics * progressive_report)
	{
	    return libdar::archive::op_diff(fs_root,
					    options,
					    progressive_report);
	}

	statistics op_test(user_interaction & dialog,
			   const archive_options_test & options,
			   statistics * progressive_report)
	{
	    return libdar::archive::op_test(options,
					    progressive_report);
	}

	void op_isolate(user_interaction & dialog,
			const path &sauv_path,
			const std::string & filename,
			const std::string & extension,
			const archive_options_isolate & options)
	{
	    libdar::archive::op_isolate(sauv_path,
					filename,
					extension,
					options);
	}

	    /// overloading get_children_of to use the user_interaction object as callback
	bool get_children_of(user_interaction & dialog,
			     const std::string & dir);

	void init_catalogue(user_interaction & dialog) const
	{
	    libdar::archive::init_catalogue();
	}

	void drop_all_filedescriptors(user_interaction & dialog)
	{
	    libdar::archive::drop_all_filedescriptors();
	}

    private:

	static void listing_callback(const std::string & the_path,
				     const libdar::list_entry & entry,
				     void *context);
    };

	/// @}

} // end of namespace

#endif
