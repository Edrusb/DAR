//*********************************************************************/
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
}

#include "tools.hpp"
#include "fichier_libcurl.hpp"
#include "cache_global.hpp"
#include "nls_swap.hpp"
#include "entrepot_libcurl.hpp"
#if defined ( LIBCURL_AVAILABLE ) && defined ( LIBTHREADAR_AVAILABLE )
#include "i_entrepot_libcurl.hpp"
#endif

using namespace std;

namespace libdar
{

    entrepot_libcurl::entrepot_libcurl(const shared_ptr<user_interaction> & dialog,         ///< for user interaction
				       remote_entrepot_type proto,        ///< network protocol to use
				       const string & login,              ///< user login on remote host
				       const secu_string & password,      ///< user password on remote host (empty for file auth or user interaction)
				       const string & host,               ///< the remote server to connect to
				       const string & port,               ///< TCP/UDP port to connec to (empty string for default)
				       bool auth_from_file,               ///< whether to check $HOME/.netrc for password
				       const string & sftp_pub_keyfile,   ///< where to fetch the public key (sftp only)
				       const string & sftp_prv_keyfile,   ///< where to fetch the private key (sftp only)
				       const string & sftp_known_hosts,   ///< location of the known_hosts file (empty string to disable this security check)
				       U_I waiting_time,
				       bool verbose                       ///< whether to have verbose messages from libcurl
	)
    {
#if defined ( LIBCURL_AVAILABLE ) && defined ( LIBTHREADAR_AVAILABLE )

	NLS_SWAP_IN;
        try
        {
	    pimpl.reset(new (nothrow) i_entrepot_libcurl(dialog,
							 proto,
							 login,
							 password,
							 host,
							 port,
							 auth_from_file,
							 sftp_pub_keyfile,
							 sftp_prv_keyfile,
							 sftp_known_hosts,
							 waiting_time,
							 verbose));

	    if(!pimpl)
		throw Ememory("entrepot_libcurl::entrepot_libcurl");
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
#else
	throw Ecompilation("libcurl library");
#endif
    }

    void entrepot_libcurl::set_location(const path & chemin)
    {
#if defined ( LIBCURL_AVAILABLE ) && defined ( LIBTHREADAR_AVAILABLE )
	pimpl->set_location(chemin);
#endif
    }

    void entrepot_libcurl::set_root(const path & p_root)
    {
#if defined ( LIBCURL_AVAILABLE ) && defined ( LIBTHREADAR_AVAILABLE )
	pimpl->set_root(p_root);
#endif
    }

    path entrepot_libcurl::get_full_path() const
    {
#if defined ( LIBCURL_AVAILABLE ) && defined ( LIBTHREADAR_AVAILABLE )
	return pimpl->get_full_path();
#else
	return path("/");
#endif
    }


    string entrepot_libcurl::get_url() const
    {
#if defined ( LIBCURL_AVAILABLE ) && defined ( LIBTHREADAR_AVAILABLE )
	string ret;

	NLS_SWAP_IN;
        try
        {
	    ret = pimpl->get_url();
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;

	return ret;
#else
	throw Efeature("libcurl library");
#endif
    }

    const path & entrepot_libcurl::get_location() const
    {
#if defined ( LIBCURL_AVAILABLE ) && defined ( LIBTHREADAR_AVAILABLE )
	return pimpl->get_location();
#else
	throw Ecompilation("Elibcurl or libthreadar");
#endif
    }

    const path & entrepot_libcurl::get_root() const
    {
#if defined ( LIBCURL_AVAILABLE ) && defined ( LIBTHREADAR_AVAILABLE )
	return pimpl->get_root();
#else
	throw Ecompilation("Elibcurl or libthreadar");
#endif
    }


    void entrepot_libcurl::change_user_interaction(const shared_ptr<user_interaction> & new_dialog)
    {
#if defined ( LIBCURL_AVAILABLE ) && defined ( LIBTHREADAR_AVAILABLE )
	NLS_SWAP_IN;
        try
        {
	    pimpl->change_user_interaction(new_dialog);
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
#else
	throw Efeature("libcurl library");
#endif
    }

    shared_ptr<user_interaction> entrepot_libcurl::get_current_user_interaction() const
    {
	#if defined ( LIBCURL_AVAILABLE ) && defined ( LIBTHREADAR_AVAILABLE )
	shared_ptr<user_interaction> ret;

	NLS_SWAP_IN;
	try
        {
	    ret = pimpl->get_current_user_interaction();
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;

	return ret;
#else
	throw Efeature("libcurl library");
#endif
    }

    void entrepot_libcurl::read_dir_reset() const
    {
#if defined ( LIBCURL_AVAILABLE ) && defined ( LIBTHREADAR_AVAILABLE )
	NLS_SWAP_IN;
        try
        {
	    pimpl->read_dir_reset();
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
#else
	throw Efeature("libcurl library");
#endif
    }

    bool entrepot_libcurl::read_dir_next(std::string & filename) const
    {
#if defined ( LIBCURL_AVAILABLE ) && defined ( LIBTHREADAR_AVAILABLE )
	bool ret;

	NLS_SWAP_IN;
        try
        {
	    ret = pimpl->read_dir_next(filename);
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;

	return ret;
#else
	throw Efeature("libcurl library");
#endif
    }

    void entrepot_libcurl::read_dir_reset_dirinfo() const
    {
#if defined ( LIBCURL_AVAILABLE ) && defined ( LIBTHREADAR_AVAILABLE )
	NLS_SWAP_IN;
        try
        {
	    pimpl->read_dir_reset_dirinfo();
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
#else
	throw Efeature("libcurl library");
#endif
    }

    bool entrepot_libcurl::read_dir_next_dirinfo(std::string & filename, inode_type & tp) const
    {
#if defined ( LIBCURL_AVAILABLE ) && defined ( LIBTHREADAR_AVAILABLE )
	bool ret;

	NLS_SWAP_IN;
        try
        {
	    ret = pimpl->read_dir_next_dirinfo(filename, tp);
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;

	return ret;
#else
	throw Efeature("libcurl library");
#endif
    }


    void entrepot_libcurl::create_dir(const std::string & dirname, U_I permission)
    {
#if defined ( LIBCURL_AVAILABLE ) && defined ( LIBTHREADAR_AVAILABLE )

	NLS_SWAP_IN;
        try
        {
	    pimpl->create_dir(dirname, permission);
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
#else
	throw Efeature("libcurl library");
#endif
    }

    fichier_global *entrepot_libcurl::inherited_open(const shared_ptr<user_interaction> & dialog,
						     const string & filename,
						     gf_mode mode,
						     bool force_permission,
						     U_I permission,
						     bool fail_if_exists,
						     bool erase) const
    {
#if defined ( LIBCURL_AVAILABLE ) && defined ( LIBTHREADAR_AVAILABLE )
	fichier_global* ret;

	NLS_SWAP_IN;
        try
        {
	    ret = pimpl->inherited_open(dialog,
					filename,
					mode,
					force_permission,
					permission,
					fail_if_exists,
					erase);
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;

	return ret;
#else
	throw Efeature("libcurl library");
#endif
    }

    void entrepot_libcurl::inherited_unlink(const string & filename) const
    {
#if defined ( LIBCURL_AVAILABLE ) && defined ( LIBTHREADAR_AVAILABLE )
	NLS_SWAP_IN;
        try
        {
	    pimpl->inherited_unlink(filename);
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
#else
	throw Efeature("libcurl library");
#endif
    }

    void entrepot_libcurl::read_dir_flush() const
    {
#if defined ( LIBCURL_AVAILABLE ) && defined ( LIBTHREADAR_AVAILABLE )
	NLS_SWAP_IN;
        try
        {
	    pimpl->read_dir_flush();
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
#else
	throw Efeature("libcurl library");
#endif
    }


} // end of namespace
