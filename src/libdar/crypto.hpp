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
// $Id: crypto.hpp,v 1.1 2003/11/19 00:42:49 edrusb Rel $
//
/*********************************************************************/
//

#ifndef CRYPTO_H
#define CRYPTO_H

#include "../my_config.h"
#include <string>

namespace libdar
{

    enum crypto_algo { crypto_none, crypto_scrambling };

    extern void crypto_split_algo_pass(const std::string & all, crypto_algo & algo, std::string & pass);

} // end of namespace

#endif
