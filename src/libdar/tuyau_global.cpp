/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2022 Denis Corbin
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
}

#include "tuyau_global.hpp"

using namespace std;

namespace libdar
{

    tuyau_global::tuyau_global(const std::shared_ptr<user_interaction> & dialog,
			       fichier_global *x_ptr):
	fichier_global(dialog, gf_read_only)
    {
	if(x_ptr == nullptr)
	    throw SRC_BUG;

	    // now we know the provided pointer is valid
	    // we can set the proper mode in place of gf_read_only
	    // passed to fichier_global constructor above
	set_mode(x_ptr->get_mode());

	ptr = x_ptr;
	current_pos = 0;
    }

    bool tuyau_global::skip(const infinint & pos)
    {
	if(pos < current_pos)
	    return false;
	else
	{
	    infinint amount = pos - current_pos;
	    U_I step_asked = 0;
	    U_I step_done = 0;

	    while(!amount.is_zero() && step_done == step_asked)
	    {
		step_asked = 0;
		amount.unstack(step_asked);
		step_done = read_and_drop(step_asked);
		current_pos += step_done;
	    }

	    return step_done == step_asked;
	}
    }

    bool tuyau_global::skip_to_eof()
    {
	U_I read;

	do
	{
	    read = read_and_drop(buffer_size);
	    current_pos += infinint(read);
	}
	while(read == buffer_size);

	return true;
    }

    bool tuyau_global::skip_relative(S_I x)
    {
	U_I read;

	if(x < 0)
	    return false;

	read = read_and_drop(U_I(x));
	current_pos += read;

	return read == U_I(x);
    }

    U_I tuyau_global::fichier_global_inherited_write(const char *a, U_I size)
    {
	ptr->write(a, size);
	current_pos += size;
	return size;
    }

    bool tuyau_global::fichier_global_inherited_read(char *a, U_I size, U_I & read, std::string & message)
    {
	current_pos += ptr->read(a, size);
	return true;
    }

    U_I tuyau_global::read_and_drop(U_I bytes)
    {
	U_I read = 0;
	U_I min;
	U_I incr;

	while(read < bytes)
	{
	    min = bytes > buffer_size ? buffer_size : bytes;
	    incr += ptr->read(buffer, min);
		// we used read, to let the object handle to exception
		// contexts and if fixed continue the read_and_drop
		// operation normally
	    read += incr;
	    if(incr < min) // reached eof
		bytes = read; // we force the loop to end
	}

	return read;
    }

    void tuyau_global::detruit()
    {
	if(ptr != nullptr)
	{
	    delete ptr;
	    ptr = nullptr;
	}
    }


} // end of namespace
