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
// $Id: test_libdar.cpp,v 1.4.2.1 2003/12/20 23:05:35 edrusb Rel $
//
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
#if HAVE_STDIO_H
#include <stdio.h>
#endif
} // end extern "C"

#include "libdar.hpp"
#include "tools.hpp"

using namespace std;
using namespace libdar;

void f1();
void warning(const string &x);
bool question(const string & x);
void f2();
void f3();

int main()
{
    f1();
    f2();
    f3();
}

#define BOOL2STR(val) ( val ? "yes" : "no" )

void f1()
{
    U_I maj, min;
    bool ea, large, nodump, special;
    U_I bits;

    get_version(maj, min);
    printf("version %u.%u\n", maj, min);
    get_compile_time_features(ea, large, nodump, special, bits);
    printf("features:\nEA = %s\nLARGE = %s\nNODUMP = %s\nSPECIAL = %s\nbits = %u\n",
	   BOOL2STR(ea),
	   BOOL2STR(large),
	   BOOL2STR(nodump),
	   BOOL2STR(special),
	   bits);
    set_warning_callback(&warning);
    set_answer_callback(&question);
}

void warning(const string &x)
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
	    printf("%s\n", ptr);
	}
	catch(...)
	{
	    delete ptr;
	    throw;
	}
	delete ptr;
    }
}

bool question(const string & x)
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

	    printf("%s\n", ptr);
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

void listing(const std::string & flag,
	     const std::string & perm,
	     const std::string & uid,
	     const std::string & gid,
	     const std::string & size,
	     const std::string & date,
	     const std::string & filename)
{
    ui_printf("[%S][%S][%S][%S][%S][%S][%S]\n", &flag, &perm, &uid, &gid, &size, &date, &filename);
}

void f2()
{
    U_16 code;
    string msg;
    op_create_noexcept("/",
		       ".",
		       NULL,
		       bool_mask(true),
		       simple_path_mask("/etc"),
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
		       bool_mask(false),
		       0,
		       false,
		       false,
		       0,
		       false,
		       code,
		       msg);
    if(code != LIBDAR_NOEXCEPT && code != LIBDAR_EUSER_ABORT)
    {
	ui_printf("exception creating archive: %S\n", &msg);
	return;
    }

    archive *arch = open_archive_noexcept(".", "toto", "dar", crypto_none, "", "", "", "", true, code, msg);
    if(code != LIBDAR_NOEXCEPT)
    {
	ui_printf("exception openning archive: %S\n", &msg);
	return;
    }

    set_op_tar_listing_callback(&listing);
    bool ret = get_children_of_noexcept(arch, "etc/rc.d", code, msg);
    if(code != LIBDAR_NOEXCEPT)
    {
	ui_printf("exception looking for children: %S\n", &msg);
	return;
    }
    if(ret)
	printf("found children\n");
    else
	printf("no found children\n");

    ret = get_children_of_noexcept(arch, "", code, msg);
    if(code != LIBDAR_NOEXCEPT)
    {
	ui_printf("exception looking for children of root\n", &msg);
	return;
    }
    if(ret)
	printf("found children\n");
    else
	printf("no found children\n");


    close_archive_noexcept(arch, code, msg);
    if(code != LIBDAR_NOEXCEPT)
    {
	ui_printf("exception closing: %S\n", &msg);
	return;
    }
}

void f3()
{
	// need to create an archive named "titi" with file recorded as removed since reference backup
    U_16 code;
    string msg;
    archive *arch = open_archive_noexcept(".", "titi", "dar", crypto_none, "", "", "", "", true, code, msg);
    if(code != LIBDAR_NOEXCEPT)
    {
	ui_printf("exception openning archive: %S\n", &msg);
	return;
    }

    set_op_tar_listing_callback(&listing);
    bool ret = get_children_of_noexcept(arch, "etc/rc.d", code, msg);
    if(code != LIBDAR_NOEXCEPT)
    {
	ui_printf("exception looking for children: %S\n", &msg);
	return;
    }
    if(ret)
	printf("found children\n");
    else
	printf("no found children\n");

    close_archive_noexcept(arch, code, msg);
    if(code != LIBDAR_NOEXCEPT)
    {
	ui_printf("exception closing: %S\n", &msg);
	return;
    }
}
