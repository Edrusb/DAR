/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2024 Denis Corbin
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

#include "infinint.hpp"
// yep, strange thing to have to include the therorically less dependent header here.
// including "mem_ui.hpp" here, lead to cyclic dependancy of headers...  this points needs to be clarified

#include "mem_ui.hpp"
#include "shell_interaction.hpp"

using namespace std;

namespace libdar
{

    mem_ui::mem_ui(const std::shared_ptr<user_interaction> & dialog): ui(dialog)
    {
	try
	{
	    if(!ui)
		ui = make_shared<shell_interaction>(cerr, cerr, false);
	}
	catch(std::bad_alloc & e)
	{
	    throw Ememory("mem_ui::mem_ui");
	}
    }

} // end of namespace

