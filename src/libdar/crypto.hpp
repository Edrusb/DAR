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

    /// \file crypto.hpp
    /// \brief the crypto algoritm definition
    /// \ingroup API

#ifndef CRYPTO_HPP
#define CRYPTO_HPP

extern "C"
{

}

#include "../my_config.h"
#include <string>

#include "datetime.hpp"

#include <list>

namespace libdar
{

	/// \ingroup Private
	/// @}

	/// the different cypher available for encryption (strong or weak)

	/// values to be used as argument of libdar API calls
	/// \ingroup API
    enum crypto_algo
    {
	crypto_none,          ///< no encryption
	crypto_scrambling,    ///< scrambling weak encryption
	crypto_blowfish,      ///< blowfish strong encryption
	crypto_aes256,        ///< AES 256 strong encryption
	crypto_twofish256,    ///< twofish 256 strong encryption
	crypto_serpent256,    ///< serpent 256 strong encryption
	crypto_camellia256    ///< camellia 256 strong encryption
    };


    struct signator
    {
	enum
	{
	    good,         //< good signature
	    bad,          //< key correct bug signature tempered
	    unknown_key,  //< no key found to check the signature
	    error         //< signature failed to be checked for other error
	} result;         //< status of the signing
	enum
	{
	    valid,        //< the key we have is neither expired nor revoked
	    expired,      //< the key we have has expired
	    revoked       //< the key we have has been revoked
	} key_validity;   //< validity of the key used to verify the signature
	std::string fingerprint; //< fingerprint of the key
	datetime signing_date;   //< date of signature
	datetime signature_expiration_date; //< date of expiration of this signature
	bool operator < (const signator & ref) const { return fingerprint < ref.fingerprint; };
	bool operator == (const signator & ref) const { return result == ref.result && key_validity == ref.key_validity && fingerprint == ref.fingerprint && signature_expiration_date == ref.signature_expiration_date; };
    };


    extern std::string crypto_algo_2_string(crypto_algo algo);

    extern char crypto_algo_2_char(crypto_algo a);
    extern crypto_algo char_2_crypto_algo(char a);

    extern bool same_signatories(const std::list<signator> & a, const std::list<signator> & b);

	/// @}

} // end of namespace

#endif
