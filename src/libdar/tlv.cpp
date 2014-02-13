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

    void tlv::write(generic_file & f) const
    {
	U_16 tmp;

	tmp = htons(type);
	f.write((char *)&tmp, 2);
	if(value != NULL)
	{
	    value->size().dump(f);
	    value->dump(f);
	}
	else
	    infinint(0).dump(f);
    }

    void tlv::read(generic_file & f)
    {
	if(value != NULL)
	{
	    delete value;
	    value = NULL;
	}
	init(f);
    }

    void tlv::set_contents(const memory_file & contents)
    {
	if(value != NULL)
	{
	    delete value;
	    value = NULL;
	}
	value = new (get_pool()) storage(contents.get_raw_data());
	if(value == NULL)
	    throw Ememory("tlv::set_contents");
    }

    void tlv::get_contents(memory_file & contents) const
    {
	if(value != NULL)
	    contents.set_raw_data(*value);
	else
	    contents.set_raw_data(storage(0));
    }

    void tlv::init(generic_file & f)
    {
	infinint length;

	f.read((char *)&type, 2);
	type = ntohs(type);
	length.read(f);
	if(length > 0)
	{
	    value = new (get_pool()) storage(f, length);
	    if(value == NULL)
		throw Ememory("tlv::init");
	}
	else
	    value = NULL;
    }

    void tlv::copy_from(const tlv & ref)
    {
	type = ref.type;
	if(ref.value != NULL)
	{
	    value = new (get_pool()) storage(*(ref.value));
	    if(value == NULL)
		throw Ememory("tlv::copy_from");
	}
	else
	    value = NULL;
    }

} // end of namespace
