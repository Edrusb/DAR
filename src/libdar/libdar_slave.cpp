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
} // end extern "C"

#include <string>
#include <new>

#include "libdar_slave.hpp"
#include "path.hpp"
#include "macro_tools.hpp"

using namespace std;

namespace libdar
{

    libdar_slave::libdar_slave(shared_ptr<user_interaction> & dialog,
			       const string & folder,
			       const string & basename,
			       const string & extension,
			       const string & input_pipe,
			       const string & output_pipe,
			       const string & execute,
			       const infinint & min_digits)
    {
	path chemin(folder);
	tuyau *input = nullptr;
	tuyau *output = nullptr;
	sar *source = nullptr;
	string base = basename;

	entrep.reset(new (nothrow) entrepot_local("", "", false));
	if(!entrep)
	    throw Ememory("libdar_slave::libdar_slave");

	entrep->set_location(chemin);

	try
	{
	    source = new (nothrow) sar(dialog,
				       base,
				       extension,
				       *entrep,
				       true,
				       min_digits,
				       false,
				       execute);
	    if(source == nullptr)
		throw Ememory("libdar_slave::libdar_slave");

	    macro_tools_open_pipes(dialog,
				   input_pipe,
				   output_pipe,
				   input,
				   output);

	    zap.reset(new (nothrow) slave_zapette(input, output, source));
	    if(!zap)
		throw Ememory("libdar_slave::libdar_slave");
            input = output = nullptr; // now managed by zap;
            source = nullptr;  // now managed by zap;
	}
	catch(...)
	{
	    if(input != nullptr)
		delete input;
	    if(output != nullptr)
		delete output;
	    if(source != nullptr)
		delete source;
	    throw;
	}
    }

} // end of namespace
