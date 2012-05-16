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
// $Id: tools.hpp,v 1.30 2004/11/14 00:07:27 edrusb Rel $
//
/*********************************************************************/


    /// \file tools.hpp
    /// \brief a set of general purpose routines


#ifndef TOOLS_HPP
#define TOOLS_HPP

#include "../my_config.h"

extern "C"
{
#if STDC_HEADERS
#include <stdarg.h>
#endif
}

#include <string>
#include <vector>
#include "path.hpp"
#include "infinint.hpp"
#include "generic_file.hpp"
#include "tuyau.hpp"
#include "integers.hpp"

#define TOOLS_SI_SUFFIX 1000
#define TOOLS_BIN_SUFFIX 1024

namespace libdar
{

	/// \addtogroup Tools
	/// \brief a set of tool routine
	///
	/// theses routines are part of the libdar API for historical
	/// reason. They are shared and used by dar, dar_slave, dar_xform,
	/// and dar_manager command. You should avoid using thoses in
	/// external program as they may be removed or changed without
	/// backward compatibility support.
	/// @{

	/// convert a string to a char *

	/// \param[in] x is the string to convert
	/// \return the address of newly allocated memory containing the equivalent string as the argument
	/// \exception Ememory is thrown if the memory allocation failed, this call never return NULL
	/// \note that the allocated memory must be released by the caller thanks to the "delete" operator
    extern char *tools_str2charptr(std::string x);

	/// write a string to a file with a '\\0' at then end

	/// \param[in] f the file to write to
	/// \param[in] s the string to write to file
    extern void tools_write_string(generic_file & f, const std::string & s);

	/// read a string from a file expecting it to terminate by '\\0'

	/// \param f the file to read from
	/// \param s the string to put the data to (except the ending '\\0')
    extern void tools_read_string(generic_file & f, std::string & s);

	/// write a string to a file, '\\0' has no special meaning nor is added at the end

	/// \param[in] f the file to write to
	/// \param[in] s the string to write to file
    extern void tools_write_string_all(generic_file & f, const std::string & s);

	/// read a string if given size from a file '\\0' has no special meaning

	/// \param[in] f is the file to read from
	/// \param[in] s is the string to put read data in
	/// \param[in] taille is the size in byte to read
    extern void tools_read_string_size(generic_file & f, std::string & s, infinint taille);

	/// retrieve the size in byte of a file

	/// \param[in] p is the path to the file which size is to get
	/// \return the size of the file in byte
    extern infinint tools_get_filesize(const path &p);

	/// convert the given string to infinint taking care of multiplication suffixes like k, M, T, etc.

	/// \param[in] s is the string to read
	/// \param[in] base is the multiplication factor (base = 1000 for SI, base = 1024 for computer science use)
	/// \return the value encoded in the given string
    extern infinint tools_get_extended_size(std::string s, U_I base);

	/// extracts the basename of a file (removing path part)

	/// \param[in] command_name is the full path of the file
	/// \return the basename of the file
	/// \exception Ememory can be thrown if memory allocation failed
	/// \note the returned value has to be release thanks to the "delete" operator by the caller of this function
    extern char *tools_extract_basename(const char *command_name);

	/// split a given full path in path part and basename part

	/// \param[in] all is the path to split
	/// \param[out] chemin is the resulting path part
	/// \param[out] base is the resulting basename
	/// \note chemin argument must be release by the call thanks to the "delete" operator.
    extern void tools_split_path_basename(const char *all, path * &chemin, std::string & base);

	/// split a given full path in path part and basename part

	/// \param[in] all is the path to split
	/// \param[out] chemin is the resulting path part
	/// \param[out] base is the resulting basename
	/// \note chemin argument must be release by the call thanks to the "delete" operator.
    extern void tools_split_path_basename(const std::string &all, std::string & chemin, std::string & base);

	/// open a pair of tuyau objects encapsulating two named pipes.

	/// \param[in,out] dialog for user interaction
	/// \param[in] input path to the input named pipe
	/// \param[in] output path to the output named pipe
	/// \param[out] in resulting tuyau object for input
	/// \param[out] out resulting tuyau object for output
	/// \note in and out parameters must be released by the caller thanks to the "delete" operator
    extern void tools_open_pipes(user_interaction & dialog, const std::string &input, const std::string & output,
				 tuyau *&in, tuyau *&out);

	/// set blocking/not blocking mode for reading on a file descriptor

	/// \param[in] fd file descriptor to read on
	/// \param[in] mode set to true for a blocking read and to false for non blocking read
    extern void tools_blocking_read(int fd, bool mode);

	/// convert uid to name in regards to the current system's configuration

	/// \param[in] uid the User ID number
	/// \return the name of the corresponding user or the uid if none corresponds
    extern std::string tools_name_of_uid(U_16 uid);

	/// convert fid to name in regards to the current system's configuration

	/// \param[in] gid the Group ID number
	/// \return the name of the corresponding group or the gid if none corresponds
    extern std::string tools_name_of_gid(U_16 gid);

	/// convert integer to string

	/// \param[in] x the integer to convert
	/// \return the decimal representation of the given integer
    extern std::string tools_int2str(S_I x);

	/// convert an integer written in decimal notation to the corresponding value

	/// \param[in] x the decimal representation of the integer
	/// \return the value corresponding to the decimal representation given
    extern U_32 tools_str2int(const std::string & x);

	/// prepend spaces before the given string

	/// \param[in] s the string to append spaces to
	/// \param[in] expected_size the minimum size of the resulting string
	/// \return a string at least as much long as expected_size with prepended leading spaces if necessary
    extern std::string tools_addspacebefore(std::string s, unsigned int expected_size);

	/// convert a date in second to its human readable representation

	/// \param[in] date the date in second
	/// \return the human representation corresponding to the argument
    extern std::string tools_display_date(infinint date);

	/// wrapper to the "system" system call.

	/// \param[in,out] dialog for user interaction
	/// \param[in] argvector the equivalent to the argv[] vector
    extern void tools_system(user_interaction & dialog, const std::vector<std::string> & argvector);

	/// write a list of string to file

	/// \param[in] f the file to write to
	/// \param[in] x the list of string to write
    extern void tools_write_vector(generic_file & f, const std::vector<std::string> & x);

	/// read a list of string from a file

	/// \param[in] f the file to read from
	/// \param[out] x the list to fill from file
    extern void tools_read_vector(generic_file & f, std::vector<std::string> & x);

	/// concatenate a vectors of strings in a single string

	/// \param[in] separator string to insert between two elements
	/// \param[in] x the list string
	/// \return the result of the concatenation of the members of the list with separtor between two consecutive members
    extern std::string tools_concat_vector(const std::string & separator,
					   const std::vector<std::string> & x);

	/// concatenate two vectors

	/// \param[in] a the first vector
	/// \param[in] b the second vector
	/// \return a vector containing the elements of a and the element of b
    std::vector<std::string> operator + (std::vector<std::string> a, std::vector<std::string> b);

	/// test the presence of a value in a list

	/// \param[in] val is the value to look for
	/// \param[in] liste is the list to look in
	/// \return true if val has been found as a member of the list
    extern bool tools_is_member(const std::string & val, const std::vector<std::string> & liste);

	/// display the compilation time features of libdar

	/// \param[in,out] dialog for user interaction
	/// \param[in] ea whether Extended Attribute support is available
	/// \param[in] largefile whether large file support is available
	/// \param[in] nodump whether nodump flag support is available
	/// \param[in] special_alloc whether special allocation is activated
	/// \param[in] bits infinint version used
	/// \param[in] thread_safe whether thread safe support is available
	/// \param[in] libz whether libz compression is available
	/// \param[in] libbz2 whether libbz2 compression is available
	/// \param[in] libcrypto whether strong encryption is available
    extern void tools_display_features(user_interaction & dialog,
				       bool ea, bool largefile, bool nodump, bool special_alloc, U_I bits, bool thread_safe,
				       bool libz,
				       bool libbz2,
				       bool libcrypto);

	/// test if two dates are equal taking care of a integer hour of difference

	/// \param[in] hourshift is the number of integer hour more or less two date can be considered equal
	/// \param[in] date1 first date to compare
	/// \param[in] date2 second date to compare to
	/// \return whether dates are equal or not
    extern bool is_equal_with_hourshift(const infinint & hourshift, const infinint & date1, const infinint & date2);

	/// ascii to integer conversion

	/// \param[in] a is the ascii string to convert
	/// \param[out] val is the resulting value
	/// \return true if the conversion could be done false if the given string does not
	/// correspond to the decimal representation of an unsigned integer
    extern bool tools_my_atoi(char *a, U_I & val);


	/// template function to add two vectors

    template <class T> std::vector<T> operator +=(std::vector<T> & a, const std::vector<T> & b)
    {
	a = a + b;
	return a;
    }


	/// isolate the value of a given variable from the environment vector

	/// \param[in] env the environment vector as retreived from the third argument of the main() function
	/// \param[in] clef the key or variable name too look for
	/// \return NULL if the key could not be find or a pointer to the env data giving the value of the requested key
	/// \note the returned value must not be released by any mean as it is just a pointer to an system allocated memory (the env vector).
    extern const char *tools_get_from_env(const char **env, char *clef);

	/// does sanity checks on a slice name, check presence and detect whether the given basename is not rather a filename

	/// \param[in,out] dialog for user interaction
	/// \param[in] loc the path where resides the slice
	/// \param[in,out] base the basename of the slice
	/// \param[in] extension the extension of dar's slices
	/// \note if user accepted the change of slice name proposed by libdar through dialog the base argument is changed
    extern void tools_check_basename(user_interaction & dialog,
				     const path & loc, std::string & base, const std::string & extension);

    	/// get current working directory

    extern std::string tools_getcwd();

	/// returns the file pointed to by a symbolic link (or transparent if the file is not a symlink).

	/// \param root the path to the file to read
	/// \return the file pointed to by the symlink or the value given in argument if it is not a symlink
	/// \note an exception can occur if lack of memory or invalid argument given (NULL or empty string), system call error...
    extern std::string tools_readlink(const char *root);

	/// test the presence of an argument

	/// \param[in] argument is the command line argument to look for
	/// \param[in] argc is the number of argument on the command line
	/// \param[in] argv is the list of argument on the command line
	/// \return true if the argument is present in the list
    extern bool tools_look_for(const char *argument, S_I argc, char *argv[]);


	/// set dates of a given file, no exception thrown

	/// \param[in] chem the path to the file to set
	/// \param[in] last_acc last access date to use
	/// \param[in] last_mod last modification date to use
    extern void tools_noexcept_make_date(const std::string & chem, const infinint & last_acc, const infinint & last_mod);

	/// set dates of a given file, may throw exception

	/// \param[in] chemin the path to the file to set
	/// \param[in] access last access date to use
	/// \param[in] modif last modification date to use
    extern void tools_make_date(const std::string & chemin, infinint access, infinint modif);

	/// compare two string in case insensitive manner

	/// \param[in] a first string to compare
	/// \param[in] b second string to compare
	/// \return whether the two string given in argument are equal in case insensitive comparison
    extern bool tools_is_case_insensitive_equal(const std::string & a, const std::string & b);

	/// \brief convert a string to upper case
	///
	/// \param[in,out] nts a NULL terminated string to convert
    extern void tools_to_upper(char *nts);

	/// remove last character of a string is it equal to a given value

	/// \param[in] c the given value to compare the last char with
	/// \param[in,out] s the string to modify
    extern void tools_remove_last_char_if_equal_to(char c, std::string & s);

	/// from a string with a range notation (min-max) extract the range values

	/// \param[in] s the string to parse
	/// \param[out] min the minimum value of the range
	/// \param[out] max the maximum value of the range
	/// \exception Erange is thrown is the string to parse is incorrect
    extern void tools_read_range(const std::string & s, U_I & min, U_I & max);


	/// make printf-like formating to a std::string

	/// \param[in] format the format string
	/// \param[in] ... list of argument to use against the format string
	/// \return the resulting string
	/// \note the supported masks for the format are:
	/// - \%s \%c \%d \%\%  (normal behavior)
        /// - \%i (matches infinint *)
        /// - \%S (matches std::string *)
	/// .
    extern std::string tools_printf(char *format, ...);

	/// make printf-like formating to a std::string

	/// \param[in] format the format string
	/// \param[in] ap list of argument to use against the format string
	/// \return the resulting string
	/// \note the supported masks for the format are:
	/// - \%s \%c \%d \%\%  (normal behavior)
        /// - \%i (matches infinint *)
        /// - \%S (matches std::string *)
	/// .
    extern std::string tools_vprintf(char *format, va_list ap);

	/// test the presence of files corresponding to a given mask in a directory

	/// \param[in] c_chemin directory where file have to looked for
	/// \param[in] file_mask glob expression which designates the files to look for
	/// \return true if some files have found matching the file_mask
    extern bool tools_do_some_files_match_mask(const char *c_chemin, const char *file_mask);

	/// remove files from a given directory

	/// \param[in,out] dialog for user interaction
	/// \param[in] c_chemin directory where files have to be removed
	/// \param[in] file_mask glob expression which designates the files to remove
	/// \param[in] info_details whether user must be displayed details of the operation
	/// \note This is equivalent to the 'rm' command with shell expansion
    extern void tools_unlink_file_mask(user_interaction & dialog, const char *c_chemin, const char *file_mask, bool info_details);


	/// prevents slice overwriting: check the presence of slice and if necessary ask the user if they can be removed

	/// \param[in,out] dialog for user interaction
	/// \param[in] chemin where slice is about to be created
	/// \param[in] x_file_mask mask corresponding to slices that will be generated
	/// \param[in] info_details whether user must be displayed details of the operation
	/// \param[in] allow_overwriting whether overwriting is allowed by the user
	/// \param[in] warn_overwriting whether a warning must be issued before overwriting (if allowed)
	/// \note may thow exceptions.
    extern void tools_avoid_slice_overwriting(user_interaction & dialog, const std::string & chemin, const std::string & x_file_mask, bool info_details, bool allow_overwriting, bool warn_overwriting);


	/// append an elastic buffer of given size to the file

	/// \param[in,out] f file to append elastic buffer to
	/// \param[in] max_size size of the elastic buffer to add
    extern void tools_add_elastic_buffer(generic_file & f, U_32 max_size);

	/// @}

} // end of namespace

#endif

