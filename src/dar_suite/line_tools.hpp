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

    /// \file line_tools.hpp
    /// \brief a set of general command line targeted routines
    /// \ingroup CMDLINE

#ifndef LINE_TOOLS_HPP
#define LINE_TOOLS_HPP

#include "../my_config.h"

#include <string>
#include "infinint.hpp"
#include "tlv_list.hpp"
#include "integers.hpp"
#include "criterium.hpp"

using namespace libdar;

    /// \addtogroup CMDLINE
    /// @{

class argc_argv
{
public:
    argc_argv(S_I size = 0);
    argc_argv(const argc_argv & ref) { throw Efeature("argc_argv"); };
    const argc_argv & operator = (const argc_argv & ref) { throw Efeature("argc_argv"); };
    ~argc_argv();

    void resize(S_I size);
    void set_arg(const std::string & arg, S_I index);
    void set_arg(generic_file & f, U_I size, S_I index);

    S_I argc() const { return x_argc; };
    char* const * argv() const { return x_argv; }; // well the const method is a bit silly, as the call has the possibility to modify what is pointed to by the returned value...

private:
    S_I x_argc;
    char **x_argv;
};

extern void line_tools_slice_ownership(const std::string & cmd, std::string & slice_permission, std::string & slice_user_ownership, std::string & slice_group_ownership);
extern void line_tools_repeat_param(const std::string & cmd, infinint & repeat_count, infinint & repeat_byte);
extern void line_tools_tlv_list2argv(user_interaction & dialog, const tlv_list & list, argc_argv & arg);

    /// returns the old position of parsing (next argument to parse)
extern S_I line_tools_reset_getopt();


std::string::const_iterator line_tools_find_first_char_out_of_parenth(const std::string & argument, unsigned char to_find);
std::string::const_iterator line_tools_find_last_char_out_of_parenth(const std::string & argument, unsigned char to_find);

std::string line_tools_expand_user_comment(const std::string & user_comment, S_I argc, char *const argv[]);

    /// split a PATH environement variable string into its components (/usr/lib:/lib => /usr/lib /lib)
std::vector<std::string> line_tools_explode_PATH(const char *the_path);

    /// return the full path of the given filename (eventually unchanged of pointing to the first file of that name present in the_path directories
std::string line_tools_get_full_path_from_PATH(const std::vector<std::string> & the_path, const char * filename);

    /// return split at the first space met the string given as first argument, and provide the two splitted string as second and third argument
void line_tools_split_at_first_space(const char *field, std::string & before_space, std::string & after_space);

void line_tools_get_min_digits(std::string arg, infinint & num, infinint & ref_num, infinint & aux_num);

    /// test the presence of a set of argument on the command line
    ///
    /// \param[in] arguments is the list of options to look for
    /// \param[in] argc is the number of argument on the command line
    /// \param[in] argv is the list of arguments on the command line
    /// \param[in] getopt_string is the parsing string to pass to getopt
#if HAVE_GETOPT_LONG
    /// \param[in] long_options is the optional list of long options  (an NULL pointer is acceptable for no long option)
#endif
    /// \param[out] presence is a subset of arguments containing the option found on command-line
extern void line_tools_look_for(const std::vector<char> & arguments,
				S_I argc,
				char *const argv[],
				const char *getopt_string,
#if HAVE_GETOPT_LONG
				const struct option *long_options,
#endif
				std::vector<char> & presence);


    /// test the presence of -Q and -j options on the command line
    ///
    /// \param[in] argc is the number of argument on the command line
    /// \param[in] argv is the list of arguments on the command line
    /// \param[in] getopt_string is the parsing string to pass to getopt
#if HAVE_GETOPT_LONG
    /// \param[in] long_options is the optional list of long options (an NULL pointer is acceptable for no long option)
#endif
    /// \param[out] j_is_present is set to true if -j option or its equivalent long option has been found on command-line
    /// \param[out] Q_is_present is set to true if -Q option or its equivalent long option has been found on command-line
extern void line_tools_look_for_jQ(S_I argc,
				   char *const argv[],
				   const char *getopt_string,
#if HAVE_GETOPT_LONG
				   const struct option *long_options,
#endif
				   bool & j_is_present,
				   bool & Q_is_present);


    /// split a line in words given the separator character (sep)

extern std::vector<std::string> line_tools_split(const std::string & val, char sep);

extern void line_tools_4_4_build_compatible_overwriting_policy(bool allow_over,
							       bool detruire,
							       bool more_recent,
							       const libdar::infinint & hourshift,
							       bool ea_erase,
							       const libdar::crit_action * & overwrite);

    /// @}

#endif
