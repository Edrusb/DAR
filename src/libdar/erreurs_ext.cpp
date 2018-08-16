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

#include "erreurs_ext.hpp"

using namespace std;

namespace libdar
{
    Ethread_cancel_with_attr::Ethread_cancel_with_attr(bool now,
						       U_64 x_flag,
						       const infinint & attr) : Ethread_cancel(now, x_flag)
    {
	x_attr = new (nothrow) infinint(attr);
	if(x_attr == nullptr)
	    throw Ememory("Ethread_cancel_with_attr::Ethread_cancel_with_attr");
    }

    Ethread_cancel_with_attr & Ethread_cancel_with_attr::operator = (Ethread_cancel_with_attr && ref) noexcept
    {
	Ethread_cancel::operator = (move(ref));
	swap(x_attr, ref.x_attr);
	return *this;
    }

    void Ethread_cancel_with_attr::detruit()
    {
	try
	{
	    if(x_attr != nullptr)
	    {
		delete x_attr;
		x_attr = nullptr;
	    }
	}
	catch(...)
	{
		// do nothing
	}
    }

    void Ethread_cancel_with_attr::copy_from(const Ethread_cancel_with_attr & ref)
    {
	x_attr = new (nothrow) infinint(*ref.x_attr);
	if(x_attr == nullptr)
	    throw Ememory("Ethread_cancel_with_attr::Ethread_cancel_with_attr");
    }

} // end of namespace
