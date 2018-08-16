/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2018 Denis Corbin
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

#include "../my_config.h"

extern "C"
{

}

#include "crypto.hpp"
#include "erreurs.hpp"

using namespace std;

namespace libdar
{

    string crypto_algo_2_string(crypto_algo algo)
    {
	switch(algo)
	{
	case crypto_algo::none:
	    return gettext("none");
	case crypto_algo::scrambling:
	    return gettext("scrambling (weak encryption)");
	case crypto_algo::blowfish:
	    return "blowfish";
	case crypto_algo::aes256:
	    return "AES 256";
	case crypto_algo::twofish256:
	    return "twofish 256";
	case crypto_algo::serpent256:
	    return "serpent 256";
	case crypto_algo::camellia256:
	    return "camellia 256";
	default:
	    throw SRC_BUG;
	}
    }

    char crypto_algo_2_char(crypto_algo a)
    {
	switch(a)
	{
	case crypto_algo::none:
	    return 'n';
	case crypto_algo::scrambling:
	    return 's';
	case crypto_algo::blowfish:
	    return 'b';
	case crypto_algo::aes256:
	    return 'a';
	case crypto_algo::twofish256:
	    return 't';
	case crypto_algo::serpent256:
	    return 'p';
	case crypto_algo::camellia256:
	    return 'c';
	default:
	    throw SRC_BUG;
	}
    }

    crypto_algo char_2_crypto_algo(char a)
    {
	switch(a)
	{
	case 'n':
	    return crypto_algo::none;
	case 's':
	    return crypto_algo::scrambling;
	case 'b':
	    return crypto_algo::blowfish;
	case 'a':
	    return crypto_algo::aes256;
	case 't':
	    return crypto_algo::twofish256;
	case 'p':
	    return crypto_algo::serpent256;
	case 'c':
	    return crypto_algo::camellia256;
	default:
	    throw Erange("char_to_sym_crypto", gettext("Unknown crypto algorithm"));
	}
    }

    bool same_signatories(const std::list<signator> & a, const std::list<signator> & b)
    {
	list<signator>::const_iterator ita = a.begin();
	list<signator>::const_iterator itb = b.begin();

	while(ita != a.end() && itb != b.end() && *ita == *itb)
	{
	    ++ita;
	    ++itb;
	}

	return (ita == a.end() && itb == b.end());
    }


} // end of namespace
