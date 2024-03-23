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
#include "nls_swap.hpp"

extern "C"
{
} // end extern "C"

#include <string>
#include <new>

#include "libdar_slave.hpp"
#include "path.hpp"
#include "macro_tools.hpp"
#include "sar.hpp"
#include "slave_zapette.hpp"
#include "entrepot_local.hpp"
#include "tools.hpp"

using namespace std;

namespace libdar
{

    class libdar_slave::i_libdar_slave
    {
    public:
	i_libdar_slave(shared_ptr<user_interaction> & dialog,
		       const string & folder,
		       const string & basename,
		       const string & extension,
		       bool input_pipe_is_fd,
		       const string & input_pipe,
		       bool output_pipe_is_fd,
		       const string & output_pipe,
		       const string & execute,
		       const infinint & min_digits);
	i_libdar_slave(const i_libdar_slave & ref) = delete;
	i_libdar_slave(i_libdar_slave && ref) noexcept = delete;
	i_libdar_slave & operator = (const i_libdar_slave & ref) = delete;
	i_libdar_slave & operator = (i_libdar_slave && ref) noexcept = delete;
	~i_libdar_slave() { zap.reset(); entrep.reset(); }; // the order is important

	inline void run() { zap->action(); };

    private:
	shared_ptr<entrepot_local> entrep; ///< must be deleted last
	unique_ptr<slave_zapette> zap; ///< must be deleted first (so we need a pointer to it)
    };


    libdar_slave::i_libdar_slave::i_libdar_slave(shared_ptr<user_interaction> & dialog,
						 const string & folder,
						 const string & basename,
						 const string & extension,
						 bool input_pipe_is_fd,
						 const string & input_pipe,
						 bool output_pipe_is_fd,
						 const string & output_pipe,
						 const string & execute,
						 const infinint & min_digits)
    {
	path chemin(folder);
	tuyau *input = nullptr;
	tuyau *output = nullptr;
	sar *source = nullptr;
	string base = basename;
	U_I input_fd;
	U_I output_fd;

	if(input_pipe.size() == 0)
	    throw Elibcall("libdar_slave::libdar_slave", "empty string given to argument input_pipe of libdar_slave constructor");
	if(output_pipe.size() == 0)
	    throw Elibcall("libdar_slave::libdar_slave", "empty string given to argument input_pipe of libdar_slave constructor");

	if(input_pipe_is_fd)
	    if(!tools_my_atoi(input_pipe.c_str(), input_fd))
		throw Elibcall("libdar_slave::libdar_slave", "non integer provided to argument input_pipe of libdar_slave constructor while input_pipe_is_fd was set");

	if(output_pipe_is_fd)
	    if(!tools_my_atoi(output_pipe.c_str(), output_fd))
		throw Elibcall("libdar_slave::libdar_slave", "non integer provided to argument output_pipe of libdar_slave constructor while output_pipe_is_fd was set");

	entrep.reset(new (nothrow) entrepot_local("", "", false));
	if(!entrep)
	    throw Ememory("libdar_slave::libdar_slave");

	entrep->set_location(chemin);

	try
	{
	    source = new (nothrow) sar(dialog,
				       base,
				       extension,
				       entrep,
				       true,
				       min_digits,
				       false,
				       false,
				       execute);
	    if(source == nullptr)
		throw Ememory("libdar_slave::libdar_slave");

	    if(input_pipe_is_fd)
		input = new (nothrow) tuyau(dialog, input_fd, gf_read_only);
	    else
		input = new (nothrow) tuyau(dialog, input_pipe, gf_read_only);

	    if(input == nullptr)
		throw Ememory("libdar_slave::libdar_slave");

	    if(output_pipe_is_fd)
		output = new (nothrow) tuyau(dialog, output_fd, gf_write_only);
	    else
		output = new (nothrow) tuyau(dialog, output_pipe, gf_write_only);

	    if(output == nullptr)
		throw Ememory("libdar_slave::libdar_slave");

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

	//////////////
	//// libdar_slave public methods


    libdar_slave::libdar_slave(std::shared_ptr<user_interaction> & dialog,
			       const std::string & folder,
			       const std::string & basename,
			       const std::string & extension,
			       bool input_pipe_is_fd,
			       const std::string & input_pipe,
			       bool output_pipe_is_fd,
			       const std::string & output_pipe,
			       const std::string & execute,
			       const infinint & min_digits)
    {
	NLS_SWAP_IN;
        try
        {
	    pimpl.reset(new (nothrow) i_libdar_slave(dialog,
						     folder,
						     basename,
						     extension,
						     input_pipe_is_fd,
						     input_pipe,
						     output_pipe_is_fd,
						     output_pipe,
						     execute,
						     min_digits));

	    if(!pimpl)
		throw Ememory("libdar_slave::libdar_slave");
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    libdar_slave::~libdar_slave() = default;

    void libdar_slave::run()
    {
	NLS_SWAP_IN;
        try
        {
	    pimpl->run();
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

} // end of namespace
