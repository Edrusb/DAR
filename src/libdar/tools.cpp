/*********************************************************************/
// da - disk archive - a backup/restoration program
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
// $Id: tools.cpp,v 1.41.2.3 2005/05/06 10:01:52 edrusb Rel $
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

#if HAVE_PWD_H
#include <pwd.h>
#endif

#if HAVE_GRP_H
#include <grp.h>
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

using namespace std;

namespace libdar
{

    static void runson(user_interaction & dialog, char *argv[]);
    static void deadson(S_I sig);
    static bool is_a_slice_available(const string & base, const string & extension);
    static string retreive_basename(const string & base, const string & extension);

    char *tools_str2charptr(string x)
    {
        U_I size = x.size();
        char *ret = new char[size+1];

        if(ret == NULL)
            throw Ememory("tools_str2charptr");
        for(register unsigned int i = 0; i < size; i++)
            ret[i] = x[i];
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
            delete [] tmp;
        }
        delete [] tmp;
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
                throw Erange("tools_get_filesize", tools_printf(gettext("Cannot get file size: %s"), strerror(errno)));
        }
        catch(...)
        {
            delete [] name;
        }

        delete [] name;
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

    char *tools_extract_basename(const char *command_name)
    {
        path commande = command_name;
        string tmp = commande.basename();
        char *name = tools_str2charptr(tmp);

        return name;
    }


    static void dummy_call(char *x)
    {
        static char id[]="$Id: tools.cpp,v 1.41.2.3 2005/05/06 10:01:52 edrusb Rel $";
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
        char *ptr = tools_str2charptr(all);

        try
        {
            tools_split_path_basename(ptr, tmp, base);
            if(tmp == NULL)
                throw SRC_BUG;
            chemin = tmp->display();
            delete tmp;
        }
        catch(...)
        {
            delete [] ptr;
            throw;
        }
        delete [] ptr;
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
            throw Erange("tools_str2int", gettext("Cannot convert the string to integer, overflow"));

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

    void tools_system(user_interaction & dialog, const vector<string> & argvector)
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
                            throw Erange("tools_system", string(gettext("Error while calling fork() to launch dar: ")) + strerror(errno));
                        case 0: // fork has succeeded, we are the child process
                            runson(dialog, argv);
                                // function that never returns
                        default:
                            if(wait(&status) <= 0)
                                throw Erange("tools_system",
                                             string(gettext("Unexpected error while waiting for dar to terminate: ")) + strerror(errno));
                            else // checking the way dar has exit
                                if(!WIFEXITED(status)) // not a normal ending
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
                                    else // normal terminaison but exit code not zero
                                        dialog.pause(string(gettext("DAR sub-process has terminated with exit code "))
						     + tools_int2str(WEXITSTATUS(status))
						     + gettext(" Continue anyway ?"));
                        }
                    }
                    while(loop);
                }
                catch(...)
                {
                    for(register U_I i = 0; i < argvector.size(); i++)
                        if(argv[i] != NULL)
                            delete [] argv[i];
                    throw;
                }
                for(register U_I i = 0; i < argvector.size(); i++)
                    if(argv[i] != NULL)
                        delete argv[i];
            }
            catch(...)
            {
                delete [] argv;
                throw;
            }
            delete [] argv;
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



    const char *tools_get_from_env(const char **env, char *clef)
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
				bool libz, bool libbz2, bool libcrypto)
    {
#define YES_NO(x) (x ? gettext("YES") : gettext("NO"))

	dialog.printf(gettext("   Libz compression (gzip)    : %s\n"), YES_NO(libz));
	dialog.printf(gettext("   Libbz2 compression (bzip2) : %s\n"), YES_NO(libbz2));
	dialog.printf(gettext("   Strong encryption          : %s\n"), YES_NO(libcrypto));
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
        regular_mask suspect = regular_mask(string(".\\.[1-9][0-9]*\\.")+extension, true);
        string old_path = (loc+base).display();

            // is basename is suspect ?
        if(!suspect.is_covered(base))
            return; // not a suspect basename

            // is there a slice available ?
        if(is_a_slice_available(old_path, extension))
            return; // yes, thus basename is not a mistake

            // removing the suspicious end (.<number>.extension)
            // and checking the avaibility of such a slice

        string new_base = retreive_basename(base, extension);
        string new_path = (loc+new_base).display();
        if(is_a_slice_available(new_path, extension))
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
		    if(errno == ERANGE) // buffer too small
		    {
			delete buffer;
			buffer = NULL;
			length *= 2;
		    }
		    else // other error
			throw Erange("tools_getcwd", string(gettext("Cannot get full path of current working directory: ")) + strerror(errno));
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
        char *filename;

        access.unstack(tmp);
        temps.actime = tmp;
        tmp = 0;
        modif.unstack(tmp);
        temps.modtime = tmp;

        filename = tools_str2charptr(chemin);
        try
        {
            if(utime(filename , &temps) < 0)
                Erange("tools_make_date", string(gettext("Cannot set last access and last modification time: ")) + strerror(errno));
        }
        catch(...)
        {
            delete [] filename;
            throw;
        }
        delete [] filename;
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

    void tools_remove_last_char_if_equal_to(char c, string & s)
    {
	if(s[s.size()-1] == c)
	    s = string(s.begin(), s.begin()+(s.size() - 1));
    }

    static void deadson(S_I sig)
    {
        signal(SIGCHLD, &deadson);
    }

    static void runson(user_interaction & dialog, char *argv[])
    {
        if(execvp(argv[0], argv) < 0)
            dialog.warning(string(gettext("Error while calling execvp:")) + strerror(errno));
        else
            dialog.warning(gettext("execvp failed but did not returned error code"));
        exit(0);
    }

    static bool is_a_slice_available(const string & base, const string & extension)
    {
        char *name = tools_str2charptr(base);
        path *chem = NULL;
        bool ret = false;

        try
        {
            char *char_chem = NULL;
            string rest;

            tools_split_path_basename(name, chem, rest);
            char_chem = tools_str2charptr(chem->display());

            try
            {
                etage contents = etage(char_chem, 0, 0);  // we don't care the dates here so we set them to zero
                regular_mask slice = regular_mask(rest + "\\.[1-9][0-9]*\\."+ extension, true);

                while(!ret && contents.read(rest))
                    ret = slice.is_covered(rest);
            }
            catch(Erange & e)
            {
                ret = false;
            }
            catch(...)
            {
                delete [] char_chem;
                throw;
            }
            delete [] char_chem;
        }
        catch(...)
        {
            delete [] name;
            if(chem != NULL)
                delete chem;
            throw;
        }
        delete [] name;
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
	string::iterator it = const_cast<string &>(s).begin();

	while(it < s.end() && *it != '-')
	    it++;

	if(it < s.end())
	{
	    min = tools_str2int(string(const_cast<string &>(s).begin(), it));
	    max = tools_str2int(string(++it, const_cast<string &>(s).end()));
	}
	else
	    min = max = tools_str2int(s);
    }


    string tools_printf(char *format, ...)
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

    string tools_vprintf(char *format, va_list ap)
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
                    ptr++;
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
                    ptr++;
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
                    ptr++;
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

    void tools_unlink_file_mask(user_interaction & dialog, const char *c_chemin, const char *file_mask, bool info_details)
    {
	simple_mask my_mask = simple_mask(string(file_mask), true);
	etage dir = etage(c_chemin, 0, 0);
	path chemin = path(string(c_chemin));
	string entry;

	while(dir.read(entry))
	    if(my_mask.is_covered(entry))
	    {
		char *c_entry = tools_str2charptr((chemin + entry).display());
		try
		{
		    if(info_details)
			dialog.warning(tools_printf(gettext("Removing file %s"), c_entry));
		    if(unlink(c_entry) != 0)
			dialog.warning(tools_printf(gettext("ERROR removing file %s : %s"), c_entry, strerror(errno)));
		}
		catch(...)
		{
		    if(c_entry != NULL)
			delete [] c_entry;
		    throw;
		}
		delete [] c_entry;
	    }
    }

    bool tools_do_some_files_match_mask(const char *c_chemin, const char *file_mask)
    {
	simple_mask my_mask = simple_mask(string(file_mask), true);
	etage dir = etage(c_chemin, 0, 0);
	string entry;
	bool ret = false;

	while(!ret && dir.read(entry))
	    if(my_mask.is_covered(entry))
		ret = true;

	return ret;
    }

    void tools_avoid_slice_overwriting(user_interaction & dialog, const string & chemin, const string & x_file_mask, bool info_details, bool allow_overwriting, bool warn_overwriting)
    {
	char *c_chemin = tools_str2charptr(chemin);
	try
	{
	    char *file_mask = tools_str2charptr(x_file_mask);
	    try
	    {
		if(tools_do_some_files_match_mask(c_chemin, file_mask))
		    if(!allow_overwriting)
			throw Erange("tools_avoid_slice_overwriting", tools_printf(gettext("Overwriting not allowed while a slice of a previous archive with the same basename has been found in the %s directory, Operation aborted"), c_chemin));
		    else
		    {
			try
			{
			    if(warn_overwriting)
				dialog.pause(tools_printf(gettext("At least one slice of an old archive with the same basename remains in the directory %s , If you do not remove theses all first, you will have difficulty identifying the last slice of the archive you are about to create, because it may be hidden in between slices of this older archive. Do we remove the old archive's slices first ?"), c_chemin));
			    tools_unlink_file_mask(dialog, c_chemin, file_mask, info_details);
			}
			catch(Euser_abort & e)
			{
				// nothing to do, just continue
			}
		    }
	    }
	    catch(...)
	    {
		if(file_mask != NULL)
		    delete [] file_mask;
		throw;
	    }
	    delete [] file_mask;
	}
	catch(...)
	{
	    if(c_chemin != NULL)
		delete [] c_chemin;
	    throw;
	}
	delete [] c_chemin;
    }

    void tools_add_elastic_buffer(generic_file & f, U_32 max_size)
    {
	elastic tic = (time(NULL) % (max_size - 1)) + 1; // range from 1 to max_size
	char *buffer = new char[max_size];

	if(buffer == NULL)
	    throw Ememory("tools_add_elastic_buffer");
	try
	{
	    tic.dump(buffer, max_size);
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

	char *filename = tools_str2charptr(file1);

	try
	{
	    if(stat(filename, &sstat) < 0)
		throw Erange("tools:tools_are_on_same_filesystem", string("Cannot get inode information for: ") +  file1 + " : " + strerror(errno));
	    id = sstat.st_dev;
	}
	catch(...)
	{
	    delete [] filename;
	    throw;
	}
	delete [] filename;

	filename = tools_str2charptr(file2);
	try
	{
	    if(stat(filename, &sstat) < 0)
		throw Erange("tools:tools_are_on_same_filesystem", string("Cannot get inode information for: ") +  file2 + " : " + strerror(errno));
	}
	catch(...)
	{
	    delete [] filename;
	    throw;
	}
	delete [] filename;


	return id == sstat.st_dev;
    }


} // end of namespace
