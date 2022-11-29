/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2022 Denis Corbin
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
#if HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "getopt_decision.h"
}

#include <new>
#include <algorithm>
#include <string>
#include <sstream>

#include "line_tools.hpp"
#include "tools.hpp"
#include "tuyau.hpp"
#include "fichier_local.hpp"
#include "etage.hpp"
#include "nls_swap.hpp"

using namespace std;
using namespace libdar;

////

#define YES_NO(x) (x ? gettext("YES") : gettext("NO"))
static string build(string::iterator a, string::iterator b);
static bool is_a_slice_available(user_interaction & ui, const string & base, const string & extension);
static string retreive_basename(const string & base, const string & extension);
static string retreive_basename2(const string & base);
static void tools_localtime(const time_t & timep, struct tm *result);
static char extract_octal_from_string(string::const_iterator & x);
static char extract_hexa_from_string(string::const_iterator & x);


////

argc_argv::argc_argv(S_I size)
{
    x_argc = size;
    if(x_argc > 0)
    {
	x_argv = new (nothrow) char *[size];
	if(x_argv == nullptr)
	    throw Ememory("argc_argv::argc_argv");

	for(S_I i = 0; i < x_argc; i++)
	    x_argv[i] = nullptr;
    }
    else
	x_argv = nullptr;
}

argc_argv::~argc_argv() noexcept(false)
{
    if(x_argv == nullptr && x_argc > 0)
	throw SRC_BUG;

    for(S_I i = 0; i < x_argc; i++)
	if(x_argv[i] != nullptr)
	{
	    delete [] x_argv[i];
	    x_argv[i] = nullptr;
	}

    if(x_argc > 0)
	delete [] x_argv;
}

void argc_argv::set_arg(const string & arg, S_I index)
{
    if(index >= x_argc)
	throw Erange("argc_argv::set_arg", gettext("Index out of range"));

    if(x_argv[index] != nullptr)
    {
	delete [] x_argv[index];
	x_argv[index] = nullptr;
    }

    x_argv[index] = new (nothrow) char[arg.size() + 1];
    if(x_argv[index] == nullptr)
	throw Ememory("argc_argv::set_arg");
    strncpy(x_argv[index], arg.c_str(), arg.size());
    x_argv[index][arg.size()] = '\0';
}

void argc_argv::set_arg(generic_file & f, U_I size, S_I index)
{
    if(index >= x_argc)
	throw Erange("argc_argv::set_arg", gettext("Index out of range"));

    if(x_argv[index] != nullptr)
    {
	delete [] x_argv[index];
	x_argv[index] = nullptr;
    }

    x_argv[index] = new (nothrow) char [size+1];
    if(x_argv[index] == nullptr)
	throw Ememory("argc_argv::set_arg");

    x_argv[index][f.read(x_argv[index], size)] = '\0';
}

void argc_argv::resize(S_I size)
{
    char **tmp = nullptr;

    if(size == x_argc)
	return;

    if(size < x_argc)
	for(S_I i = size; i < x_argc; i++)
	    if(x_argv[i] != nullptr)
	    {
		delete [] x_argv[i];
		x_argv[i] = nullptr;
	    }

    tmp = new (nothrow) char*[size];
    if(tmp == nullptr)
	throw Ememory("argc_argv::resize");

    try
    {
	S_I min = size < x_argc ? size : x_argc;

	for(S_I i = 0; i < min; i++)
	    tmp[i] = x_argv[i];

	for(S_I i = min; i < size; i++)
	    tmp[i] = nullptr;

	if(x_argc > 0)
	    delete [] x_argv;
	x_argv = tmp;
	tmp = nullptr;
	x_argc = size;
    }
    catch(...)
    {
	if(tmp != nullptr)
	    delete [] tmp;
	throw;
    }
}

void line_tools_slice_ownership(const string & cmd, string & slice_permission, string & slice_user_ownership, string & slice_group_ownership)
{
    string::iterator s1, s2;

    s1 = const_cast<string *>(&cmd)->begin();

	// looking for the first ':'
    while(s1 != cmd.end() && *s1 != ':')
	s1++;

    s2 = s1;
    if(s2 != cmd.end())
	s2++;

	// looking for the second ':'
    while(s2 != cmd.end() && *s2 != ':')
	s2++;

    if(s1 == cmd.begin())
	slice_permission = "";
    else
	slice_permission = build(const_cast<string *>(&cmd)->begin(), s1);
    if(s1 == cmd.end())
	slice_user_ownership = "";
    else
	slice_user_ownership = build(s1+1, s2);
    if(s2 == cmd.end())
	slice_group_ownership = "";
    else
	slice_group_ownership = build(s2+1, const_cast<string *>(&cmd)->end());
}

void line_tools_repeat_param(const string & cmd, infinint & repeat_count, infinint & repeat_byte)
{
    string::iterator s1;
    string tmp1, tmp2;

    s1 = const_cast<string *>(&cmd)->begin();

	// looking for the first ':'
    while(s1 != cmd.end() && *s1 != ':')
	s1++;

    if(s1 != cmd.end()) // thus *s1 == ':'
    {
	tmp1 = build(const_cast<string *>(&cmd)->begin(), s1);
	tmp2 = build(s1+1, const_cast<string *>(&cmd)->end());
    }
    else
    {
	tmp1 = cmd;
	tmp2 = "0";
    }

    try
    {
	    // note that the namespace specification is necessary
	    // due to similar existing name in std namespace under
	    // certain OS (FreeBSD 10.0)
	libdar::deci x1 = tmp1;
	libdar::deci x2 = tmp2;

	repeat_count = x1.computer();
	repeat_byte = x2.computer();
    }
    catch(Edeci & e)
    {
	throw Erange("line_tools_repeat_param", string(gettext("Syntax error in --retry-on-change argument: ")) + e.get_message());
    }
}

void line_tools_tlv_list2argv(user_interaction & dialog, tlv_list & list, argc_argv & arg)
{
    memory_file mem = memory_file();
    U_I transfert = 0;
    infinint size;

    arg.resize(list.size());
    for(S_I i = 0; i < arg.argc() ; i++)
    {
	if(list[i].get_type() != 0)
		// we only use type 0 here
	    throw Erange("line_tools_tlv_list2argv", gettext("Unknown TLV record type"));
	size = list[i].size();
	transfert = 0;
	size.unstack(transfert);
	if(!size.is_zero())
	    throw Erange("line_tools_tlv_list2argv", "Too long argument found in TLV to be handled by standard library routine");
	list[i].skip(0);
	arg.set_arg(list[i], transfert, i);
    }
}

S_I line_tools_reset_getopt()
{
    S_I ret = optind;

#if HAVE_OPTRESET
    optreset = 1;
    optind = 1;
#else
    optind = 0;
#endif

    return ret;
}

string::const_iterator line_tools_find_first_char_out_of_parenth(const string & argument, unsigned char to_find)
{
    string::const_iterator it = argument.begin();
    U_I parenth = 0;

    while(it != argument.end() && (*it != to_find || parenth > 0))
    {
	switch(*it)
	{
	case '(':
	    ++parenth;
	    break;
	case ')':
 	    if(parenth > 0)
		--parenth;
	    else
		throw Erange("line_tools_find_first_char_out_of_parenth", string(gettext("Unbalanced parenthesis in expression: ")) + argument);
	    break;

		// no default: statement needed
	}
	++it;
    }

    return it;
}

string::const_iterator line_tools_find_last_char_out_of_parenth(const string & argument, unsigned char to_find)
{
    string::const_iterator it = argument.begin();
    string::const_iterator back = it;
    U_I parenth = 0;

    while(it != argument.end())
    {
	if(*it == to_find && parenth == 0)
	    back = it;
	switch(*it)
	{
	case '(':
	    ++parenth;
	    break;
	case ')':
 	    if(parenth > 0)
		--parenth;
	    else
		throw Erange("line_tools_find_first_char_out_of_parenth", string(gettext("Unbalanced parenthesis in expression: ")) + argument);
	    break;

		// no default: statement needed
	}
	++it;
    }

    if(back == argument.begin())
	if(back != argument.end() && *back != to_find)
	    back = argument.end();

    return back;
}

string line_tools_expand_user_comment(const string & user_comment, S_I argc, char *const argv[])
{
    string::const_iterator it = user_comment.begin();
    string::const_iterator st = it;
    string ret = "";

    while(it != user_comment.end())
    {
	if(*it == '%')
	{
	    it++;
	    if(it != user_comment.end())
	    {
		ret += string(st, it - 1);
		st = it+1;

		switch(*it)
		{
		case 'c':
		    ret += string("\"") + argv[0] + "\"";
		    for(S_I i = 1; i < argc; ++i)
			if(strcmp(argv[i], "-K") == 0
			   || strcmp(argv[i], "-J") == 0
			   || strcmp(argv[i], "-$") == 0
			   || strcmp(argv[i], "-#") == 0
			   || strcmp(argv[i], "-*") == 0
			   || strcmp(argv[i], "-%") == 0
			   || strcmp(argv[i], "--key") == 0
			   || strcmp(argv[i], "--ref-key") == 0
			   || strcmp(argv[i], "--aux-key") == 0
			   || strcmp(argv[i], "--crypto-block") == 0
			   || strcmp(argv[i], "--ref-crypto-block") == 0
			   || strcmp(argv[i], "--aux-crypto-block") == 0)
			    ++i;
			else
			    ret += string(" \"") + argv[i] + "\"";
		    break;
		case 'd':
		    ret += line_tools_get_date_utc();
		    break;
		case 'u':
		    ret += line_tools_get_euid();
		    break;
		case 'g':
		    ret += line_tools_get_egid();
		    break;
		case 'h':
		    ret += line_tools_get_hostname();
		    break;
		case '%':
		    ret += "%";
		    break;
		default:
		    throw Erange("line_tools_expand_user_comment", tools_printf(gettext("Unknown macro %%%d in user comment"), *it));
		}
	    }
	}

	it++;
    }


    if(st != user_comment.end())
	ret += string(st, user_comment.end());

    return ret;
}

deque<string> line_tools_explode_PATH(const char *the_path)
{
    deque<string> ret;
    const char *it = the_path;
    const char *last = it;

    ret.clear();
    if(it == nullptr)
	return ret;

    while(*it != '\0')
    {
	if(*it == ':')
	{
	    ret.push_back(string(last, it));
	    ++it;
	    last = it;
	}
	else
	    ++it;
    }

    ret.push_back(string(last, it));

    return ret;
}

string line_tools_get_full_path_from_PATH(const deque<string> & the_path, const char * filename)
{
    string ret;
    bool no_path = false;

    if(filename == nullptr)
	throw SRC_BUG;
    else
    {
	try
	{
	    path tmp(filename);

	    if(!tmp.is_relative())
		no_path = false; // no need to check if file exist using the_path, path is absolute
	    else
		if(tmp.degre() != 1)
		    no_path = false;  // this is a composed path, we must not inspect the_path
		else
		    no_path = true; // this is just the filename without path indication, so we check the_path
	}
	catch(Erange & e)
	{
	    if(e.get_source() == "path::path")
		no_path = false; // not a valid path
	    else
		throw;
	}
    }

    if(!no_path)
	ret = filename;
    else
    {
	deque<string>::const_iterator it = the_path.begin();
	bool found = false;

	while(!found && it != the_path.end())
	{
	    try
	    {
		string where = (path(*it).append(filename)).display();

		fichier_local tmp = fichier_local(where, false);
		found = true;
		ret = where;
	    }
	    catch(...)
	    {
		++it;
	    }
	}

	if(!found)
	    ret = filename;
    }

    return ret;
}

void line_tools_split_at_first_space(const char *field, string & before_space, string & after_space)
{
    const char *ptr = field;

    if(field == nullptr)
	throw SRC_BUG;
    while(*ptr != ' ' && *ptr != '\0')
	++ptr;

    if(*ptr == '\0')
    {
	before_space = field;
	after_space = "";
    }
    else
    {
	before_space = string(field, ptr);
	after_space = string(ptr + 1);
    }
}

void line_tools_get_min_digits(string the_arg, infinint & num, infinint & ref_num, infinint & aux_num)
{
    num = ref_num = aux_num = 0;
    string::iterator it1, it2;


    if(the_arg == "")
	return;

    try
    {
	it1 = line_tools_find_first_char_of(the_arg, ',');
	if(it1 == the_arg.end()) // a single number is provided
	{
		// note that the namespace specification is necessary
		// due to similar existing name in std namespace under
		// certain OS (FreeBSD 10.0)
	    libdar::deci tmp = the_arg;
	    num = tmp.computer();
	}
	else // at least two numbers are provided
	{
	    if(the_arg.begin() != it1)
	    {
		    // note that the namespace specification is necessary
		    // due to similar existing name in std namespace under
		    // certain OS (FreeBSD 10.0)
		libdar::deci convert = string(the_arg.begin(), it1);
		num = convert.computer();
	    }
		// else we ignore the leading ','

	    ++it1;
	    if(it1 == the_arg.end())
		return; // trailing ',' has to be ignored
	    string tmp2 = string(it1, the_arg.end());
	    it2 = line_tools_find_first_char_of(tmp2, ',');
	    if(it2 == tmp2.end()) // just two number have been provided
	    {
		    // note that the namespace specification is necessary
		    // due to similar existing name in std namespace under
		    // certain OS (FreeBSD 10.0)
		libdar::deci convert = tmp2;
		ref_num = convert.computer();
	    }
	    else
	    {
		if(tmp2.begin() != it2)
		{
			// note that the namespace specification is necessary
			// due to similar existing name in std namespace under
			// certain OS (FreeBSD 10.0)
		    libdar::deci convert = string(tmp2.begin(), it2);
		    ref_num = convert.computer();
		}
		++it2;
		if(it2 != tmp2.end())
		{
			// note that the namespace specification is necessary
			// due to similar existing name in std namespace under
			// certain OS (FreeBSD 10.0)
		    libdar::deci convert = string(it2, tmp2.end());
		    aux_num = convert.computer();
		}
	    }
	}
    }
    catch(Edeci & e)
    {
	throw Erange("line_tools_get_min_digits", tools_printf(gettext("Invalid number in string: %S"), &the_arg));
    }
}

void line_tools_look_for(const deque<char> & arguments,
			 S_I argc,
			 char *const argv[],
			 const char *getopt_string,
#if HAVE_GETOPT_LONG
			 const struct option *long_options,
#endif
			 char stop_scan,
			 deque<char> & presence)
{
    S_I lu;
    presence.clear();

    (void)line_tools_reset_getopt();
#if HAVE_GETOPT_LONG
    const struct option *ptr_long_opt = long_options;
    const struct option voided = { nullptr, 0, nullptr, 0 };
    if(long_options == nullptr)
	ptr_long_opt = &voided;

    while((lu = getopt_long(argc, argv, getopt_string, ptr_long_opt, nullptr)) != EOF && stop_scan != lu)
#else
	while((lu = getopt(argc, argv, getopt_string)) != EOF && stop_scan != lu)
#endif
	{
	    deque<char>::const_iterator it = find(arguments.begin(), arguments.end(), (char)lu);

	    if(it != arguments.end())
		presence.push_back(lu);
	}
    (void)line_tools_reset_getopt();
}

void line_tools_look_for_Q(S_I argc,
			   char *const argv[],
			   const char *getopt_string,
#if HAVE_GETOPT_LONG
			   const struct option *long_options,
#endif
			    char stop_scan,
			    bool & Q_is_present)
{
    deque<char> arguments;
    deque<char> presence;
    deque<char>::const_iterator it;

    Q_is_present = false;

    arguments.push_back('j');
    arguments.push_back('Q');
    line_tools_look_for(arguments,
			argc,
			argv,
			getopt_string,
#if HAVE_GETOPT_LONG
			long_options,
#endif
			stop_scan,
			presence);

    it = find(presence.begin(), presence.end(), 'Q');
    if(it != presence.end())
	Q_is_present = true;
}

set<string> line_tools_deque_to_set(const deque<string> & list)
{
    set<string> ret;
    deque<string>::const_iterator it = list.begin();

    while(it != list.end())
    {
	ret.insert(*it);
	++it;
    }

    return ret;
}

void line_tools_4_4_build_compatible_overwriting_policy(bool allow_over,
							bool detruire,
							bool more_recent,
							const infinint & hourshift,
							bool ea_erase,
							const crit_action * & overwrite)
{
    crit_action *tmp1 = nullptr;
    crit_action *tmp2 = nullptr; // tmp1 and tmp2 are used for construction of the overwriting policy
    overwrite = nullptr;

    try
    {
	if(allow_over)
	{
	    if(ea_erase)
		overwrite = new (nothrow) crit_constant_action(data_overwrite, EA_overwrite);
	    else
		overwrite = new (nothrow) crit_constant_action(data_overwrite, EA_merge_overwrite);
	    if(overwrite == nullptr)
		throw Ememory("tools_build_compatible_overwriting_policy");

	    tmp1 = new (nothrow) crit_constant_action(data_preserve, EA_preserve);
	    if(tmp1 == nullptr)
		throw Ememory("tools_build_compatible_overwriting_policy");

	    if(more_recent)
	    {
		tmp2 = new (nothrow) testing(crit_in_place_data_more_recent(hourshift), *tmp1, *overwrite);
		if(tmp2 == nullptr)
		    throw Ememory("tools_build_compatible_overwriting_policy");

		delete overwrite;
		overwrite = tmp2;
		tmp2 = nullptr;
	    }

	    if(!detruire)
	    {
		tmp2 = new (nothrow) testing(crit_invert(crit_in_place_is_inode()), *overwrite, *tmp1);
		if(tmp2 == nullptr)
		    throw Ememory("tools_build_compatible_overwriting_policy");
		delete overwrite;
		overwrite = tmp2;
		tmp2 = nullptr;
	    }

	    delete tmp1;
	    tmp1 = nullptr;
	}
	else
	{
	    overwrite = new (nothrow) crit_constant_action(data_preserve, EA_preserve);
	    if(overwrite == nullptr)
		throw Ememory("tools_build_compatible_overwriting_policy");
	}

	if(overwrite == nullptr)
	    throw SRC_BUG;
	if(tmp1 != nullptr)
	    throw SRC_BUG;
	if(tmp2 != nullptr)
	    throw SRC_BUG;
    }
    catch(...)
    {
	if(tmp1 != nullptr)
	    delete tmp1;
	if(tmp2 != nullptr)
	    delete tmp2;
	if(overwrite != nullptr)
	{
	    delete overwrite;
	    overwrite = nullptr;
	}
	throw;
    }
}

void line_tools_crypto_split_algo_pass(const secu_string & all,
				       crypto_algo & algo,
				       secu_string & pass,
				       bool & no_cipher_given,
				       vector<string> & recipients)
{
	// split from "algo:pass" "algo" "pass" "algo:" ":pass" syntaxes

    const char *it = all.c_str();
    const char *fin = all.c_str() + all.get_size(); // points past the last byte of the secu_string "all"
    secu_string tmp;
    recipients.clear();

    if(all.get_size() == 0)
    {
	algo = crypto_algo::none;
	pass.clear();
    }
    else
    {
	while(it != fin && *it != ':')
	    ++it;

	if(it != fin) // a ':' is present in the given string
	{
		// tmp holds the string before ':'
	    tmp = secu_string(all.c_str(), it - all.c_str());
	    ++it;

		// pass holds the string after ':'
	    pass = secu_string(it, fin - it);

	    no_cipher_given = (tmp == "");

	    if(tmp == "scrambling" || tmp == "scram")
		algo = crypto_algo::scrambling;
	    else
		if(tmp == "none")
		    algo = crypto_algo::none;
		else
		    if(tmp == "blowfish" || tmp == "bf")
			algo = crypto_algo::blowfish;
		    else
			if(tmp == "aes" || tmp == "aes256" || tmp == "")
			    algo = crypto_algo::aes256; // AES is the default cypher ("")
			else
			    if(tmp == "twofish" || tmp == "twofish256")
				algo = crypto_algo::twofish256;
			    else
				if(tmp == "serpent" || tmp == "serpent256")
				    algo = crypto_algo::serpent256;
				else
				    if(tmp == "camellia" || tmp == "camellia256")
					algo = crypto_algo::camellia256;
				    else
					if(tmp == "gnupg")
					{
					    secu_string emails;
					    line_tools_crypto_split_algo_pass(pass, algo, emails, no_cipher_given, recipients);
					    line_tools_split(emails.c_str(), ',', recipients);
					    pass.clear();
					}
					else
					    throw Erange("crypto_split_algo_pass", string(gettext("unknown cryptographic algorithm: ")) + tmp.c_str());
	}
	else // no ':' using blowfish as default cypher
	{
	    no_cipher_given = true;
	    algo = crypto_algo::aes256;
	    pass = all;
	}
    }
}

void line_tools_display_signatories(user_interaction & ui, const list<signator> & gnupg_signed)
{
    list<signator>::const_iterator it = gnupg_signed.begin();
    string tmp;

    if(gnupg_signed.empty())
	return;

    ui.printf(        "+-----------------+----------------+----------------------------------------+-------------------------+");
    ui.printf(gettext("| Signature Status|  Key Status    |  Finger Print                          | Signature Date          |"));
    ui.printf(        "+-----------------+----------------+----------------------------------------+-------------------------+");
    while(it != gnupg_signed.end())
    {
	tmp = "";

	switch(it->result)
	{
	case signator::good:
	    tmp += "|      Good       |";
	    break;
	case signator::bad:
	    tmp += "|    BAD !!!      |";
	    break;
	case signator::unknown_key:
	    tmp += "|   Unknown Key   |";
	    break;
	case signator::error:
	    tmp += "|  System Error   |";
	    break;
	default:
	    throw SRC_BUG;
	}

	switch(it->key_validity)
	{
	case signator::valid:
	    tmp += "    Valid       |";
	    break;
	case signator::expired:
	    tmp += "   EXPIRED      |";
	    break;
	case signator::revoked:
	    tmp += "   REVOKED      |";
	    break;
	default:
	    throw SRC_BUG;
	}
	tmp += it->fingerprint + "|";
	tmp += tools_display_date(it->signing_date) + " |";
	ui.message(tmp);
	if(it->key_validity == signator::expired)
	{
	    tmp = "                  |" + tools_display_date(it->signature_expiration_date);
	    ui.message(tmp);
	}

	++it;
    }
    ui.printf("------------------+----------------+----------------------------------------+-------------------------+");
    ui.printf(" For more information about a key, use the command: gpg --list-key <fingeprint>");
}

void line_tools_read_from_pipe(shared_ptr<user_interaction> & dialog, S_I fd, tlv_list & result)
{
    tuyau tube = tuyau(dialog, fd);
    result.read(tube);
}

void line_tools_extract_basename(const char *command_name, string &basename)
{
    basename = path(command_name).basename();
}

string::iterator line_tools_find_first_char_of(string &s, unsigned char v)
{
    string::iterator it = s.begin();

    while(it != s.end() && *it != v)
	++it;

    return it;
}

string::iterator line_tools_find_last_char_of(string & s, unsigned char v)
{
    string::iterator ret = s.end();
    string::iterator it = s.begin();

    while(it != s.end())
    {
	if(*it == v)
	    ret = it;
	++it;
    }

    return ret;
}

void line_tools_split_path_basename(const char *all, path * &chemin, string & base)
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
	throw Ememory("line_tools_split_path_basename");
}

void line_tools_split_path_basename(const string & all, string & chemin, string & base)
{
    path *tmp = nullptr;

    line_tools_split_path_basename(all.c_str(), tmp, base);
    if(tmp == nullptr)
	throw SRC_BUG;
    chemin = tmp->display();
    delete tmp;
}
bool line_tools_split_entrepot_path(const string &all,
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

    enum { s_proto, s_login, s_login_escape, s_pass, s_hote, s_port, s_path, s_end } state = s_proto;

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
		state = s_hote;
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
	    case '\\':
		++cursor;
		state = s_login_escape;
		break;
	    default:
		login += ch[cursor];
		++cursor;
	    }
	    break;
	case s_login_escape:
	    login += ch[cursor];
	    ++cursor;
	    state = s_login;
	    break;
	case s_pass:
	    switch(ch[cursor])
	    {
	    case '@':
		state = s_hote;
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
	case s_hote:
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

S_I line_tools_str2signed_int(const string & x)
{
    stringstream tmp(x);
    S_I ret;
    string residu;

    if((tmp >> ret).fail())
	throw Erange("line_tools_str2string", string(dar_gettext("Invalid number: ")) + x);

    tmp >> residu;
    for(U_I i = 0; i < residu.size(); ++i)
	if(residu[i] != ' ')
	    throw Erange("line_tools_str2string", string(dar_gettext("Invalid number: ")) + x);

    return ret;
}

infinint line_tools_convert_date(const string & repres)
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
		    throw Erange("line_tools_convert_date", dar_gettext("date before 1970 is not allowed"));
		when.tm_year = tmp - 1900;
		break;
	    case month:
		if(tmp < 1 || tmp > 12)
		    throw Erange("line_tools_convert_date", dar_gettext("Incorrect month"));
		when.tm_mon = tmp - 1;
		break;
	    case day:
		if(tmp < 1 || tmp > 31)
		    throw Erange("line_tools_convert_date", dar_gettext("Incorrect day of month"));
		when.tm_mday = tmp;
		break;
	    case hour:
		if(tmp < 0 || tmp > 23)
		    throw Erange("line_tools_convert_date", dar_gettext("Incorrect hour"));
		when.tm_hour = tmp;
		break;
	    case min:
		if(tmp < 0 || tmp > 59)
		    throw Erange("line_tools_convert_date", dar_gettext("Incorrect minute"));
		when.tm_min = tmp;
		break;
	    case sec:
		if(tmp < 0 || tmp > 59)
		    throw Erange("line_tools_convert_date", dar_gettext("Incorrect second"));
		when.tm_sec = tmp;
		break;
	    case error:
		throw Erange("line_tools_convert_date", dar_gettext("Bad formatted date expression"));
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
	throw Erange("line_tools_convert_date", dar_gettext("Given date must be in the past"));
    ret = when;

    return ret;
}

void line_tools_display_features(user_interaction & dialog)
{
    NLS_SWAP_IN;
    try
    {
	const char *endy = nullptr;
	string time_accuracy = "";
	string threadar_version = "";
	string curl_version = "";

	dialog.printf(gettext("   gzip compression (libz)      : %s"), YES_NO(compile_time::libz()));
	dialog.printf(gettext("   bzip2 compression (libbzip2) : %s"), YES_NO(compile_time::libbz2()));
	dialog.printf(gettext("   lzo compression (liblzo2)    : %s"), YES_NO(compile_time::liblzo()));
	dialog.printf(gettext("   xz compression (liblzma)     : %s"), YES_NO(compile_time::libxz()));
	dialog.printf(gettext("   zstd compression (libzstd)   : %s"), YES_NO(compile_time::libzstd()));
	dialog.printf(gettext("   lz4 compression (liblz4)     : %s"), YES_NO(compile_time::liblz4()));
	dialog.printf(gettext("   Strong encryption (libgcrypt): %s"), YES_NO(compile_time::libgcrypt()));
	dialog.printf(gettext("   Public key ciphers (gpgme)   : %s"), YES_NO(compile_time::public_key_cipher()));
	dialog.printf(gettext("   Extended Attributes support  : %s"), YES_NO(compile_time::ea()));
	dialog.printf(gettext("   Large files support (> 2GB)  : %s"), YES_NO(compile_time::largefile()));
	dialog.printf(gettext("   ext2fs NODUMP flag support   : %s"), YES_NO(compile_time::nodump()));
	if(compile_time::bits() == 0)
	    dialog.printf(gettext("   Integer size used            : unlimited"));
	else
	    dialog.printf(gettext("   Integer size used            : %d bits"), compile_time::bits());
	dialog.printf(gettext("   Thread safe support          : %s"), YES_NO(compile_time::thread_safe()));
	dialog.printf(gettext("   Furtive read mode support    : %s"), YES_NO(compile_time::furtive_read()));
	dialog.printf(gettext("   Linux ext2/3/4 FSA support   : %s"), YES_NO(compile_time::FSA_linux_extX()));
	dialog.printf(gettext("   Mac OS X HFS+ FSA support    : %s"), YES_NO(compile_time::FSA_birthtime()));
	dialog.printf(gettext("   Linux statx() support        : %s"), YES_NO(compile_time::Linux_statx()));

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
	if(compile_time::nanosecond_read())
	    time_accuracy = "1 nanosecond";
	else
	{
	    if(compile_time::microsecond_read())
		time_accuracy = "1 microsecond";
	    else
		time_accuracy = "1 second";
	}
	dialog.printf(gettext("   Timestamp read accuracy      : %S"), &time_accuracy);
	if(compile_time::nanosecond_write())
	    time_accuracy = "1 nanosecond";
	else
	{
	    if(compile_time::microsecond_write())
		time_accuracy = "1 microsecond";
	    else
		time_accuracy = "1 second";
	}
	dialog.printf(gettext("   Timestamp write accuracy     : %S"), &time_accuracy);
	dialog.printf(gettext("   Restores dates of symlinks   : %s"), YES_NO(compile_time::symlink_restore_dates()));
	if(compile_time::libthreadar())
	    threadar_version = string("(") + compile_time::libthreadar_version() + ")";
	else
	    threadar_version = "";
	dialog.printf(gettext("   Multiple threads (libthreads): %s %s"), YES_NO(compile_time::libthreadar()), threadar_version.c_str());
	dialog.printf(gettext("   Delta compression (librsync) : %s"), YES_NO(compile_time::librsync()));
	if(compile_time::remote_repository())
	    curl_version = string("(") + compile_time::libcurl_version() + ")";
	else
	    curl_version = "";
	dialog.printf(gettext("   Remote repository (libcurl)  : %s %s"), YES_NO(compile_time::remote_repository()), curl_version.c_str());
	dialog.printf(gettext("   argon2 hashing (libargon2)   : %s"), YES_NO(compile_time::libargon2()));
	dialog.printf(gettext("   Whirlpool hashing (librhash) : %s"), YES_NO(compile_time::whirlpool_hash()));
    }
    catch(...)
    {
	NLS_SWAP_OUT;
	throw;
    }
    NLS_SWAP_OUT;
}

const char *line_tools_get_from_env(const char **env, const char *clef)
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

void line_tools_check_basename(user_interaction & dialog, const path & loc, string & base, const string & extension, bool create)
{
    NLS_SWAP_IN;
    try
    {
	regular_mask suspect(string(".+\\.[0-9]+\\.")+extension+string("$"), true);
	regular_mask suspect2(string("\\.0*$"), true);
	string old_path = (loc.append(base)).display();

	    // is there a slice available ?
	if(is_a_slice_available(dialog, old_path, extension))
	    return; // yes, thus basename is not a mistake

	    // is basename is suspect
	if(suspect.is_covered(base)) // full slice name
	{
		// removing the suspicious end (.<number>.extension)
		// and checking the avaibility of such a slice

	    string new_base = retreive_basename(base, extension);
	    string new_path = (loc.append(new_base)).display();

	    if(is_a_slice_available(dialog, new_path, extension))
	    {
		dialog.message(tools_printf(gettext("Warning, no archive exist with \"%S\" as basename, however a slice part of an archive which basename is \"%S\" exists. Assuming a shell completion occurred and using that basename instead"), &base, &new_base));
		base = new_base;
	    }
	    else // no slice exist neither with that slice name
	    {
		try
		{
		    if(create)
		    {
			dialog.pause(tools_printf(gettext("Warning, the provided basename \"%S\" may lead to confusion as it looks like a slice name. It is advise to rather use \"%S\" as basename instead. Do you want to change the basename accordingly?"), &base, &new_base));
			base = new_base;
		    }
		}
		catch(Euser_abort & e)
		{
		    dialog.message(tools_printf(gettext("OK, keeping %S as basename"), &base));
		}
	    }
	}
	else
	    if(suspect2.is_covered(base))
	    {
		    // shell completed up to the slice number in a file

		string new_base = retreive_basename2(base); // removing the ending dot eventually followed by zeros
		string new_path = (loc.append(new_base)).display();

		if(is_a_slice_available(dialog, new_path, extension))
		{
		    dialog.message(tools_printf(gettext("Warning, no archive exist with \"%S\" as basename, however a slice part of an archive which basename is \"%S\" exists. Assuming a shell completion occurred and using that basename instead"), &base, &new_base));
		    base = new_base;
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

void line_tools_check_min_digits(user_interaction & dialog, const path & loc, const string & base, const string & extension, infinint & num_digits)
{
    bool found = false;
    string cur;

    etage contents(dialog, loc.display().c_str(), datetime(0), datetime(0), false, false);

    regular_mask slice = regular_mask(base + "\\.0+[1-9][0-9]*\\." + extension + "$", true);

    while(!found && contents.read(cur))
    {
	found = slice.is_covered(cur);
	if(found)
	{
		// extract the number of digits from the slice name
	    S_I index_last = cur.find_last_not_of(string(".")+extension);
	    cur = string(cur.begin(), cur.begin() + index_last + 1);
	    S_I index_first = cur.find_last_not_of("0123456789");
	    if(num_digits.is_zero() && index_last - index_first > 1)
	    {
		num_digits = index_last - index_first;
		dialog.printf(gettext("Auto detecting min-digits to be %i"), &num_digits);
	    }
	}
    }
}

string line_tools_getcwd()
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
		throw Ememory("line_tools_getcwd()");
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
		    throw Erange("line_tools_getcwd", string(dar_gettext("Cannot get full path of current working directory: ")) + tools_strerror_r(errno));
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

void line_tools_read_range(const string & s, S_I & min, U_I & max)
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
	min = line_tools_str2signed_int(s);
	max = 0;
    }
}

string line_tools_build_regex_for_exclude_mask(const string & prefix,
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

string line_tools_get_euid()
{
    string ret;
    uid_t uid = geteuid();
    libdar::deci conv = infinint(uid);

    ret += tools_name_of_uid(uid) + "("+ conv.human() + ")";

    return ret;
}

string line_tools_get_egid()
{
    string ret;
    uid_t gid = getegid();
    libdar::deci conv = infinint(gid);

    ret += tools_name_of_gid(gid) + "("+ conv.human() + ")";

    return ret;
}

string line_tools_get_hostname()
{
    string ret;
    struct utsname uts;

    if(uname(&uts) < 0)
	throw Erange("line_tools_get_hostname", string(dar_gettext("Error while fetching hostname: ")) + tools_strerror_r(errno));

    ret = string(uts.nodename);

    return ret;
}

string line_tools_get_date_utc()
{
    string ret;
    datetime now = datetime(::time(nullptr), 0, datetime::tu_second);

    ret = tools_display_date(now);

    return ret;
}

void line_tools_merge_to_deque(deque<string> & a, const deque<string> & b)
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

deque<string> line_tools_substract_from_deque(const deque<string> & a, const deque<string> & b)
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

delta_sig_block_size::fs_function_t line_tools_string_to_sig_block_size_function(const std::string & funname)
{
    if(funname == "fixed")
	return delta_sig_block_size::fixed;
    if(funname == "linear")
	return delta_sig_block_size::linear;
    if(funname == "log2")
	return delta_sig_block_size::log2;
    if(funname == "square2")
	return delta_sig_block_size::square2;
    if(funname == "square3")
	return delta_sig_block_size::square3;
    throw Erange("line_tools_string_to_sig_block_function", gettext("unknown name give for delta signature block len function"));
}

void line_tools_split_compression_algo(const char *arg, U_I base, compression & algo, U_I & level, U_I & block_size)
{
    if(arg == nullptr)
        throw SRC_BUG;
    else
    {
        string working = arg;
	deque<string> split;
	infinint tmp;

	line_tools_split(working, ':', split);

	switch(split.size())
	{
	case 1:
	    if(!tools_my_atoi(working.c_str(), level))
            {
                    // argument to -z is not an integer, testing whether this is an algorithm
                try
                {
                    algo = string2compression(working.c_str());
                    level = 9; // argument is a compression algo, level is 9 by default
                }
                catch(Erange & e)
                {
                    throw Erange("split_compression_algo", tools_printf(gettext("%s does not name a compression \"[algorithm][:][level]\" , like for examples \"gzip\", \"lzo\", \"bzip2\", \"lzo:3\", \"gzip:2\", \"8\" or \"1\". Please review the man page about -z option"), working.c_str()));
                }
            }
            else // argument is a compression level, algorithm is gzip by default
                algo = compression::gzip;

	    block_size = 0;
	    break;
	case 2:
	    if(split[0] != "")
		algo = string2compression(split[0]);
	    else
		algo = compression::gzip; // default algorithm

	     if(split[1] != "")
	    {
		if(!tools_my_atoi(split[1].c_str(), level) || (level > 9 && algo != compression::zstd) || level < 1)
		    throw Erange("split_compression_algo", gettext("Compression level must be between 1 and 9, included"));
	    }
	    else
		level = 9; // default compression level

	    block_size = 0;
	    break;
	case 3:
	    if(split[0] != "")
		algo = string2compression(split[0]);
	    else
		algo = compression::gzip;

	    if(split[1] != "")
	    {
		if(!tools_my_atoi(split[1].c_str(), level) || (level > 9 && algo != compression::zstd) || level < 1)
		    throw Erange("split_compression_algo", gettext("Compression level must be between 1 and 9, included"));
	    }
	    else
		level = 9;

	    tmp = tools_get_extended_size(split[2], base);
	    block_size = 0;
	    tmp.unstack(block_size);
	    if(!tmp.is_zero())
		throw Erange("split_compression_algo", gettext("Compression block size too large for this operating system"));
	    break;
	default:
	    throw Erange("split_compression_algo", gettext("invalid argument given for compression scheme"));
	}
    }
}

void line_tools_split_mask_list_from_eols(const string & arg, string & filename, deque<string> & eols)
{
    string local_arg = arg;
    string::iterator sep = line_tools_find_last_char_of(local_arg, ':');

    filename = string(local_arg.begin(), sep);
    if(sep != local_arg.end())
    {
	string eols_str = string(++sep, local_arg.end());

	    // split eols based on comma, stick toghether with a piece that ends with a backslash
	line_tools_split(eols_str, ',', eols);


	    // we'll now check each sequence

	deque<string>::iterator it = eols.begin();

	while(it != eols.end())
	{
	    bool last_is_escaped = false; // true if last backslash of the sequence has been escaped
	    string::iterator eolit = it->begin();

		// 1 - remove the first backslask from "\\"

	    while(eolit != it->end())
	    {
		if(*eolit == '\\')
		{
		    string::iterator nextone = eolit + 1;

		    if(nextone != it->end())
		    {
			eolit = nextone;
			last_is_escaped = true;
		    }
		    else
			last_is_escaped = false;
		}
		else
		    last_is_escaped = false;

		++eolit;
	    }

	    	// if the sequence ends with \ we stick it with the next one (the comma was escaped)
	    if(it->size() > 0 && (*it)[it->size() - 1] == '\\' && !last_is_escaped)
	    {
		deque<string>::iterator nextone = it + 1;

		it->pop_back(); // remove the last char of the string: i.e. the '\' char
		if(nextone != eols.end())
		{
		    *it = *it + ',' + *nextone;
		    eols.erase(nextone);
		    continue; // to avoid incrementing 'it'
		}
		else
		    *it = *it + ',';
	    }

	    *it = line_tools_replace_escape_sequences(*it);

	    ++it;
	}
    }
    else
	eols.clear();
}

string line_tools_replace_escape_sequences(const string & arg)
{
    string ret;
    string::const_iterator it = arg.begin();

	// doubling any % met in string for they cast back as an single % by sprintf

    while(it != arg.end())
    {
	if(*it == '\\')
	{
	    string::const_iterator nextone = it + 1;

	    if(nextone != arg.end())
	    {
		switch(*nextone)
		{
		case '\'':
		case '\"':
		case '\\':
		    ret += *nextone;
		    it = nextone + 1;
		    break;
		case 'n':
		    ret += '\n';
		    it = nextone + 1;
		    break;
		case 'r':
		    ret += '\r';
		    it = nextone + 1;
		    break;
		case 't':
		    ret += '\t';
		    it = nextone + 1;
		    break;
		case 'b':
		    ret += '\b';
		    it = nextone + 1;
		    break;
		case 'f':
		    ret += '\f';
		    it = nextone + 1;
		    break;
		case 'v':
		    ret += '\v';
		    it = nextone + 1;
		    break;
		case '0':  // octal notation
		    ++nextone;
		    if(nextone == arg.end())
			ret += '\0';
		    else
		    {
			ret += extract_octal_from_string(nextone);
			it = nextone;
		    }
		    break;
		case 'x':
		    ++nextone;
		    if(nextone == arg.end())
			throw Erange("line_tools_replace_escape_sequences",
				     tools_printf(gettext("Do not know how to handle \\x at end of string: %s"), arg.c_str()));
		    else
		    {
			try
			{
			    ret += extract_hexa_from_string(nextone);
			    it = nextone;
			}
			catch(Erange & e)
			{
			    e.prepend_message(tools_printf(gettext("Error in string %s: "), arg.c_str()));
			    throw;
			}
		    }
		    break;
		default:
		    throw Erange("line_tools_replace_escape_sequences", tools_printf(gettext("Unknown escape character: \\%c"), *nextone));
		}
	    }
	    else
		throw Erange("line_tools_replace_escape_sequences",
			     tools_printf(gettext("Do not know how to handle a single backslash at end of string, either use double it or remove it: %s"), arg.c_str()));
	}
	else
	{
	    ret += *it;
	    ++it;
	}
    }

    return ret;
}

///////////////////

static string build(string::iterator a, string::iterator b)
{
    string ret = "";

    while(a != b)
	ret += *a++;

    return ret;
}

static bool is_a_slice_available(user_interaction & ui, const string & base, const string & extension)
{
    path *chem = nullptr;
    bool ret = false;

    try
    {
	string rest;

	line_tools_split_path_basename(base.c_str(), chem, rest);

	try
	{
	    etage contents = etage(ui, chem->display().c_str(), datetime(0), datetime(0), false, false);  // we don't care the dates here so we set them to zero
	    regular_mask slice = regular_mask(rest + "\\.[0-9]+\\."+ extension, true);

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

static string retreive_basename2(const string & base)
{
    string new_base = base;
    S_I index;

    index = new_base.find_last_not_of(string("0")); // this will point before the last dot preceeding the zeros
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

static char extract_octal_from_string(string::const_iterator & x)
{
    char ret = 0;
    U_I digit = 0;

    while((*x >= '0' && *x <= '7') && digit < 3)
    {
	ret *= 8;
	ret += *x - '0';
	++digit;
	++x;
    }

    return ret;

}

static char extract_hexa_from_string(string::const_iterator & x)
{
    char ret = 0;
    U_I digit = 0;
    U_I val;

    while(digit < 2)
    {
	if(*x >= '0' && *x <= '9')
	    val = *x - '0';
	else if(*x >= 'A' && *x <= 'F')
	    val = *x - 'A' + 10;
	else if(*x > 'a' && *x <= 'f')
	    val = *x - 'a' + 10;
	else
	    throw Erange("extract_hexa_from_string", gettext("uncomplete hexadecimal number in escape sequence"));
	ret *= 16;
	ret += val;
	++digit;
	++x;
    }

    return ret;

}
