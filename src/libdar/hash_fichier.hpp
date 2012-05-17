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

    /// \file hash_fichier.hpp
    /// \brief class hash_fichier definition.
    /// \ingroup Private
    ///
    /// This is an inherited class from class fichier
    /// Objects of that class are write-only objects that provide a hash of the written data
    /// other hash algorithm may be added in the future

#ifndef HASH_FICHIER_HPP
#define HASH_FICHIER_HPP

#include "../my_config.h"

extern "C"
{
#if HAVE_GCRYPT_H
#include <gcrypt.h>
#endif
}

#include <string>

#include "generic_file.hpp"
#include "fichier.hpp"
#include "integers.hpp"

namespace libdar
{

	/// the available hashing algorithms
	/// \ingroup API

    enum hash_algo
    {
	hash_none, //< no hashing algorithm
	hash_md5,  //< MD5 algorithm
	hash_sha1  //< SHA1 algorithm
    };


	/// \addtogroup Private
	/// @{

    extern std::string hash_algo_to_string(hash_algo algo);

    class hash_fichier : public fichier_global
    {
    public:

	    /// hash_file constructor
	    ///
	    /// \param[in] dialog for user interaction
	    /// \param[in] under points to an object where to write data to
	    /// \param[in] under_filename name of the plain file we write to, this argument is required to build the hash file
	    /// \param[in] hash_file points to an object where to drop the hash file once writings are finished
	    /// \param[in] algo hash algorithm to use. hash_none is not an acceptable value
	    /// \note if the constructor succeed, the objects pointed to by under and hash_file are owned and deleted by this hash_file object

        hash_fichier(user_interaction & dialog,
		     fichier_global *under,
		     const std::string & under_filename,
		     fichier_global *hash_file,
		     hash_algo algo);

	    // copy constructor
	hash_fichier(const hash_fichier & ref) : fichier_global(ref) { throw SRC_BUG; };

	    // assignment operator
	const hash_fichier & operator = (const hash_fichier & ref) { throw SRC_BUG; };

	    // destructor
	~hash_fichier();

	    // inherited from fichier_global
	void change_ownership(const std::string & user, const std::string & group) { if(ref == NULL || hash_ref == NULL) throw SRC_BUG; ref->change_ownership(user, group); hash_ref->change_ownership(user, group); };
	void change_permission(U_I perm) { if(ref == NULL || hash_ref == NULL) throw SRC_BUG; ref->change_permission(perm); hash_ref->change_permission(perm); };
	infinint get_size() const { if(ref == NULL) throw SRC_BUG; return ref->get_size(); };
	void fadvise(advise adv) const { if(ref == NULL) throw SRC_BUG; ref->fadvise(adv); };
	void fsync() const { if(ref == NULL) throw SRC_BUG; ref->fsync(); };

	    // inherited from generic_file

        bool skip(const infinint & pos) {if(ref == NULL || pos != ref->get_position()) throw SRC_BUG; else return true; };
        bool skip_to_eof() { throw SRC_BUG; };
        bool skip_relative(S_I x) { if(x != 0) throw SRC_BUG; else return true; };
	infinint get_position() { if(ref == NULL) throw SRC_BUG; return ref->get_position(); };

#ifdef LIBDAR_SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(hash_fichier);
#endif

    protected:
	    // inherited from fichier_global
	U_I fichier_global_inherited_write(const char *a, U_I size);
	bool fichier_global_inherited_read(char *a, U_I size, U_I & read, std::string & message) { throw SRC_BUG; };

	    // inherited from generic_file
	void inherited_sync_write() { if(ref == NULL || hash_ref == NULL) throw SRC_BUG; ref->sync_write(); hash_ref->sync_write(); };
	void inherited_terminate();

    private:
	fichier_global *ref;
	fichier_global *hash_ref;
#if CRYPTO_AVAILABLE
	gcry_md_hd_t hash_handle;
#endif
	std::string ref_filename;
	U_I hash_gcrypt;
	bool eof;
	bool hash_dumped;
    };

	/// @}

} // end of namespace


#endif
