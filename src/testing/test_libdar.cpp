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
// to contact the author : dar.linux@free.fr
/*********************************************************************/
// $Id: test_libdar.cpp,v 1.22 2004/11/12 21:58:18 edrusb Rel $
//
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
void f2();
void f3();
void f4();
void f5();

static user_interaction_callback ui = user_interaction_callback(warning, question, getstring, (void *)1000);

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
    bool ea, large, nodump, special, thread, libz, libbz2, libcrypto;
    U_I bits;

    get_version(maj, med, min);
    printf("version %u.%u.%u\n", maj, med, min);
    get_compile_time_features(ea, large, nodump, special, bits, thread, libz, libbz2, libcrypto);
    printf("features:\nEA = %s\nLARGE = %s\nNODUMP = %s\nSPECIAL = %s\nbits = %u\nlibz =%s\nlibbz2 = %s\nlibcrypto = %s\n",
	   BOOL2STR(ea),
	   BOOL2STR(large),
	   BOOL2STR(nodump),
	   BOOL2STR(special),
	   bits,
	   BOOL2STR(libz),
	   BOOL2STR(libbz2),
	   BOOL2STR(libcrypto));
}

void warning(const string &x, void *context)
{
    U_16 code;
    string msg;
    char *ptr = libdar_str2charptr_noexcept(x, code, msg);
    if(code != LIBDAR_NOEXCEPT)
    {
	printf("cannot convert message to char*, cannot display it\n");
	if(ptr != NULL)
	    throw SRC_BUG;
    }
    else
    {
	try
	{
	    printf("[%d]%s\n", (U_I)context, ptr);
	}
	catch(...)
	{
	    delete ptr;
	    throw;
	}
	delete ptr;
    }
}

bool question(const string & x, void *context)
{
    U_16 code;
    string msg;
    bool rep = false;
    char *ptr = libdar_str2charptr_noexcept(x, code, msg);
    if(code != LIBDAR_NOEXCEPT)
    {
	printf("cannot convert message to char*, cannot display it\n");
	if(ptr != NULL)
	    throw SRC_BUG;
    }
    else
    {
	try
	{
	    char r;

	    printf("[%d]%s\n", (U_I)context, ptr);
	    scanf("%c", &r);
	    rep = r == 'y';
	}
	catch(...)
	{
	    delete ptr;
	    throw;
	}
	delete ptr;
    }

    return rep;
}

string getstring(const string &x, bool echo, void *context)
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
    ui.printf("[[%d]][%S][%S][%S][%S][%S][%S][%S][%s][%s]\n", (U_I)context, &flag, &perm, &uid, &gid, &size, &date, &filename, is_dir ? "dir" : "not_dir", has_children ? "has children" : "no children");
}

void f2()
{
    U_16 code;
    string msg;
    statistics st;
    archive *toto = create_archive_noexcept(ui,
					    "/",
					    ".",
					    NULL,
					    bool_mask(true),
					    simple_path_mask("/etc", true),
					    "toto",
					    "dar",
					    true,
					    true,
					    true,
					    false, // no pause
					    true,
					    none,
					    1,
					    0,
					    0,
					    false,
					    false,
					    "",
					    crypto_none,
					    "",
					    0,
					    bool_mask(false),
					    0,
					    false,
					    false,
					    0,
					    false,
					    false,
					    false,
					    st,
					    code,
					    msg);
    if(code != LIBDAR_NOEXCEPT && code != LIBDAR_EUSER_ABORT)
    {
	ui.printf("exception creating archive: %S\n", &msg);
	return;
    }
    if(toto != NULL)
    {
	op_listing_noexcept(ui, toto, true, false, bool_mask(true), false, code, msg);
	if(code != LIBDAR_NOEXCEPT && code != LIBDAR_EUSER_ABORT)
	{
	    ui.printf("exception creating archive: %S\n", &msg);
	    return;
	}
	close_archive_noexcept(toto, code, msg);
    }

    archive *arch = open_archive_noexcept(ui, ".", "toto", "dar", crypto_none, "", 0, "", "", "", true, code, msg);
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
	printf("found children\n");
    else
	printf("no found children\n");

    ret = get_children_of_noexcept(ui, arch, "", code, msg);
    if(code != LIBDAR_NOEXCEPT)
    {
	ui.printf("exception looking for children of root\n", &msg);
	return;
    }
    if(ret)
	printf("found children\n");
    else
	printf("no found children\n");


    close_archive_noexcept(arch, code, msg);
    if(code != LIBDAR_NOEXCEPT)
    {
	ui.printf("exception closing: %S\n", &msg);
	return;
    }
}

void f3()
{
	// need to create an archive named "titi" with file recorded as removed since reference backup
    U_16 code;
    string msg;
    archive *arch = open_archive_noexcept(ui, ".", "titi", "dar", crypto_none, "", 0, "", "", "", true, code, msg);
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
	printf("found children\n");
    else
	printf("no found children\n");

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
	pthread_t tod;
	bool ret = cancel_current(tod);
	cancel_clear();
	cancel_thread(tid);
	ret = cancel_current(tod);
	cancel_clear();
	ret = cancel_current(tod);
	null_file fake = null_file(ui, gf_read_write);
	fake.write("coucouc les amsi", 10);
	cancel_thread(tid);
	fake.write("coucouc les amsi", 10);
	ui.printf("this statement should never be reached\n");
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
    user_interaction *dialog = shell_interaction_init(&cout, &cerr, false);

    try
    {
	string ret;

	ret = dialog->get_string("Mot de passe svp :", false);
	cout << "---[" << ret << "]---" << endl;
	ret = dialog->get_string("Mot de passe svp :", true);
	cout << "---[" << ret << "]---" << endl;

    }
    catch(...)
    {
	delete dialog;
	shell_interaction_close();
    }
}
