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
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#if HAVE_ERRNO_H
#include <errno.h>
#endif
} // end extern "C"

#include <iomanip>
#include <iostream>
#include <deque>
#include "nls_swap.hpp"
#include "database.hpp"
#include "i_database.hpp"

using namespace libdar;
using namespace std;

namespace libdar
{

    database::database(const shared_ptr<user_interaction> & dialog)
    {
	NLS_SWAP_IN;
	try
	{
	    pimpl.reset(new (nothrow) i_database(dialog));

	    if(!pimpl)
		throw Ememory("database::database");
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    database::database(const shared_ptr<user_interaction> & dialog,
		       const string & base,
		       const database_open_options & opt)
    {
	NLS_SWAP_IN;
	try
	{
	    pimpl.reset(new (nothrow) i_database(dialog,
						 base,
						 opt));

	    if(!pimpl)
		throw Ememory("database::database");
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    database::~database() = default;

    void database::dump(const std::string & filename,
			const database_dump_options & opt) const
    {
	NLS_SWAP_IN;
	try
	{
	    pimpl->dump(filename,
			opt);
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void database::add_archive(const archive & arch,
			       const string & chemin,
			       const string & basename,
			       const database_add_options & opt)
    {
	NLS_SWAP_IN;
	try
	{
	    pimpl->add_archive(arch,
			       chemin,
			       basename,
			       opt);
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void database::remove_archive(archive_num min,
				  archive_num max,
				  const database_remove_options & opt)
    {
	NLS_SWAP_IN;
	try
	{
	    pimpl->remove_archive(min,
				  max,
				  opt);
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void database::set_permutation(archive_num src, archive_num dst)
    {
	NLS_SWAP_IN;
	try
	{
	    pimpl->set_permutation(src, dst);
	}
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
    }

    void database::change_name(archive_num num,
			       const string & basename,
			       const database_change_basename_options &opt)
    {
	NLS_SWAP_IN;
	try
	{
	    pimpl->change_name(num,
			       basename,
			       opt);
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void database::set_path(archive_num num,
			    const string & chemin,
			    const database_change_path_options & opt)
    {
	NLS_SWAP_IN;
	try
	{
	    pimpl->set_path(num,
			    chemin,
			    opt);
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void database::change_crypto_algo_pass(archive_num num,
					   crypto_algo algo,
					   const secu_string & pass,
					   const database_change_crypto_options & opt)
    {
	NLS_SWAP_IN;
	try
	{
	    pimpl->change_crypto_algo_pass(num,
					   algo,
					   pass,
					   opt);
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void database::clear_crypto_algo_pass(archive_num num, const database_numbering & opt)
    {
	NLS_SWAP_IN;
	try
	{
	    pimpl->clear_crypto_algo_pass(num, opt);
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void database::set_options(const vector<string> & opt)
    {
	NLS_SWAP_IN;
	try
	{
	    pimpl->set_options(opt);
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void database::set_dar_path(const string & chemin)
    {
	NLS_SWAP_IN;
	try
	{
	    pimpl->set_dar_path(chemin);
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void database::set_compression(compression algozip) const
    {
	NLS_SWAP_IN;
	try
	{
	    pimpl->set_compression(algozip);
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void database::set_compression_level(U_I compr_level) const
    {
	NLS_SWAP_IN;
	try
	{
	    pimpl->set_compression_level(compr_level);
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }


    void database::set_database_crypto_algo(crypto_algo algo)
    {
	NLS_SWAP_IN;
	try
	{
	    pimpl->set_database_crypto_algo(algo);
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void database::set_database_crypto_pass(const secu_string & key)
    {
	NLS_SWAP_IN;
	try
	{
	    pimpl->set_database_crypto_pass(key);
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void database::set_database_kdf_hash(hash_algo val)
    {
	NLS_SWAP_IN;
	try
	{
	    pimpl->set_database_kdf_hash(val);
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void database::set_database_kdf_iteration_count(const infinint & val)
    {
	NLS_SWAP_IN;
	try
	{
	    pimpl->set_database_kdf_iteration_count(val);
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void database::set_database_crypto_block_size(U_32 val)
    {
	NLS_SWAP_IN;
	try
	{
	    pimpl->set_database_crypto_block_size(val);;
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }


    database_archives_list database::get_contents() const
    {
	database_archives_list ret;

	NLS_SWAP_IN;
	try
	{
	    ret = pimpl->get_contents();
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;

	return ret;
    }

    vector<string> database::get_options() const
    {
	vector<string> ret;

	NLS_SWAP_IN;
	try
	{
	    ret = pimpl->get_options();
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;

	return ret;
    }

    string database::get_dar_path() const
    {
	string ret;

	NLS_SWAP_IN;
	try
	{
	    ret = pimpl->get_dar_path();
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;

	return ret;
    }

    compression database::get_compression() const
    {
	compression ret;

	NLS_SWAP_IN;
	try
	{
	    ret = pimpl->get_compression();
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;

	return ret;
    }

    U_I database::get_compression_level() const
    {
	U_I ret;

	NLS_SWAP_IN;
	try
	{
	    ret = pimpl->get_compression_level();
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;

	return ret;
    }

    string database::get_database_version() const
    {
	string ret;

	NLS_SWAP_IN;
	try
	{
	    ret = pimpl->get_database_version();
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;

	return ret;
    }

    crypto_algo database::get_database_crypto_algo() const
    {
	crypto_algo ret;

	NLS_SWAP_IN;
	try
	{
	    ret = pimpl->get_database_crypto_algo();
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;

	return ret;
    }

    hash_algo database::get_database_kdf_hash() const
    {
	hash_algo ret;

	NLS_SWAP_IN;
	try
	{
	    ret = pimpl->get_database_kdf_hash();
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;

	return ret;
    }

    const infinint & database::get_database_kdf_iteration() const
    {
	const infinint *ret = nullptr;

	NLS_SWAP_IN;
	try
	{
	    ret = &(pimpl->get_database_kdf_iteration());
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;

	if(ret == nullptr)
	    throw SRC_BUG;

	return *ret;
    }


    U_32 database::get_database_crypto_block_size() const
    {
	U_32 ret = 0;

	NLS_SWAP_IN;
	try
	{
	    ret = pimpl->get_database_crypto_block_size();
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;

	return ret;
    }


    void database::get_files(database_listing_show_files_callback callback,
			     void *context,
			     archive_num num,
			     const database_used_options & opt) const
    {
	NLS_SWAP_IN;
	try
	{
	    pimpl->get_files(callback,
			     context,
			     num,
			     opt);
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void database::get_version(database_listing_get_version_callback callback,
			       void *context,
			       path chemin) const
    {
	NLS_SWAP_IN;
	try
	{
	    pimpl->get_version(callback,
			       context,
			       chemin);
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void database::show_most_recent_stats(database_listing_statistics_callback callback,
					  void *context) const
    {
	NLS_SWAP_IN;
	try
	{
	    pimpl->show_most_recent_stats(callback,
					  context);
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void database::restore(const vector<string> & filename,
			   const database_restore_options & opt)
    {
	NLS_SWAP_IN;
	try
	{
	    pimpl->restore(filename,
			   opt);
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void database::restore(const archive_options_read & read_options,
			   const path & fs_root,
			   const archive_options_extract & extract_options,
			   const database_restore_options & opt,
			   statistics* progressive_report)
    {
	NLS_SWAP_IN;
	try
	{
	    pimpl->restore(read_options,
			   fs_root,
			   extract_options,
			   opt,
			   progressive_report);
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }


    bool database::check_order() const
    {
	bool ret;

	NLS_SWAP_IN;
	try
	{
	    ret = pimpl->check_order();
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;

	return ret;
    }

} // end of namespace

