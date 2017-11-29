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

#include "infinint.hpp"
// yep, strange thing to have to include the therorically less dependent header here.
// including "mem_ui.hpp" here, lead to cyclic dependancy of headers...  this points needs to be clarified

#include "mem_ui.hpp"
#include "tools.hpp"

using namespace std;

namespace libdar
{

    user_interaction & mem_ui::get_ui() const
    {
	if(ui == nullptr)
	    throw SRC_BUG;

	return *(const_cast<mem_ui *>(this)->ui);
    }

    void mem_ui::detruire()
    {
	if(ui != nullptr)
	{
	    if(cloned)
	    {
		delete ui;
		ui = nullptr;
	    }
	    else
		ui = nullptr;
	}
    }

    void mem_ui::copy_from(const mem_ui & ref)
    {
	if(ref.ui == nullptr)
	    ui = nullptr;
	else
	{
	    if(ref.cloned)
	    {
		set_ui(*(ref.ui));
		cloned = true;
	    }
	    else
	    {
		ui = ref.ui;
		cloned = false;
	    }
	}
    }

    void mem_ui::move_from(mem_ui && ref) noexcept
    {
	swap(ui, ref.ui);
	cloned = move(ref.cloned);
    }

    void mem_ui::set_ui(const user_interaction & dialog)
    {
	ui = dialog.clone();
	if(ui == nullptr)
	    throw Ememory("mem_ui::set_ui");
	cloned = true;
    }

} // end of namespace

