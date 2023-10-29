//*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2023 Denis Corbin
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
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
} // end extern "C"

#include "libdar.hpp"
#include "no_comment.hpp"
#include "config_file.hpp"
#include "cygwin_adapt.hpp"
#include "shell_interaction.hpp"
#include "user_interaction.hpp"
#include "entrepot_libcurl.hpp"
#include "fichier_local.hpp"
#include "mycurl_protocol.hpp"

using namespace libdar;

void usage(int argc,
	   char *argv[]);

void get_args(int argc,
	      char *argv[],
	      mycurl_protocol & proto,
	      string & login,
	      secu_string & pass,
	      string & host,
	      string & chemin,
	      string & port);

void f1(int argc, char *argv[]);
void f2(int argc, char *argv[]);
void f3(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    U_I maj, med, min;

    try
    {
	get_version(maj, med, min);
	    // f1(argc, argv);
	    // f2(argc, argv);
	f3(argc, argv);
    }
    catch(Egeneric & e)
    {
	cout << "Execption caught: " << e.dump_str() << endl;
    }
}

void usage(int argc,
	   char *argv[])
{
    cout << "usage: { ftp | sftp } <login> <pass> <host> [ <chemin> <port> ]" << endl;
}

void get_args(int argc,
	      char *argv[],
	      mycurl_protocol & proto,
	      string & login,
	      secu_string & pass,
	      string & host,
	      string & chemin,
	      string & port)
{
    string tmp;

    if(argc < 5 || argc > 7)
    {
	usage(argc, argv);
	exit(1);
    }

    proto = string_to_mycurl_protocol(argv[1]);
    login = argv[2];
    tmp = argv[3];
    pass = secu_string(tmp.c_str(), tmp.size());
    host = argv[4];
    if(argc > 5)
	chemin = argv[5];
    else
	chemin = "";
    if(argc > 6)
	port = argv[6];
    else
	port = "";
}

void f1(int argc, char *argv[])
{
    mycurl_protocol proto;
    string login;
    secu_string pass;
    string host;
    string port;
    string chemin;
    get_args(argc,
	     argv,
	     proto,
	     login,
	     pass,
	     host,
	     chemin,
	     port);

    shared_ptr<user_interaction> ui(new shell_interaction(cout, cerr, true));
    entrepot_libcurl reposito(ui,
			      proto,
			      login,
			      pass,
			      host,
			      port,
			      false,
			      "",
			      "",
			      "",
			      3);

    string entry;
    bool isdir;
    ui->printf("Directory content");
    reposito.read_dir_reset_dirinfo();
    while(reposito.read_dir_next_dirinfo(entry, isdir))
	ui->printf("%s | %s", isdir? "DIR" : "xxx", entry.c_str());


    fichier_local readme("/etc/fstab");
    fichier_local *writetome = new fichier_local(ui,
						 "/tmp/test.tmp",
						 gf_write_only,
						 0644,
						 false,
						 true,
						 false);
    fichier_local *writetomepart = new fichier_local(ui,
						     "/tmp/test-part.tmp",
						     gf_write_only,
						     0644,
						     false,
						     true,
						     false);

    try
    {
	string tmp;
	U_I fast_retry = 30;

	if(writetome == nullptr || writetomepart == nullptr)
	    throw Ememory("f1");


	while(--fast_retry > 0)
	{
	    if(chemin != "")
		reposito.set_location(chemin);
	    cout << "Listing: " << reposito.get_url() << endl;
	    reposito.read_dir_reset();
	    while(reposito.read_dir_next(tmp))
		cout << " -> " << tmp << endl;
	    cout << endl;
	}

	try
	{
	    tmp = "cuicui";
	    cout << "removing file " << tmp << endl;
	    reposito.unlink(tmp);
	    cout << endl;
	}
	catch(Erange & e)
	{
	    ui->message(e.get_message());
	}

	cout << "Listing: " << reposito.get_url() << endl;
	reposito.read_dir_reset();
	while(reposito.read_dir_next(tmp))
	    cout << " -> " << tmp << endl;
	cout << endl;

	fichier_global *remotew = reposito.open(ui,
						"cuicui",
						gf_write_only,
						false,
						0644,
						true,
						true,
						hash_algo::none);
	if(remotew == nullptr)
	    throw SRC_BUG;
	try
	{
	    readme.copy_to(*remotew);
	    remotew->terminate();
	}
	catch(...)
	{
	    delete remotew;
	    throw;
	}
	delete remotew;

	fichier_global *fic = reposito.open(ui,
					    "cuicui",
					    gf_read_only,
					    false,
					    0,
					    false,
					    false,
					    hash_algo::none);
	if(fic == nullptr)
	    throw SRC_BUG;

	try
	{
	    infinint file_size = fic->get_size();
	    ui->printf("size = %i", &file_size);

	    fic->copy_to(*writetome);
	}
	catch(...)
	{
	    delete fic;
	    throw;
	}
	delete fic;
	fic = nullptr;
	delete writetome;
	writetome = nullptr;

	fichier_global *foc = reposito.open(ui,
					    "cuicui",
					    gf_read_only,
					    false,
					    0,
					    false,
					    false,
					    hash_algo::none);

	if(foc == nullptr)
	    throw SRC_BUG;

	try
	{
	    foc->skip(20);
	    foc->copy_to(*writetomepart);
	}
	catch(...)
	{
	    delete foc;
	    throw;
	}
	delete foc;
	foc = nullptr;
	delete writetomepart;
	writetomepart = nullptr;

	fichier_global *fac = reposito.open(ui,
					    "cuicui",
					    gf_read_only,
					    false, // force permission
					    0,     // permission
					    false, // fail if exist
					    false, // erase
					    hash_algo::none);
	const U_I BUFSIZE = 1000;
	char buf[BUFSIZE];
	infinint tamp;
	U_I utamp;
	U_I step = 600;

	if(fac == nullptr)
	    throw SRC_BUG;

	try
	{
	    fac->skip(10);

	    tamp = fac->read(buf, step);
	    utamp = 0;
	    tamp.unstack(utamp);
	    buf[utamp] = '\0';
	    cout << "reading " << step << " first chars: " << buf << endl;
	    tamp = fac->get_position();
	    ui->printf("position = %i", &tamp);
	    tamp = fac->get_size();
	    ui->printf("file size = %i", &tamp);

	    tamp = fac->read(buf, step);
	    utamp = 0;
	    tamp.unstack(utamp);
	    buf[utamp] = '\0';
	    cout << "reading " << step << " next chars:  " << buf << endl;
	    tamp = fac->get_position();
	    ui->printf("position = %i", &tamp);
	}
	catch(...)
	{
	    delete fac;
	    throw;
	}
	delete fac;


	fichier_global *fec = reposito.open(ui,
					    "cuicui",
					    gf_write_only,
					    false, // force permission
					    0644,  // permission
					    false, // fail if exist
					    false,  // erase
					    hash_algo::none);
	if(fec == nullptr)
	    throw SRC_BUG;
	try
	{
	    fec->skip_to_eof();
	    readme.skip(0);
	    readme.copy_to(*fec);
	}
	catch(...)
	{
	    delete fec;
	    throw;
	}
	delete fec;

    }
    catch(...)
    {
	if(writetome != nullptr)
	    delete writetome;
	if(writetomepart != nullptr)
	    delete writetomepart;
	throw;
    }

    if(writetome != nullptr)
	delete writetome;
    if(writetomepart != nullptr)
	delete writetomepart;
}

void f2(int argc, char* argv[])
{
    shared_ptr<user_interaction> ui(new shell_interaction(cout, cerr, true));

    entrepot_local repo("", "", false);
    repo.set_location(path("/tmp"));
    string pwd = repo.get_full_path().display();

    cout << pwd << endl;
    cout << repo.get_url() << endl;

    repo.create_dir("COUCOU", 0750);
    repo.set_location(repo.get_full_path() + string("COUCOU"));
}

void f3(int argc, char* argv[])
{
    mycurl_protocol proto;
    string login;
    secu_string pass;
    string host;
    string port;
    string chemin;
    get_args(argc,
	     argv,
	     proto,
	     login,
	     pass,
	     host,
	     chemin,
	     port);

    shared_ptr<user_interaction> ui(new shell_interaction(cout, cerr, true));
    entrepot_libcurl repo(ui,
			  proto,
			  login,
			  pass,
			  host,
			  port,
			  false,
			  "",
			  "",
			  "",
			  3);

    repo.set_location(path("/home/dar-check"));

    string pwd = repo.get_full_path().display();

    cout << pwd << endl;
    cout << repo.get_url() << endl;

    repo.create_dir("COUCOU", 0750);
    repo.set_location(repo.get_full_path() + string("COUCOU"));
}
