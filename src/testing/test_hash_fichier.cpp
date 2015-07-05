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
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
} // end extern "C"


#include <new>

#include "libdar.hpp"
#include "hash_fichier.hpp"
#include "shell_interaction.hpp"
#include "fichier_local.hpp"

using namespace libdar;

void f1(const string & src_filename, const string & dst_filename, hash_algo algo);
void error(const string & argv0);
libdar::hash_algo str2hash(const string & val);

static user_interaction *ui = nullptr;

int main(int argc, char *argv[])
{
    U_I maj, med, min;

    get_version(maj, med, min);
    ui = new (nothrow) shell_interaction(&cout, &cerr, true);
    if(ui == nullptr)
	cout << "ERREUR !" << endl;

    try
    {
	try
	{
	    try
	    {
		if(argc != 4)
		    error(argv[0]);
		else
		    f1(argv[1], argv[2], str2hash(argv[3]));
	    }
	    catch(Egeneric & e)
	    {
		ui->warning(e.get_message());
		e.dump();
	    }
	}
	catch(...)
	{
	    if(ui != nullptr)
		delete ui;
	    throw;
	}
	if(ui != nullptr)
	    delete ui;
    }
    catch(Egeneric & e)
    {
	cout << e.dump_str() << endl;
    }
}

void f1(const string & src_filename, const string & dst_filename, hash_algo algo)
{
    fichier_local *dst_hash = new (nothrow) fichier_local(*ui,
							  src_filename + "." + hash_algo_to_string(algo),
							  gf_write_only,
							  tools_octal2int("0777"),
							  true, // fail if exsts
							  false, // erase
							  false); // furtive read mode
    fichier_local *dst_data = new (nothrow) fichier_local(*ui,
							  dst_filename,
							  gf_write_only,
							  tools_octal2int("0777"),
							  false,
							  false,
							  false);

    try
    {
	if(dst_hash == nullptr
	   || dst_data == nullptr)
	    throw Ememory("f1");
	else
	{
	    hash_fichier dst = hash_fichier(*ui,
					    dst_data,
					    src_filename,
					    dst_hash,
					    algo);
	    dst_hash = nullptr; // now owned by dst
	    dst_data = nullptr; // now owned by dst
	    fichier_local src = fichier_local(src_filename);

	    dst.set_only_hash();
	    src.copy_to(dst);
	}
    }
    catch(...)
    {
	if(dst_hash != nullptr)
	    delete dst_hash;
	if(dst_data != nullptr)
	    delete dst_data;
	throw;
    }
}

void error(const string & argv0)
{
    ui->printf("usage: %S <source filename> <dest filename> <hash algo>", &argv0);
}

libdar::hash_algo str2hash(const string & val)
{
    if(val == "md5")
	return libdar::hash_md5;
    if(val == "sha1")
	return libdar::hash_sha1;
    throw Erange("str2hash", "unknown hash algorithm");
}
