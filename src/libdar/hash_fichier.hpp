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
#ifndef GCRYPT_NO_DEPRECATED
#define GCRYPT_NO_DEPRECATED
#endif
#include <gcrypt.h>
#endif
}

#include "generic_file.hpp"
#include "fichier.hpp"

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

    class hash_fichier : public fichier
    {
    public:

	    // constructors (same as those of class fichier)

	hash_fichier(user_interaction & dialog, S_I fd);
	hash_fichier(user_interaction & dialog, const char *name, gf_mode m, U_I perm, bool furtive_mode = false);
        hash_fichier(user_interaction & dialog, const std::string & chemin, gf_mode m, U_I perm,  bool furtive_mode = false);
	hash_fichier(const std::string & chemin, bool furtive_mode = false) : fichier(chemin, furtive_mode) { throw SRC_BUG; };
	hash_fichier(const hash_fichier & ref) : fichier(ref) { throw SRC_BUG; };

	    // assignment operator
	const hash_fichier & operator = (const hash_fichier & ref) { throw SRC_BUG; };

	    // destructor
	~hash_fichier();

	    /// defines the name of the file where to write the hash result
	    /// when terminate() will be executed for this object

	    /// \param[in] filename is the name to associate the hash to
	    /// \param[in] algo is the hash algorithm to use
	    /// \param[in] extension gives the filename to create to drop the hash into "filename.extension" (do not provide the dot in extension)
	    /// \note the filename is dropped into the hash file to follow the format used by md5sum or sha1sum
	    /// the filename is not used to 'open' or read the filename, this is just used to add a reference to
	    /// the hashed file beside the hash result.
	void set_hash_file_name(const std::string & filename, hash_algo algo, const std::string & extension);

	    /// overriden method from class fichier
	void change_permission(U_I perm) { x_perm = perm; force_perm = true; fichier::change_permission(perm); };
	void change_ownership(const std::string & user, const std::string & group) { user_ownership = user; group_ownership = group; fichier::change_ownership(user, group); };

	    // inherited from generic_file

        bool skip(const infinint & pos) { if(pos != fichier::get_position()) throw SRC_BUG; else return true; };
        bool skip_to_eof() { throw SRC_BUG; };
        bool skip_relative(S_I x) { if(x != 0) throw SRC_BUG; else return true; };
	    // no need to overwrite the get_position() method

#ifdef LIBDAR_SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(hash_fichier);
#endif
	    /// for debugging purposes only
	void set_only_hash() { only_hash = true; };

    protected:
	U_I inherited_read(char *a, U_I size) { throw SRC_BUG; };
        void inherited_write(const char *a, U_I size);
	    // no need to overwrite inherited_sync_write() method
	void inherited_terminate();

    private:
	bool only_hash; //< if set, avoids copying data to file, only compute hash (debugging purpose)
	bool hash_ready;
	std::string hash_filename;
	std::string hash_extension;
	bool force_perm;
	U_I x_perm;
	std::string user_ownership;
	std::string group_ownership;
#if CRYPTO_AVAILABLE
	gcry_md_hd_t hash_handle;
#endif
	U_I hash_gcrypt;
	bool eof;


	void dump_hash();
    };

	/// @}

} // end of namespace


#endif
