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
#if HAVE_STDIO_H
#include <stdio.h>
#endif
#if MUTEX_WORKS
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif
#endif
} // end extern "C"

#include "libdar.hpp"
#include "tools.hpp"
#include "null_file.hpp"
#include "shell_interaction.hpp"

using namespace std;
using namespace libdar;

void f1();
void warning(const string &x, void *context);
bool question(const string &x, void *context);
string getstring(const string &x, bool echo, void *context);
secu_string getsecustring(const string &x, bool echo, void *context);
void f2();
void f3();
void f4();
void f5();

static user_interaction_callback ui = user_interaction_callback(warning, question, getstring, getsecustring,(void *)1000);

int main()
{
    f1();
    f2();
    f3();
    f4();
    f5();
}

#define BOOL2STR(val) ( val ? "yes" : "no" )

void f1()
{
    U_I maj, med, min;
    bool ea, large, nodump, special, thread, libz, libbz2, liblzo2, libcrypto, furtive;
    U_I bits;
    libdar::compile_time::endian endy;

    get_version(maj, med, min);
    ui.printf("version %u.%u.%u\n", maj, med, min);
    ea = libdar::compile_time::ea();
    large = libdar::compile_time::largefile();
    nodump = libdar::compile_time::nodump();
    special = libdar::compile_time::special_alloc();
    bits = libdar::compile_time::bits();
    thread = libdar::compile_time::thread_safe();
    libz = libdar::compile_time::libz();
    libbz2 = libdar::compile_time::libbz2();
    liblzo2 = libdar::compile_time::liblzo();
    libcrypto = libdar::compile_time::libgcrypt();
    furtive = libdar::compile_time::furtive_read();
    endy = libdar::compile_time::system_endian();
    ui.printf("features:\nEA = %s\nLARGE = %s\nNODUMP = %s\nSPECIAL = %s\nbits = %u\nthread = %s\nlibz =%s\nlibbz2 = %s\nliblzo = %s\nlibcrypto = %s\nfurtive = %s\nendian = %c\n",
	      BOOL2STR(ea),
	      BOOL2STR(large),
	      BOOL2STR(nodump),
	      BOOL2STR(special),
	      bits,
	      BOOL2STR(thread),
	      BOOL2STR(libz),
	      BOOL2STR(libbz2),
	      BOOL2STR(liblzo2),
	      BOOL2STR(libcrypto),
	      BOOL2STR(furtive),
	      endy);
}

void warning(const string &x, void *context)
{
    cout << "[" << context << "]" << x.c_str() << endl;
}

bool question(const string & x, void *context)
{
    bool rep = false;
    char r;

    printf("[%p]%s\n", context, x.c_str());
    scanf("%c", &r);
    rep = r == 'y';

    return rep;
}

string getstring(const string &x, bool echo, void *context)
{
    throw SRC_BUG;
}

secu_string getsecustring(const string &x, bool echo, void *context)
{
    throw SRC_BUG;
}

void listing(const std::string & flag,
	     const std::string & perm,
	     const std::string & uid,
	     const std::string & gid,
	     const std::string & size,
	     const std::string & date,
	     const std::string & filename,
	     bool is_dir,
	     bool has_children,
	     void *context)
{
    ui.printf("[[%p]][%S][%S][%S][%S][%S][%S][%S][%s][%s]\n", context, &flag, &perm, &uid, &gid, &size, &date, &filename, is_dir ? "dir" : "not_dir", has_children ? "has children" : "no children");
}

void f2()
{
    U_16 code;
    string msg;
    statistics st;
    archive_options_read read_options;
    archive_options_create create_options;

    create_options.set_subtree(simple_path_mask("/etc", true));
    archive *toto = create_archive_noexcept(ui,
					    "/",
					    ".",
					    "toto",
					    "dar",
					    create_options,
					    &st,
					    code,
					    msg);
    if(code != LIBDAR_NOEXCEPT && code != LIBDAR_EUSER_ABORT)
    {
	ui.printf("exception creating archive: %S\n", &msg);
	return;
    }
    if(toto != nullptr)
    {
	archive_options_listing options;

	options.clear();
	options.set_info_details(true);
	options.set_list_mode(archive_options_listing::normal);
	options.set_selection(bool_mask(true));
	options.set_filter_unsaved(false);
	op_listing_noexcept(ui, toto, options, code, msg);
	if(code != LIBDAR_NOEXCEPT && code != LIBDAR_EUSER_ABORT)
	{
	    ui.printf("exception creating archive: %S\n", &msg);
	}
	close_archive_noexcept(toto, code, msg);
    }

    read_options.clear();
    read_options.set_info_details(true);
    archive *arch = open_archive_noexcept(ui, ".", "toto", "dar", read_options, code, msg);
    if(code != LIBDAR_NOEXCEPT)
    {
	ui.printf("exception openning archive: %S\n", &msg);
	return;
    }

    ui.set_listing_callback(&listing);
    bool ret = get_children_of_noexcept(ui, arch, "etc/rc.d", code, msg);
    if(code != LIBDAR_NOEXCEPT)
    {
	ui.printf("exception looking for children: %S\n", &msg);
	return;
    }
    if(ret)
	ui.printf("found children\n");
    else
	ui.printf("no found children\n");

    ret = get_children_of_noexcept(ui, arch, "", code, msg);
    if(code != LIBDAR_NOEXCEPT)
    {
	ui.printf("exception looking for children of root: %S\n", &msg);
	return;
    }
    if(ret)
	ui.printf("found children\n");
    else
	ui.printf("no found children\n");


    close_archive_noexcept(arch, code, msg);
    if(code != LIBDAR_NOEXCEPT)
    {
	ui.printf("exception closing: %S\n", &msg);
	return;
    }
}

void f3()
{
    archive_options_read read_options;

	// need to create an archive named "titi" with file recorded as removed since reference backup
    U_16 code;
    string msg;
    read_options.clear();
    read_options.set_info_details(true);
    archive *arch = open_archive_noexcept(ui, ".", "toto", "dar", read_options, code, msg);
    if(code != LIBDAR_NOEXCEPT)
    {
	ui.printf("exception openning archive: %S\n", &msg);
	return;
    }

    ui.set_listing_callback(&listing);
    bool ret = get_children_of_noexcept(ui, arch, "etc/rc.d", code, msg);
    if(code != LIBDAR_NOEXCEPT)
    {
	ui.printf("exception looking for children: %S\n", &msg);
	return;
    }
    if(ret)
	ui.printf("found children\n");
    else
	ui.printf("no found children\n");

    arch->init_catalogue(ui);
    vector<list_entry> contents = arch->get_children_in_table("etc");
    vector<list_entry>::iterator it = contents.begin();
    while(it != contents.end())
    {
	string line = it->get_name() + " " + (it->has_data_present_in_the_archive() ? "SAVED" : "not saved") + "\n";
	cout << line;
	++it;
    }

    close_archive_noexcept(arch, code, msg);
    if(code != LIBDAR_NOEXCEPT)
    {
	ui.printf("exception closing: %S\n", &msg);
	return;
    }


}

void f4()
{
#if MUTEX_WORKS
    try
    {
	pthread_t tid = pthread_self();
	pthread_t tod = 0;
	bool ret = cancel_status(tod);
	cancel_clear(tid);
	cancel_thread(tid);
 	ret = cancel_status(tod);
	cancel_clear(tod);
	ret = cancel_status(tod);
	null_file fake = null_file(gf_read_write);
	fake.write("coucouc les amsi", 10);
	cancel_thread(tid);
	fake.write("coucouc les amsi", 10);
	ui.printf("this statement should never be reached\n");
	ret = ret+1; // avoid warning of unused variable
    }
    catch(Egeneric & e)
    {
	cout << "Exception caught: " << e.get_message() << endl;
    }
    catch(...)
    {
	ui.printf("unknown expcetion caught\n");
    }
#endif
}

void f5()
{
    string ret;

    ret = ui.get_string("Mot de passe svp :", false);
    cout << "---[" << ret << "]---" << endl;
    ret = ui.get_string("Mot de passe svp :", true);
    cout << "---[" << ret << "]---" << endl;
}
