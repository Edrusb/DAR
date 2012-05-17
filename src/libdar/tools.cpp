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
// $Id: tools.cpp,v 1.54.2.16 2008/05/09 20:58:27 edrusb Rel $
//
/*********************************************************************/


#include "../my_config.h"

extern "C"
{
#if STDC_HEADERS
# include <string.h>
#else
# if !HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
    char *strchr (), *strrchr ();
# if !HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifndef WEXITSTATUS
# define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif
#ifndef WIFEXITED
# define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif
#ifndef WIFSTOPPED
#define WIFSTOPPED(status)    (((status) & 0xff) == 0x7f)
#endif
#ifndef WIFSIGNALED
# define WIFSIGNALED(status)  (!WIFSTOPPED(status) && !WIFEXITED(status))
#endif
#ifndef WTERMSIG
#define WTERMSIG(status)      ((status) & 0x7f)
#endif

#if HAVE_ERRNO_H
#include <errno.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_SIGNAL_H
#include <signal.h>
#endif

#if HAVE_SYS_TYPE_H
#include <sys/types.h>
#endif

#if HAVE_UTIME_H
#include <utime.h>
#endif

#if HAVE_CTYPE_H
#include <ctype.h>
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
} // end extern "C"

#include <iostream>
#include "tools.hpp"
#include "erreurs.hpp"
#include "deci.hpp"
#include "user_interaction.hpp"
#include "integers.hpp"
#include "mask.hpp"
#include "etage.hpp"
#include "elastic.hpp"
#ifdef __DYNAMIC__
#include "user_group_bases.hpp"
#endif

using namespace std;

namespace libdar
{

#ifdef __DYNAMIC__
	// Yes, this is a static variable,
	// it contains the necessary mutex to keep libdar thread-safe
    static const user_group_bases user_group;
#endif

    static void runson(user_interaction & dialog, char * const argv[]);
    static void deadson(S_I sig);
    static bool is_a_slice_available(user_interaction & ui, const string & base, const string & extension);
    static string retreive_basename(const string & base, const string & extension);
    static string tools_make_word(generic_file &fic, off_t start, off_t end);

    char *tools_str2charptr(const string &x)
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
            throw Erange("tools_read_string", gettext("Not a zero terminated string in file"));
    }

    void tools_write_string_all(generic_file & f, const string & s)
    {
        const char *tmp = s.c_str();

	U_I len = s.size();
	U_I wrote = 0;

	while(wrote < len)
	    wrote += f.write(tmp+wrote, len-wrote);
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

	if(lstat(p.display().c_str(), &buf) < 0)
	    throw Erange("tools_get_filesize", tools_printf(gettext("Cannot get file size: %s"), strerror(errno)));

	return (U_32)buf.st_size;
    }

    infinint tools_get_extended_size(string s, U_I base)
    {
        U_I len = s.size();
        infinint factor = 1;

        if(len < 1)
            return false;
        switch(s[len-1])
        {
        case 'K':
        case 'k': // kilobyte
            factor = base;
            break;
        case 'M': // megabyte
            factor = infinint(base).power((U_I)2);
            break;
        case 'G': // gigabyte
            factor = infinint(base).power((U_I)3);
            break;
        case 'T': // terabyte
            factor = infinint(base).power((U_I)4);
            break;
        case 'P': // petabyte
            factor = infinint(base).power((U_I)5);
            break;
        case 'E': // exabyte
            factor = infinint(base).power((U_I)6);
            break;
        case 'Z': // zettabyte
            factor = infinint(base).power((U_I)7);
            break;
        case 'Y':  // yottabyte
            factor = infinint(base).power((U_I)8);
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
            throw Erange("command_line get_extended_size", tools_printf(gettext("Unknown suffix [%c] in string %S"), s[len-1], &s));
        }

        if(factor != 1)
            s = string(s.begin(), s.end()-1);

        deci tmp = s;
        factor *= tmp.computer();

        return factor;
    }

    void tools_extract_basename(const char *command_name, string &basename)
    {
        basename = path(command_name).basename();
    }

    static void dummy_call(char *x)
    {
        static char id[]="$Id: tools.cpp,v 1.54.2.16 2008/05/09 20:58:27 edrusb Rel $";
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
                base = chemin->basename();
                delete chemin;
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
        path *tmp = NULL;

	tools_split_path_basename(all.c_str(), tmp, base);
	if(tmp == NULL)
	    throw SRC_BUG;
	chemin = tmp->display();
	delete tmp;
    }

    void tools_open_pipes(user_interaction & dialog, const string &input, const string & output, tuyau *&in, tuyau *&out)
    {
        in = out = NULL;
        try
        {
            if(input != "")
                in = new tuyau(dialog, input, gf_read_only);
            else
                in = new tuyau(dialog, 0, gf_read_only); // stdin by default
            if(in == NULL)
                throw Ememory("tools_open_pipes");

            if(output != "")
                out = new tuyau(dialog, output, gf_write_only);
            else
                out = new tuyau(dialog, 1, gf_write_only); // stdout by default
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
	if(flags < 0)
	    throw Erange("tools_blocking_read", string(gettext("Cannot read fcntl file's flags : "))+strerror(errno));
        if(!mode)
            flags |= O_NONBLOCK;
        else
            flags &= ~O_NONBLOCK;
        if(fcntl(fd, F_SETFL, flags) < 0)
	    throw Erange("tools_blocking_read", string(gettext("Cannot set fcntl file's flags : "))+strerror(errno));
    }

    string tools_name_of_uid(U_16 uid)
    {
#ifndef  __DYNAMIC__
	string name = "";
#else
        string name = user_group.get_username(uid);
#endif

        if(name.empty()) // uid not associated with a name
        {
            infinint tmp = uid;
            deci d = tmp;
            return d.human();
        }
        else
            return name;
    }

    string tools_name_of_gid(U_16 gid)
    {
#ifndef __DYNAMIC__
	string name = "";
#else
        string name = user_group.get_groupname(gid);
#endif

        if(name.empty()) // uid not associated with a name
        {
            infinint tmp = gid;
            deci d = tmp;
            return d.human();
        }
        else
            return name;
    }

    string tools_uword2str(U_16 x)
    {
    	infinint tmp = x;
    	deci d = tmp;
    	return d.human();
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
            throw Erange("tools_str2int", gettext("Cannot convert the string to integer, overflow"));

        return ret;
    }

    string tools_addspacebefore(string s, U_I expected_size)
    {
    	return string(expected_size - s.size(), ' ') + s;
    }

    string tools_display_date(infinint date)
    {
        time_t pas = 0;
        string ret;

        date.unstack(pas);
        ret = ctime(&pas);

        return string(ret.begin(), ret.end() - 1); // -1 to remove the ending '\n'
    }

    infinint tools_convert_date(const string & repres)
    {
	enum status { init, year, month, day, hour, min, sec, error, finish };

	    /// first we define a helper class
	class scan
	{
	public:
	    scan(const tm & now)
	    {
		etat = init;
		when = now;
		when.tm_sec = when.tm_min = when.tm_hour = 0;
		when.tm_wday = 0;            // ignored by mktime
		when.tm_yday = 0;            // ignored by mktime
		tmp = 0;
	    };

	    status get_etat() const { return etat; };
	    tm get_struct() const { return when; };
	    void add_digit(char a)
	    {
		if(a < 48 && a > 57) // ascii code for zero is 48, for nine is 57
		    throw SRC_BUG;
		tmp = tmp*10 + (a-48);
	    };

	    void set_etat(const status & val)
	    {
		switch(etat)
		{
		case year:
		    if(tmp < 1970)
			throw Erange("tools_convert_date", gettext("date before 1970 is not allowed"));
		    when.tm_year = tmp - 1900;
		    break;
		case month:
		    if(tmp < 1 || tmp > 12)
			throw Erange("tools_convert_date", gettext("Incorrect month"));
		    when.tm_mon = tmp - 1;
		    break;
		case day:
		    if(tmp < 1 || tmp > 31)
			throw Erange("tools_convert_date", gettext("Incorrect day of month"));
		    when.tm_mday = tmp;
		    break;
		case hour:
		    if(tmp < 0 || tmp > 23)
			throw Erange("tools_convert_date", gettext("Incorrect hour of day"));
		    when.tm_hour = tmp;
		    break;
		case min:
		    if(tmp < 0 || tmp > 59)
			throw Erange("tools_convert_date", gettext("Incorrect minute"));
		    when.tm_min = tmp;
		    break;
		case sec:
		    if(tmp < 0 || tmp > 59)
			throw Erange("tools_convert_date", gettext("Incorrect second"));
		    when.tm_sec = tmp;
		    break;
		case error:
		    throw Erange("tools_convert_date", gettext("Bad formatted date expression"));
		default:
		    break; // nothing to do
		}
		tmp = 0;
		etat = val;
	    };

	private:
	    struct tm when;
	    status etat;
	    S_I tmp;
	};

	    // then we define local variables
	time_t now = time(NULL), when;
	scan scanner = scan(*(localtime(&now)));
	U_I c, size = repres.size(), ret;
	struct tm tmp;

	    // now we parse the string to update the stucture tm "when"

	    // first, determining initial state
	switch(tools_count_in_string(repres, '/'))
	{
	case 0:
	    switch(tools_count_in_string(repres, '-'))
	    {
	    case 0:
		scanner.set_etat(hour);
		break;
	    case 1:
		scanner.set_etat(day);
		break;
	    default:
		scanner.set_etat(error);
	    }
	    break;
	case 1:
	    scanner.set_etat(month);
	    break;
	case 2:
	    scanner.set_etat(year);
	    break;
	default:
	    scanner.set_etat(error);
	}

	    // second, parsing the string
	for(c = 0; c < size && scanner.get_etat() != error; ++c)
	    switch(repres[c])
	    {
	    case '/':
		switch(scanner.get_etat())
		{
		case year:
		    scanner.set_etat(month);
		    break;
		case month:
		    scanner.set_etat(day);
		    break;
		default:
		    scanner.set_etat(error);
		}
		break;
	    case ':':
		switch(scanner.get_etat())
		{
		case hour:
		    scanner.set_etat(min);
		    break;
		case min:
		    scanner.set_etat(sec);
		    break;
		default:
		    scanner.set_etat(error);
		}
		break;
	    case '-':
		switch(scanner.get_etat())
		{
		case day:
		    scanner.set_etat(hour);
		    break;
		default:
		    scanner.set_etat(error);
		}
		break;
	    case ' ':
	    case '\t':
	    case '\n':
	    case '\r':
		break; // we ignore spaces, tabs, CR and LF
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
		scanner.add_digit(repres[c]);
		break;
	    default:
		scanner.set_etat(error);
	    }

	scanner.set_etat(finish);
	tmp = scanner.get_struct();
	when = mktime(&tmp);
	if(when > now)
	    throw Erange("tools_convert_date", gettext("Given date must be in the past"));
	ret = when;

	return ret;
    }

    void tools_system(user_interaction & dialog, const vector<string> & argvector)
    {
        if(argvector.empty())
            return; // nothing to do

	    // ISO C++ forbids variable-size array
	char **argv = new char * [argvector.size()+1];

	for(register U_I i = 0; i <= argvector.size(); i++)
	    argv[i] = NULL;

	try
	{
	    S_I status;
	    bool loop;

	    for(register U_I i = 0; i < argvector.size(); i++)
		argv[i] = tools_str2charptr(argvector[i]);
	    argv[argvector.size()] = NULL; // this is already done above but that does not hurt doing it twice :-)

	    do
	    {
		deadson(0);
		loop = false;
		S_I pid = fork();

		switch(pid)
		{
		case -1:
		    throw Erange("tools_system", string(gettext("Error while calling fork() to launch dar: ")) + strerror(errno));
		case 0: // fork has succeeded, we are the child process
		    try
		    {
			runson(dialog, argv); // function that never returns or throws exceptions
			throw SRC_BUG; // just in case the previous function returned
		    }
		    catch(...)
		    {
			throw SRC_BUG;
		    }
		default:
		    if(wait(&status) <= 0)
			throw Erange("tools_system",
				     string(gettext("Unexpected error while waiting for dar to terminate: ")) + strerror(errno));
		    else // checking the way dar has exit
			if(WIFSIGNALED(status)) // exited because of a signal
			    {
				try
				{
				    dialog.pause(string(gettext("DAR terminated upon signal reception: "))
#if HAVE_DECL_SYS_SIGLIST
						 + (WTERMSIG(status) < NSIG ? sys_siglist[WTERMSIG(status)] : tools_int2str(WTERMSIG(status)))
#else
						 + tools_int2str(WTERMSIG(status))
#endif
						 + gettext(" . Retry to launch dar as previously ?"));
				    loop = true;
				}
				catch(Euser_abort & e)
				{
				    dialog.pause(gettext(" Continue anyway ?"));
				}
 			    }
			else // normal terminaison checking exit status code
			    if(WEXITSTATUS(status) != 0)
				dialog.pause(string(gettext("DAR sub-process has terminated with exit code "))
					     + tools_int2str(WEXITSTATUS(status))
					     + gettext(" Continue anyway ?"));
		}
	    }
	    while(loop);
	}
	catch(...)
	{
	    for(register U_I i = 0; i <= argvector.size(); i++)
		if(argv[i] != NULL)
		    delete [] argv[i];
	    delete argv;
	    throw;
	}

	for(register U_I i = 0; i <= argvector.size(); i++)
	    if(argv[i] != NULL)
		delete [] argv[i];
	delete argv;
    }

    void tools_write_vector(generic_file & f, const vector<string> & x)
    {
        infinint tmp = x.size();
        vector<string>::const_iterator it = x.begin();

        tmp.dump(f);
        while(it != x.end())
            tools_write_string(f, *it++);
    }

    void tools_read_vector(generic_file & f, vector<string> & x)
    {
        infinint tmp = infinint(f.get_gf_ui(), NULL, &f);
        string elem;

        x.clear();
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
        vector<string>::const_iterator it = x.begin();

        while(it != x.end())
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



    const char *tools_get_from_env(const char **env, const char *clef)
    {
        unsigned int index = 0;
        const char *ret = NULL;

        if(env == NULL || clef == NULL)
            return NULL;

        while(ret == NULL && env[index] != NULL)
        {
            unsigned int letter = 0;
            while(clef[letter] != '\0'
                  && env[index][letter] != '\0'
                  && env[index][letter] != '='
                  && clef[letter] == env[index][letter])
                letter++;
            if(clef[letter] == '\0' && env[index][letter] == '=')
                ret = env[index]+letter+1;
            else
                index++;
        }

        return ret;
    }


    bool tools_is_member(const string & val, const vector<string> & liste)
    {
        U_I index = 0;

        while(index < liste.size() && liste[index] != val)
            index++;

        return index <liste.size();
    }

    void tools_display_features(user_interaction & dialog, bool ea, bool largefile, bool nodump,
                                bool special_alloc, U_I bits, bool thread_safe,
                                bool libz, bool libbz2, bool libcrypto, bool new_blowfish)
    {
#define YES_NO(x) (x ? gettext("YES") : gettext("NO"))

        dialog.printf(gettext("   Libz compression (gzip)    : %s\n"), YES_NO(libz));
        dialog.printf(gettext("   Libbz2 compression (bzip2) : %s\n"), YES_NO(libbz2));
        dialog.printf(gettext("   Strong encryption          : %s\n"), YES_NO(libcrypto));
	dialog.printf(gettext("   New Blowfish implementation: %s\n"), YES_NO(new_blowfish));
        dialog.printf(gettext("   Extended Attributes support: %s\n"), YES_NO(ea));
        dialog.printf(gettext("   Large files support (> 2GB): %s\n"), YES_NO(largefile));
        dialog.printf(gettext("   ext2fs NODUMP flag support : %s\n"), YES_NO(nodump));
        dialog.printf(gettext("   Special allocation scheme  : %s\n"), YES_NO(special_alloc));
        if(bits == 0)
            dialog.printf(gettext("   Integer size used          : unlimited\n"));
        else
            dialog.printf(gettext("   Integer size used          : %d bits\n"), bits);
        dialog.printf(gettext("   Thread safe support        : %s\n"), YES_NO(thread_safe));
    }

    bool is_equal_with_hourshift(const infinint & hourshift, const infinint & date1, const infinint & date2)
    {
        infinint delta = date1 > date2 ? date1-date2 : date2-date1;
        infinint num, rest;

            // delta = num*3600 + rest
            // with 0 <= rest < 3600
            // (this euclidian division)
        euclide(delta, 3600, num, rest);

        if(rest != 0)
            return false;
        else // rest == 0
            return num <= hourshift;
    }

    bool tools_my_atoi(char *a, U_I & val)
    {
        infinint tmp;
        bool ret = true;

        try
        {
            tmp = deci(a).computer();
            val = 0;
            tmp.unstack(val);
        }
        catch(Edeci & e)
        {
            ret = false;
        }

        return ret;
    }

    void tools_check_basename(user_interaction & dialog, const path & loc, string & base, const string & extension)
    {
        regular_mask suspect = regular_mask(string(".+\\.[1-9][0-9]*\\.")+extension, true);
        string old_path = (loc+base).display();

            // is basename is suspect ?
        if(!suspect.is_covered(base))
            return; // not a suspect basename

            // is there a slice available ?
        if(is_a_slice_available(dialog, old_path, extension))
            return; // yes, thus basename is not a mistake

            // removing the suspicious end (.<number>.extension)
            // and checking the avaibility of such a slice

        string new_base = retreive_basename(base, extension);
        string new_path = (loc+new_base).display();
        if(is_a_slice_available(dialog, new_path, extension))
        {
            try
            {
                dialog.pause(tools_printf(gettext("Warning, %S seems more to be a slice name than a base name. Do you want to replace it by %S ?"), &base, &new_base));
                base = new_base;
            }
            catch(Euser_abort & e)
            {
                dialog.warning(tools_printf(gettext("OK, keeping %S as basename"), &base));
            }
        }
    }

    string tools_getcwd()
    {
	size_t length = 10240;
	char *buffer = NULL, *ret;
	string cwd;
	try
	{
	    do
	    {
		buffer = new char[length];
		if(buffer == NULL)
		    throw Ememory("tools_getcwd()");
		ret = getcwd(buffer, length-1); // length-1 to keep a place for ending '\0'
		if(ret == NULL) // could not get the CWD
		{
		    if(errno == ERANGE) // buffer too small
		    {
			delete buffer;
			buffer = NULL;
			length *= 2;
		    }
		    else // other error
			throw Erange("tools_getcwd", string(gettext("Cannot get full path of current working directory: ")) + strerror(errno));
		}
	    }
	    while(ret == NULL);

	    buffer[length - 1] = '\0';
	    cwd = buffer;
	}
	catch(...)
	{
	    if(buffer != NULL)
		delete [] buffer;
	    throw;
	}
	if(buffer != NULL)
	    delete [] buffer;
	return cwd;
    }

    string tools_readlink(const char *root)
    {
	size_t length = 10240;
	char *buffer = NULL;
	S_I lu;
	string ret = "";

	if(root == NULL)
	    throw Erange("tools_readlink", gettext("NULL argument given to tools_readlink"));
	if(strcmp(root, "") == 0)
	    throw Erange("tools_readlink", gettext("Empty string given as argument to tools_readlink"));

	try
	{
	    do
	    {
		buffer = new char[length];
		if(buffer == NULL)
		    throw Ememory("tools_readlink");
		lu = readlink(root, buffer, length-1); // length-1 to have room to add '\0' at the end

		if(lu < 0) // error occured with readlink
		{
		    switch(errno)
		    {
		    case EINVAL: // not a symbolic link (thus we return the given argument)
			ret = root;
			break;
		    case ENAMETOOLONG: // too small buffer
			delete [] buffer;
			buffer = NULL;
			length *= 2;
			break;
		    default: // other error
			throw Erange("get_readlink", tools_printf(gettext("Cannot read file information for %s : %s"), root, strerror(errno)));
		    }
		}
		else // got the correct real path of symlink
		    if((U_I)(lu) < length)
		    {
			buffer[lu] = '\0';
			ret = buffer;
		    }
		    else // "lu" should not be greater than length: readlink system call error
		    {
			    // trying to workaround with a larger buffer
			delete [] buffer;
			buffer = NULL;
			length *= 2;
		    }
	    }
	    while(ret == "");
	}
	catch(...)
	{
	    if(buffer != NULL)
		delete [] buffer;
	    throw;
	}
	if(buffer != NULL)
	    delete [] buffer;
	return ret;
    }


    bool tools_look_for(const char *argument, S_I argc, char *argv[])
    {
        S_I count = 0;

        while(count < argc && strcmp(argv[count], argument) != 0)
            count++;

        return count < argc;
    }

    void tools_make_date(const string & chemin, infinint access, infinint modif)
    {
        struct utimbuf temps;
        time_t tmp = 0;

        access.unstack(tmp);
        temps.actime = tmp;
        tmp = 0;
        modif.unstack(tmp);
        temps.modtime = tmp;

	if(utime(chemin.c_str() , &temps) < 0)
	    Erange("tools_make_date", string(gettext("Cannot set last access and last modification time: ")) + strerror(errno));
    }

    void tools_noexcept_make_date(const string & chem, const infinint & last_acc, const infinint & last_mod)
    {
        try
        {
            if(last_acc != 0 || last_mod != 0)
                tools_make_date(chem, last_acc, last_mod);
                // else the directory could not be openned properly
                // and time could not be retrieved, so we don't try
                // to restore them
        }
        catch(Erange & e)
        {
                // cannot restore dates, ignoring
        }
    }

    bool tools_is_case_insensitive_equal(const string & a, const string & b)
    {
        U_I curs = 0;
        if(a.size() != b.size())
            return false;

        while(curs < a.size() && tolower(a[curs]) == tolower(b[curs]))
            curs++;

        return curs >= a.size();
    }

    void tools_to_upper(char *nts)
    {
        char *ptr = nts;

        if(ptr == NULL)
            throw Erange("tools_to_upper", gettext("NULL given as argument"));

        while(*ptr != '\0')
        {
            *ptr = (char)toupper((int)*ptr);
            ptr++;
        }
    }

    void tools_to_upper(string & r)
    {
        U_I taille = r.size();

        for(U_I x = 0; x < taille; ++x)
            r[x] = toupper(r[x]);
    }

    void tools_remove_last_char_if_equal_to(char c, string & s)
    {
        if(s[s.size()-1] == c)
            s = string(s.begin(), s.begin()+(s.size() - 1));
    }

    static void deadson(S_I sig)
    {
        signal(SIGCHLD, &deadson);
    }

    static void runson(user_interaction & dialog, char * const argv[])
    {
        if(execvp(argv[0], argv) < 0)
	    dialog.warning(string(gettext("Error while calling execvp:")) + strerror(errno));
        else
	    dialog.warning(string(gettext("execvp failed but did not returned error code")));
#ifndef EXIT_ERROR
#define EXIT_ERROR 2
	exit(EXIT_ERROR);
	    // we need the appropriate dar exit status
	    // but we are in libdar thus cannot include dar_suite.hpp header
	    // we thus copy
#undef EXIT_ERROR
#else
	exit(EXIT_ERROR);
#endif
    }

    static bool is_a_slice_available(user_interaction & ui, const string & base, const string & extension)
    {
        path *chem = NULL;
        bool ret = false;

        try
        {
            string rest;

            tools_split_path_basename(base.c_str(), chem, rest);

            try
            {
                etage contents = etage(ui, chem->display().c_str(), 0, 0, false);  // we don't care the dates here so we set them to zero
                regular_mask slice = regular_mask(rest + "\\.[1-9][0-9]*\\."+ extension, true);

                while(!ret && contents.read(rest))
                    ret = slice.is_covered(rest);
            }
            catch(Erange & e)
            {
                ret = false;
            }
        }
        catch(...)
        {
            if(chem != NULL)
                delete chem;
            throw;
        }
        if(chem != NULL)
            delete chem;

        return ret;
    }

    static string retreive_basename(const string & base, const string & extension)
    {
        string new_base = base;
        S_I index;

        if(new_base.size() < 2+1+extension.size())
            throw SRC_BUG; // must be a slice filename
        index = new_base.find_last_not_of(string(".")+extension);
        new_base = string(new_base.begin(), new_base.begin()+index);
        index = new_base.find_last_not_of("0123456789");
        new_base = string(new_base.begin(), new_base.begin()+index);

        return new_base;
    }

    void tools_read_range(const string & s, U_I & min, U_I & max)
    {
        string::const_iterator it = s.begin();

        while(it < s.end() && *it != '-')
            it++;

        if(it < s.end())
        {
            min = tools_str2int(string(s.begin(), it));
            max = tools_str2int(string(++it, s.end()));
        }
        else
            min = max = tools_str2int(s);
    }


    string tools_printf(const char *format, ...)
    {
        va_list ap;
        va_start(ap, format);
        string output = "";
        try
        {
            output = tools_vprintf(format, ap);
        }
        catch(...)
        {
            va_end(ap);
            throw;
        }
        va_end(ap);
        return output;
    }

    string tools_vprintf(const char *format, va_list ap)
    {
        bool end;
        U_32 taille = strlen(format)+1;
        char *copie;
        string output = "";

        U_I test;

        copie = new char[taille];
        if(copie == NULL)
            throw Ememory("tools_printf");
        try
        {
            char *ptr = copie, *start = copie;

            strcpy(copie, format);
            copie[taille-1] = '\0';

            do
            {
                while(*ptr != '%' && *ptr != '\0')
                    ++ptr;
                if(*ptr == '%')
                {
                    *ptr = '\0';
                    end = false;
                }
                else
                    end = true;
                output += start;
                if(!end)
                {
                    ++ptr;
                    switch(*ptr)
                    {
                    case '%':
                        output += "%";
                        break;
                    case 'd':
                        output += tools_int2str(va_arg(ap, S_I));
                        break;
                    case 'u':
                        test = va_arg(ap, U_I);
                        output += deci(test).human();
                        break;
                    case 's':
                        output += va_arg(ap, char *);
                        break;
                    case 'c':
                        output += static_cast<char>(va_arg(ap, S_I));
                        break;
                    case 'i':
                        output += deci(*(va_arg(ap, infinint *))).human();
                        break;
                    case 'S':
                        output += *(va_arg(ap, string *));
                        break;
                    default:
                        throw Efeature(tools_printf(gettext("%%%c is not implemented in tools_printf format argument"), *ptr));
                    }
                    ++ptr;
                    start = ptr;
                }
            }
            while(!end);
        }
        catch(...)
        {
            delete [] copie;
            throw;
        }
        delete [] copie;

        return output;
    }

    void tools_unlink_file_mask(user_interaction & dialog, const string & c_chemin, const string & file_mask, bool info_details)
    {
	simple_mask my_mask = simple_mask(file_mask, true);
	etage dir = etage(dialog, c_chemin.c_str(), 0, 0, false);
	path chemin = path(c_chemin);
	string entry;

	while(dir.read(entry))
	    if(my_mask.is_covered(entry))
	    {
		const string c_entry = (chemin + entry).display();
		if(info_details)
		    dialog.warning(tools_printf(gettext("Removing file %s"), c_entry.c_str()));
		if(unlink(c_entry.c_str()) != 0)
		    dialog.warning(tools_printf(gettext("ERROR removing file %s : %s"), c_entry.c_str(), strerror(errno)));
	    }
    }

    bool tools_do_some_files_match_mask(user_interaction & ui, const string & c_chemin, const string & file_mask)
    {
	simple_mask my_mask = simple_mask(file_mask, true);
	etage dir = etage(ui, c_chemin.c_str(), 0, 0, false);
	string entry;
	bool ret = false;

	while(!ret && dir.read(entry))
	    if(my_mask.is_covered(entry))
		ret = true;

	return ret;
    }

    void tools_avoid_slice_overwriting(user_interaction & dialog, const path & chemin, const string & file_mask, bool info_details, bool allow_overwriting, bool warn_overwriting, bool dry_run)
    {
	const string c_chemin = chemin.display();
	if(tools_do_some_files_match_mask(dialog, c_chemin, file_mask))
	{
	    if(!allow_overwriting)
		throw Erange("tools_avoid_slice_overwriting", tools_printf(gettext("Overwriting not allowed while a slice of a previous archive with the same basename has been found in the %s directory, Operation aborted"), c_chemin.c_str()));
	    else
	    {
		try
		{
		    if(warn_overwriting)
			dialog.pause(tools_printf(gettext("At least one slice of an old archive with the same basename remains in the directory %s , If you do not remove theses all first, you will have difficulty identifying the last slice of the archive you are about to create, because it may be hidden in between slices of this older archive. Do we remove the old archive's slices first ?"), c_chemin.c_str()));
		    if(!dry_run)
			tools_unlink_file_mask(dialog, c_chemin, file_mask, info_details);
		}
		catch(Euser_abort & e)
		{
			// nothing to do, just continue
		}
	    }
	}
    }

    void tools_add_elastic_buffer(generic_file & f, U_32 max_size)
    {
        elastic tic = (time(NULL) % (max_size - 1)) + 1; // range from 1 to max_size
        char *buffer = new char[max_size];

	if(buffer == NULL)
	    throw Ememory("tools_add_elastic_buffer");
	try
	{
	    tic.dump((unsigned char *)buffer, max_size);
	    f.write(buffer, tic.get_size());
	}
	catch(...)
	{
	    delete [] buffer;
	    throw;
	}
	delete [] buffer;
    }

    bool tools_are_on_same_filesystem(const string & file1, const string & file2)
    {
        dev_t id;
        struct stat sstat;

	if(stat(file1.c_str(), &sstat) < 0)
	    throw Erange("tools:tools_are_on_same_filesystem", string(gettext("Cannot get inode information for: ")) +  file1 + " : " + strerror(errno));
	id = sstat.st_dev;

	if(stat(file2.c_str(), &sstat) < 0)
	    throw Erange("tools:tools_are_on_same_filesystem", string(gettext("Cannot get inode information for: ")) +  file2 + " : " + strerror(errno));

        return id == sstat.st_dev;
    }

    path tools_relative2absolute_path(const path & src, const path & cwd)
    {
        if(src.is_relative())
            if(cwd.is_relative())
                throw Erange("tools_relative2absolute_path", gettext("Current Working Directory cannot be a relative path"));
            else
                return cwd + src;
        else
            return src;
    }

    void tools_block_all_signals(sigset_t &old_mask)
    {
	sigset_t all;

	sigfillset(&all);
	if(sigprocmask(SIG_BLOCK, &all, &old_mask) != 0)
	    throw Erange("thread_cancellation:block_all_signals", string(gettext("Cannot block signals: "))+strerror(errno));
    }

    void tools_set_back_blocked_signals(sigset_t old_mask)
    {
	if(sigprocmask(SIG_SETMASK, &old_mask, NULL))
	    throw Erange("thread_cancellation:block_all_signals", string(gettext("Cannot unblock signals: "))+strerror(errno));
    }

    U_I tools_count_in_string(const string & s, const char a)
    {
	U_I ret = 0, c, size = s.size();

	for(c = 0; c < size; ++c)
	    if(s[c] == a)
		++ret;
	return ret;
    }

    infinint tools_get_mtime(const std::string & s)
    {
        struct stat buf;

	if(lstat(s.c_str(), &buf) < 0)
	    throw Erange("tools_get_mtime", tools_printf(gettext("Cannot get mtime: %s"), strerror(errno)));

        return buf.st_mtime;
    }

    vector<string> tools_split_in_words(generic_file & f)
    {
	vector <string> mots;
	vector <char> quotes;
	char a;
	off_t start = 0;
	off_t end = 0;
	bool loop = true;


        while(loop)
        {
            if(f.read(&a, 1) != 1) // reached end of file
            {
                loop = false;
                a = ' '; // to close the last word
            }

            if(quotes.empty()) // outside a word
                switch(a)
                {
                case ' ':
                case '\t':
                case '\n':
                case '\r':
                    start++;
                    break;
                case '"':
                case '\'':
                case '`':
                    quotes.push_back(a);
                    end = start;
                    start++;
                    break;
                default:
                    quotes.push_back(' '); // the quote space means no quote
                    end = start;
                    break;
                }
            else // inside a word
                switch(a)
                {
                case '\t':
                    if(quotes.back() != ' ')
                    {
                        end++;
                        break;
                    }
                        // no break !
                case '\n':
                case '\r':
                    a = ' ';
                        // no break !
                case ' ':
                case '"':
                case '\'':
                case '`':
                    if(a == quotes.back()) // "a" is an ending quote
                    {
                        quotes.pop_back();
                        if(quotes.empty()) // reached end of word
                        {
                            mots.push_back(tools_make_word(f, start, end));
                            if(a != ' ')
                                end++;  // skip the trailing quote
                            if(! f.skip(end+1))
                                loop = false; // reached end of file
                            start = end+1;
                        }
                        else
                            end++;
                    }
                    else // "a" is a nested starting quote
                    {
                        if(a != ' ') // quote ' ' does not have ending quote
                            quotes.push_back(a);
                        end++;
                    }
                    break;
                default:
                    end++;
                }
        }
        if(!quotes.empty())
            throw Erange("make_args_from_file", tools_printf(gettext("Parse error: Unmatched %c"), quotes.back()));

	return mots;
    }

    static string tools_make_word(generic_file &fic, off_t start, off_t end)
    {
	off_t longueur = end - start + 1;
	char *tmp = new char[longueur+1];
	string ret;

	if(tmp == NULL)
	    throw Ememory("make_word");
	try
	{
	    S_I lu = 0, delta;

	    if(! fic.skip(start))
		throw Erange("tools_make_word", gettext("End of file reached while skipping to the begin of a word"));

	    do
	    {
		delta = fic.read(tmp+lu, longueur-lu);
		if(delta > 0)
		    lu += delta;
		else
		    if(delta < 0)
			throw SRC_BUG;
		    else // delta == 0
			throw Erange("make_word", gettext("Reached EOF while reading a word"));
	    }
	    while(lu < longueur);
	    tmp[longueur] = '\0';
	    ret = tmp;
	}
	catch(...)
	{
	    delete tmp;
	    throw;
	}
	delete tmp;

	return ret;
    }

    string tools_build_regex_for_exclude_mask(const string & prefix,
					      const string & relative_part)
    {
	string result = "^";
	string::const_iterator it = prefix.begin();

	    // prepending any non alpha numeric char of the root by a anti-slash

	for( ; it != prefix.end() ; ++it)
	{
 	    if(isalnum(*it) || *it == '/' || *it == ' ')
		result += *it;
	    else
	    {
		result += '\\';
		result += *it;
	    }
	}

	    // adding a trailing / if necessary

 	string::reverse_iterator tr = result.rbegin();
	if(tr == result.rend() || *tr != '/')
	    result += '/';

	    // adapting and adding the relative_part

	it = relative_part.begin();

	if(it != relative_part.end() && *it == '^')
	    it++; // skipping leading ^
	else
	    result += ".*"; // prepending wilde card sub-expression

	for( ; it != relative_part.end() && *it != '$' ; ++it)
	    result += *it;

	result += "(/.+)?$";

	return result;
    }

    string tools_output2xml(const string & src)
    {
	string ret = "";
	U_I cur = 0, size = src.size();

	while(cur < size)
	{
	    switch(src[cur])
	    {
	    case '<':
		ret += "&lt;";
		break;
	    case '>':
		ret += "&gt;";
	        break;
	    case '&':
		ret += "&amp;";
		break;
	    case '\'':
		ret += "&apos;";
		break;
	    case'\"':
		ret += "&quot;";
    	        break;
	    default:
		ret += src[cur];
	    }
	    ++cur;
	}

	return ret;
    }


    void tools_memxor(void *dest, const void *src, size_t n)
    {
	unsigned char *d = (unsigned char *) dest;
	const unsigned char *s = (const unsigned char *) src;

	for (size_t i = 0; i < n; i++)
	    *d++ ^= *s++;
    }



} // end of namespace
