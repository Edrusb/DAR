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
	case crypto_none:
	    return gettext("none");
	case crypto_scrambling:
	    return gettext("scrambling (weak encryption)");
	case crypto_blowfish:
	    return "blowfish";
	case crypto_aes256:
	    return "AES 256";
	case crypto_twofish256:
	    return "twofish 256";
	case crypto_serpent256:
	    return "serpent 256";
	case crypto_camellia256:
	    return "camellia 256";
	default:
	    throw SRC_BUG;
	}
    }

} // end of namespace
