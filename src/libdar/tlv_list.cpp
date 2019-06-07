/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2019 Denis Corbin
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
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

#include "tlv_list.hpp"

using namespace std;

namespace libdar
{

    void tlv_list::dump(generic_file & f) const
    {
	infinint number = contents.size();
	deque<tlv>::iterator it = const_cast<tlv_list *>(this)->contents.begin();
	deque<tlv>::iterator fin = const_cast<tlv_list *>(this)->contents.end();

	number.dump(f);
	while(it != fin)
	{
	    it->dump(f);
	    it++;
	}
    }

    tlv & tlv_list::operator[] (U_I item) const
    {
	tlv_list *me = const_cast<tlv_list *>(this);

	if(item > contents.size())
	    throw Erange("tlv_list::operator[]", "index out of range when accessing a tlv_list object");
	if(me == nullptr)
	    throw SRC_BUG;

	return me->contents[item];
    }

    void tlv_list::init(generic_file & f)
    {
	infinint number;

	number.read(f); // read from file the number of tlv stored

	contents.clear(); // erase list contents
	while(!number.is_zero()) // read each tlv from file
	{
	    contents.push_back(tlv(f));
	    number--;
	}
    }


} // end of namespace
