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
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
}

#include "tlv.hpp"

using namespace std;

namespace libdar
{

    void tlv::dump(generic_file & f) const
    {
	U_16 tmp;
	tlv *me = const_cast<tlv *>(this);

	if(me == nullptr)
	    throw SRC_BUG;
	tmp = htons(type);
	f.write((char *)&tmp, 2);
	size().dump(f);
	me->skip(0);
	me->copy_to(f);
    }

    void tlv::setup(generic_file & f)
    {
	init(f);
    }

    void tlv::init(generic_file & f)
    {
	infinint length;

	f.read((char *)&type, 2);
	type = ntohs(type);
	length.read(f);
	reset();
	if(f.copy_to(*this, length) != length)
	    throw Erange("tlv::init",gettext("Missing data to initiate a TLV object"));
    }

} // end of namespace
