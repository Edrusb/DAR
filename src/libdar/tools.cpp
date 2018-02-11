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
#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_STRINGS_H
#include <strings.h>
#endif

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

#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#if HAVE_CTYPE_H
#include <ctype.h>
#endif

#if HAVE_PWD_H
#include <pwd.h>
#endif

#if HAVE_GRP_H
#include <grp.h>
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#if HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif

#if HAVE_WCHAR_H
#include <wchar.h>
#endif

#if HAVE_WCTYPE_H
#include <wctype.h>
#endif

#if HAVE_STDDEF_H
#include <stddef.h>
#endif

#if HAVE_DIRENT_H
#include <dirent.h>
#endif
} // end extern "C"

#include <iostream>
#include <algorithm>
#include <sstream>

#include "nls_swap.hpp"
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
#include "compile_time_features.hpp"
#include "memory_file.hpp"

#define YES_NO(x) (x ? gettext("YES") : gettext("NO"))

using namespace std;

namespace libdar
{

#ifdef __DYNAMIC__
        // Yes, this is a static variable,
        // it contains the necessary mutex to keep libdar thread-safe
    static const user_group_bases *user_group = nullptr;
#endif

        // the following variable is static this breaks the threadsafe support
        // while it also concerns the signaling which is out process related

    static void runson(user_interaction & dialog, char * const argv[]);
    static void ignore_deadson(S_I sig);
    static void abort_on_deadson(S_I sig);
    static bool is_a_slice_available(user_interaction & ui, const string & base, const string & extension);
    static string retreive_basename(const string & base, const string & extension);
    static void tools_localtime(const time_t & timep, struct tm *result);

    void tools_init()
    {
#ifdef __DYNAMIC__
        if(user_group == nullptr)
        {
            user_group = new (nothrow) user_group_bases();
            if(user_group == nullptr)
                throw Ememory("tools_init");
        }
#endif
    }

    void tools_end()
    {
#ifdef __DYNAMIC__
        if(user_group != nullptr)
        {
            delete user_group;
            user_group = nullptr;
        }
#endif
    }


    char *tools_str2charptr(const string &x)
    {
        U_I size = x.size();
        char *ret = new (nothrow) char[size+1];

        if(ret == nullptr)
            throw Ememory("tools_str2charptr");
        (void)memcpy(ret, x.c_str(), size);
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
            throw Erange("tools_read_string", dar_gettext("Not a zero terminated string in file"));
    }

    void tools_write_string_all(generic_file & f, const string & s)
    {
        f.write(s.c_str(), s.size());
    }

    void tools_read_string_size(generic_file & f, string & s, infinint taille)
    {
        U_16 small_read = 0;
        U_I max_read = 0;
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
        {
            string tmp = tools_strerror_r(errno);
            throw Erange("tools_get_filesize", tools_printf(dar_gettext("Cannot get file size: %s"), tmp.c_str()));
        }

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
            throw Erange("command_line get_extended_size", tools_printf(dar_gettext("Unknown suffix [%c] in string %S"), s[len-1], &s));
        }

        if(factor != 1)
            s = string(s.begin(), s.end()-1);

        deci tmp = s;
        factor *= tmp.computer();

        return factor;
    }

    string tools_display_integer_in_metric_system(infinint number, const string & unit, bool binary)
    {
        string ret = "";
        infinint multiple = binary ? 1024 : 1000;
        U_I power = 0;
            // 1 = 'k', 2 = 'M', 3 = 'G', 4 = 'T', 5 = 'P', 6 = 'E', 7 = 'Z', 8 = 'Y'

        while(number >= multiple && power < 8)
        {
            ++power;
            number /= multiple;
        }

        ret = deci(number).human();
        if(unit.size() > 0)
            ret += " "; // a space is required by convention to separate the number from its unit

        switch(power)
        {
        case 0:
            if(!number.is_zero())
                ret += unit;
                // not displaying unit for zero for clarity in particular when octets symbol is used
                // which would give "0 o" that is somehow not very easy to read/understand
            break;
        case 1:
            ret += (binary ? "ki" : "k") + unit;
            break;
        case 2:
            ret += (binary ? "Mi" : "M") + unit;
            break;
        case 3:
            ret += (binary ? "Gi" : "G") + unit;
            break;
        case 4:
            ret += (binary ? "Ti" : "T") + unit;
            break;
        case 5:
            ret += (binary ? "Pi" : "P") + unit;
            break;
        case 6:
            ret += (binary ? "Ei" : "E") + unit;
            break;
        case 7:
            ret += (binary ? "Zi" : "Z") + unit;
            break;
        default:
            ret += (binary ? "Yi" : "Y") + unit;
            break;
        }

        return ret;
    }

    void tools_extract_basename(const char *command_name, string &basename)
    {
        basename = path(command_name).basename();
    }

    string::iterator tools_find_last_char_of(string &s, unsigned char v)
    {
        if(s.empty())
            return s.end();

        string::iterator back, it = s.begin();
        bool valid = (it != s.end()) && (*it == v);

        while(it != s.end())
        {
            back = it;
            it = find(it + 1, s.end(), v);
        }

        if(!valid && (back == s.begin())) // no char found at all (back has been sticked at the beginning and the first character is not the one we look for
            return s.end();
        else
            return back;
    }


    string::iterator tools_find_first_char_of(string &s, unsigned char v)
    {
        string::iterator it = s.begin();

        while(it != s.end() && *it != v)
            ++it;

        return it;
    }

    void tools_split_path_basename(const char *all, path * &chemin, string & base)
    {
        chemin = nullptr;
        string src = all;
        string::iterator it = tools_find_last_char_of(src, '/');

        if(it != src.end()) // path separator found (pointed to by "it")
        {
	    it += 1;
            base = string(it, src.end());
            chemin = new (nothrow) path(string(src.begin(), it), true);
        }
        else
        {
            base = src;
            chemin = new (nothrow) path(".");
        }

        if(chemin == nullptr)
            throw Ememory("tools_split_path_basename");
    }

    void tools_split_path_basename(const string & all, string & chemin, string & base)
    {
        path *tmp = nullptr;

        tools_split_path_basename(all.c_str(), tmp, base);
        if(tmp == nullptr)
            throw SRC_BUG;
        chemin = tmp->display();
        delete tmp;
    }

    bool tools_split_entrepot_path(const string &all,
				   string & proto,
				   string & login,
				   secu_string & password,
				   string & hostname,
				   string & port,
				   string & path_basename)
    {
	bool ret = true;
	const char *ch = all.c_str();
	U_I cursor = 0;
	U_I ref_cur = 0;
	U_I tmp;
	U_I max = all.size();

	proto.clear();
	login.clear();
	password.clear();
	hostname.clear();
	port.clear();
	path_basename.clear();

	enum { s_proto, s_login, s_pass, s_host, s_port, s_path, s_end } state = s_proto;

	while(state != s_end && cursor < max)
	{
	    switch(state)
	    {
	    case s_proto:
		switch(ch[cursor])
		{
		case ':':
		    ++cursor;
		    if(ch[cursor] != '/')
		    {
			state = s_end;
			ret = false;
		    }
		    else
		    {
			++cursor;
			if(ch[cursor] != '/')
			{
			    state = s_end;
			    ret = false;
			}
			else
			{
			    ++cursor;
			    state = s_login;
			}
		    }
		    break;
		case '/':
		case '@':
		    state = s_end;
		    ret = false;
		    break;
		default:
		    proto += ch[cursor];
		    ++cursor;
		}
		break;
	    case s_login:
		switch(ch[cursor])
		{
		case '@':
		    state = s_host;
		    ++cursor;
		    break;
		case ':':
		    state = s_pass;
		    ++cursor;
		    ref_cur = cursor;
		    break;
		case '/':
		    hostname = login;
		    login.clear();
		    state = s_path;
		    ++cursor;
		    break;
		default:
		    login += ch[cursor];
		    ++cursor;
		}
		break;
	    case s_pass:
		switch(ch[cursor])
		{
		case '@':
		    state = s_host;
		    if(ref_cur > cursor)
			throw SRC_BUG;
		    tmp = cursor - ref_cur;
		    password.resize(tmp);
		    password.append(ch + ref_cur, tmp);
		    ++cursor;
		    break;
		case '/':
		    hostname = login;
		    port = string(ch+ref_cur, ch+cursor);
		    login.clear();
		    password.clear();
		    state = s_path;
		    ++cursor;
		    break;
		case ':':
		    state = s_end;
		    ret = false;
		    break;
		default:
		    ++cursor;
		}
		break;
	    case s_host:
		switch(ch[cursor])
		{
		case '@':
		    state = s_end;
		    ret = false;
		    break;
		case '/':
		    state = s_path;
		    ++cursor;
		    break;
		case ':':
		    state = s_port;
		    ++cursor;
		    break;
		default:
		    hostname += ch[cursor];
		    ++cursor;
		}
		break;
	    case s_port:
		switch(ch[cursor])
		{
		case '@':
		case ':':
		    state = s_end;
		    ret = false;
		    break;
		case '/':
		    state = s_path;
		    ++cursor;
		    break;
		default:
		    port += ch[cursor];
		    ++cursor;
		}
		break;
	    case s_path:
		path_basename += ch[cursor];
		++cursor;
		break;
	    case s_end:
		throw SRC_BUG; // while loop should end with that status
	    default:
		throw SRC_BUG; // unknown status
	    }
	}

	    // sanity checks

	if(ret)
	{
	    if(state != s_path)
		ret = false;
	    if(hostname.size() == 0)
		ret = false;
	    if(path_basename.size() == 0)
	    {
		if(max > 0 && ch[max-1] != '/')
		    ret = false;
		else
		    path_basename = '/';
	    }
	}

	return ret;
    }

    void tools_open_pipes(user_interaction & dialog,
                          const string &input,
                          const string & output,
                          tuyau *&in,
                          tuyau *&out)
    {
        in = out = nullptr;
        try
        {
            if(input != "")
                in = new (nothrow) tuyau(dialog, input, gf_read_only);
            else
                in = new (nothrow) tuyau(dialog, 0, gf_read_only); // stdin by default
            if(in == nullptr)
                throw Ememory("tools_open_pipes");

            if(output != "")
                out = new (nothrow) tuyau(dialog, output, gf_write_only);
            else
                out = new (nothrow) tuyau(dialog, 1, gf_write_only); // stdout by default
            if(out == nullptr)
                throw Ememory("tools_open_pipes");

        }
        catch(...)
        {
            if(in != nullptr)
                delete in;
            if(out != nullptr)
                delete out;
            throw;
        }
    }

    void tools_blocking_read(S_I fd, bool mode)
    {
        S_I flags = fcntl(fd, F_GETFL, 0);
        if(flags < 0)
            throw Erange("tools_blocking_read", string(dar_gettext("Cannot read \"fcntl\" file's flags : "))+tools_strerror_r(errno));
        if(!mode)
            flags |= O_NONBLOCK;
        else
            flags &= ~O_NONBLOCK;
        if(fcntl(fd, F_SETFL, flags) < 0)
            throw Erange("tools_blocking_read", string(dar_gettext("Cannot set \"fcntl\" file's flags : "))+tools_strerror_r(errno));
    }

    string tools_name_of_uid(const infinint & uid)
    {
#ifndef  __DYNAMIC__
        string name = "";
#else
        string name;
        if(user_group != nullptr)
            name = user_group->get_username(uid);
        else
            throw SRC_BUG;
#endif

        if(name.empty()) // uid not associated with a name
        {
            deci d = uid;
            return d.human();
        }
        else
            return name;
    }

    string tools_name_of_gid(const infinint & gid)
    {
#ifndef __DYNAMIC__
        string name = "";
#else
        string name;
        if(user_group != nullptr)
            name = user_group->get_groupname(gid);
        else
            throw SRC_BUG;
#endif

        if(name.empty()) // uid not associated with a name
        {
            deci d = gid;
            return d.human();
        }
        else
            return name;
    }

    string tools_uword2str(U_16 x)
    {
        ostringstream tmp;

        tmp << x;

        return tmp.str();
    }

    string tools_int2str(S_I x)
    {
        ostringstream tmp;

        tmp << x;

        return tmp.str();
    }

    string tools_uint2str(U_I x)
    {
        ostringstream tmp;

        tmp << x;

        return tmp.str();
    }


    U_I tools_str2int(const string & x)
    {
        stringstream tmp(x);
        U_I ret;
        string residu;

        if((tmp >> ret).fail())
            throw Erange("tools_str2string", string(dar_gettext("Invalid number: ")) + x);

        tmp >> residu;
        for(U_I i = 0; i < residu.size(); ++i)
            if(residu[i] != ' ')
                throw Erange("tools_str2string", string(dar_gettext("Invalid number: ")) + x);

        return ret;
    }

    S_I tools_str2signed_int(const string & x)
    {
        stringstream tmp(x);
        S_I ret;
        string residu;

        if((tmp >> ret).fail())
            throw Erange("tools_str2string", string(dar_gettext("Invalid number: ")) + x);

        tmp >> residu;
        for(U_I i = 0; i < residu.size(); ++i)
            if(residu[i] != ' ')
                throw Erange("tools_str2string", string(dar_gettext("Invalid number: ")) + x);

        return ret;
    }

    bool tools_my_atoi(const char *a, U_I & val)
    {
        try
        {
            val = tools_str2int(a);
            return true;
        }
        catch(Erange & e)
        {
            val = 0;
            return false;
        }
    }

    string tools_addspacebefore(string s, U_I expected_size)
    {
        return string(expected_size - s.size(), ' ') + s;
    }

    string tools_display_date(const datetime & date)
    {
        time_t pas = 0;
        time_t frac = 0;
	string ret;

        if(!date.get_value(pas, frac, datetime::tu_second)) // conversion to system type failed. Using a replacement string
            return deci(date.get_second_value()).human();
        else
        {
	    char *val = nullptr;
#if HAVE_CTIME_R
	    char *str = new (nothrow) char [50]; //< minimum required is 26 bytes
	    if(str == nullptr)
		throw Ememory("tools_display_date");
	    try
	    {
		val = ctime_r(&pas, str);
#else
		val = ctime(&pas);
#endif
		if(val == nullptr) // ctime() failed
		    ret = tools_int2str(pas);
		else
		    ret = val;
#if HAVE_CTIME_R
	    }
	    catch(...)
	    {
		delete [] str;
		throw;
	    }
	    delete [] str;
#else
	ret = val;
#endif
	}

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
                when.tm_isdst = 1;           // provided time is local daylight saving time
                tmp = 0;
            };
	    scan(const scan & ref) = default;
	    scan & operator = (const scan & ref) = default;
	    ~scan() = default;

            status get_etat() const { return etat; };
            tm get_struct() const { return when; };
            void add_digit(char a)
            {
                if(a < 48 || a > 57) // ascii code for zero is 48, for nine is 57
                    throw SRC_BUG;
                tmp = tmp*10 + (a-48);
            };

            void set_etat(const status & val)
            {
                switch(etat)
                {
                case year:
                    if(tmp < 1970)
                        throw Erange("tools_convert_date", dar_gettext("date before 1970 is not allowed"));
                    when.tm_year = tmp - 1900;
                    break;
                case month:
                    if(tmp < 1 || tmp > 12)
                        throw Erange("tools_convert_date", dar_gettext("Incorrect month"));
                    when.tm_mon = tmp - 1;
                    break;
                case day:
                    if(tmp < 1 || tmp > 31)
                        throw Erange("tools_convert_date", dar_gettext("Incorrect day of month"));
                    when.tm_mday = tmp;
                    break;
                case hour:
                    if(tmp < 0 || tmp > 23)
                        throw Erange("tools_convert_date", dar_gettext("Incorrect hour"));
                    when.tm_hour = tmp;
                    break;
                case min:
                    if(tmp < 0 || tmp > 59)
                        throw Erange("tools_convert_date", dar_gettext("Incorrect minute"));
                    when.tm_min = tmp;
                    break;
                case sec:
                    if(tmp < 0 || tmp > 59)
                        throw Erange("tools_convert_date", dar_gettext("Incorrect second"));
                    when.tm_sec = tmp;
                    break;
                case error:
                    throw Erange("tools_convert_date", dar_gettext("Bad formatted date expression"));
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
        time_t now = ::time(nullptr), when;
	struct tm result;
	tools_localtime(now, &result);
        scan scanner = scan(result);
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
            throw Erange("tools_convert_date", dar_gettext("Given date must be in the past"));
        ret = when;

        return ret;
    }

    void tools_system(user_interaction & dialog, const vector<string> & argvector)
    {
        if(argvector.empty())
            return; // nothing to do

            // ISO C++ forbids variable-size array
        char **argv = new (nothrow) char * [argvector.size()+1];

        for(U_I i = 0; i <= argvector.size(); i++)
            argv[i] = nullptr;

        try
        {
            S_I status;
            bool loop;

            for(U_I i = 0; i < argvector.size(); i++)
                argv[i] = tools_str2charptr(argvector[i]);
            argv[argvector.size()] = nullptr; // this is already done above but that does not hurt doing it twice :-)

            do
            {
                ignore_deadson(0);
                loop = false;
                S_I pid = fork();

                switch(pid)
                {
                case -1:
                    throw Erange("tools_system", string(dar_gettext("Error while calling fork() to launch dar: ")) + tools_strerror_r(errno));
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
                                     string(dar_gettext("Unexpected error while waiting for dar to terminate: ")) + tools_strerror_r(errno));
                    else // checking the way dar has exit
                        if(WIFSIGNALED(status)) // exited because of a signal
                        {
                            try
                            {
                                dialog.pause(string(dar_gettext("DAR terminated upon signal reception: "))
#if HAVE_DECL_SYS_SIGLIST
                                             + (WTERMSIG(status) < NSIG ? sys_siglist[WTERMSIG(status)] : tools_int2str(WTERMSIG(status)))
#else
                                             + tools_int2str(WTERMSIG(status))
#endif
                                             + dar_gettext(" . Retry to launch dar as previously ?"));
                                loop = true;
                            }
                            catch(Euser_abort & e)
                            {
                                dialog.pause(dar_gettext(" Continue anyway ?"));
                            }
                        }
                        else // normal terminaison checking exit status code
                            if(WEXITSTATUS(status) != 0)
                                dialog.pause(string(dar_gettext("DAR sub-process has terminated with exit code "))
                                             + tools_int2str(WEXITSTATUS(status))
                                             + dar_gettext(" Continue anyway ?"));
                }
            }
            while(loop);
        }
        catch(...)
        {
            for(U_I i = 0; i <= argvector.size(); i++)
                if(argv[i] != nullptr)
                    delete [] argv[i];
            delete [] argv;
            throw;
        }

        for(U_I i = 0; i <= argvector.size(); i++)
            if(argv[i] != nullptr)
                delete [] argv[i];
        delete [] argv;
    }

    void tools_system_with_pipe(user_interaction & dialog,
                                const string & dar_cmd,
                                const vector<string> & argvpipe)
    {
        const char *argv[] = { dar_cmd.c_str(), "--pipe-fd", nullptr, nullptr };
        bool loop = false;

        do
        {
            tuyau *tube = nullptr;

            try
            {
                tube = new (nothrow) tuyau(dialog);
                if(tube == nullptr)
                    throw Ememory("tools_system_with_pipe");

                const string read_fd = tools_int2str(tube->get_read_fd());
                tlv_list pipeargs;
                S_I status;

                argv[2] = read_fd.c_str();
                signal(SIGCHLD, &abort_on_deadson); // do not accept child death

                loop = false;
                S_I pid = fork();

                switch(pid)
                {
                case -1:
                    throw Erange("tools_system_with_pipe", string(dar_gettext("Error while calling fork() to launch dar: ")) + tools_strerror_r(errno));
                case 0: // fork has succeeded, we are the child process
                    try
                    {
                        if(tube != nullptr)
                        {
                            tube->do_not_close_read_fd();
                            delete tube; // C++ object is destroyed but read filedescriptor has been kept open
                            tube = nullptr;
                            runson(dialog, const_cast<char * const*>(argv));
                            throw SRC_BUG;
                        }
                        else
                            throw SRC_BUG;
                    }
                    catch(...)
                    {
                        throw SRC_BUG;
                    }
                default: // fork has succeeded, we are the parent process
                    tube->close_read_fd();
                    pipeargs = tools_string2tlv_list(dialog, 0, argvpipe);
                    pipeargs.dump(*tube);
                    ignore_deadson(0); // now we can ignore SIGCHLD signals just before destroying the pipe filedescriptor, which will trigger and EOF while reading on pipe
                        // in the child process
                    delete tube;
                    tube = nullptr;

                    if(wait(&status) <= 0)
                        throw Erange("tools_system",
                                     string(dar_gettext("Unexpected error while waiting for dar to terminate: ")) + tools_strerror_r(errno));
                    else // checking the way dar has exit
                        if(WIFSIGNALED(status)) // exited because of a signal
                        {
                            try
                            {
                                dialog.pause(string(dar_gettext("DAR terminated upon signal reception: "))
#if HAVE_DECL_SYS_SIGLIST
                                             + (WTERMSIG(status) < NSIG ? sys_siglist[WTERMSIG(status)] : tools_int2str(WTERMSIG(status)))
#else
                                             + tools_int2str(WTERMSIG(status))
#endif
                                             + dar_gettext(" . Retry to launch dar as previously ?"));
                                loop = true;
                            }
                            catch(Euser_abort & e)
                            {
                                dialog.pause(dar_gettext(" Continue anyway ?"));
                            }
                        }
                        else // normal terminaison checking exit status code
                            if(WEXITSTATUS(status) != 0)
                                dialog.pause(string(dar_gettext("DAR sub-process has terminated with exit code "))
                                             + tools_int2str(WEXITSTATUS(status))
                                             + dar_gettext(" Continue anyway ?"));

                }
            }
            catch(...)
            {
                if(tube != nullptr)
                    delete tube;
                throw;
            }
            if(tube != nullptr)
                delete tube;
        }
        while(loop);

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
        infinint tmp = infinint(f);
        string elem;

        x.clear();
        while(!tmp.is_zero())
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
        const char *ret = nullptr;

        if(env == nullptr || clef == nullptr)
            return nullptr;

        while(ret == nullptr && env[index] != nullptr)
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

    void tools_display_features(user_interaction & dialog)
    {
        NLS_SWAP_IN;
        try
        {
            const char *endy = nullptr;
            string time_accuracy = "";

            dialog.printf(gettext("   Libz compression (gzip)      : %s\n"), YES_NO(compile_time::libz()));
            dialog.printf(gettext("   Libbz2 compression (bzip2)   : %s\n"), YES_NO(compile_time::libbz2()));
            dialog.printf(gettext("   Liblzo2 compression (lzo)    : %s\n"), YES_NO(compile_time::liblzo()));
            dialog.printf(gettext("   Liblzma compression (xz)     : %s\n"), YES_NO(compile_time::libxz()));
            dialog.printf(gettext("   Strong encryption (libgcrypt): %s\n"), YES_NO(compile_time::libgcrypt()));
            dialog.printf(gettext("   Public key ciphers (gpgme)   : %s\n"), YES_NO(compile_time::public_key_cipher()));
            dialog.printf(gettext("   Extended Attributes support  : %s\n"), YES_NO(compile_time::ea()));
            dialog.printf(gettext("   Large files support (> 2GB)  : %s\n"), YES_NO(compile_time::largefile()));
            dialog.printf(gettext("   ext2fs NODUMP flag support   : %s\n"), YES_NO(compile_time::nodump()));
            dialog.printf(gettext("   Special allocation scheme    : %s\n"), YES_NO(compile_time::special_alloc()));
            if(compile_time::bits() == 0)
                dialog.printf(gettext("   Integer size used            : unlimited\n"));
            else
                dialog.printf(gettext("   Integer size used            : %d bits\n"), compile_time::bits());
            dialog.printf(gettext("   Thread safe support          : %s\n"), YES_NO(compile_time::thread_safe()));
            dialog.printf(gettext("   Furtive read mode support    : %s\n"), YES_NO(compile_time::furtive_read()));
            dialog.printf(gettext("   Linux ext2/3/4 FSA support   : %s\n"), YES_NO(compile_time::FSA_linux_extX()));
            dialog.printf(gettext("   Mac OS X HFS+ FSA support    : %s\n"), YES_NO(compile_time::FSA_birthtime()));

            switch(compile_time::system_endian())
            {
            case compile_time::big:
                endy = gettext("big");
                break;
            case compile_time::little:
                endy = gettext("little");
                break;
            case compile_time::error:
                endy = gettext("error!");
                break;
            default:
                throw SRC_BUG;
            }
            dialog.printf(gettext("   Detected system/CPU endian   : %s"), endy);
            dialog.printf(gettext("   Posix fadvise support        : %s"), YES_NO(compile_time::posix_fadvise()));
            dialog.printf(gettext("   Large dir. speed optimi.     : %s"), YES_NO(compile_time::fast_dir()));
            if(compile_time::microsecond_read())
                time_accuracy = "1 microsecond";
            else
                time_accuracy = "1 s";
            dialog.printf(gettext("   Timestamp read accuracy      : %S\n"), &time_accuracy);
            if(compile_time::microsecond_write())
                time_accuracy = "1 microsecond";
            else
                time_accuracy = "1 s";
            dialog.printf(gettext("   Timestamp write accuracy     : %S\n"), &time_accuracy);
            dialog.printf(gettext("   Restores dates of symlinks   : %s\n"), YES_NO(compile_time::symlink_restore_dates()));
	    dialog.printf(gettext("   Can uses multiple threads    : %s\n"), YES_NO(compile_time::libthreadar()));
	    dialog.printf(gettext("   Delta compression support    : %s\n"), YES_NO(compile_time::librsync()));
	    dialog.printf(gettext("   Remote repository support    : %s\n"), YES_NO(compile_time::remote_repository()));
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
    }

    bool tools_is_equal_with_hourshift(const infinint & hourshift, const datetime & date1, const datetime & date2)
    {
        infinint num, rest;
        datetime t_delta = date1 > date2 ? date1.loose_diff(date2) : date2.loose_diff(date1);
        infinint delta;

	if(t_delta.is_null())
	    return true; // both args are equal without any hourshift consideration

        if(!t_delta.is_integer_second())
            return false; // difference is not an integer number of second
        else
            delta = t_delta.get_second_value();

            // delta = 3600*num + rest
            // with 0 <= rest < 3600
            // (this is euclidian division)
        euclide(delta, 3600, num, rest);

        if(!rest.is_zero())
            return false;  // difference is not a integer number of hour
        else // rest == 0
            return num <= hourshift;
    }

    void tools_check_basename(user_interaction & dialog, const path & loc, string & base, const string & extension)
    {
        NLS_SWAP_IN;
        try
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
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
    }

    string tools_getcwd()
    {
        const U_I step = 1024;
        U_I length = step;
        char *buffer = nullptr, *ret;
        string cwd;
        try
        {
            do
            {
                buffer = new (nothrow) char[length];
                if(buffer == nullptr)
                    throw Ememory("tools_getcwd()");
                ret = getcwd(buffer, length-1); // length-1 to keep a place for ending '\0'
                if(ret == nullptr) // could not get the CWD
                {
                    if(errno == ERANGE) // buffer too small
                    {
                        delete [] buffer;
                        buffer = nullptr;
                        length += step;
                    }
                    else // other error
                        throw Erange("tools_getcwd", string(dar_gettext("Cannot get full path of current working directory: ")) + tools_strerror_r(errno));
                }
            }
            while(ret == nullptr);

            buffer[length - 1] = '\0';
            cwd = buffer;
        }
        catch(...)
        {
            if(buffer != nullptr)
                delete [] buffer;
            throw;
        }
        if(buffer != nullptr)
            delete [] buffer;
        return cwd;
    }

    string tools_readlink(const char *root)
    {
        U_I length = 10240;
        char *buffer = nullptr;
        S_I lu;
        string ret = "";

        if(root == nullptr)
            throw Erange("tools_readlink", dar_gettext("nullptr argument given to tools_readlink()"));
        if(strcmp(root, "") == 0)
            throw Erange("tools_readlink", dar_gettext("Empty string given as argument to tools_readlink()"));

        try
        {
            do
            {
                buffer = new (nothrow) char[length];
                if(buffer == nullptr)
                    throw Ememory("tools_readlink");
                lu = readlink(root, buffer, length-1); // length-1 to have room to add '\0' at the end

                if(lu < 0) // error occured with readlink
                {
                    string tmp;

                    switch(errno)
                    {
                    case EINVAL: // not a symbolic link (thus we return the given argument)
                        ret = root;
                        break;
                    case ENAMETOOLONG: // too small buffer
                        delete [] buffer;
                        buffer = nullptr;
                        length *= 2;
                        break;
                    default: // other error
                        tmp = tools_strerror_r(errno);
                        throw Erange("get_readlink", tools_printf(dar_gettext("Cannot read file information for %s : %s"), root, tmp.c_str()));
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
                        buffer = nullptr;
                        length *= 2;
                    }
            }
            while(ret == "");
        }
        catch(...)
        {
            if(buffer != nullptr)
                delete [] buffer;
            throw;
        }
        if(buffer != nullptr)
            delete [] buffer;
        return ret;
    }

    void tools_make_date(const std::string & chemin, bool symlink, const datetime & access, const datetime & modif, const datetime & birth)
    {
#ifdef LIBDAR_MICROSECOND_WRITE_ACCURACY
        struct timeval temps[2];
#else
        struct utimbuf temps;
#endif
        time_t tmp = 0;
        time_t usec = 0;
        int ret;

        if(!access.get_value(tmp, usec, datetime::tu_microsecond))
            throw Erange("tools_make_date", "cannot set atime of file, value too high for the system integer type");

            // the first time, setting modification time to the value of birth time
            // systems that supports birth time update birth time if the given mtime is older than the current birth time
            // so here we assume birth < modif (if not the birth time will be set to modif)
            // we run a second time the same call but with the real mtime, which should not change the birthtime if this
            // one is as expected older than mtime.
        else
        {
#ifdef LIBDAR_MICROSECOND_WRITE_ACCURACY
            temps[0].tv_sec = tmp;
            temps[0].tv_usec = usec;
#else
            temps.actime = tmp;
#endif
        }

        if(birth != modif)
        {
            if(!birth.get_value(tmp, usec, datetime::tu_microsecond))
                throw Erange("tools_make_date", "cannot set birth time of file, value too high for the system integer type");
            else
            {
#ifdef LIBDAR_MICROSECOND_WRITE_ACCURACY
                temps[1].tv_sec = tmp;
                temps[1].tv_usec = usec;
#else
                temps.modtime = tmp;
#endif
            }

#ifdef LIBDAR_MICROSECOND_WRITE_ACCURACY
#ifdef HAVE_LUTIMES
            ret = lutimes(chemin.c_str(), temps);
#else
            if(symlink)
                return; // not able to restore dates of symlinks
            ret = utimes(chemin.c_str(), temps);
#endif
#else
            if(symlink)
                return; // not able to restore dates of symlinks
            ret = utime(chemin.c_str() , &temps);
#endif
            if(ret < 0)
                Erange("tools_make_date", string(dar_gettext("Cannot set birth time: ")) + tools_strerror_r(errno));
        }

            // we set atime and mtime here
        if(!modif.get_value(tmp, usec, datetime::tu_microsecond))
            throw Erange("tools_make_date", "cannot set last modification time of file, value too high for the system integer type");
        else
        {
#ifdef LIBDAR_MICROSECOND_WRITE_ACCURACY
            temps[1].tv_sec = tmp;
            temps[1].tv_usec = usec;
#else
            temps.modtime = tmp;
#endif
        }

#ifdef LIBDAR_MICROSECOND_WRITE_ACCURACY
#ifdef HAVE_LUTIMES
        ret = lutimes(chemin.c_str(), temps);
#else
        if(symlink)
            return; // not able to restore dates of symlinks
        ret = utimes(chemin.c_str(), temps);
#endif
#else
        if(symlink)
            return; // not able to restore dates of symlinks
        ret = utime(chemin.c_str() , &temps);
#endif
        if(ret < 0)
            throw Erange("tools_make_date", string(dar_gettext("Cannot set last access and last modification time: ")) + tools_strerror_r(errno));
    }

    void tools_noexcept_make_date(const string & chem, bool symlink, const datetime & last_acc, const datetime & last_mod, const datetime & birth)
    {
        try
        {
            if(!last_acc.is_null() || !last_mod.is_null())
                tools_make_date(chem, symlink, last_acc, last_mod, birth);
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

    void tools_to_upper(const string & r, string & uppered)
    {
#if HAVE_WCTYPE_H && HAVE_WCHAR_H
        try
        {
            wstring tmp = tools_string_to_wstring(r);
            tools_to_wupper(tmp);
            uppered = tools_wstring_to_string(tmp);
        }
        catch(Erange & e)
        {
            U_I taille = r.size();
            uppered = r;

            for(U_I x = 0; x < taille; ++x)
                uppered[x] = toupper(uppered[x]);
        }
#else
        U_I taille = r.size();
        uppered = r;

        for(U_I x = 0; x < taille; ++x)
            uppered[x] = toupper(uppered[x]);
#endif
    }

#if HAVE_WCTYPE_H
    void tools_to_wupper(wstring & r)
    {
        wstring::iterator it = r.begin();

        while(it != r.end())
        {
            *it = towupper(*it);
            ++it;
        }
    }
#endif

    void tools_remove_last_char_if_equal_to(char c, string & s)
    {
        if(s[s.size()-1] == c)
            s = string(s.begin(), s.begin()+(s.size() - 1));
    }

    static void ignore_deadson(S_I sig)
    {
        signal(SIGCHLD, &ignore_deadson);
    }

    static void abort_on_deadson(S_I sig)
    {
            // we cannot throw exception in a handler (it would not be caught) we have no other way to report to standard error
        cerr << dar_gettext("Aborting program: child process died unexpectedly") << endl;
    }

    static void runson(user_interaction & dialog, char * const argv[])
    {
        if(execvp(argv[0], argv) < 0)
        {
            string tmp = tools_strerror_r(errno);
            dialog.warning(tools_printf(dar_gettext("Error trying to run %s: %s"), argv[0], tmp.c_str()));
        }
        else
            dialog.warning(string(dar_gettext("execvp() failed but did not returned error code")));
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
        path *chem = nullptr;
        bool ret = false;

        try
        {
            string rest;

            tools_split_path_basename(base.c_str(), chem, rest);

            try
            {
                etage contents = etage(ui, chem->display().c_str(), datetime(0), datetime(0), false, false);  // we don't care the dates here so we set them to zero
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
            if(chem != nullptr)
                delete chem;
            throw;
        }
        if(chem != nullptr)
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

    static void tools_localtime(const time_t & timep, struct tm *result)
    {
#if HAVE_LOCALTIME_R
	struct tm *ret = localtime_r(&timep, result);
	if(ret == nullptr)
	{
	    string err = tools_strerror_r(errno);
	    throw Erange("tools_localtime",
			 tools_printf(gettext("Error met while retrieving current time: %S"), &err));
	}
#else
	struct tm *ret = localtime(&timep);
	if(ret == nullptr)
	{
	    string err = tools_strerror_r(errno);
	    throw Erange("tools_localtime",
			 tools_printf(gettext("Error met while retrieving current time: %S"), &err));
	}

	*result = *ret;
#endif
    }

    void tools_read_range(const string & s, S_I & min, U_I & max)
    {
        string::const_iterator it = s.begin();

        while(it < s.end() && *it != '-')
            it++;

        try
        {
            if(it < s.end())
            {
                min = tools_str2int(string(s.begin(), it));
                max = tools_str2int(string(++it, s.end()));
            }
            else
                min = max = tools_str2int(s);
        }
        catch(Erange & e)
        {
            min = tools_str2signed_int(s);
            max = 0;
        }
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

        copie = new (nothrow) char[taille];
        if(copie == nullptr)
            throw Ememory("tools_printf");
        try
        {
            char *ptr = copie, *start = copie;

            strncpy(copie, format, taille);
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
                    case 'x':
                        test = va_arg(ap, U_I);
                        output += tools_string_to_hexa(deci(test).human());
                        break;
		    case 'o':
			test = va_arg(ap, U_I);
			output += tools_int2octal(test);
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
                        throw Efeature(tools_printf("%%%c is not implemented in tools_printf format argument", *ptr));
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

    void tools_unlink_file_mask_regex(user_interaction & dialog,
				      const entrepot & ent,
				      const string & file_mask,
				      bool info_details)
    {
        regular_mask my_mask = regular_mask(file_mask, true);
        path chemin = path(ent.get_url(), true);
        string entry;

	ent.read_dir_reset();
        while(ent.read_dir_next(entry))
            if(my_mask.is_covered(entry))
            {
		const string c_entry = (chemin + entry).display();
                if(info_details)
                    dialog.warning(tools_printf(dar_gettext("Removing file %s"), c_entry.c_str()));

		try
		{
		    ent.unlink(entry);
		}
		catch(Euser_abort & e)
		{
		    throw;
		}
		catch(Ebug & e)
		{
		    throw;
		}
		catch(Egeneric & e)
                {
                    string tmp = tools_strerror_r(errno);
                    dialog.warning(tools_printf(dar_gettext("Error removing file %s: %s"), c_entry.c_str(), tmp.c_str()));
                }
            }
    }

    bool tools_do_some_files_match_mask_regex(const entrepot & ent,
					      const string & file_mask)
    {
        bool ret = false;
        regular_mask my_mask = regular_mask(file_mask, true);
        string entry;

	ent.read_dir_reset();
        while(!ret && ent.read_dir_next(entry))
            if(my_mask.is_covered(entry))
                ret = true;

        return ret;
    }

    void tools_avoid_slice_overwriting_regex(user_interaction & dialog,
					     const entrepot & ent,
					     const string & basename,
					     const string & extension,
					     bool info_details,
					     bool allow_overwriting,
					     bool warn_overwriting,
					     bool dry_run)
    {
        const string c_chemin = ent.get_url();
	const string file_mask = string("^") + tools_escape_chars_in_string(basename, "[].+|!*?{}()^$-,\\") + "\\.[0-9]+\\." + extension + "(\\.(md5|sha1|sha512))?$";
        if(tools_do_some_files_match_mask_regex(ent, file_mask))
        {
            if(!allow_overwriting)
                throw Erange("tools_avoid_slice_overwriting", tools_printf(dar_gettext("Overwriting not allowed while a slice of a previous archive with the same basename has been found in the %s directory, Operation aborted"), c_chemin.c_str()));
            else
            {
                try
                {
                    if(warn_overwriting)
                        dialog.pause(tools_printf(dar_gettext("At least one slice of an old archive with the same name remains in the directory %s. It is advised to remove all the old archive's slices before creating an archive of same name. Can I remove these old slices?"), c_chemin.c_str()));
                    if(!dry_run)
                        tools_unlink_file_mask_regex(dialog, ent, file_mask, info_details);
                }
                catch(Euser_abort & e)
                {
                        // nothing to do, just continue
                }
            }
        }
    }

    void tools_add_elastic_buffer(generic_file & f,
				  U_32 max_size,
				  U_32 modulo,
				  U_32 offset)
    {
	U_32 size = tools_pseudo_random(max_size-1) + 1; // range from 1 to max_size;

	if(modulo > 0)
	{
	    U_32 shift = modulo - (offset % modulo);
	    size = (size/modulo)*modulo + shift;
	}

        elastic tic = size;
        char *buffer = new (nothrow) char[tic.get_size()];

        if(buffer == nullptr)
            throw Ememory("tools_add_elastic_buffer");
        try
        {
            tic.dump((unsigned char *)buffer, tic.get_size());
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
        {
            string tmp = tools_strerror_r(errno);
            throw Erange("tools:tools_are_on_same_filesystem", tools_printf(dar_gettext("Cannot get inode information for %s: %s"), file1.c_str(), tmp.c_str()));
        }
        id = sstat.st_dev;

        if(stat(file2.c_str(), &sstat) < 0)
        {
            string tmp = tools_strerror_r(errno);
            throw Erange("tools:tools_are_on_same_filesystem", tools_printf(dar_gettext("Cannot get inode information for %s: %s"), file2.c_str(), tmp.c_str()));
        }

        return id == sstat.st_dev;
    }

    path tools_relative2absolute_path(const path & src, const path & cwd)
    {
        if(src.is_relative())
            if(cwd.is_relative())
                throw Erange("tools_relative2absolute_path", dar_gettext("Current Working Directory cannot be a relative path"));
            else
                return cwd + src;
        else
            return src;
    }

    void tools_block_all_signals(sigset_t &old_mask)
    {
        sigset_t all;

        sigfillset(&all);
#if HAVE_LIBPTHREAD
        if(pthread_sigmask(SIG_BLOCK, &all, &old_mask) != 0)
#else
            if(sigprocmask(SIG_BLOCK, &all, &old_mask) != 0)
#endif
                throw Erange("tools_block_all_signals", string(dar_gettext("Cannot block signals: "))+tools_strerror_r(errno));
    }

    void tools_set_back_blocked_signals(sigset_t old_mask)
    {
#if HAVE_LIBPTHREAD
        if(pthread_sigmask(SIG_SETMASK, &old_mask, nullptr))
#else
            if(sigprocmask(SIG_SETMASK, &old_mask, nullptr))
#endif
                throw Erange("tools_set_back_block_all_signals", string(dar_gettext("Cannot unblock signals: "))+tools_strerror_r(errno));
    }

    U_I tools_count_in_string(const string & s, const char a)
    {
        U_I ret = 0, c, size = s.size();

        for(c = 0; c < size; ++c)
            if(s[c] == a)
                ++ret;
        return ret;
    }

    datetime tools_get_mtime(user_interaction & dialog,
			     const std::string & s,
			     bool auto_zeroing,
			     bool silent,
			     const set<string> & ignored_as_symlink)
    {
        struct stat buf;
	bool use_stat = ignored_as_symlink.find(s) != ignored_as_symlink.end();
	int sysval;

	if(use_stat)
	    sysval = stat(s.c_str(), &buf);
	else
	    sysval = lstat(s.c_str(), &buf);

        if(sysval < 0)
        {
            string tmp = tools_strerror_r(errno);
            throw Erange("tools_get_mtime", tools_printf(dar_gettext("Cannot get last modification date: %s"), tmp.c_str()));
        }

#ifdef LIBDAR_MICROSECOND_READ_ACCURACY
	tools_check_negative_date(buf.st_mtim.tv_sec,
				  dialog,
				  s.c_str(),
				  "mtime",
				  auto_zeroing,
				  silent);
        datetime val = datetime(buf.st_mtim.tv_sec, buf.st_mtim.tv_nsec/1000, datetime::tu_microsecond);
        if(val.is_null() && !auto_zeroing) // assuming an error avoids getting time that way
            val = datetime(buf.st_mtime, 0, datetime::tu_second);
#else
	tools_check_negative_date(buf.st_mtime,
				  dialog,
				  s.c_str(),
				  "mtime",
				  auto_zeroing,
				  silent);
        datetime val = datetime(buf.st_mtime, 0, datetime::tu_second);
#endif

        return val;
    }

    infinint tools_get_size(const std::string & s)
    {
        struct stat buf;

        if(lstat(s.c_str(), &buf) < 0)
        {
            string tmp = tools_strerror_r(errno);
            throw Erange("tools_get_size", tools_printf(dar_gettext("Cannot get last modification date: %s"), tmp.c_str()));
        }

        if(!S_ISREG(buf.st_mode))
            throw Erange("tools_get_size", tools_printf(dar_gettext("Cannot get size of %S: not a plain file"), &s));

        return buf.st_size;
    }


    datetime tools_get_ctime(const std::string & s)
    {
        struct stat buf;

        if(lstat(s.c_str(), &buf) < 0)
        {
            string tmp = tools_strerror_r(errno);
            throw Erange("tools_get_mtime", tools_printf(dar_gettext("Cannot get mtime: %s"), tmp.c_str()));
        }

#ifdef LIBDAR_MICROSECOND_READ_ACCURACY
        datetime ret = datetime(buf.st_ctim.tv_sec, buf.st_ctim.tv_nsec/1000, datetime::tu_microsecond);
        if(ret.is_null()) // assuming an error avoids getting time that way
            ret = datetime(buf.st_ctime, 0, datetime::tu_second);
#else
        datetime ret = datetime(buf.st_ctime, 0, datetime::tu_second);
#endif
        return ret;
    }

    bool tools_find_next_char_out_of_parenthesis(const string & data, const char what,  U_32 start, U_32 & found)
    {
        U_32 nested_parenth = 0;

        while(start < data.size() && (nested_parenth != 0 || data[start] != what))
        {
            if(data[start] == '(')
                nested_parenth++;
            if(data[start] == ')' && nested_parenth > 0)
                nested_parenth--;
            start++;
        }

        if(start < data.size() && nested_parenth == 0 && data[start] == what)
        {
            found = start;
            return true;
        }
        else
            return false;
    }

    string tools_substitute(const string & hook,
                            const map<char, string> & corres)
    {
        string ret = "";
        string::iterator it = const_cast<string &>(hook).begin();

        while(it != hook.end())
        {
            if(*it == '%')
            {
                it++;
                if(it != hook.end())
                {
                    map<char, string>::const_iterator mptr = corres.find(*it);
                    if(mptr == corres.end())
                        throw Escript("tools_substitute", string(dar_gettext("Unknown substitution string: %")) + *it);
                    else
                        ret += mptr->second;
                    it++;
                }
                else // reached end of "hook" string
                {
                    throw Escript("tools_substitute", dar_gettext("last char of user command-line to execute is '%', (use '%%' instead to avoid this message)"));
                }
            }
            else
            {
                ret += *it;
                it++;
            }
        }

        return ret;
    }

    string tools_hook_substitute(const string & hook,
                                 const string & path,
                                 const string & basename,
                                 const string & num,
                                 const string & padded_num,
                                 const string & ext,
                                 const string & context,
				 const string & base_url)
    {
        map<char, string> corres;

        corres['%'] = "%";
        corres['p'] = path;
        corres['b'] = basename;
        corres['n'] = num;
        corres['N'] = padded_num;
        corres['e'] = ext;
        corres['c'] = context;
	corres['u'] = base_url;

        return tools_substitute(hook, corres);
    }


    void tools_hook_execute(user_interaction & ui,
                            const string & cmd_line)
    {
        NLS_SWAP_IN;
        try
        {
            const char *ptr = cmd_line.c_str();
            bool loop = false;
            do
            {
                try
                {
                    S_I code = system(ptr);
                    switch(code)
                    {
                    case 0:
                        loop = false;
                        break; // All is fine, script did not report error
                    case 127:
                        throw Erange("tools_hook_execute", gettext("execve() failed. (process table is full ?)"));
                    case -1:
                        throw Erange("tools_hook_execute", string(gettext("system() call failed: ")) + tools_strerror_r(errno));
                    default:
                        throw Erange("tools_hook_execute", tools_printf(gettext("execution of [ %S ] returned error code: %d"), &cmd_line, code));
                    }
                }
                catch(Erange & e)
                {
                    try
                    {
                        ui.pause(string(gettext("Error during user command line execution: ")) + e.get_message() + gettext(" . Retry command-line ?"));
                        loop = true;
                    }
                    catch(Euser_abort & f)
                    {
                        ui.pause(gettext("Ignore previous error on user command line and continue ?"));
                        loop = false;
                    }
                }
            }
            while(loop);
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
    }

    extern void tools_hook_substitute_and_execute(user_interaction & ui,
                                                  const string & hook,
                                                  const string & path,
                                                  const string & basename,
                                                  const string & num,
                                                  const string & padded_num,
                                                  const string & ext,
                                                  const string & context,
						  const string & base_url)
    {
        string cmd_line;

        cmd_line = tools_hook_substitute(hook,
                                         path,
                                         basename,
                                         num,
                                         padded_num,
                                         ext,
                                         context,
					 base_url);

        try
        {
            tools_hook_execute(ui, cmd_line);
        }
        catch(Euser_abort & g)
        {
            throw Escript("sar::hook_execute", string(dar_gettext("Fatal error on user command line: ")) + g.get_message());
        }
    }


        /*************************************************************/



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

    U_I tools_octal2int(const std::string & perm)
    {
        U_I len = perm.size();
        U_I ret = 0;
        enum { init , octal , trail, error } etat = init;

        if(perm == "")
            return 0666; // permission used by default (compatible with dar's previous behavior)

        for(U_I i = 0; i < len ; i++)
            switch(etat)
            {
            case init:
                switch(perm[i])
                {
                case ' ':
                case '\t':
                case '\n':
                case '\r':
                    break;
                case '0':
                    etat = octal;
                    break;
                default:
                    etat = error;
                    break;
                }
                break;
            case octal:
                if(perm[i] == ' ')
                    etat = trail;
                else
                    if(perm[i] >= '0' && perm[i] <= '7')
                        ret = ret*8 + perm[i] - '0';
                    else
                        etat = error;
                break;
            case trail:
                if(perm[i] != ' ')
                    etat = error;
                break;
            case error:
                throw Erange("tools_octal2int", dar_gettext("Badly formated octal number"));
            default:
                throw SRC_BUG;
            }

        if(etat == error || etat == init)
            throw Erange("tools_octal2int", dar_gettext("Badly formated octal number"));

        return ret;
    }

    string tools_int2octal(const U_I & perm)
    {
        deque<U_I> digits = tools_number_base_decomposition_in_big_endian(perm, (U_I)8);
        deque<U_I>::iterator it = digits.begin();
        string ret = "";

        while(it != digits.end())
        {
            string tmp;
            tmp += '0' + (*it);
            ret = tmp + ret;
            ++it;
        }

        return string("0") + ret;  // leading zero for octal format indication
    }

    string tools_get_permission_string(char type, U_32 perm, bool hard)
    {
        string ret = hard ? "*" : " ";

        if(type == 'f') // plain files
            type = '-';
        if(type == 'o') // door "files"
            type = 'D';
        ret += type;

        if((perm & 0400) != 0)
            ret += 'r';
        else
            ret += '-';
        if((perm & 0200) != 0)
            ret += 'w';
        else
            ret += '-';
        if((perm & 0100) != 0)
            if((perm & 04000) != 0)
                ret += 's';
            else
                ret += 'x';
        else
            if((perm & 04000) != 0)
                ret += 'S';
            else
                ret += '-';
        if((perm & 040) != 0)
            ret += 'r';
        else
            ret += '-';
        if((perm & 020) != 0)
            ret += 'w';
        else
            ret += '-';
        if((perm & 010) != 0)
            if((perm & 02000) != 0)
                ret += 's';
            else
                ret += 'x';
        else
            if((perm & 02000) != 0)
                ret += 'S';
            else
                ret += '-';
        if((perm & 04) != 0)
            ret += 'r';
        else
            ret += '-';
        if((perm & 02) != 0)
            ret += 'w';
        else
            ret += '-';
        if((perm & 01) != 0)
            if((perm & 01000) != 0)
                ret += 't';
            else
                ret += 'x';
        else
            if((perm & 01000) != 0)
                ret += 'T';
            else
                ret += '-';

        return ret;
    }


    U_I tools_get_permission(S_I fd)
    {
        struct stat buf;
        int err = fstat(fd, &buf);

        if(err < 0)
            throw Erange("tools_get_permission", string(gettext("Cannot get effective permission given a file descriptor: ")) + tools_strerror_r(errno));

        return buf.st_mode & ~(S_IFMT);
    }


    void tools_set_permission(S_I fd, U_I perm)
    {
        NLS_SWAP_IN;
        try
        {
            if(fd < 0)
                throw SRC_BUG;
            if(fchmod(fd, (mode_t) perm) < 0)
            {
                string tmp = tools_strerror_r(errno);
                throw Erange("tools_set_permission", tools_printf(gettext("Error while setting file permission: %s"), tmp.c_str()));
            }
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
    }

    uid_t tools_ownership2uid(const string & user)
    {
        uid_t ret = -1;

        NLS_SWAP_IN;
        try
        {
            bool direct_uid_set = false;

            if(user.empty())
                throw Erange("tools_ownership2uid", gettext("An empty string is not a valid user name"));

            try
            {
                ret = tools_str2int(user);
                direct_uid_set = true;
            }
            catch(Erange & e)
            {
                    // the given user is not an uid
            }

            if(!direct_uid_set)
            {
#ifdef __DYNAMIC__
		const char *c_user = user.c_str();
#if HAVE_GETPWNAM_R
		struct passwd puser;
		struct passwd *result;
		S_I size = sysconf(_SC_GETPW_R_SIZE_MAX);
		char *buf = nullptr;
		if(size == -1)
		    size = 16384;
		try
		{
		    buf = new (nothrow) char[size];
		    if(buf == nullptr)
			throw Ememory("tools_ownership2uid");

		    int val = getpwnam_r(c_user,
					 &puser,
					 buf,
					 size,
					 &result);

		    if(val != 0
		       || result == nullptr)
		    {
			string err = val == 0 ? gettext("Unknown user") : tools_strerror_r(errno);
			throw Erange("tools_ownership2uid",
				     tools_printf(gettext("Error found while looking for UID of user %s: %S"),
						  c_user,
						  &err));
		    }

		    ret = result->pw_uid;
		}
		catch(...)
		{
		    if(buf != nullptr)
			delete [] buf;
		    throw;
		}
		if(buf != nullptr)
		    delete [] buf;
#else
		errno = 0;
		struct passwd *puser = getpwnam(c_user);
		if(puser == nullptr)
		{
		    string err = (errno == 0) ? gettext("Unknown user") : tools_strerror_r(errno);
		    throw Erange("tools_ownership2uid",
				 tools_printf(gettext("Error found while looking for UID of user %s: %S"),
					      c_user,
					      &err));
		}

		ret = puser->pw_uid;
#endif
#else
		throw Erange("tools_ownership2uid", dar_gettext("Cannot convert username to uid in statically linked binary, either directly provide the UID or run libdar from a dynamically linked executable"));
 #endif
	    }
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;

        return ret;
    }


    gid_t tools_ownership2gid(const string & group)
    {
        gid_t ret = -1;

        NLS_SWAP_IN;
        try
        {
            bool direct_gid_set = false;

            if(group.empty())
                throw Erange("tools_ownership2gid", gettext("An empty string is not a valid group name"));

            try
            {
                ret = tools_str2int(group);
                direct_gid_set = true;
            }
            catch(Erange & e)
            {
                    // the given group is not an gid
            }

            if(!direct_gid_set)
            {
#ifdef __DYNAMIC__
                const char *c_group = group.c_str();
#if HAVE_GETGRNAM_R
		struct group pgroup;
		struct group *result;
		U_I size = sysconf(_SC_GETGR_R_SIZE_MAX);
		char *buf = nullptr;
		try
		{
		    buf = new (nothrow) char[size];
		    if(buf == nullptr)
			throw Ememory("tools_ownsership2gid");

		    S_I val = getgrnam_r(c_group,
					 &pgroup,
					 buf,
					 size,
					 &result);

		    if(val != 0
		       || result == nullptr)
		    {
			string err = (val == 0) ? gettext("Unknown group") : tools_strerror_r(errno);
			throw Erange("tools_ownership2gid",
				     tools_printf(gettext("Error found while looking fo GID of group %s: %S"),
						  c_group,
						  &err));
		    }

		    ret = result->gr_gid;

		}
		catch(...)
		{
		    if(buf != nullptr)
			delete [] buf;
		    throw;
		}
		if(buf != nullptr)
		    delete [] buf;
#else
		errno = 0;
		struct group *pgroup = getgrnam(c_group);
		if(pgroup == nullptr)
		{
		    string err = (errno == 0) ? gettext("Unknown group") : tools_strerror_r(errno);
		    throw Erange("tools_ownership2gid",
				 tools_printf(gettext("Error found while looking for GID of group %s: %S"),
					      c_group,
					      &err));
		}

		ret = pgroup->gr_gid;
#endif
#else
		throw Erange("tools_ownership2gid", dar_gettext("Cannot convert username to uid in statically linked binary, either directly provide the UID or run libdar from a dynamically linked executable"));
#endif
	    }
        }
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;

        return ret;
    }

    void tools_set_ownership(int filedesc, const std::string & user, const std::string & group)
    {
        uid_t uid = -1;
        uid_t gid = -1;

        if(user != "")
            uid = tools_ownership2uid(user);
        if(group != "")
            gid = tools_ownership2gid(group);

        if(uid != (uid_t)(-1) || gid != (gid_t)(-1))
        {
            if(fchown(filedesc, uid, gid) < 0)
            {
                string tmp = tools_strerror_r(errno);
                throw Erange("tools_set_ownership", tools_printf(gettext("Error while setting file user ownership: %s"), tmp.c_str()));
            }
        }
    }

    void tools_memxor(void *dest, const void *src, U_I n)
    {
        unsigned char *d = (unsigned char *) dest;
        const unsigned char *s = (const unsigned char *) src;

        for(U_I i = 0; i < n; i++)
            *d++ ^= *s++;
    }

    tlv_list tools_string2tlv_list(user_interaction & dialog, const U_16 & type, const vector<string> & data)
    {
        vector<string>::const_iterator it = data.begin();
        tlv tmp;
        tlv_list ret;

        tmp.set_type(type);
        while(it != data.end())
        {
            tmp.reset();
            tmp.write(it->c_str(), it->size());
            ret.add(tmp);
            it++;
        }
        return ret;
    }

    U_I tools_pseudo_random(U_I max)
    {
	return (U_I)(max*((float)(rand())/RAND_MAX));
    }

    void tools_read_from_pipe(user_interaction & dialog, S_I fd, tlv_list & result)
    {
        tuyau tube = tuyau(dialog, fd);
        result.read(tube);
    }

    string tools_unsigned_char_to_hexa(unsigned char x)
    {
        string ret;
        deque<U_I> digit = tools_number_base_decomposition_in_big_endian(x, (U_I)(16));
        deque<U_I>::reverse_iterator itr = digit.rbegin();

        switch(digit.size())
        {
        case 0:
            ret = "00";
            break;
        case 1:
            ret = "0";
            break;
        case 2:
            break;
        default:
            throw SRC_BUG;
        }

        while(itr != digit.rend())
        {
            U_I t = *itr;
            if(t > 9)
                ret += ('a' + (t - 10));
            else
                ret += ('0' + t);

            ++itr;
        }

        return ret;
    }

    string tools_string_to_hexa(const string & input)
    {
        string::const_iterator it = input.begin();
        string ret;

        while(it != input.end())
        {
            ret += tools_unsigned_char_to_hexa((unsigned char)(*it));
            ++it;
        }

        return ret;
    }

    infinint tools_file_size_to_crc_size(const infinint & size)
    {
        const infinint ratio = tools_get_extended_size("1G",1024);
        infinint r;
        infinint crc_size;

        if(!size.is_zero())
        {
            euclide(size, ratio, crc_size, r);
            if(!r.is_zero())
                ++crc_size;
            crc_size *= 4;   // smallest value is 4 bytes, 4 bytes more per each additional 1 Gbyte of data
        }
        else
            crc_size = 1; // minimal value for no data to protect by checksum

        return crc_size;
    }

    string tools_get_euid()
    {
        string ret;
        uid_t uid = geteuid();
        deci conv = infinint(uid);

        ret += tools_name_of_uid(uid) + "("+ conv.human() + ")";

        return ret;
    }

    string tools_get_egid()
    {
        string ret;
        uid_t gid = getegid();
        deci conv = infinint(gid);

        ret += tools_name_of_gid(gid) + "("+ conv.human() + ")";

        return ret;
    }

    string tools_get_hostname()
    {
        string ret;
        struct utsname uts;

        if(uname(&uts) < 0)
            throw Erange("tools_get_hostname", string(dar_gettext("Error while fetching hostname: ")) + tools_strerror_r(errno));

        ret = string(uts.nodename);

        return ret;
    }

    string tools_get_date_utc()
    {
        string ret;
        datetime now = datetime(::time(nullptr), 0, datetime::tu_second);

        ret = tools_display_date(now);

        return ret;
    }

    string tools_get_compression_ratio(const infinint & storage_size, const infinint & file_size, bool compressed)
    {
	static const char * not_compressed = "     ";

        if(!compressed)
            return not_compressed;
        else
            if(file_size >= storage_size)
		if(!file_size.is_zero())
		    return tools_addspacebefore(deci(((file_size - storage_size)*100)/file_size).human(), 4) +"%";
		else
		    return not_compressed;
            else
                return gettext("Worse");
    }

#define MSGSIZE 200

    string tools_strerror_r(int errnum)
    {
        char buffer[MSGSIZE];
        string ret;

#ifdef HAVE_STRERROR_R
#ifdef HAVE_STRERROR_R_CHAR_PTR
        char *val = strerror_r(errnum, buffer, MSGSIZE);
        if(val != buffer)
            strncpy(buffer, val, MSGSIZE);
#else
            // we expect the XSI-compliant strerror_r
        int val = strerror_r(errnum, buffer, MSGSIZE);
        if(val != 0)
	{
	    string tmp = tools_printf(gettext("Error code %d to message conversion failed"), errnum);
            strncpy(buffer, tmp.c_str(), tools_min((size_t)(tmp.size()+1), (size_t)(MSGSIZE)));
	}
#endif
#else
	char *tmp = strerror(errnum);
	(void)strncpy(buffer, tmp, MSGSIZE);
#endif
        buffer[MSGSIZE-1] = '\0';
        ret = buffer;

        return ret;
    }

#ifdef GPGME_SUPPORT
    string tools_gpgme_strerror_r(gpgme_error_t err)
    {
        char buffer[MSGSIZE];
        string ret;

        switch(gpgme_strerror_r(err, buffer, MSGSIZE))
        {
        case 0:
            break;
        case ERANGE:
            strncpy(buffer, "Lack of memory to display gpgme error message", MSGSIZE);
            break;
        default:
            throw SRC_BUG;
        }
        buffer[MSGSIZE-1] = '\0';
        ret = buffer;

        return ret;
    }
#endif


#if HAVE_WCHAR_H
    wstring tools_string_to_wstring(const string & val)
    {
        wstring ret;
        wchar_t *dst = new (nothrow) wchar_t[val.size() + 1];

        if(dst == nullptr)
            throw Ememory("tools_string_to_wcs");
        try
        {
            mbstate_t state_wc;
            const char *src = val.c_str();
            size_t len;

            memset(&state_wc, '\0', sizeof(state_wc)); // initializing the shift structure
            len = mbsrtowcs(dst, &src, val.size(), &state_wc);
            if(len == (size_t)-1)
                throw Erange("tools_string_to_wcs", string(gettext("Invalid wide-char found in string: ")) + tools_strerror_r(errno));
            dst[len] = '\0';

                // converting dst to wstring

            ret = dst;
        }
        catch(...)
        {
            if(dst != nullptr)
                delete [] dst;
            throw;
        }
        if(dst != nullptr)
            delete [] dst;

        return ret;
    }

    string tools_wstring_to_string(const wstring & val)
    {
        string ret;
        const wchar_t *src = val.c_str();
        mbstate_t state_wc;
        size_t len;

        memset(&state_wc, '\0', sizeof(state_wc)); // initializing the shift structure
        len = wcsrtombs(nullptr, &src, 0, &state_wc);
        if(len == (size_t)-1)
            throw SRC_BUG; // all components of wstring should be valid

        char *dst = new (nothrow) char[len + 1];
        if(dst == nullptr)
            throw Ememory("tools_wstring_to_string");
        try
        {
            size_t len2;
            memset(&state_wc, '\0', sizeof(state_wc)); // initializing the shift structure
            src = val.c_str();
            len2 = wcsrtombs(dst, &src, len, &state_wc);
            if(len != len2)
                throw SRC_BUG;
            if(len2 == (size_t)-1)
                throw SRC_BUG; // problem should have already raised above
            dst[len2] = '\0';

                // converting dst to string

            ret = dst;
        }
        catch(...)
        {
            if(dst != nullptr)
                delete [] dst;
            throw;
        }
        if(dst != nullptr)
            delete [] dst;

        return ret;
    }
#endif

    void tools_merge_to_deque(deque<string> & a, const deque<string> & b)
    {
        deque<string>::const_iterator ptrb = b.begin();

        while(ptrb != b.end())
        {
            deque<string>::const_iterator ptra = a.begin();

            while(ptra != a.end() && *ptra != *ptrb)
                ++ptra;

            if(ptra == a.end())
                a.push_back(*ptrb);

            ++ptrb;
        }
    }

    deque<string> tools_substract_from_deque(const deque<string> & a, const deque<string> & b)
    {
        deque<string> ret;
        deque<string>::const_iterator ptra = a.begin();

        while(ptra != a.end())
        {
            deque<string>::const_iterator ptrb = b.begin();

            while(ptrb != b.end() && *ptra != *ptrb)
                ++ptrb;

            if(ptrb == b.end())
                ret.push_back(*ptra);
            ++ptra;
        }

        return ret;
    }


    struct dirent *tools_allocate_struct_dirent(const std::string & path_name, U_64 & max_name_length)
    {
	struct dirent *ret;
	S_64 name_max = pathconf(path_name.c_str(), _PC_NAME_MAX);
	U_I len;

	if(name_max == -1)
	    name_max = NAME_MAX;
	if(name_max < NAME_MAX)
	    name_max = NAME_MAX;
	len = offsetof(struct dirent, d_name) + name_max + 1;

	ret = (struct dirent *) new (nothrow) char[len];
	if(ret == nullptr)
	    throw Ememory("tools_allocate_struc_dirent");
	memset(ret, '\0', len);
#ifdef _DIRENT_HAVE_D_RECLEN
	ret->d_reclen = (len / sizeof(long))*sizeof(long);
#endif
	max_name_length = name_max;

	return ret;
    }

    void tools_release_struct_dirent(struct dirent *ptr)
    {
	if(ptr != nullptr)
	    delete [] ((char *)(ptr));
    }

    void tools_secu_string_show(user_interaction & dialog, const string & msg, const secu_string & key)
    {
	string res = msg + tools_printf(" (size=%d) [", key.get_size());
	U_I max = key.get_size() - 1;

	for(U_I index = 0; index < max; ++index)
	    res += tools_printf(" %d |", key[index]);

	res += tools_printf(" %d ]", key[max]);
	dialog.warning(res);
    }

    void tools_unlink(const std::string & filename)
    {
	U_I ret = unlink(filename.c_str());

	if(ret != 0)
	{
	    string err = tools_strerror_r(errno);

	    throw Erange("tools_unlink", tools_printf(gettext("Error unlinking %S: %s"), &filename, &err));
	}
    }

    string tools_escape_chars_in_string(const string & val, const char *to_escape)
    {
	string ret;
	string::const_iterator it = val.begin();

	while(it != val.end())
	{
	    U_I curs = 0;
	    while(to_escape[curs] != '\0' && to_escape[curs] != *it)
		++curs;
	    if(to_escape[curs] != '\0')
		ret += "\\";
	    ret += *it;
	    ++it;
	}

	return ret;
    }

    bool tools_infinint2U_64(infinint val, U_64 & res)
    {
	res = 0;
	val.unstack(res);
	return val.is_zero();
    }

    infinint tools_double2infinint(double arg)
    {
	if(arg < 0)
	    throw Erange("tools_double2infinint", gettext("Cannot convert negative floating point value to unsigned (positive) integer"));

	U_I tmp = (U_I)arg;
	if(arg - (double)tmp > 0.5)
	    ++tmp;

	return infinint(tmp);
    }

} // end of namespace
