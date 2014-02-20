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
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if HAVE_ERRNO_H
#include <errno.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif

} // end extern "C"

#include "database_header.hpp"
#include "compressor.hpp"
#include "tools.hpp"
#include "user_interaction.hpp"
#include "integers.hpp"
#include "cygwin_adapt.hpp"
#include "fichier_local.hpp"

using namespace std;

namespace libdar
{

    static const unsigned char database_version = 4;

#define HEADER_OPTION_NONE 0x00

    struct database_header
    {
	unsigned char version;
	unsigned char options;

	void read(generic_file & f) { f.read((char *)&version, 1); f.read((char *)&options, 1); };
	void write(generic_file & f) { f.write((char *)&version, 1); f.write((char *)&options, 1); };
    };

    generic_file *database_header_create(user_interaction & dialog, memory_pool *pool, const string & filename, bool overwrite)
    {
	generic_file *ret = NULL;

	struct stat buf;
	database_header h;
	compressor *comp;

	if(stat(filename.c_str(), &buf) >= 0 && !overwrite)
	    throw Erange("database_header_create", gettext("Cannot create database, file exists"));
	ret = new (pool) fichier_local(dialog, filename, gf_write_only, 0666, true, true, false);
	if(ret == NULL)
	    throw Ememory("database_header_create");

	try
	{
	    h.version = database_version;
	    h.options = HEADER_OPTION_NONE;
	    h.write(*ret);

	    comp = new (pool) compressor(gzip, ret); // upon success, ret is owned by compr
	    if(comp == NULL)
		throw Ememory("database_header_create");
	    else
		ret = comp;
	}
	catch(...)
	{
	    delete ret;
	    throw;
	}

	return ret;
    }

    generic_file *database_header_open(user_interaction & dialog, memory_pool *pool, const string & filename, unsigned char & db_version)
    {
	generic_file *ret = NULL;

	try
	{
	    database_header h;
	    compressor *comp;

	    try
	    {
		ret = new (pool) fichier_local(filename, false);
	    }
	    catch(Erange & e)
	    {
		throw Erange("database_header_open", tools_printf(gettext("Error reading database %S : "), &filename) + e.get_message());
	    }
	    if(ret == NULL)
		throw Ememory("database_header_open");

	    h.read(*ret);
	    if(h.version > database_version)
		throw Erange("database_header_open", gettext("The format version of this database is too high for that software version, use a more recent software to read or modify this database"));
	    db_version = h.version;
	    if(h.options != HEADER_OPTION_NONE)
		throw Erange("database_header_open", gettext("Unknown header option in database, aborting\n"));

	    comp = new (pool) compressor(gzip, ret);
	    if(comp == NULL)
		throw Ememory("database_header_open");
	    else
		ret = comp;
	}
	catch(...)
	{
	    if(ret != NULL)
		delete ret;
	    throw;
	}

	return ret;
    }

    extern const unsigned char database_header_get_supported_version()
    {
	return database_version;
    }

} // end of namespace
