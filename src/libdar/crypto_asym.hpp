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

    /// \file crypto_asym.hpp
    /// \brief the asymetric cryptographical algoritms relying on gpgme
    /// \ingroup Private

#ifndef CRYPTO_ASYM_HPP
#define CRYPTO_ASYM_HPP

extern "C"
{
#if HAVE_GPGME_H
#include <gpgme.h>
#endif
}

#include "../my_config.h"
#include "generic_file.hpp"
#include "generic_file_overlay_for_gpgme.hpp"
#include "erreurs.hpp"
#include "mem_ui.hpp"
#include "crypto.hpp"

#include <list>

namespace libdar
{
	/// \ingroup Private
	/// @}

    class crypto_asym : public mem_ui
    {
    public:

	    /// general use constructor
	crypto_asym(const user_interaction & ui) : mem_ui(ui) { build_context(); has_signatories = false; };

	    /// the destructor
	~crypto_asym() { release_context(); };

	    /// disabling copy constructor
	crypto_asym(const crypto_asym & ref): mem_ui(ref) { throw SRC_BUG; };

	    /// disabling object assignment
	const crypto_asym & operator = (const crypto_asym & ref) { throw SRC_BUG; };

	    /// defines the list of email which associated key will be used for signing
	void set_signatories(const std::vector<std::string> & signatories);

	    /// encrypt (and sign if signatures have been given using set_signatories) data for the given recipients
	    ///
	    /// \param[in] recipients_email list of email of recipient that will be able to read the encrypted data
	    /// \param[in] clear where to read from clear data to be encrypted (the object must be readable)
	    /// \param[out] ciphered where to write down encrypted data (the object must be writable)
	    /// \note this assumes the GnuPG keyring has the public keys of the recipient listed
	void encrypt(const std::vector<std::string> & recipients_email, generic_file & clear, generic_file & ciphered);

	    /// un-cipher data
	    ///
	    /// \param[in] ciphered contains the encrypted data to decipher
	    /// \param[out] clear resulting un-ciphered (thus clear) data (the object must be readable)
	    /// \note this assumes the GnuPG keyring has an appropriated private key (the objet must be writable)
	void decrypt(generic_file & ciphered, generic_file & clear);

	    /// after un-ciphering data retrieve the list of signature that were used beside encryption
	    /// return a sorted list of signatories
	const std::list<signator> & verify() const { return signing_result; };

	    /// exposing to public visibility the protected method of mem_ui
	user_interaction & get_ui() const { return mem_ui::get_ui(); };

    private:
	bool has_signatories;
	std::list<signator> signing_result;
#if GPGME_SUPPORT
	gpgme_ctx_t context;                     //< GPGME context

	void release_context() { gpgme_release(context); };
	void build_key_list(const std::vector<std::string> & recipients_email,  //< list of email to find a key for
			    gpgme_key_t * & ciphering_keys,                     //< resulting nullptr terminated list of keys
			    bool signatories);                                  //< false if email key need encryption capability, true for signing
	void release_key_list(gpgme_key_t * & ciphering_keys);
	void fill_signing_result();
#else
	void release_context() {};
#endif

	void build_context();
    };

	/// @}

} // end of namespace

#endif
