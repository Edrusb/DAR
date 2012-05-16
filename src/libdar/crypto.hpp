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
// to contact the author : dar.linux@free.fr
/*********************************************************************/
// $Id: crypto.hpp,v 1.7 2004/12/04 23:07:25 edrusb Rel $
//
/*********************************************************************/
//

    /// \file crypto.hpp
    /// \brief the crypto algoritm definition

#ifndef CRYPTO_H
#define CRYPTO_H

extern "C"
{
#if HAVE_OPENSSL_BLOWFISH_H
#include <openssl/blowfish.h>
#endif
}

#include "../my_config.h"
#include <string>

#include "tronconneuse.hpp"

namespace libdar
{

	/// the different cypher available for encryption (strong or weak)

	/// values to be used as argument of libdar API calls
	/// \ingroup API
    enum crypto_algo
    {
	crypto_none,        ///< no encryption
	crypto_scrambling,  ///< scrambling weak encryption
	crypto_blowfish     ///< blowfish strong encryption
    };

    extern void crypto_split_algo_pass(const std::string & all, crypto_algo & algo, std::string & pass);

	/// blowfish implementation of encryption

	/// inherited class from tronconneuse class
	/// \ingroup Private
    class blowfish : public tronconneuse
    {
    public:
	blowfish(user_interaction & dialog, U_32 block_size, const std::string & key, generic_file & encrypted_side);
	    // destructor does not seems to be required for BF_KEY

    protected:
	U_32 encrypted_block_size_for(U_32 clear_block_size);
	U_32 clear_block_allocated_size_for(U_32 clear_block_size);
	U_32 encrypt_data(const infinint & block_num,
			  const char *clear_buf, const U_32 clear_size, const U_32 clear_allocated,
			  char *crypt_buf, U_32 crypt_size);
	U_32 decrypt_data(const infinint & block_num,
			  const char *crypt_buf, const U_32 crypt_size,
			  char *clear_buf, U_32 clear_size);

    private:
#if HAVE_OPENSSL_BLOWFISH_H
	BF_KEY clef;
#endif

	void make_ivec(const infinint & ref, unsigned char ivec[8]);

    };

} // end of namespace

#endif
