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
// $Id: crypto.cpp,v 1.12.2.1 2006/02/04 14:47:15 edrusb Rel $
//
/*********************************************************************/
//

#include "../my_config.h"

extern "C"
{
#if HAVE_STRING_H
#include <string.h>
#endif
}

#include "crypto.hpp"
#include "erreurs.hpp"
#include "tools.hpp"
#include "elastic.hpp"

using namespace std;

namespace libdar
{

    void crypto_split_algo_pass(const string & all, crypto_algo & algo, string & pass)
    {
	    // split from "algo:pass" syntax
	string::iterator debut = const_cast<string &>(all).begin();
	string::iterator fin = const_cast<string &>(all).end();
	string::iterator it = debut;
	string tmp;

	if(all == "")
	{
	    algo = crypto_none;
	    pass = "";
	}
	else
	{
	    while(it != fin && *it != ':')
		it++;

	    if(it != fin) // a ':' is present in the given string
	    {
		tmp = string(debut, it);
		it++;
		pass = string(it, fin);
		if(tmp == "scrambling" || tmp == "scram")
		    algo = crypto_scrambling;
		else
		    if(tmp == "none")
			algo = crypto_none;
		    else
			if(tmp == "blowfish" || tmp == "bf" || tmp == "")
			    algo = crypto_blowfish; // blowfish is the default cypher ("")
			else
			    throw Erange("crypto_split_algo_pass", string(gettext("unknown cryptographic algorithm: ")) + tmp);
	    }
	    else // no ':' using blowfish as default cypher
	    {
		algo = crypto_blowfish;
		pass = all;
	    }
	}
    }

    static void dummy_call(char *x)
    {
        static char id[]="$Id: crypto.cpp,v 1.12.2.1 2006/02/04 14:47:15 edrusb Rel $";
        dummy_call(id);
    }

///////////////////////////// BLOWFISH IMPLEMENTATION ////////////////////////////////

#define BF_BLOCK_SIZE 8

    blowfish::blowfish(user_interaction & dialog, U_32 block_size, const string & key, generic_file & encrypted_side)
	: tronconneuse(dialog, block_size, encrypted_side)
    {
#if CRYPTO_AVAILABLE
	char * c_key = tools_str2charptr(key);
	try
	{
	    BF_set_key(&clef, key.size(), (unsigned char *)c_key);
	}
	catch(...)
	{
	    delete [] c_key;
	    throw;
	}
	delete [] c_key;
#else
	throw Ecompilation(gettext("blowfish strong encryption support"));
#endif
    }


    U_32 blowfish::encrypted_block_size_for(U_32 clear_block_size)
    {
	return ((clear_block_size / BF_BLOCK_SIZE) + 1) * BF_BLOCK_SIZE;
	    // round to the upper 8 byte block of data.
	    // and add one 8 byte block if no rounding is necessary.
    }

    U_32 blowfish::clear_block_allocated_size_for(U_32 clear_block_size)
    {
	return encrypted_block_size_for(clear_block_size);
    }

    U_32 blowfish::encrypt_data(const infinint & block_num,
				const char *clear_buf, const U_32 clear_size, const U_32 clear_allocated,
				char *crypt_buf, U_32 crypt_size)
    {
#if CRYPTO_AVAILABLE
	U_32 size_to_fill = encrypted_block_size_for(clear_size);

	    // sanity checks
	    //
	if(crypt_size < size_to_fill)
	    throw SRC_BUG; // not enough room to write encrypted data
	if(clear_allocated < size_to_fill)  // note : clear_block_size_for() returns the same as encrypted_block_size_for()
	    throw SRC_BUG; // not large enough allocated memory in clear buffer to add padding
	    //
	    // end of sanity checks

	if(clear_size < size_to_fill)
	{
	    unsigned char ivec[8];
	    elastic stic = elastic(size_to_fill - clear_size);

	    stic.dump(const_cast<char *>(clear_buf + clear_size), (U_32)(clear_allocated - clear_size));
	    make_ivec(block_num, ivec);
	    BF_cbc_encrypt((const unsigned char *)clear_buf, (unsigned char *)crypt_buf, size_to_fill, &clef, ivec, BF_ENCRYPT);
	    return size_to_fill;
	}
	else
	    throw SRC_BUG;
#else
	throw Ecompilation(gettext("blowfish strong encryption support"));
#endif
    }

    U_32 blowfish::decrypt_data(const infinint & block_num, const char *crypt_buf, const U_32 crypt_size, char *clear_buf, U_32 clear_size)
    {
#if CRYPTO_AVAILABLE
	unsigned char ivec[8];

	make_ivec(block_num, ivec);
	BF_cbc_encrypt((const unsigned char *)crypt_buf, (unsigned char *)clear_buf, crypt_size, &clef, ivec, BF_DECRYPT);

	elastic stoc = elastic(clear_buf, crypt_size, elastic_backward);
	return crypt_size - stoc.get_size();
#else
	throw Ecompilation(gettext("blowfish strong encryption support"));
#endif
    }

    void blowfish::make_ivec(const infinint & ref, unsigned char ivec[8])
    {
	infinint upper = ref >> 32;
	U_32 high = 0, low = 0;

	high = upper % (U_32)(0xFFFF); // for bytes (high weight)
	low = ref % (U_32)(0xFFFF); // for bytes (lowest weight)

	ivec[0] = low % 8;
	ivec[1] = (low >> 8) % 8;
	ivec[2] = (low >> 16) % 8;
	ivec[3] = (low >> 24) % 8;
	ivec[4] = high % 8;
	ivec[5] = (high >> 8) % 8;
	ivec[6] = (high >> 16) % 8;
	ivec[7] = (high >> 24) % 8;
    }

} // end of namespace

