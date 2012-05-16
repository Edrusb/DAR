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
// $Id: crypto.cpp,v 1.2 2003/12/10 21:26:21 edrusb Rel $
//
/*********************************************************************/
//

#include "crypto.hpp"
#include "erreurs.hpp"

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
			throw Erange("crypto_split_algo_pass", string("unknown cryptographic algorithm: ") + tmp);
	    }
	    else // no ':', thus assuming scrambling for compatibility with older versions
	    {
		algo = crypto_scrambling;
		pass = all;
	    }
	}
    }

    static void dummy_call(char *x)
    {
        static char id[]="$Id: crypto.cpp,v 1.2 2003/12/10 21:26:21 edrusb Rel $";
        dummy_call(id);
    }



} // end of namespace

