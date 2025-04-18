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

#include "../my_config.h"

extern "C"
{
	// to allow compilation under Cygwin we need <sys/types.h>
	// else Cygwin's <netinet/in.h> lack __int16_t symbol !?!
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
} // end extern "C"

#include "archive_num.hpp"
#include "generic_file.hpp"

using namespace std;

namespace libdar
{

    void archive_num::read_from_file(generic_file &f)
    {
	char buffer[val_size];
	U_16 *ptr = (U_16 *)&(buffer[0]);

	f.read(buffer, val_size);
	val = ntohs(*ptr);
    }

    void archive_num::write_to_file(generic_file &f) const
    {
	char buffer[val_size];
	U_16 *ptr = (U_16 *)&(buffer[0]);

	*ptr = htons(val);
	f.write(buffer, val_size);
    }

} // end of namespace
