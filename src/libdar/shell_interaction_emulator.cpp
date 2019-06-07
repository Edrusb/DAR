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

#include "../my_config.h"
#include "shell_interaction_emulator.hpp"

extern "C"
{
} // end extern "C"

using namespace std;
using namespace libdar;

namespace libdar
{

    shell_interaction_emulator::shell_interaction_emulator(user_interaction *emulator):
	shell_interaction(std::cerr, std::cerr, true)
    {
	if(emulator != nullptr)
	    emul = emulator;
	else
	    throw SRC_BUG;
    }

    bool shell_interaction_emulator::inherited_pause(const std::string & message)
    {
	bool ret = true;
	try
	{
	    emul->pause(message);
	}
	catch(Euser_abort & e)
	{
	    ret = false;
	}

	return ret;
    }

} // end of namespace

