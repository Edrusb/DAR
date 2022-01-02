//*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2022 Denis Corbin
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
#if HAVE_ERRNO_H
#include <errno.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_STRINGS_H
#include <strings.h>
#endif
} // end of extern "C"

#include "tools.hpp"
#include "crypto_asym.hpp"
#include "integers.hpp"
#include "generic_file_overlay_for_gpgme.hpp"
#include "thread_cancellation.hpp"

#include <string>

using namespace std;

namespace libdar
{

#if GPGME_SUPPORT
    static gpgme_error_t read_passphrase(void *hook, const char *uid_hint, const char *passphrase_info, int prev_was_bad, int fd);
#endif


    void crypto_asym::set_signatories(const std::vector<std::string> & signatories)
    {
#if GPGME_SUPPORT
	gpgme_key_t *signatories_key = nullptr;

	if(signatories.empty())
	{
	    gpgme_signers_clear(context);
	    has_signatories = false;
	}
	else
	{
	    build_key_list(signatories, signatories_key, true);
	    try
	    {
		gpgme_signers_clear(context);
		gpgme_key_t *ptr = signatories_key;
		gpgme_error_t err;

		if(ptr == nullptr)
		    throw SRC_BUG;  // build_key_list failed
		while(*ptr != nullptr)
		{
		    err = gpgme_signers_add(context, *ptr);
		    switch(gpgme_err_code(err))
		    {
		    case GPG_ERR_NO_ERROR:
			break;
		    default:
			throw Erange("crypto_asym::encrypt", string(gettext("Unexpected error reported by GPGME: ")) + tools_gpgme_strerror_r(err));
		    }
		    ++ptr;
		}
	    }
	    catch(...)
	    {
		release_key_list(signatories_key);
		gpgme_signers_clear(context);
		has_signatories = false;
		throw;
	    }
	    release_key_list(signatories_key);
	    has_signatories = true;
	}
#else
	throw Ecompilation("Asymetric Strong encryption algorithms using GPGME");
#endif
    }

    void crypto_asym::encrypt(const vector<string> & recipients_email, generic_file & clear, generic_file & ciphered)
    {
#if GPGME_SUPPORT
	gpgme_key_t *ciphering_keys = nullptr;

	build_key_list(recipients_email, ciphering_keys, false);
	try
	{
	    generic_file_overlay_for_gpgme o_clear(&clear);
	    generic_file_overlay_for_gpgme o_ciphered(&ciphered);
	    gpgme_error_t err;

	    if(!has_signatories)
		err = gpgme_op_encrypt(context,
				       ciphering_keys,
				       (gpgme_encrypt_flags_t)(GPGME_ENCRYPT_NO_ENCRYPT_TO|GPGME_ENCRYPT_ALWAYS_TRUST),
				       o_clear.get_gpgme_handle(),
				       o_ciphered.get_gpgme_handle());
	    else
		err = gpgme_op_encrypt_sign(context,
					    ciphering_keys,
					    (gpgme_encrypt_flags_t)(GPGME_ENCRYPT_NO_ENCRYPT_TO|GPGME_ENCRYPT_ALWAYS_TRUST),
					    o_clear.get_gpgme_handle(),
					    o_ciphered.get_gpgme_handle());
	    switch(gpgme_err_code(err))
	    {
	    case GPG_ERR_NO_ERROR:
		break;
	    case GPG_ERR_INV_VALUE:
		throw SRC_BUG;
	    case GPG_ERR_UNUSABLE_PUBKEY:
		throw Erange("crypto_asym::encrypt", gettext("Key found but users are not all trusted"));
	    default:
		throw Erange("crypto_asym::encrypt", string(gettext("Unexpected error reported by GPGME: ")) + tools_gpgme_strerror_r(err));
	    }
	}
	catch(...)
	{
	    release_key_list(ciphering_keys);
	    throw;
	}
	release_key_list(ciphering_keys);
#else
	throw Ecompilation("Asymetric Strong encryption algorithms using GPGME");
#endif
    }

    void crypto_asym::decrypt(generic_file & ciphered, generic_file & clear)
    {
#if GPGME_SUPPORT
	generic_file_overlay_for_gpgme o_clear(&clear);
	generic_file_overlay_for_gpgme o_ciphered(&ciphered);
	gpgme_error_t err = gpgme_op_decrypt_verify(context, o_ciphered.get_gpgme_handle(), o_clear.get_gpgme_handle());

	signing_result.clear();
	switch(gpgme_err_code(err))
	{
	case GPG_ERR_NO_ERROR:
	    fill_signing_result();
	    break;
	case GPG_ERR_INV_VALUE:
	    throw SRC_BUG;
	case GPG_ERR_NO_DATA:
	    throw Erange("crypto_asym::decrypt", gettext("No data to decrypt"));
	case GPG_ERR_DECRYPT_FAILED:
	    throw Erange("crypto_asym::decrypt", gettext("Invalid Cipher text"));
	case GPG_ERR_BAD_PASSPHRASE:
	    throw Erange("crypto_asym::decrypt", gettext("Failed retreiving passphrase"));
	default:
	    throw Erange("crypto_asym::decrypt", string(gettext("Unexpected error reported by GPGME: ")) + tools_gpgme_strerror_r(err));
	}
#else
	throw Ecompilation("Asymetric Strong encryption algorithms using GPGME");
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
    void crypto_asym::build_key_list(const vector<string> & recipients_email, gpgme_key_t * & ciphering_keys, bool signatories)
    {
	U_I size = recipients_email.size() + 1;

	ciphering_keys = new (nothrow) gpgme_key_t[size];
	if(ciphering_keys == nullptr)
	    throw Ememory("crypto_asym::build_key_list");

	    // clearing all fields in order to be able to know which
	    // index has been allocated and need to be restored in case of error
	for(U_I i = 0; i < size ; ++i)
	    ciphering_keys[i] = nullptr;

	try
	{
	    gpgme_error_t err = GPG_ERR_NO_ERROR;
	    gpgme_user_id_t id = nullptr;
	    gpgme_subkey_t subk = nullptr;
	    bool found = false;
	    bool eof = false;
	    bool search_email = false;
	    U_I offset = 0;

		// for each recipient, listing all keys to find a match

	    for(U_I i = 0; i < recipients_email.size(); ++i)
	    {
		err = gpgme_op_keylist_start(context, nullptr, 0);
		switch(gpgme_err_code(err))
		{
		case GPG_ERR_NO_ERROR:
		    break;
		case GPG_ERR_INV_VALUE:
		    throw SRC_BUG;
		default:
		    throw Erange("crypto_asym::decrypt", string(gettext("Unexpected error reported by GPGME: ")) + tools_gpgme_strerror_r(err));
		}

		    // the the recipient string contains an '@' we assume we have
		    // to look for an email
		    // else we assume we have to look for an keyid
		search_email = recipients_email[i].find_first_of('@') != string::npos;

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
			if(search_email)
			{
				// looking in email field

			    id = ciphering_keys[i - offset]->uids;
			    while(!found && id != nullptr)
			    {

				    // for each key, listing all emails associated with it to find a match

				found =
				    (strncmp(recipients_email[i].c_str(), id->email, recipients_email[i].size()) == 0)
				    && (id->email[recipients_email[i].size()] == '\0');
				id = id->next;
			    }
			}
			else
			{
				// looking in keyid field of all subkeys

			    subk = ciphering_keys[i - offset]->subkeys;
			    while(!found && subk != nullptr)
			    {
				    // for each key, listing all keyid associated with it to find a match
				found =
				    (strncmp(recipients_email[i].c_str(), subk->keyid, recipients_email[i].size()) == 0)
				    && (subk->keyid[recipients_email[i].size()] == '\0');
				subk = subk->next;
			    }
			}

			    // analysing search result

			if(found)
			{
			    if(ciphering_keys[i - offset]->revoked
			       || ciphering_keys[i - offset]->expired
			       || ciphering_keys[i - offset]->disabled
			       || ciphering_keys[i - offset]->invalid)
				found = false;
			    if(signatories)
			    {
				if(!ciphering_keys[i - offset]->can_sign)
				    found = false;
			    }
			    else
			    {
				if(!ciphering_keys[i - offset]->can_encrypt)
				    found = false;
			    }
			}

			if(!found)
			{
			    gpgme_key_unref(ciphering_keys[i - offset]);
			    ciphering_keys[i - offset] = nullptr;
			}
			break;
		    default:
			throw Erange("crypto_asym::decrypt", string(gettext("Unexpected error reported by GPGME: ")) + tools_gpgme_strerror_r(err));
		    }
		}
		while(!found && !eof);

		    // if we exit before gpgme_op_keylist_next() return GPG_ERR_EOF
		    // we must update the state of the context to end the key listing operation

		if(!eof)
		    (void)gpgme_op_keylist_end(context);

		if(!found)
		{
		    if(signatories)
			get_ui().printf(gettext("No valid signing key could be find for %S"), &(recipients_email[i]));
		    else
			get_ui().printf(gettext("No valid encryption key could be find for %S"), &(recipients_email[i]));
		    get_ui().pause("Do you want to continue without this recipient?");
		    ++offset;
		}
	    }

		// if no recipient could be found at all aborting the operation

	    if(offset + 1 >= size)
	    {
		if(signatories)
		    throw Erange("crypto_asym::build_key_list", gettext("No signatory remain with a valid key, signing is impossible, aborting"));
		else
		    throw Erange("crypto_asym::build_key_list", gettext("No recipient remain with a valid key, encryption is impossible, aborting"));
	    }

		// the key list must end wth a nullptr entry

	    if(ciphering_keys[size - 1 - offset] != nullptr)
		throw SRC_BUG;
	}
	catch(...)
	{
	    release_key_list(ciphering_keys);
	    throw;
	}
    }

    void crypto_asym::release_key_list(gpgme_key_t * & ciphering_keys)
    {
	if(ciphering_keys != nullptr)
	{
	    for(U_I i = 0; ciphering_keys[i] != nullptr; ++i)
		gpgme_key_unref(ciphering_keys[i]);

	    delete [] ciphering_keys;
	    ciphering_keys = nullptr;
	}
    }

    void crypto_asym::fill_signing_result()
    {
	gpgme_verify_result_t inter = gpgme_op_verify_result(context);
	gpgme_signature_t res = nullptr;
	signator tmp;

	signing_result.clear();

	if(inter != nullptr)
	    res = inter->signatures;
	else
	    res = nullptr;

	while(res != nullptr)
	{
	    if(res->summary & (GPGME_SIGSUM_VALID|GPGME_SIGSUM_GREEN))
		tmp.result = signator::good;
	    else if(res->summary & GPGME_SIGSUM_RED)
		tmp.result = signator::bad;
	    else if(res->summary & GPGME_SIGSUM_KEY_MISSING)
		tmp.result = signator::unknown_key;
	    else
		tmp.result = signator::error;

	    if(res->summary & GPGME_SIGSUM_KEY_REVOKED)
		tmp.key_validity = signator::revoked;
	    else if(res->summary & GPGME_SIGSUM_KEY_EXPIRED)
		tmp.key_validity = signator::expired;
	    else
		tmp.key_validity = signator::valid;

	    tmp.fingerprint = res->fpr;

	    tmp.signing_date = datetime(res->timestamp);
	    tmp.signature_expiration_date = datetime(res->exp_timestamp);

	    res = res->next;
	    signing_result.push_back(tmp);
	}

	signing_result.sort();
    }

    static gpgme_error_t read_passphrase(void *hook, const char *uid_hint, const char *passphrase_info, int prev_was_bad, int fd)
    {
	crypto_asym *obj = (crypto_asym *)(hook);
	const char * precision = gettext("Passphrase required for key %s :");
	string message;
	secu_string pass;
	ssize_t wrote;
	gpgme_error_t ret = GPG_ERR_NO_ERROR;
	thread_cancellation th;

	if(obj == nullptr)
	    throw SRC_BUG;

	if(uid_hint != nullptr)
	    message = tools_printf(precision, uid_hint);
	else
	{
	    if(passphrase_info != nullptr)
		message = tools_printf(precision, passphrase_info);
	    else
		message = tools_printf(precision, "");
	}

	if(prev_was_bad)
	    obj->get_ui().message(gettext("Error, invalid passphrase given, try again:"));
	pass = obj->get_ui().get_secu_string(message, false);
	th.check_self_cancellation();

	wrote = write(fd, pass.c_str(), pass.get_size());
	if(wrote < 0 || (U_I)(wrote) != pass.get_size())
	{
	    if(wrote == -1)
		obj->get_ui().message(string(gettext("Error, while sending the passphrase to GPGME:")) + tools_strerror_r(errno));
	    else
		obj->get_ui().message(gettext("Failed sending the totality of the passphrase to GPGME"));
	    ret = GPG_ERR_CANCELED;
	}
	else
	{
	    wrote = write(fd, "\n", 1);
	    if(wrote != 1)
		obj->get_ui().message(gettext("Failed sending CR after the passphrase"));
	    ret = GPG_ERR_NO_ERROR;
	}

	return ret;
    }
#endif

} // end of namespace
