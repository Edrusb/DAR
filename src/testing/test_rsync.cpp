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

#include "libdar.hpp"
#include "shell_interaction.hpp"
#include "fichier_local.hpp"
#include "generic_rsync.hpp"
#include "fichier_local.hpp"
#include "null_file.hpp"

extern "C"
{
}


using namespace std;
using namespace libdar;

static shared_ptr<shell_interaction> ui(new shell_interaction(cout, cerr, false));

void usage(const char *argv0)
{
    cout << "usage: " << argv0 << " sig   <src file> <sig result>" << endl;
    cout << "       " << argv0 << " delta <sig file> <new file> <delta result>" << endl;
    cout << "       " << argv0 << " patch <base file> <sig file> <patched result>" << endl;
}

void go_sig(const string & src_file,
	    const string & sig_result);

void go_delta(const string & sig_file,
	      const string & new_file,
	      const string & delta_result);

void go_patch(const string & base_file,
	      const string & sig_file,
	      const string & patched_result);

int main(int argc, char *argv[])
{
    U_I maj, med, min;

    get_version(maj, med, min);

    try
    {
	if(argc >= 4)
	{
	    if(string(argv[1]) == "sig")
		go_sig(argv[2], argv[3]);
	    else
	    {
		if(argc == 5)
		{
		    if(string(argv[1]) == "delta")
			go_delta(argv[2], argv[3], argv[4]);
		    else
		    {
			if(string(argv[1]) == "patch")
			    go_patch(argv[2], argv[3], argv[4]);
			else
			    usage(argv[0]);
		    }
		}
	    }
	}
	else
	    usage(argv[0]);
    }
    catch(Egeneric & e)
    {
	ui->printf("Exception caught: %S", &(e.get_message()));
	cout << e.dump_str() << endl;
    }
    catch(...)
    {
	ui->printf("unknown exception caught");
    }

    return 0;
}

void go_sig(const string & src_file,
	    const string & sig_result)
{
    fichier_local src = fichier_local(src_file);
    fichier_local res = fichier_local(ui,
				      sig_result,
				      gf_write_only,
				      0777,
				      false,
				      true,
				      false);
    generic_rsync go(&res,
		     &src);
    fichier_local control = fichier_local(ui,
					  src_file + ".bak",
					  gf_write_only,
					  0777,
					  false,
					  true,
					  false);
    go.copy_to(control);
}

void go_delta(const string & sig_file,
	      const string & new_file,
	      const string & delta_result)
{
    fichier_local sig = fichier_local(sig_file);
    fichier_local src = fichier_local(new_file);
    fichier_local res = fichier_local(ui,
				      delta_result,
				      gf_write_only,
				      0777,
				      false,
				      true,
				      false);
    generic_rsync go(&sig,
		     &src,
		     true);
    go.copy_to(res);
}

void go_patch(const string & base_file,
	      const string & sig_file,
	      const string & patched_result)
{
    U_I crc_size = 4;

    fichier_local base = fichier_local(base_file);
    fichier_local sig = fichier_local(sig_file);
    fichier_local res = fichier_local(ui,
				      patched_result,
				      gf_write_only,
				      0777,
				      false,
				      true,
				      false);
    null_file black_hole = null_file(gf_read_write);
    base.reset_crc(crc_size);
    base.copy_to(black_hole);
    crc *ori = base.get_crc();
    base.skip(0);
    generic_rsync go(&base,
		     &sig,
		     ori);
    go.copy_to(res);
    go.terminate();
}
