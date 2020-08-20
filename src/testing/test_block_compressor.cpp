/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2020 Denis Corbin
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

extern "C"
{
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_TIME_H
#include <time.h>
#endif
}

#include "libdar.hpp"
#include "lz4_module.hpp"
#include "parallel_block_compressor.hpp"
#include "tools.hpp"
#include "fichier_local.hpp"

using namespace libdar;
using namespace std;


void f1();
void f2(const char *src, const char *dst, bool encrypt, U_I num, const char *pass);


static shared_ptr<user_interaction>ui;

int main(int argc, char* argv[])
{
    U_I maj, med, min;

    get_version(maj, med, min);
    ui.reset(new (nothrow) shell_interaction(cout, cerr, false));
    if(!ui)
	cout << "ERREUR !" << endl;
    try
    {
	f1();

	if(argc < 6)
	    ui->message(tools_printf("usage: %s { -e | -d } <src> <dst> <num> <pass>", argv[0]));
	else
	{
	    if(strcmp(argv[1], "-d") != 0 && strcmp(argv[1], "-e") != 0)
		ui->message("use -d to decrypt or -e to encrypt");
	    else
		f2(argv[2], argv[3], strcmp(argv[1], "-e") == 0, atoi(argv[4]), argv[5]);
	}
    }
    catch(Ebug & e)
    {
	ui->message(e.dump_str());
    }
    catch(Egeneric & e)
    {
	ui->message(string("Aborting on exception: ") + e.get_message());
    }
    ui.reset();
}

#define SOURCE "toto.txt"
#define ZIPPED "titi.txt"
#define BACK  "tutu.txt"

void f1()
{
    if(true)
    {
	unique_ptr<fichier_local> src = make_unique<fichier_local>(SOURCE, false);
	unique_ptr<fichier_local> dst = make_unique<fichier_local>(ui, ZIPPED, gf_write_only, 0644, false, true, false);
	unique_ptr<compress_module> lz4 = make_unique<lz4_module>();
	parallel_block_compressor comp(2,
				       lz4,
				       *dst);

	src->copy_to(comp);
    }

    if(true)
    {
	unique_ptr<fichier_local> src = make_unique<fichier_local>(ZIPPED, false);
	unique_ptr<fichier_local> dst = make_unique<fichier_local>(ui, BACK, gf_write_only, 0644, false, true, false);
	unique_ptr<compress_module> lz4 = make_unique<lz4_module>();
	parallel_block_compressor decomp(2,
					 lz4,
					 *src);

	decomp.copy_to(*dst);
    }
}

void f2(const char *src, const char *dst, bool encrypt, U_I num, const char *pass)
{
}
