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

#include <string>

#include "fichier_global.hpp"
#include "integers.hpp"
#include "archive_aux.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

	/// manages the generation of a hash

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

        hash_fichier(const std::shared_ptr<user_interaction> & dialog,
		     fichier_global *under,
		     const std::string & under_filename,
		     fichier_global *hash_file,
		     hash_algo algo);

	    /// copy constructor
	hash_fichier(const hash_fichier & ref) = delete;

	    /// move constructor
	hash_fichier(hash_fichier && ref) noexcept = delete;

	    /// assignment operator
	hash_fichier & operator = (const hash_fichier & ref) = delete;

	    /// move operator
	hash_fichier & operator = (hash_fichier && ref) noexcept = delete;

	    /// destructor
	~hash_fichier();

	    // inherited from fichier_global
	virtual void change_ownership(const std::string & user, const std::string & group) override { if(ref == nullptr || hash_ref == nullptr) throw SRC_BUG; ref->change_ownership(user, group); hash_ref->change_ownership(user, group); };
	virtual void change_permission(U_I perm) override { if(ref == nullptr || hash_ref == nullptr) throw SRC_BUG; ref->change_permission(perm); hash_ref->change_permission(perm); };
	virtual infinint get_size() const override { if(ref == nullptr) throw SRC_BUG; return ref->get_size(); };
	virtual void fadvise(advise adv) const override { if(ref == nullptr) throw SRC_BUG; ref->fadvise(adv); };

	    // inherited from generic_file
	virtual bool skippable(skippability direction, const infinint & amount) override { return false; };
        virtual bool skip(const infinint & pos) override {if(ref == nullptr || pos != ref->get_position()) throw SRC_BUG; else return true; };
        virtual bool skip_to_eof() override { if(get_mode() == gf_write_only) return true; else throw SRC_BUG; };
        virtual bool skip_relative(S_I x) override { if(x != 0) throw SRC_BUG; else return true; };
	virtual bool truncatable(const infinint & pos) const override { return false; };
	virtual infinint get_position() const override { if(ref == nullptr) throw SRC_BUG; return ref->get_position(); };

	    /// for debugging purposes only
	void set_only_hash() { only_hash = true; };

    protected:
	    // inherited from fichier_global
	virtual void inherited_read_ahead(const infinint & amount) override { ref->read_ahead(amount); };
	virtual U_I fichier_global_inherited_write(const char *a, U_I size) override;
 	virtual bool fichier_global_inherited_read(char *a, U_I size, U_I & read, std::string & message) override;

	    // inherited from generic_file
	virtual void inherited_truncate(const infinint & pos) override { throw SRC_BUG; }; // truncate not supported on hash files
	virtual void inherited_sync_write() override {};
	virtual void inherited_flush_read() override {};
	virtual void inherited_terminate() override;

    private:
	fichier_global *ref;
	fichier_global *hash_ref;
	bool only_hash; ///< if set, avoids copying data to file, only compute hash (debugging purpose)
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
