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

#if MUTEX_WORKS
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif
#endif
}

#include "../my_config.h"
#include "generic_file.hpp"
#include "generic_file_overlay_for_gpgme.hpp"
#include "erreurs.hpp"
#include "user_interaction.hpp"
#include "mem_ui.hpp"
#include "tuyau.hpp"

namespace libdar
{
	/// \ingroup Private
	/// @}

    class crypto_asym : public generic_file, public mem_ui
    {
    public:
	    /// create an object for ciphering (write-only)
	    ///
	    /// \param[in] ui for user interaction
	    /// \param[in] below where to write down encrypted data
	    /// \param[in] recipients_email list of email of recipient that will be able to read the encrypted data
	    /// \note this assumes the GnuPG keyring has the public keys of the recipient listed
	crypto_asym(user_interaction & ui, generic_file *below, const std::vector<std::string> & recipients_email);

	    /// create an object for unciphering (read-only)
	    ///
	    /// \param[in] ui for user interaction like passphrase input
	    /// \param[in] below generic_file containing the encrypted data to decipher
	    /// \note this assumes the GnuPG keyring has an appropriated private key
	crypto_asym(user_interaction &ui, generic_file *below);

	    /// the destructor
	~crypto_asym() { terminate(); };

	    /// disabling copy constructor
	crypto_asym(const crypto_asym & ref): generic_file(gf_read_only), mem_ui(ref), ciphered(ref.ciphered) { throw SRC_BUG; };

	    /// disabling object assignment
	const crypto_asym & operator = (const crypto_asym & ref) { throw SRC_BUG; };


	    // inherited from generic_file

	virtual bool skip(const infinint & pos) { check_clear(); return clear_side->skip(pos); };
        virtual bool skip_to_eof() { check_clear(); return clear_side->skip_to_eof(); };
        virtual bool skip_relative(S_I x) { check_clear(); return clear_side->skip_relative(x); };
        virtual infinint get_position() { check_clear(); return clear_side->get_position(); };

    protected:

	    // inherited from generic_file

        virtual U_I inherited_read(char *a, U_I size) { check_clear(); return clear_side->read(a, size); };
	virtual void inherited_write(const char *a, U_I size) { check_clear(); return clear_side->write(a, size); };
	virtual void inherited_sync_write() { check_clear(); clear_side->sync_write(); };
	virtual void inherited_terminate();

    private:
	generic_file_overlay_for_gpgme ciphered; //< where to write ciphered data to, or read it from
	generic_file_overlay_for_gpgme *clear;   //< where to write clear data to; or read it from
#if GPGME_SUPPORT
	gpgme_ctx_t context;                     //< GPGME context
#endif
#if MUTEX_WORKS
	static pthread_mutex_t lock_wait;        //< in multi-threaded environement, gpgme_wait() is not thread safe
#endif
	tuyau *reader_side;                      //< object wrapping the read side of the anonymous pipe
	tuyau *writer_side;                      //< object wrapping the write side of the anonymous pipe
	tuyau *clear_side;                       //< points either to reader_size of writer_size depending on the ciphering/unciphering mode

	void check_clear() { if(clear_side == NULL) throw SRC_BUG; };
	void build_context();
#if GPGME_SUPPORT
	void release_context() { gpgme_release(context); };
	void build_key_list(const std::vector<std::string> & recipients_email, gpgme_key_t * & ciphering_keys);
	void release_key_list(gpgme_key_t * & ciphering_keys);
#endif
	void build_pipes_and_the_clear_overlay();
	void release_pipes_and_the_clear_overlay();
    };

	/// @}

} // end of namespace

#endif
