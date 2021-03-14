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
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
} // extern "C"

#include "i_archive.hpp"
#include "nls_swap.hpp"

using namespace std;

namespace libdar
{

	// opens an already existing archive

    archive::archive(const std::shared_ptr<user_interaction> & dialog,
                     const path & chem,
		     const string & basename,
		     const string & extension,
		     const archive_options_read & options)
    {
        NLS_SWAP_IN;
        try
        {
	    pimpl.reset(new (nothrow) i_archive(dialog,
						chem,
						basename,
						extension,
						options));

	    if(!pimpl)
		throw Ememory("archive::archive");
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

	// creates a new archive

    archive::archive(const std::shared_ptr<user_interaction> & dialog,
                     const path & fs_root,
                     const path & sauv_path,
                     const string & filename,
                     const string & extension,
		     const archive_options_create & options,
                     statistics * progressive_report)
    {
        NLS_SWAP_IN;
        try
        {
	    pimpl.reset(new (nothrow) i_archive(dialog,
						fs_root,
						sauv_path,
						filename,
						extension,
						options,
						progressive_report));
	    if(!pimpl)
		throw Ememory("archive::archive");

	}
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
    }


	// merge constructor

    archive::archive(const std::shared_ptr<user_interaction> & dialog,
		     const path & sauv_path,
		     shared_ptr<archive> ref_arch1,
		     const string & filename,
		     const string & extension,
		     const archive_options_merge & options,
		     statistics * progressive_report)
    {
	NLS_SWAP_IN;
	try
	{
	    pimpl.reset(new (nothrow) i_archive(dialog,
						sauv_path,
						ref_arch1,
						filename,
						extension,
						options,
						progressive_report));
	    if(!pimpl)
		throw Ememory("archive::archive");
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    archive::archive(const std::shared_ptr<user_interaction> & dialog,
		     const path & chem_src,
		     const string & basename_src,
		     const string & extension_src,
		     const archive_options_read & options_read,
		     const path & chem_dst,
		     const string & basename_dst,
		     const string & extension_dst,
		     const archive_options_repair & options_repair)
    {
	NLS_SWAP_IN;
	try
	{
	    pimpl.reset(new (nothrow) i_archive(dialog,
						chem_src,
						basename_src,
						extension_src,
						options_read,
						chem_dst,
						basename_dst,
						extension_dst,
						options_repair));
	    if(!pimpl)
		throw Ememory("archive::archive");
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    archive::~archive() = default;


    statistics archive::op_extract(const path & fs_root,
				   const archive_options_extract & options,
				   statistics * progressive_report)
    {
	statistics tmp;

        NLS_SWAP_IN;
        try
        {
	    tmp = pimpl->op_extract(fs_root,
				    options,
				    progressive_report);
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;

        return tmp;
    }

    void archive::summary()
    {
        NLS_SWAP_IN;
        try
        {
	    pimpl->summary();
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    archive_summary archive::summary_data()
    {
	archive_summary tmp;

        NLS_SWAP_IN;
        try
        {
	    tmp = pimpl->summary_data();
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;

        return tmp;
    }


    void archive::op_listing(archive_listing_callback callback,
			     void *context,
			     const archive_options_listing & options) const
    {
        NLS_SWAP_IN;
        try
        {
	    pimpl->op_listing(callback,
			      context,
			      options);
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
    }

    statistics archive::op_diff(const path & fs_root,
				const archive_options_diff & options,
				statistics * progressive_report)
    {
	statistics tmp;

        NLS_SWAP_IN;
        try
        {
	    tmp = pimpl->op_diff(fs_root,
				 options,
				 progressive_report);
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;

        return tmp;
    }

    statistics archive::op_test(const archive_options_test & options,
				statistics * progressive_report)
    {
	statistics tmp;

        NLS_SWAP_IN;
        try
        {
	    tmp = pimpl->op_test(options,
				 progressive_report);
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;

        return tmp;
    }


    void archive::op_isolate(const path &sauv_path,
			     const string & filename,
			     const string & extension,
			     const archive_options_isolate & options)
    {
        NLS_SWAP_IN;
        try
        {
	    pimpl->op_isolate(sauv_path,
			      filename,
			      extension,
			      options);
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
    }

    bool archive::get_children_of(archive_listing_callback callback,
				  void *context,
                                  const string & dir,
				  bool fetch_ea)
    {
	bool tmp;

        NLS_SWAP_IN;
        try
        {
	    tmp = pimpl->get_children_of(callback,
					 context,
					 dir,
					 fetch_ea);
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;

        return tmp;
    }


    const vector<list_entry> archive::get_children_in_table(const string & dir,
							    bool fetch_ea) const
    {
	vector<list_entry> tmp;

        NLS_SWAP_IN;
        try
        {
	    tmp = pimpl->get_children_in_table(dir,
					       fetch_ea);
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;

        return tmp;
    }

    bool archive::has_subdirectory(const string & dir) const
    {
	bool tmp;

        NLS_SWAP_IN;
        try
        {
	    tmp = pimpl->has_subdirectory(dir);
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;

        return tmp;
    }

    const entree_stats archive::get_stats() const
    {
	entree_stats tmp;
        NLS_SWAP_IN;
        try
        {
	    tmp = pimpl->get_stats();
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;

        return tmp;
    }


    const list<signator> & archive::get_signatories() const
    {
	const list<signator>* tmp;

        NLS_SWAP_IN;
        try
        {
	    tmp = &(pimpl->get_signatories());
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;

        return *tmp;
    }

    void archive::init_catalogue() const
    {
        NLS_SWAP_IN;
        try
        {
	    pimpl->init_catalogue();
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
    }

    void archive::drop_all_filedescriptors()
    {
        NLS_SWAP_IN;
        try
        {
	    pimpl->drop_all_filedescriptors();
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
    }

    void archive::set_to_unsaved_data_and_FSA()
    {
	NLS_SWAP_IN;
        try
        {
	    pimpl->set_to_unsaved_data_and_FSA();
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
    }

    U_64 archive::get_first_slice_header_size() const
    {
	U_64 tmp;

        NLS_SWAP_IN;
        try
        {
	    tmp = pimpl->get_first_slice_header_size();
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;

        return tmp;
    }

    U_64 archive::get_non_first_slice_header_size() const
    {
	U_64 tmp;

        NLS_SWAP_IN;
        try
        {
	    tmp = pimpl->get_non_first_slice_header_size();
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;

        return tmp;
    }

} // end of namespace
