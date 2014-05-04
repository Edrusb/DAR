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

extern "C"
{
#if HAVE_ERRNO_H
#incldue <errno.h>
#endif
} // end of extern "C"

#include "../my_config.h"
#include "tools.hpp"
#include "crypto_asym.hpp"
#include "integers.hpp"

#include <string>

using namespace std;

namespace libdar
{

#if GPGME_SUPPORT
    static gpgme_error_t read_passphrase(void *hook, const char *uid_hint, const char *passphrase_info, int prev_was_bad, int fd);
#endif

#if MUTEX_WORKS
    pthread_mutex_t crypto_asym::lock_wait = PTHREAD_MUTEX_INITIALIZER;
#endif


    crypto_asym::crypto_asym(user_interaction & ui, generic_file *below, const vector<string> & recipients_email):
	generic_file(gf_write_only),
	mem_ui(ui),
	ciphered(below)
    {
#if GPGME_SUPPORT
	gpgme_key_t *ciphering_keys = NULL;

	build_context();
	try
	{
	    build_key_list(recipients_email, ciphering_keys);

	    try
	    {
		build_pipes_and_the_clear_overlay();
		try
		{
		    gpgme_error_t err = gpgme_op_encrypt_start(context,
							       ciphering_keys,
							       (gpgme_encrypt_flags_t)0,
							       clear->get_gpgme_handle(),
							       ciphered.get_gpgme_handle());
		    switch(gpgme_err_code(err))
		    {
		    case GPG_ERR_NO_ERROR:
			break;
		    case GPG_ERR_INV_VALUE:
			throw SRC_BUG;
		    case GPG_ERR_UNUSABLE_PUBKEY:
			throw Erange("crypto_asym::crypto_asym", gettext("Key found but users are not all trusted"));
		    default:
			throw SRC_BUG;
		    }
		}
		catch(...)
		{
		    release_pipes_and_the_clear_overlay();
		    throw;
		}
	    }
	    catch(...)
	    {
		release_key_list(ciphering_keys);
		throw;
	    }
		// ciphering_keys is local, we must now release it
		// even if no exception occurred
	    release_key_list(ciphering_keys);
	}
	catch(...)
	{
	    release_context();
	    throw;
	}
#else
	throw Efeature("Asymetric Strong encryption algorithms using GPGME");
#endif
    }

    crypto_asym::crypto_asym(user_interaction &ui, generic_file *below):
	generic_file(gf_read_only),
	mem_ui(ui),
	ciphered(below)
    {
#if GPGME_SUPPORT
	build_context();
	try
	{
	    build_pipes_and_the_clear_overlay();
	    try
	    {
		gpgme_error_t err = gpgme_op_decrypt_start(context, ciphered.get_gpgme_handle(), clear->get_gpgme_handle());
		switch(gpgme_err_code(err))
		{
		case GPG_ERR_NO_ERROR:
		    break;
		case GPG_ERR_INV_VALUE:
		    throw SRC_BUG;
		default:
		    throw SRC_BUG;
		}
	    }
	    catch(...)
	    {
		release_pipes_and_the_clear_overlay();
		throw;
	    }
	}
	catch(...)
	{
	    release_context();
	    throw;
	}
#else
	throw Efeature("Asymetric Strong encryption algorithms using GPGME");
#endif
    }

    void crypto_asym::inherited_terminate()
    {
#if GPGME_SUPPORT
	gpgme_error_t err;

	    // first closing the clear_side object
	if(clear_side == writer_side)
	    writer_side->terminate();

	    // extract from gpgme documentation:
	    // In a multi-threaded environment, only one thread should ever call gpgme_wait at any time, irregardless if ctx is specified or not

#if MUTEX_WORKS
	    // acquiring lock_wait before calling gpgme_wait
	sigset_t mask_memory;
	tools_block_all_signals(mask_memory);
	pthread_mutex_lock(&lock_wait);

	try
	{
#endif
	    (void)gpgme_wait(context, &err, true);
#if MUTEX_WORKS
		// setting backup blocked signals to their initial values
	}
	catch(...)
	{
		// no re-throw;
	}
	pthread_mutex_unlock(&lock_wait);
	tools_set_back_blocked_signals(mask_memory);
#endif
	release_pipes_and_the_clear_overlay();
	release_context();
#endif
    }

    void crypto_asym::build_context()
    {
#if GPGME_SUPPORT
	gpgme_error_t err = gpgme_new(&context);

	if(gpgme_err_code(err) != GPG_ERR_NO_ERROR)
	    throw Erange("crypto_asym::crypto_asym", string(gettext("Failed creating GPGME context: ")) + tools_gpgme_strerror_r(err));

	err = gpgme_set_protocol(context, GPGME_PROTOCOL_OpenPGP);

	if(gpgme_err_code(err) != GPG_ERR_NO_ERROR)
	    throw Erange("crypto_asym::crypto_asym", string(gettext("Failed setting GPGME context with OpenPGP protocol: ")) + tools_gpgme_strerror_r(err));

	gpgme_set_passphrase_cb(context, read_passphrase, (void *)this);
#endif
    }

#if GPGME_SUPPORT
    void crypto_asym::build_key_list(const vector<string> & recipients_email, gpgme_key_t * & ciphering_keys)
    {
	U_I size = recipients_email.size() + 1;

	ciphering_keys = new (nothrow) gpgme_key_t[size];
	if(ciphering_keys == NULL)
	    throw Ememory("crypto_asym::build_key_list");

	    // clearing all fields in order to be able to know which
	    // index has been allocated and need to be restored in case of error
	for(U_I i = 0; i < size ; ++i)
	    ciphering_keys[i] = NULL;

	try
	{
	    gpgme_error_t err = GPG_ERR_NO_ERROR;
	    gpgme_user_id_t id = NULL;
	    bool found = false;
	    bool eof = false;
	    bool loop = false;
	    U_I offset = 0;

		// for each recipient, listing all keys to find a match

	    for(U_I i = 0; i < recipients_email.size(); ++i)
	    {
		err = gpgme_op_keylist_start(context, NULL, 0);
		switch(gpgme_err_code(err))
		{
		case GPG_ERR_NO_ERROR:
		    break;
		case GPG_ERR_INV_VALUE:
		    throw SRC_BUG;
		default:
		    throw SRC_BUG;
		}

		found = false;
		eof = false;
		do
		{
			// getting the next key

		    err = gpgme_op_keylist_next(context, &(ciphering_keys[i - offset]));
		    switch(gpgme_err_code(err))
		    {
		    case GPG_ERR_EOF:
			eof = true;
			break;
		    case GPG_ERR_NO_ERROR:
			id = ciphering_keys[i - offset]->uids;
			loop = true;

			    // for each key, listing all identies associated with it to find a match
			do
			{
			    found = (strncmp(recipients_email[i].c_str(), id->email, recipients_email[i].size()) == 0);

			    if(found)
			    {
				if(ciphering_keys[i - offset]->revoked
				   || ciphering_keys[i - offset]->expired
				   || ciphering_keys[i - offset]->disabled
				   || ciphering_keys[i - offset]->invalid)
				    found = false;
			    }

			    if(!found && id->next != NULL)
				id = id->next;
			    else
				loop = false;
			}
			while(loop);

			    // if not identity match found for that key deleting the key

			if(!found)
			{
			    gpgme_key_unref(ciphering_keys[i - offset]);
			    ciphering_keys[i - offset] = NULL;
			}
			break;
		    default:
			throw SRC_BUG; // unknown condition met
		    }
		}
		while(!found && !eof);

		    // if we exit before gpgme_op_keylist_next() return GPG_ERR_EOF
		    // we must update the state of the context to end the key listing operation

		if(!eof)
		    (void)gpgme_op_keylist_end(context);

		if(!found)
		{
		    get_ui().printf(gettext("No valid key could be find for recipient %S"), &(recipients_email[i]));
		    get_ui().pause("Do you want to abort the operation to fix this key issue first?");
		    ++offset;
		}
	    }

		// if no recipient could be found at all aborting the operation

	    if(offset >= size)
		throw Erange("crypto_asym::build_key_list", gettext("No recipient remain with a valid key, encryption is impossible, aborting"));

		// the key list must end wth a NULL entry

	    if(ciphering_keys[size - 1 - offset] != NULL)
		throw SRC_BUG;
	}
	catch(...)
	{
	    release_key_list(ciphering_keys);
	    throw;
	}
    }
#endif

    void crypto_asym::release_key_list(gpgme_key_t * & ciphering_keys)
    {
	if(ciphering_keys != NULL)
	{
	    for(U_I i = 0; ciphering_keys[i] != NULL; ++i)
		gpgme_key_unref(ciphering_keys[i]);

	    delete [] ciphering_keys;
	    ciphering_keys = NULL;
	}
    }

    void crypto_asym::build_pipes_and_the_clear_overlay()
    {
#if GPGME_SUPPORT
	reader_side = NULL;
	writer_side = NULL;
	clear_side = NULL;
	clear = NULL;

	try
	{
	    writer_side = new (get_pool()) tuyau(get_ui());
	    if(writer_side == NULL)
		throw Ememory("crypto_asym::build_pipes_and_clear_overlay");
	    reader_side = new (get_pool()) tuyau(get_ui(), writer_side->get_read_fd());
	    if(reader_side == NULL)
		throw Ememory("crypto_asym::build_pipes_and_clear_overlay");
	    writer_side->do_not_close_read_fd(); // read fd will be closed by reader_side's destructor
	    switch(get_mode())
	    {
	    case gf_read_only:
		    // unciphering data
		clear_side = reader_side;
		clear = new (get_pool()) generic_file_overlay_for_gpgme(writer_side);
		break;
	    case gf_write_only:
		clear_side = writer_side;
		clear = new (get_pool()) generic_file_overlay_for_gpgme(reader_side);
		break;
	    case gf_read_write:
		throw SRC_BUG;
	    default:
		throw SRC_BUG;
	    }
	    if(clear == NULL)
		throw Ememory("crypto_asym::build_pipes_and_clear_overlay");
	}
	catch(...)
	{
	    release_pipes_and_the_clear_overlay();
	    throw;
	}
#endif
    }

    void crypto_asym::release_pipes_and_the_clear_overlay()
    {
#if GPGME_SUPPORT
	if(clear != NULL)
	{
	    delete clear;
	    clear = NULL;
	}
	if(reader_side != NULL)
	{
	    delete reader_side;
	    reader_side = NULL;
	}
	if(writer_side != NULL)
	{
	    delete writer_side;
	    writer_side = NULL;
	}
	if(clear_side != NULL)
	{
		// no not delete, it's juste a pointer
	    clear_side = NULL;
	}
#endif
    }

#if GPGME_SUPPORT
    static gpgme_error_t read_passphrase(void *hook, const char *uid_hint, const char *passphrase_info, int prev_was_bad, int fd)
    {
	crypto_asym *obj = (crypto_asym *)(hook);
	string precision = gettext("Passphrase required");
	secu_string pass;
	ssize_t wrote;
	gpgme_error_t ret = GPG_ERR_NO_ERROR;

	if(obj == NULL)
	    throw SRC_BUG;

	if(uid_hint != NULL)
	    precision += tools_printf(gettext(" [User ID = %s]"), uid_hint);
	if(passphrase_info != NULL)
	    precision += tools_printf(gettext(" [Info = %s]"), passphrase_info);
	precision += ":";

	if(prev_was_bad)
	    obj->get_ui().warning(gettext("Error, invalid passphrase given, try again:"));
	pass = obj->get_ui().get_secu_string(precision, false);

	wrote = write(fd, pass.c_str(), pass.size());
	if(wrote < 0 || (U_I)(wrote) != pass.size())
	{
	    if(wrote == -1)
		obj->get_ui().warning(string(gettext("Error, while sending the passphrase to GPGME:")) + tools_strerror_r(errno));
	    else
		obj->get_ui().warning(gettext("Failed sending the totality of the passphrase to GPGME"));
	    ret = GPG_ERR_CANCELED;
	}
	else
	{
	    wrote = write(fd, "\n", 1);
	    if(wrote != 1)
		obj->get_ui().warning(gettext("Failed sending CR after the passphrase"));
	    ret = GPG_ERR_CANCELED;
	}

	return ret;
    }
#endif

} // end of namespace
