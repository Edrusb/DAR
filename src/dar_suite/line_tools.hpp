/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2025 Denis Corbin
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

    /// \file line_tools.hpp
    /// \brief a set of general command line targeted routines
    /// \ingroup CMDLINE

#ifndef LINE_TOOLS_HPP
#define LINE_TOOLS_HPP

#include "../my_config.h"

#include <string>
#include <vector>
#include <deque>
#include <memory>
#include "libdar.hpp"
#include "tlv_list.hpp"
#include "tools.hpp"

using namespace libdar;

    /// \addtogroup CMDLINE
    /// @{

constexpr U_I LINE_TOOLS_SI_SUFFIX = 1000;
constexpr U_I LINE_TOOLS_BIN_SUFFIX = 1024;

class argc_argv
{
public:
    argc_argv(S_I size = 0);
    argc_argv(const argc_argv & ref) { throw Efeature("argc_argv"); };
    argc_argv & operator = (const argc_argv & ref) { throw Efeature("argc_argv"); };
    ~argc_argv() noexcept(false);

    void resize(S_I size);
    void set_arg(const std::string & arg, S_I index);
    void set_arg(generic_file & f, U_I size, S_I index);

    S_I argc() const { return x_argc; };
    char* const * argv() const { return x_argv; }; // well, the const method is a bit silly, as the caller has the possibility to modify what is pointed to by the returned value...

private:
    S_I x_argc;
    char **x_argv;
};

extern void line_tools_slice_ownership(const std::string & cmd, std::string & slice_permission, std::string & slice_user_ownership, std::string & slice_group_ownership);
extern void line_tools_repeat_param(const std::string & cmd, infinint & repeat_count, infinint & repeat_byte);
extern void line_tools_tlv_list2argv(user_interaction & dialog, tlv_list & list, argc_argv & arg);

    /// returns the old position of parsing (next argument to parse)
extern S_I line_tools_reset_getopt();


std::string::const_iterator line_tools_find_first_char_out_of_parenth(const std::string & argument, unsigned char to_find);
std::string::const_iterator line_tools_find_last_char_out_of_parenth(const std::string & argument, unsigned char to_find);

std::string line_tools_expand_user_comment(const std::string & user_comment, S_I argc, char *const argv[]);

    /// split a PATH environement variable string into its components (/usr/lib:/lib => /usr/lib /lib)
std::deque<std::string> line_tools_explode_PATH(const char *the_path);

    /// return the full path of the given filename (eventually unchanged of pointing to the first file of that name present in the_path directories
std::string line_tools_get_full_path_from_PATH(const std::deque<std::string> & the_path, const char * filename);

    /// return split at the first space met the string given as first argument, and provide the two splitted string as second and third argument
void line_tools_split_at_first_space(const char *field, std::string & before_space, std::string & after_space);

void line_tools_get_min_digits(std::string arg, infinint & num, infinint & ref_num, infinint & aux_num);

    /// test the presence of a set of argument on the command line

    /// \param[in] arguments is the list of options to look for
    /// \param[in] argc is the number of argument on the command line
    /// \param[in] argv is the list of arguments on the command line
    /// \param[in] getopt_string is the parsing string to pass to getopt
#if HAVE_GETOPT_LONG
    /// \param[in] long_options is the optional list of long options  (an nullptr pointer is acceptable for no long option)
#endif
    /// \param[in] stop_scan if this (char) option is met, stop scanning for wanted options
    /// \param[out] presence is a subset of arguments containing the option found on command-line
extern void line_tools_look_for(const std::deque<char> & arguments,
				S_I argc,
				char *const argv[],
				const char *getopt_string,
#if HAVE_GETOPT_LONG
				const struct option *long_options,
#endif
				char stop_scan,
				std::deque<char> & presence);


    /// test the presence of -Q and -j options on the command line

    /// \param[in] argc is the number of argument on the command line
    /// \param[in] argv is the list of arguments on the command line
    /// \param[in] getopt_string is the parsing string to pass to getopt
#if HAVE_GETOPT_LONG
    /// \param[in] long_options is the optional list of long options (an nullptr pointer is acceptable for no long option)
#endif
    /// \param[in] stop_scan if this (char) option is met, stop scanning for -j and -Q options
    /// \param[out] Q_is_present is set to true if -Q option or its equivalent long option has been found on command-line
extern void line_tools_look_for_Q(S_I argc,
				   char *const argv[],
				   const char *getopt_string,
#if HAVE_GETOPT_LONG
				   const struct option *long_options,
#endif
				   char stop_scan,
				   bool & Q_is_present);


    /// split a line in words given the separator character (sep)

template <class T> void line_tools_split(const std::string & val, char sep, T & split)
{
    std::string::const_iterator be = val.begin();
    std::string::const_iterator ne = val.begin();
    split.clear();

    while(ne != val.end())
    {
	if(*ne != sep)
	    ++ne;
	else
	{
	    split.push_back(std::string(be, ne));
	    ++ne;
	    be = ne;
	}
    }

    if(be != val.end())
	split.push_back(std::string(be, ne));
}

extern std::set<std::string> line_tools_deque_to_set(const std::deque<std::string> & list);

extern void line_tools_4_4_build_compatible_overwriting_policy(bool allow_over,
							       bool detruire,
							       bool more_recent,
							       const libdar::infinint & hourshift,
							       bool ea_erase,
							       const libdar::crit_action * & overwrite);

    /// split the argument to -K, -J and -$ in their different parts
    /// \param[in] all is what the user provided on command-line
    /// \param[out] algo is the symmetrical algorithm to use
    /// \param[out] pass is either the passphrase
    /// \param[out] no_cipher_given is true if the use did not specified the cipher (which defaults to blowfish)
    /// \param[out] recipients emails recipients to use (empty list if gnupg has not to be used)
extern void line_tools_crypto_split_algo_pass(const secu_string & all,
					      crypto_algo & algo,
					      secu_string & pass,
					      bool & no_cipher_given,
					      std::vector<std::string> & recipients);

    /// display information about the signatories
extern void line_tools_display_signatories(user_interaction & ui, const std::list<signator> & gnupg_signed);

    /// Extract from anonymous pipe a tlv_list

    /// \param[in,out] dialog for user interaction
    /// \param[in] fd the filedescriptor for the anonymous pipe's read extremity
    /// \param[out] result the resulting tlv_list
extern void line_tools_read_from_pipe(std::shared_ptr<user_interaction> & dialog, S_I fd, tlv_list & result);

    /// extracts the basename of a file (removing path part)

    /// \param[in] command_name is the full path of the file
    /// \param[out] basename the basename of the file
    /// \exception Ememory can be thrown if memory allocation failed
extern void line_tools_extract_basename(const char *command_name, std::string & basename);

    /// give a pointer to the last character of the given value in the given string

    /// \param[in] s is the given string
    /// \param[in] v is the given char value
    /// \return a interator on s, pointing on the first char of s equal to v or a pointing to s.end() if no such char could be found is "s"
    /// \note the arguments are not modified neither the data they are pointing to. However the const statement has not been used to
    /// be able to return a iterator on the string (and not a const_interator). There is probably other ways to do that (using const_cast) for example
extern std::string::iterator line_tools_find_first_char_of(std::string &s, unsigned char v);

    /// split a given full path in path part and basename part

    /// \param[in] all is the path to split
    /// \param[out] chemin is the resulting path part, it points to a newly allocated path object
    /// \param[out] base is the resulting basename
    /// \note chemin argument must be release by the caller thanks to the "delete" operator.
extern void line_tools_split_path_basename(const char *all, path * &chemin, std::string & base);

    /// split a given full path in path part and basename part

    /// \param[in] all is the path to split
    /// \param[out] chemin is the resulting path part, it points to a newly allocated path object
    /// \param[out] base is the resulting basename
    /// \note chemin argument must be release by the caller thanks to the "delete" operator.
extern void line_tools_split_path_basename(const std::string &all, std::string & chemin, std::string & base);

    /// split a given full remote repository path in parts

    /// \param[in] all is the argument to split in parts
    /// \param[out] proto is the protocol field
    /// \param[out] login is the login field (empty string is returned if not provided)
    /// \param[out] password is the password field (empty string if not provided)
    /// \param[out] hostname is the hostname field
    /// \param[out] port is the port field (empty string if not provided)
    /// \param[out] path_basename is the path+basename remaing field
    /// \return false if the all argument does not follow the remote repository syntax
extern bool line_tools_split_entrepot_path(const std::string &all,
					   std::string & proto,
					   std::string & login,
					   secu_string & password,
					   std::string & hostname,
					   std::string & port,
					   std::string & path_basename);

    /// convert a signed integer written in decimal notation to the corresponding value

    /// \param[in] x the decimal representation of the integer
    /// \return the value corresponding to the decimal representation given
extern S_I line_tools_str2signed_int(const std::string & x);

    /// convert a human readable date representation in number of second since the system reference date

    /// \param[in] repres the date's human representation
    /// \return the corresponding number of seconds (computer time)
    /// \note the string expected format is "[[[year/]month/]day-]hour:minute[:second]"
extern infinint line_tools_convert_date(const std::string & repres);

    /// display the compilation time features of libdar

    /// \param[in,out] dialog for user interaction
    /// \note this call uses the compile_time:: routines, and will
    /// not change its interface upon new feature addition
extern void line_tools_display_features(user_interaction & dialog);

    /// isolate the value of a given variable from the environment vector

    /// \param[in] env the environment vector as retreived from the third argument of the main() function
    /// \param[in] clef the key or variable name too look for
    /// \return nullptr if the key could not be found or a pointer to the env data giving the value of the requested key
    /// \note the returned value must not be released by any mean as it is just a pointer to an system allocated memory (the env vector).
extern const char *line_tools_get_from_env(const char **env, const char *clef);

    /// does sanity checks on a slice name, check presence and detect whether the given basename is not rather a filename

    /// \param[in,out] dialog for user interaction
    /// \param[in] loc the path where resides the slice
    /// \param[in,out] base the basename of the slice
    /// \param[in] extension the extension of dar's slices
    /// \param[in] create whether this is a new archive that is about to be created by this name
    /// \note if user accepted the change of slice name proposed by libdar through dialog the base argument is changed
extern void line_tools_check_basename(user_interaction & dialog,
				      const path & loc,
				      std::string & base,
				      const std::string & extension,
				      bool create);

    /// if a slice number 1 is met with the provided basename, set the num_digits accordingly

    /// \param[in,out] dialog for user interaction
    /// \param[in] loc the path where are expected the slices to be present
    /// \param[in] base the basename of the archive
    /// \param[in] extension the extension of dar's slices
    /// \param[in,out] num_digits the min width of slice number (0 padded numbers)
void line_tools_check_min_digits(user_interaction & dialog,
				 const path & loc,
				 const std::string & base,
				 const std::string & extension,
				 infinint & num_digits);


    /// from a string with a range notation (min-max) extract the range values

    /// \param[in] s the string to parse
    /// \param[out] min the minimum value of the range
    /// \param[out] max the maximum value of the range
    /// \exception Erange is thrown is the string to parse is incorrect
    /// \note: either a single number (positive or negative) is returned in min
    /// (max is set to min if min is positive or to zero if min is negative)
    /// or a range of positive numbers.
extern void line_tools_read_range(const std::string & s, S_I & min, U_I & max);

    /// read a file and split its contents into words

    /// \param[in,out] f is the file to read
    /// \param[out] mots std container to receive the split result
    /// \note The different quotes are taken into account
template <class T> void line_tools_split_in_words(generic_file & f, T & mots)
{
    std::deque <char> quotes;
    std::string current = "";
    char a;
    bool loop = true;
    bool escaped = false;

    mots.clear();
    while(loop)
    {
	if(f.read(&a, 1) != 1) // reached end of file
	{
	    loop = false;
	    a = ' '; // to close the last word
	}

	if(escaped)
	{
	    current += a; // added without consideration of quoting of any sort
	    escaped = false;
	    continue; // continuing at beginning of the while loop
	}
	else
	{
	    if(a == '\\')
	    {
		escaped = true;
		continue; // continuing at beginning of the while loop
	    }
	}

	if(quotes.empty()) // outside a word
	    switch(a)
	    {
	    case ' ':
	    case '\t':
	    case '\n':
	    case '\r':
		break;
	    case '"':
	    case '\'':
	    case '`':
		quotes.push_back(a);
		break;
	    default:
		quotes.push_back(' '); // the quote space means no quote
		current += a; // a new argument is starting
		break;
	    }
	else // inside a word
	    switch(a)
	    {
	    case '\t':
		if(quotes.back() != ' ')
		{
			// this is the end of the wor(l)d ;-)
			// ...once again... 1000, 1999, 2012, and the next ones to come...
		    break;
		}
		    // no break !
	    case '\n':
	    case '\r':
		a = ' '; // replace carriage return inside quoted string by a space
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
			mots.push_back(current);
			current = "";
		    }
		    else
			current += a;
		}
		else // "a" is a nested starting quote
		{
		    if(a != ' ') // quote ' ' does not have ending quote
			quotes.push_back(a);
		    current += a;
		}
		break;
	    default:
		current += a;
	    }
    }
    if(!quotes.empty())
	throw Erange("make_args_from_file", tools_printf(dar_gettext("Parse error: Unmatched `%c'"), quotes.back()));
}



    /// read a std::string and split its contents into words

    /// \param[in] arg is the string to read
    /// \param[out] mots a std container to receive the split result
    /// \note The different quotes are taken into account
template <class T> void line_tools_split_in_words(const std::string & arg, T & mots)
{
    memory_file mem;

    mem.write(arg.c_str(), arg.size());
    mem.skip(0);
    line_tools_split_in_words(mem, mots);
}

    /// builds a regex from root directory and user provided regex to be applied to the relative path

    /// \param[in] prefix is the root portion of the path
    /// \param[in] relative_part is the user provided regex to be applied to the relative path
    /// \return the corresponding regex to be applied to full absolute path
extern std::string line_tools_build_regex_for_exclude_mask(const std::string & prefix,
							   const std::string & relative_part);

    /// return a string containing the Effective UID
extern std::string line_tools_get_euid();

    /// return a string containing the Effective UID
extern std::string line_tools_get_egid();

    /// return a string containing the hostname of the current host
extern std::string line_tools_get_hostname();

    /// return a string containing the current time (UTC)
extern std::string line_tools_get_date_utc();

    /// add in 'a', element of 'b' not already found in 'a'
extern void line_tools_merge_to_deque(std::deque<std::string> & a, const  std::deque<std::string> & b);

    /// remove from 'a' elements found in 'b' and return the resulting deque
extern std::deque<std::string> line_tools_substract_from_deque(const std::deque<std::string> & a, const std::deque<std::string> & b);

    /// converts string name to function

    /// \note throws Erange in case of error
extern delta_sig_block_size::fs_function_t line_tools_string_to_sig_block_size_function(const std::string & funname);

extern void line_tools_split_compression_algo(const char *arg,     ///< input string to analyse
					      U_I base,            ///< base value for number suffix
					      compression & algo,  ///< returned compression algorithm
					      U_I & level,         ///< returned compression level
					      U_I & block_size     ///< returned compression block size
    );


    /// @}

#endif
