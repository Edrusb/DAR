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
#include <vector>
#include <string>

#include "line_tools.hpp"
#include "deci.hpp"
#include "erreurs.hpp"
#include "tools.hpp"
#include "fichier_local.hpp"

using namespace std;
using namespace libdar;


////

argc_argv::argc_argv(S_I size)
{
    x_argc = size;
    if(x_argc > 0)
    {
	x_argv = new (nothrow) char *[size];
	if(x_argv == NULL)
	    throw Ememory("argc_argv::argc_argv");

	for(register S_I i = 0; i < x_argc; i++)
	    x_argv[i] = NULL;
    }
    else
	x_argv = NULL;
}

argc_argv::~argc_argv()
{
    if(x_argv == NULL && x_argc > 0)
	throw SRC_BUG;

    for(register S_I i = 0; i < x_argc; i++)
	if(x_argv[i] != NULL)
	{
	    delete [] x_argv[i];
	    x_argv[i] = NULL;
	}

    if(x_argc > 0)
	delete [] x_argv;
}

void argc_argv::set_arg(const string & arg, S_I index)
{
    if(index >= x_argc)
	throw Erange("argc_argv::set_arg", gettext("Index out of range"));

    if(x_argv[index] != NULL)
    {
	delete [] x_argv[index];
	x_argv[index] = NULL;
    }

    x_argv[index] = new (nothrow) char[arg.size() + 1];
    if(x_argv[index] == NULL)
	throw Ememory("argc_argv::set_arg");
    strncpy(x_argv[index], arg.c_str(), arg.size());
    x_argv[index][arg.size()] = '\0';
}

void argc_argv::set_arg(generic_file & f, U_I size, S_I index)
{
    if(index >= x_argc)
	throw Erange("argc_argv::set_arg", gettext("Index out of range"));

    if(x_argv[index] != NULL)
    {
	delete [] x_argv[index];
	x_argv[index] = NULL;
    }

    x_argv[index] = new (nothrow) char [size+1];
    if(x_argv[index] == NULL)
	throw Ememory("argc_argv::set_arg");

    x_argv[index][f.read(x_argv[index], size)] = '\0';
}

void argc_argv::resize(S_I size)
{
    char **tmp = NULL;

    if(size == x_argc)
	return;

    if(size < x_argc)
	for(register S_I i = size; i < x_argc; i++)
	    if(x_argv[i] != NULL)
	    {
		delete [] x_argv[i];
		x_argv[i] = NULL;
	    }

    tmp = new (nothrow) char*[size];
    if(tmp == NULL)
	throw Ememory("argc_argv::resize");

    try
    {
	S_I min = size < x_argc ? size : x_argc;

	for(register S_I i = 0; i < min; i++)
	    tmp[i] = x_argv[i];

	for(register S_I i = min; i < size; i++)
	    tmp[i] = NULL;

	if(x_argc > 0)
	    delete [] x_argv;
	x_argv = tmp;
	tmp = NULL;
	x_argc = size;
    }
    catch(...)
    {
	if(tmp != NULL)
	    delete [] tmp;
	throw;
    }
}

////

static string build(string::iterator a, string::iterator b);

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
    for(register S_I i = 0; i < arg.argc() ; i++)
    {
	if(list[i].get_type() != 0)
		// we only use type 0 here
	    throw Erange("line_tools_tlv_list2argv", gettext("Unknown TLV record type"));
	size = list[i].size();
	transfert = 0;
	size.unstack(transfert);
	if(size != 0)
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
		    for(register S_I i = 1; i < argc; ++i)
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
		    ret += tools_get_date_utc();
		    break;
		case 'u':
		    ret += tools_get_euid();
		    break;
		case 'g':
		    ret += tools_get_egid();
		    break;
		case 'h':
		    ret += tools_get_hostname();
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

vector<string> line_tools_explode_PATH(const char *the_path)
{
    vector<string> ret;
    const char *it = the_path;
    const char *last = it;

    ret.clear();
    if(it == NULL)
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

string line_tools_get_full_path_from_PATH(const vector<string> & the_path, const char * filename)
{
    string ret;
    bool no_path = false;

    if(filename == NULL)
	throw SRC_BUG;
    else
    {
	try
	{
	    path tmp = filename;

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
	vector<string>::const_iterator it = the_path.begin();
	bool found = false;

	while(!found && it != the_path.end())
	{
	    try
	    {
		string where = (path(*it) + filename).display();

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

    if(field == NULL)
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
	it1 = tools_find_first_char_of(the_arg, ',');
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
	    it2 = tools_find_first_char_of(tmp2, ',');
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

void line_tools_look_for(const vector<char> & arguments,
			 S_I argc,
			 char *const argv[],
			 const char *getopt_string,
#if HAVE_GETOPT_LONG
			 const struct option *long_options,
#endif
			 vector<char> & presence)
{
    S_I lu;
    presence.clear();

    (void)line_tools_reset_getopt();
#if HAVE_GETOPT_LONG
    const struct option *ptr_long_opt = long_options;
    const struct option voided = { NULL, 0, NULL, 0 };
    if(long_options == NULL)
	ptr_long_opt = &voided;

    while((lu = getopt_long(argc, argv, getopt_string, ptr_long_opt, NULL)) != EOF)
#else
	while((lu = getopt(argc, argv, getopt_string)) != EOF)
#endif
	{
	    vector<char>::const_iterator it = find(arguments.begin(), arguments.end(), (char)lu);

	    if(it != arguments.end())
		presence.push_back(lu);
	}
    (void)line_tools_reset_getopt();
}

void line_tools_look_for_jQ(S_I argc,
			    char *const argv[],
			    const char *getopt_string,
#if HAVE_GETOPT_LONG
			    const struct option *long_options,
#endif
			    bool & j_is_present,
			    bool & Q_is_present)
{
    vector<char> arguments;
    vector<char> presence;
    vector<char>::const_iterator it;

    j_is_present = false;
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
			presence);

    it = find(presence.begin(), presence.end(), 'j');
    if(it != presence.end())
	j_is_present = true;

    it = find(presence.begin(), presence.end(), 'Q');
    if(it != presence.end())
	Q_is_present = true;
}


vector<string> line_tools_split(const string & val, char sep)
{
    vector<string> ret;
    string::const_iterator be = val.begin();
    string::const_iterator ne = val.begin();

    while(ne != val.end())
    {
	if(*ne != sep)
	    ++ne;
	else
	{
	    ret.push_back(string(be, ne));
	    ++ne;
	    be = ne;
	}
    }

    if(be != val.end())
	ret.push_back(string(be, ne));

    return ret;
}

void line_tools_4_4_build_compatible_overwriting_policy(bool allow_over,
							bool detruire,
							bool more_recent,
							const infinint & hourshift,
							bool ea_erase,
							const crit_action * & overwrite)
{
    crit_action *tmp1 = NULL;
    crit_action *tmp2 = NULL; // tmp1 and tmp2 are used for construction of the overwriting policy
    overwrite = NULL;

    try
    {
	if(allow_over)
	{
	    if(ea_erase)
		overwrite = new crit_constant_action(data_overwrite, EA_overwrite);
	    else
		overwrite = new crit_constant_action(data_overwrite, EA_merge_overwrite);
	    if(overwrite == NULL)
		throw Ememory("tools_build_compatible_overwriting_policy");

	    tmp1 = new crit_constant_action(data_preserve, EA_preserve);
	    if(tmp1 == NULL)
		throw Ememory("tools_build_compatible_overwriting_policy");

	    if(more_recent)
	    {
		tmp2 = new testing(crit_invert(crit_in_place_data_more_recent(hourshift)), *overwrite, *tmp1);
		if(tmp2 == NULL)
		    throw Ememory("tools_build_compatible_overwriting_policy");

		delete overwrite;
		overwrite = tmp2;
		tmp2 = NULL;
	    }

	    if(!detruire)
	    {
		tmp2 = new testing(crit_invert(crit_in_place_is_inode()), *overwrite, *tmp1);
		if(tmp2 == NULL)
		    throw Ememory("tools_build_compatible_overwriting_policy");
		delete overwrite;
		overwrite = tmp2;
		tmp2 = NULL;
	    }

	    delete tmp1;
	    tmp1 = NULL;
	}
	else
	{
	    overwrite = new crit_constant_action(data_preserve, EA_preserve);
	    if(overwrite == NULL)
		throw Ememory("tools_build_compatible_overwriting_policy");
	}

	if(overwrite == NULL)
	    throw SRC_BUG;
	if(tmp1 != NULL)
	    throw SRC_BUG;
	if(tmp2 != NULL)
	    throw SRC_BUG;
    }
    catch(...)
    {
	if(tmp1 != NULL)
	    delete tmp1;
	if(tmp2 != NULL)
	    delete tmp2;
	if(overwrite != NULL)
	{
	    delete overwrite;
	    overwrite = NULL;
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
    const char *fin = all.c_str() + all.size(); // points past the last byte of the secu_string "all"
    secu_string tmp;
    recipients.clear();

    if(all.size() == 0)
    {
	algo = crypto_none;
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
		algo = crypto_scrambling;
	    else
		if(tmp == "none")
		    algo = crypto_none;
		else
		    if(tmp == "blowfish" || tmp == "bf" || tmp == "")
			algo = crypto_blowfish; // blowfish is the default cypher ("")
		    else
			if(tmp == "aes" || tmp == "aes256")
			    algo = crypto_aes256;
			else
			    if(tmp == "twofish" || tmp == "twofish256")
				algo = crypto_twofish256;
			    else
				if(tmp == "serpent" || tmp == "serpent256")
				    algo = crypto_serpent256;
				else
				    if(tmp == "camellia" || tmp == "camellia256")
					algo = crypto_camellia256;
				    else
					if(tmp == "gnupg")
					{
					    secu_string emails;
					    line_tools_crypto_split_algo_pass(pass, algo, emails, no_cipher_given, recipients);
					    recipients = line_tools_split(emails.c_str(), ',');
					    pass.clear();
					}
					else
					    throw Erange("crypto_split_algo_pass", string(gettext("unknown cryptographic algorithm: ")) + tmp.c_str());
	}
	else // no ':' using blowfish as default cypher
	{
	    no_cipher_given = true;
	    algo = crypto_blowfish;
	    pass = all;
	}
    }
}

void line_tools_display_signatories(user_interaction & ui, const vector<signator> & gnupg_signed)
{
    vector<signator>::const_iterator it = gnupg_signed.begin();
    string tmp;

    if(gnupg_signed.empty())
	return;

    ui.printf(gettext("+-----------------+----------------+----------------------------------------+-------------------------+"));
    ui.printf(gettext("| Signature Status|  Key Status    |  Finger Print                          | Signature Date          |"));
    ui.printf(gettext("+-----------------+----------------+----------------------------------------+-------------------------+"));
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
	ui.warning(tmp);
	if(it->key_validity == signator::expired)
	{
	    tmp = "                  |" + tools_display_date(it->signature_expiration_date);
	    ui.warning(tmp);
	}

	++it;
    }
    ui.printf(gettext("------------------+----------------+----------------------------------------+-------------------------+"));
    ui.printf(" For more information about a key, use the command: gpg --list-key <fingeprint>");
}

///////////////////

static string build(string::iterator a, string::iterator b)
{
    string ret = "";

    while(a != b)
	ret += *a++;

    return ret;
}


