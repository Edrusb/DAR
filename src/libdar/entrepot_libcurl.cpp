//*********************************************************************/
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
// $Id: entrepot.cpp,v 1.1 2012/04/27 11:24:30 edrusb Exp $
//
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

    entrepot_libcurl::entrepot_libcurl(const shared_ptr<user_interaction> & dialog,         //< for user interaction
				       mycurl_protocol proto,             //< network protocol to use
				       const string & login,              //< user login on remote host
				       const secu_string & password,      //< user password on remote host (empty for file auth or user interaction)
				       const string & host,               //< the remote server to connect to
				       const string & port,               //< TCP/UDP port to connec to (empty string for default)
				       bool auth_from_file,               //< whether to check $HOME/.netrc for password
				       const string & sftp_pub_keyfile,   //< where to fetch the public key (sftp only)
				       const string & sftp_prv_keyfile,   //< where to fetch the private key (sftp only)
				       const string & sftp_known_hosts,   //< location of the known_hosts file (empty string to disable this security check)
				       U_I waiting_time)
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
							 waiting_time));

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
	throw Efeature("libcurl library");
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
#endif
    }

    const path & entrepot_libcurl::get_root() const
    {
#if defined ( LIBCURL_AVAILABLE ) && defined ( LIBTHREADAR_AVAILABLE )
	return pimpl->get_root();
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

    void entrepot_libcurl::read_dir_flush()
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
