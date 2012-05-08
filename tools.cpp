/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002 Denis Corbin
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
// $Id: tools.cpp,v 1.13 2002/12/08 20:03:07 edrusb Rel $
//
/*********************************************************************/

#include <iostream>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <signal.h>
#include <stdlib.h>
#include "tools.hpp"
#include "erreurs.hpp"
#include "deci.hpp"
#include "user_interaction.hpp"
#include "dar_suite.hpp"
#include "integers.hpp"

static void runson(char *argv[]);
static void deadson(S_I sig);

char *tools_str2charptr(string x)
{
    U_I size = x.size();
    char *ret = new char[size+1];

    if(ret == NULL)
	throw Ememory("tools_str2charptr");
    memcpy(ret, x.c_str(), size);
    ret[size] = '\0';

    return ret;
}

void tools_write_string(generic_file & f, const string & s)
{
    tools_write_string_all(f, s);
    f.write("", 1); // adding a '\0' at the end;
}

void tools_read_string(generic_file & f, string & s)
{
    char a[2] = { 0, 0 };
    S_I lu;

    s = "";
    do
    {
	lu = f.read(a, 1);
	if(lu == 1  && a[0] != '\0')
	    s += a;
    }
    while(lu == 1 && a[0] != '\0');

    if(lu != 1 || a[0] != '\0')
	throw Erange("tools_read_string", "not a zero terminated string in file");
}

void tools_write_string_all(generic_file & f, const string & s)
{
    char *tmp = tools_str2charptr(s);

    if(tmp == NULL)
	throw Ememory("tools_write_string_all");
    try
    {
	U_I len = s.size();
	U_I wrote = 0;

	while(wrote < len)
	    wrote += f.write(tmp+wrote, len-wrote);
    }
    catch(...)
    {
	delete tmp;
    }
    delete tmp;
}

void tools_read_string_size(generic_file & f, string & s, infinint taille)
{
    U_16 small_read = 0;
    size_t max_read = 0;
    S_I lu = 0;
    const U_I buf_size = 10240;
    char buffer[buf_size];

    s = "";
    do
    {
	if(small_read > 0)
	{
	    max_read = small_read > buf_size ? buf_size : small_read;
	    lu = f.read(buffer, max_read);
	    small_read -= lu;
	    s += string((char *)buffer, (char *)buffer+lu);
	}
	taille.unstack(small_read);
    }
    while(small_read > 0);
}

infinint tools_get_filesize(const path &p)
{
    struct stat buf;
    char *name = tools_str2charptr(p.display());

    if(name == NULL)
	throw Ememory("tools_get_filesize");

    try
    {
	if(lstat(name, &buf) < 0)
	    throw Erange("tools_get_filesize", strerror(errno));
    }
    catch(...)
    {
	delete name;
    }

    delete name;
    return (U_32)buf.st_size;
}

infinint tools_get_extended_size(string s)
{
    U_I len = s.size();
    infinint factor = 1;

    if(len < 1)
	return false;
    switch(s[len-1])
    {
    case 'K':
    case 'k': // kilobyte
	factor = 1024;
	break;
    case 'M': // megabyte
	factor = infinint(1024)*infinint(1024);
	break;
    case 'G': // gigabyte
	factor = infinint(1024)*infinint(1024)*infinint(1024);
	break;
    case 'T': // terabyte
	factor = infinint(1024)*infinint(1024)*infinint(1024)*infinint(1024);
	break;
    case 'P': // petabyte
	factor = infinint(1024)*infinint(1024)*infinint(1024)*infinint(1024)*infinint(1024);
	break;
    case 'E': // exabyte
	factor = infinint(1024)*infinint(1024)*infinint(1024)*infinint(1024)*infinint(1024)*infinint(1024);
	break;
    case 'Z': // zettabyte
	factor = infinint(1024)*infinint(1024)*infinint(1024)*infinint(1024)*infinint(1024)*infinint(1024)*infinint(1024);
	break;
    case 'Y':  // yottabyte
	factor = infinint(1024)*infinint(1024)*infinint(1024)*infinint(1024)*infinint(1024)*infinint(1024)*infinint(1024)*infinint(1024);
	break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
	break;
    default :
	throw Erange("command_line get_extended_size", string("unknown sufix in string : ")+s);
    }

    if(factor != 1)
	s = string(s.begin(), s.end()-1);

    deci tmp = s;
    factor *= tmp.computer();

    return factor;
}

char *tools_extract_basename(const char *command_name)
{
    path commande = command_name;
    string tmp = commande.basename();
    char *name = tools_str2charptr(tmp);

    return name;
}


static void dummy_call(char *x)
{
    static char id[]="$Id: tools.cpp,v 1.13 2002/12/08 20:03:07 edrusb Rel $";
    dummy_call(id);
}

void tools_split_path_basename(const char *all, path * &chemin, string & base)
{
    chemin = new path(all);
    if(chemin == NULL)
	throw Ememory("tools_split_path_basename");

    try
    {
	if(chemin->degre() > 1)
	{
	    if(!chemin->pop(base))
		throw SRC_BUG; // a path of degree > 1 should be able to pop()
	}
	else
	{
	    delete chemin;
	    base = all;
	    chemin = new path(".");
	    if(chemin == NULL)
		throw Ememory("tools_split_path_basename");
	}
    }
    catch(...)
    {
	if(chemin != NULL)
	    delete chemin;
	throw;
    }
}

void tools_split_path_basename(const string & all, string & chemin, string & base)
{
    path tmp = all;

    if(!tmp.pop(base))
    {
	chemin = "";
	base = all;
    }
    else
	chemin = tmp.display();
}

void tools_open_pipes(const string &input, const string & output, tuyau *&in, tuyau *&out)
{
    in = out = NULL;
    try
    {
	if(input != "")
	    in = new tuyau(input, gf_read_only);
	else
	    in = new tuyau(0, gf_read_only); // stdin by default
	if(in == NULL)
	    throw Ememory("tools_open_pipes");

	if(output != "")
	    out = new tuyau(output, gf_write_only);
	else
	    out = new tuyau(1, gf_write_only); // stdout by default
	if(out == NULL)
	    throw Ememory("tools_open_pipes");

    }
    catch(...)
    {
	if(in != NULL)
	    delete in;
	if(out != NULL)
	    delete out;
	throw;
    }
}

void tools_blocking_read(S_I fd, bool mode)
{
    S_I flags = fcntl(fd, F_GETFL, 0);
    if(!mode)
	flags |= O_NDELAY;
    else
	flags &= ~O_NDELAY;
    fcntl(fd, F_SETFL, flags);
}

string tools_name_of_uid(U_16 uid)
{
    struct passwd *pwd = getpwuid(uid);

    if(pwd == NULL) // uid not associated with a name
    {
	infinint tmp = uid;
	deci d = tmp;
	return d.human();
    }
    else
	return pwd->pw_name;
}

string tools_name_of_gid(U_16 gid)
{
    struct group *gr = getgrgid(gid);

    if(gr == NULL) // uid not associated with a name
    {
	infinint tmp = gid;
	deci d = tmp;
	return d.human();
    }
    else
	return gr->gr_name;
}

string tools_int2str(S_I x)
{
    infinint tmp = x >= 0 ? x : -x;
    deci d = tmp;

    return (x >= 0 ? string("") : string("-")) + d.human();
}

U_32 tools_str2int(const string & x)
{
    deci d = x;
    infinint t = d.computer();
    U_32 ret = 0;

    t.unstack(ret);
    if(t != 0)
	throw Erange("tools_str2int", "cannot convert the string to integer, overflow");

    return ret;
}

string tools_addspacebefore(string s, U_I expected_size)
{
    while(s.size() < expected_size)
	s = string(" ") + s;

    return s;
}

string tools_display_date(infinint date)
{
    time_t pas = 0;
    string ret;

    date.unstack(pas);
    ret = ctime(&pas);

    return string(ret.begin(), ret.end() - 1); // -1 to remove the ending '\n'
}

void tools_system(const vector<string> & argvector)
{
    if(argvector.size() == 0)
	return; // nothing to do
    else
    {
	char **argv = new char *[argvector.size()+1];

	if(argv == NULL)
	    throw Ememory("tools_system");
	try
	{
		// make an (S_I, char *[]) couple
	    for(register U_I i = 0; i <= argvector.size(); i++)
		argv[i] = NULL;

	    try
	    {
		S_I status;
		bool loop;

		for(register U_I i = 0; i < argvector.size(); i++)
		    argv[i] = tools_str2charptr(argvector[i]);

		do
		{
		    deadson(0);
		    loop = false;
		    S_I pid = fork();

		    switch(pid)
		    {
		    case -1:
			throw Erange("tools_system", string("Error while calling fork() to launch dar: ") + strerror(errno));
		    case 0: // fork has succeeded, we are the child process
			runson(argv);
			    // function that never returns
		    default:
			if(wait(&status) <= 0)
			    throw Erange("tools_system",
					 string("Unexpected error while waiting for dar to terminate: ") + strerror(errno));
			else // checking the way dar has exit
			    if(!WIFEXITED(status)) // not a normal ending
			    {
				if(WIFSIGNALED(status)) // exited because of a signal
				{
				    try
				    {
					user_interaction_pause(string("DAR terminated upon signal reception: ")
#ifdef USE_SYS_SIGLIST
							       + (WTERMSIG(status) < NSIG ? sys_siglist[WTERMSIG(status)] : tools_int2str(WTERMSIG(status)))
#else
							       + tools_int2str(WTERMSIG(status))
#endif
							       + " . Retry to launch dar as previously ?");
					loop = true;
				    }
				    catch(Euser_abort & e)
				    {
					user_interaction_pause("Continue anyway ?");
				    }
				}
				else // normal terminaison but exit code not zero
				    user_interaction_pause(string("DAR has terminated with exit code ")
							   + tools_int2str(WEXITSTATUS(status))
							   + " Continue anyway ?");
			    }
		    }
		}
		while(loop);
	    }
	    catch(...)
	    {
		for(register U_I i = 0; i < argvector.size(); i++)
		    if(argv[i] != NULL)
			delete argv[i];
		throw;
	    }
	    for(register U_I i = 0; i < argvector.size(); i++)
		if(argv[i] != NULL)
		    delete argv[i];
	}
	catch(...)
	{
	    delete argv;
	    throw;
	}
	delete argv;
    }
}

void tools_write_vector(generic_file & f, const vector<string> & x)
{
    infinint tmp = x.size();
    vector<string>::iterator it = const_cast<vector<string> *>(&x)->begin();
    vector<string>::iterator fin = const_cast<vector<string> *>(&x)->end();

    tmp.dump(f);
    while(it != fin)
	tools_write_string(f, *it++);
}

void tools_read_vector(generic_file & f, vector<string> & x)
{
    infinint tmp;
    string elem;

    x.clear();
    tmp.read_from_file(f);
    while(tmp > 0)
    {
	tools_read_string(f, elem);
	x.push_back(elem);
	tmp--;
    }
}

string tools_concat_vector(const string & separator, const vector<string> & x)
{
    string ret = separator;
    vector<string>::iterator it = const_cast<vector<string> *>(&x)->begin();
    vector<string>::iterator fin = const_cast<vector<string> *>(&x)->end();

    while(it != fin)
	ret += *it++ + separator;

    return ret;
}

vector<string> operator + (vector<string> a, vector<string> b)
{
    vector<string>::iterator it = b.begin();

    while(it != b.end())
	a.push_back(*it++);

    return a;
}


static void deadson(S_I sig)
{
    signal(SIGCHLD, &deadson);
}

static void runson(char *argv[])
{
    if(execvp(argv[0], argv) < 0)
	user_interaction_warning(string("Error while calling execvp:") + strerror(errno));
    else
	user_interaction_warning("execvp failed but did not returned error code");
    exit(EXIT_SYNTAX);
}
