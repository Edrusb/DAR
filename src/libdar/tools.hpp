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

/// \file tools.hpp
/// \brief a set of general purpose routines
/// \ingroup Private


#ifndef TOOLS_HPP
#define TOOLS_HPP

#include "../my_config.h"

extern "C"
{
#if STDC_HEADERS
#include <stdarg.h>
#endif
#if HAVE_SIGNAL_H
#include <signal.h>
#endif
#if HAVE_GPGME_H
#include <gpgme.h>
#endif
}

#include <string>
#include <vector>
#include <deque>
#include <map>
#include <set>
#include "path.hpp"
#include "infinint.hpp"
#include "generic_file.hpp"
#include "integers.hpp"
#include "tlv_list.hpp"
#include "datetime.hpp"
#include "entrepot.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{


	/// libdar internal use only: it is launched from get_version() and initializes tools internal variables
    extern void tools_init();
	/// libdar internal use only: it is launched from close_and_clean() and releases tools internal variables
    extern void tools_end();

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

	/// convert an integer to its decimal representation with the highest unit of metric system
	/// \param[in] number is the integer to convert
	/// \param[in] unit unit symbol (o for octet, m for meter, etc.) to apply metric system to, this may be nullptr
	/// \param[in] binary if set to true using the ki, Gi, Mi ... scale instead of the traditional k, G, M, ... prefixes
	/// \return the string representing the number in metric system (ex: "1 ko", "200 Mio", ...)
    extern std::string tools_display_integer_in_metric_system(infinint number, const std::string & unit, bool binary);

	/// give a pointer to the last character of the given value in the given string

	/// \param[in] s is the given string
	/// \param[in] v is the given char value
	/// \return a interator on s, pointing on the last char of s equal to v or a pointing to s.end() if no such char could be found is "s"
	/// \note the arguments are not modified neither the data they are pointing to. However the const statement has not been used to
	/// be able to return a iterator on the string (and not a const_interator). There is probably other ways to do that (using const_cast) for example
    extern std::string::iterator tools_find_last_char_of(std::string &s, unsigned char v);

	/// set blocking/not blocking mode for reading on a file descriptor

	/// \param[in] fd file descriptor to read on
	/// \param[in] mode set to true for a blocking read and to false for non blocking read
    extern void tools_blocking_read(int fd, bool mode);

	/// convert uid to name in regards to the current system's configuration

	/// \param[in] uid the User ID number
	/// \return the name of the corresponding user or the uid if none corresponds
    extern std::string tools_name_of_uid(const infinint & uid);

	/// convert gid to name in regards of the current system's configuration

	/// \param[in] gid the Group ID number
	/// \return the name of the corresponding group or the gid if none corresponds
    extern std::string tools_name_of_gid(const infinint & gid);

	/// convert unsigned word to string

	/// \param[in] x the unsigned word to convert
	/// \return the decimal representation of the given integer
    extern std::string tools_uword2str(U_16 x);

	/// convert integer to string

	/// \param[in] x the integer to convert
	/// \return the decimal representation of the given integer
    extern std::string tools_int2str(S_I x);
    extern std::string tools_uint2str(U_I x);

	/// prepend spaces before the given string

	/// \param[in] s the string to append spaces to
	/// \param[in] expected_size the minimum size of the resulting string
	/// \return a string at least as much long as expected_size with prepended leading spaces if necessary
    extern std::string tools_addspacebefore(std::string s, U_I expected_size);

	/// convert a date in second to its human readable representation

	/// \param[in] date the date in second
	/// \return the human representation corresponding to the argument
    extern std::string tools_display_date(const datetime & date);

        /// convert a string to a char *

	/// \param[in] x is the string to convert
	/// \return the address of newly allocated memory containing the equivalent string as the argument
	/// \exception Ememory is thrown if the memory allocation failed, this call never return nullptr
	/// \note Do not use this function, use std::string::c_str(). The allocated memory must be released by the caller thanks to the "delete []" operator
    extern char *tools_str2charptr(const std::string &x);

        /// convert an integer written in decimal notation to the corresponding value

	/// \param[in] x the decimal representation of the integer
	/// \return the value corresponding to the decimal representation given
    extern U_I tools_str2int(const std::string & x);

	/// wrapper to the "system" system call.

	/// \param[in,out] dialog for user interaction
	/// \param[in] argvector the equivalent to the argv[] vector
    extern void tools_system(user_interaction & dialog, const std::vector<std::string> & argvector);

	/// wrapper to the "system" system call using anonymous pipe to tranmit arguments to the child process

	/// \param[in,out] dialog for user interaction
	/// \param[in] dar_cmd the path to the executable to run
	/// \param[in] argvpipe the list of arguments to pass through anonymous pipe
	/// \note the command to execute must understand the --pipe-fd option that
	/// gives the filedescriptor to read from the command-line options
    extern void tools_system_with_pipe(const std::shared_ptr<user_interaction> & dialog,
				       const std::string & dar_cmd,
				       const std::vector<std::string> & argvpipe);

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


	/// test if two dates are equal taking care of a integer hour of difference

	/// \param[in] hourshift is the number of integer hour more or less two date can be considered equal
	/// \param[in] date1 first date to compare
	/// \param[in] date2 second date to compare to
	/// \return whether dates are equal or not
    extern bool tools_is_equal_with_hourshift(const infinint & hourshift, const datetime & date1, const datetime & date2);

	/// template function to add two vectors

    template <class T> std::vector<T> operator +=(std::vector<T> & a, const std::vector<T> & b)
    {
        a = a + b;
        return a;
    }



	/// returns the file pointed to by a symbolic link (or transparent if the file is not a symlink).

	/// \param root the path to the file to read
	/// \return the file pointed to by the symlink or the value given in argument if it is not a symlink
	/// \note an exception can occur if lack of memory or invalid argument given (nullptr or empty string), system call error...
    extern std::string tools_readlink(const char *root);

	/// set dates of a given file, no exception thrown

	/// \param[in] chem the path to the file to set
	/// \param[in] symlink true if the file is a symlink
	/// \param[in] last_acc last access date to use
	/// \param[in] last_mod last modification date to use
	/// \param[in] birth creation date of the file, if not known, use the value of last_mod for efficiency
    extern void tools_noexcept_make_date(const std::string & chem, bool symlink, const datetime & last_acc, const datetime & last_mod, const datetime & birth);

	/// set dates of a given file, may throw exception

	/// \param[in] chemin the path to the file to set
	/// \param[in] symlink true if the file is a symlink
	/// \param[in] access last access date to use
	/// \param[in] modif last modification date to use
	/// \param[in] birth time of creation of the file
	/// \note if birth time is not known, it should be set to the value of modif for efficiency
    extern void tools_make_date(const std::string & chemin, bool symlink, const datetime & access, const datetime & modif, const datetime & birth);

	/// compare two string in case insensitive manner

	/// \param[in] a first string to compare
	/// \param[in] b second string to compare
	/// \return whether the two string given in argument are equal in case insensitive comparison
    extern bool tools_is_case_insensitive_equal(const std::string & a, const std::string & b);

	/// \brief convert a string to upper case
	///
	/// \param[in] r the string to convert
	/// \param[out] uppered resulting upper cased string
	/// \note in case of invalid wide char met in source string, the upper case convertion
	/// is done in ASCII mode (byte by byte)
    extern void tools_to_upper(const std::string & r, std::string & uppered);

#if HAVE_WCTYPE_H
	/// \brief convert a wstring to upper case
	///
	/// \param[in,out] r to convert
	/// \note wstring is a string of wchar_t (wide-char) type used to store
	/// on variable lenght of byte sequence the many characters defined with UTF
	/// like cirillic and greek letters.
    extern void tools_to_wupper(std::wstring & r);
#endif

	/// make printf-like formating to a std::string

	/// \param[in] format the format string
	/// \param[in] ... list of argument to use against the format string
	/// \return the resulting string
	/// \note the supported masks for the format are:
	/// - \%s \%c \%d \%o \%\%  (usual behavior)
	/// - \%x display an integer under hexadecimal notation
	/// - \%i (matches infinint *)
	/// - \%S (matches std::string *)
	/// .
    extern std::string tools_printf(const char *format, ...);

	/// make printf-like formating to a std::string

	/// \param[in] format the format string
	/// \param[in] ap list of argument to use against the format string
	/// \return the resulting string
	/// \note the supported masks for the format are:
	/// - \%s \%c \%d \%\%  (normal behavior)
	/// - \%i (matches infinint *)
	/// - \%S (matches std::string *)
	/// .
    extern std::string tools_vprintf(const char *format, va_list ap);

	/// test the presence of files corresponding to a given mask in a directory (regex mask)

	/// \param[in] ent entrepot where to read filesystem from
	/// \param[in] file_mask regex expression which designates the files to look for
	/// \return true if some files have found matching the file_mask
    extern bool tools_do_some_files_match_mask_regex(const entrepot & ent,
						     const std::string & file_mask);


	/// remove files from a given directory

	/// \param[in,out] dialog for user interaction
	/// \param[in] ent entrepot where to remove files from
	/// \param[in] file_mask regex expression which designates the files to remove
	/// \param[in] info_details whether user must be displayed details of the operation
	/// \note This is equivalent to the 'rm' command with regex expression in place of glob one
    extern void tools_unlink_file_mask_regex(user_interaction & dialog,
					     const entrepot & ent,
					     const std::string & file_mask,
					     bool info_details);


	/// prevents slice overwriting: check the presence of slice and if necessary ask the user if they can be removed

	/// \param[in,out] dialog for user interaction
	/// \param[in] ent entrepot where to check for slice overwriting
	/// \param[in] basename is the archive basename
	/// \param[in] extension is the archive filename extension
	/// \param[in] info_details whether user must be displayed details of the operation
	/// \param[in] allow_overwriting whether overwriting is allowed by the user
	/// \param[in] warn_overwriting whether a warning must be issued before overwriting (if allowed)
	/// \param[in] dry_run do a dry-run exection (no filesystem modification is performed)
	/// \note may thow exceptions.
    extern void tools_avoid_slice_overwriting_regex(user_interaction & dialog,
						    const entrepot & ent,
						    const std::string & basename,
						    const std::string & extension,
						    bool info_details,
						    bool allow_overwriting,
						    bool warn_overwriting,
						    bool dry_run);

	/// tells whether two files are on the same mounted filesystem

	/// \param[in] file1 first file
	/// \param[in] file2 second file
	/// \return true if the two files are located under the same mounting point
	/// \note if one of the file is not present or if the filesystem information
	/// is not possible to be read an exception is throw (Erange)
    extern bool tools_are_on_same_filesystem(const std::string & file1, const std::string & file2);

	/// transform a relative path to an absolute one given the current directory value

	/// \param[in] src the relative path to transform
	/// \param[in] cwd the value to take for the current directory
	/// \return the corresponding absolute path
    extern path tools_relative2absolute_path(const path & src, const path & cwd);

	/// block all signals (based on POSIX sigprocmask)

	/// \param[out] old_mask is set to the old mask value (for later unmasking signals)
	/// \exception Erange is thrown if system call failed for some reason
    extern void tools_block_all_signals(sigset_t &old_mask);

	/// unblock signals according to given mask

	/// \param[in] old_mask value to set to blocked signal mask
	/// \exception Erange is thrown if system call failed for some reason
    extern void tools_set_back_blocked_signals(sigset_t old_mask);

	/// counts the number of a given char in a given string

	/// \param[in] s string to look inside of
	/// \param[in] a char to look for
	/// \return the number of char found
    extern U_I tools_count_in_string(const std::string & s, const char a);

	/// returns the last modification date of the given file

	/// \param[in,out] dialog for user interaction
	/// \param[in] s path of the file to get the last mtime
	/// \param[in] auto_zeroing whether to just warn instead of asking user confirmation
	/// \param[in] silent if set do not warn nor ask
	/// \param[in] ignored_as_symlink for inodes present in this set,
	/// get the symlink pointed to date rather than the date of they symlink itself
	/// (no impact if the inode is not a symlink)
	/// \return the mtime of the given file
    extern datetime tools_get_mtime(user_interaction & dialog,
				    const std::string & s,
				    bool auto_zeroing,
				    bool silent,
				    const std::set<std::string> & ignored_as_symlink = std::set<std::string>());

	/// returns the size of the given plain file

	/// \param[in] s path of the file to get the size
	/// \return the size if the file in byte
    extern infinint tools_get_size(const std::string & s);

        /// convert the given string to infinint taking care of multiplication suffixes like k, M, T, etc.

	/// \param[in] s is the string to read
	/// \param[in] base is the multiplication factor (base = 1000 for SI, base = 1024 for computer science use)
	/// \return the value encoded in the given string
    extern infinint tools_get_extended_size(std::string s, U_I base);

	/// produce the string resulting from the substition of % macro defined in the map

	/// \param[in] hook is the user's expression in which to proceed to substitution
	/// \param[in] corres is a map telling which char following a % sign to replace by which string
	/// \return the resulting string of the substitution
    extern std::string tools_substitute(const std::string & hook,
					const std::map<char, std::string> & corres);


	/// produces the string resulting from the substitution of %... macro

	/// \param[in] hook the string in which to substitute
	/// \param[in] path is by what %p will be replaced
	/// \param[in] basename is by what %b will be replaced
	/// \param[in] num is by what %n will be replaced
	/// \param[in] padded_num is by what %N will be replaced
	/// \param[in] ext is by what %e will be replaced
	/// \param[in] context is by what %c will be replaced
	/// \param[in] base_url remote repository base URL
	/// \return the substitued resulting string
	/// \note it now relies on tools_substitue
    extern std::string tools_hook_substitute(const std::string & hook,
					     const std::string & path,
					     const std::string & basename,
					     const std::string & num,
					     const std::string & padded_num,
					     const std::string & ext,
					     const std::string & context,
					     const std::string & base_url);


	/// execute and retries at user will a given command line

	/// \param[in] ui which way to ask the user whether to continue upon command line error
	/// \param[in] cmd_line the command line to execute
    extern void tools_hook_execute(user_interaction & ui,
				   const std::string & cmd_line);


	/// subsititue and execute command line

	/// \param[in,out] ui this is the way to contact the user
	/// \param[in] hook the string in which to substitute
	/// \param[in] path is by what %p will be replaced
	/// \param[in] basename is by what %b will be replaced
	/// \param[in] num is by what %n will be replaced
	/// \param[in] padded_num is by what %N will be replaced
	/// \param[in] ext is by what %e will be replaced
	/// \param[in] context is by what %c will be replaced
    	/// \param[in] base_url remote repository base URL
    extern void tools_hook_substitute_and_execute(user_interaction & ui,
						  const std::string & hook,
						  const std::string & path,
						  const std::string & basename,
						  const std::string & num,
						  const std::string & padded_num,
						  const std::string & ext,
						  const std::string & context,
						  const std::string & base_url);


	/// convert string for xml output

	/// \note any < > & quote and double quote are replaced by adequate sequence for unicode
	/// \note second point, nothing is done here to replace system native strings to unicode
    extern std::string tools_output2xml(const std::string & src);

	/// convert octal string to integer

	/// \param perm is a string representing a number in octal (string must have a leading zero)
	/// \return the corresponding value as an integer
    extern U_I tools_octal2int(const std::string & perm);


	/// convert a number to a string corresponding to its octal representation

	/// \param perm is the octal number
	/// \return the corresponding octal string
    extern std::string tools_int2octal(const U_I & perm);

	/// change the libdar internal type representation to the usual unix char type

    extern char tools_cast_type_to_unix_type(char type);

	/// convert a permission number into its string representation (rwxrwxrwx)

    extern std::string tools_get_permission_string(char type, U_32 perm, bool hard);

	/// change the permission of the file which descriptor is given

	/// \param[in] fd file's descriptor
	/// \param[in] perm file permission to set the file to
    extern void tools_set_permission(S_I fd, U_I perm);

	/// obtain the permission of the file which descriptor is given

	/// \param[in] fd file's descriptor
	/// \return permission of the given file
	/// \note in case of error exception may be thrown
    extern U_I tools_get_permission(S_I fd);

	/// change ownership of the file which descriptor is given

	/// convert string user name or uid to numeric uid value

	/// \param[in] user string username
	/// \return uid value
    extern uid_t tools_ownership2uid(const std::string & user);

	/// convert string group name or gid to numeric gid value

	/// \param[in] group string username
	/// \return uid value
    extern uid_t tools_ownership2gid(const std::string & group);

	/// change ownership of the file which descriptor is given

	/// \param[in] filedesc file's descriptor
	/// \param[in] slice_user the user to set the file to. For empty string, no attempts to change the user ownership is done
	/// \param[in] slice_group the group to set the file to. For empty string, no attempts to change the group ownership is done
	/// \note this call may throw Erange exception upon system error
    extern void tools_set_ownership(S_I filedesc, const std::string & slice_user, const std::string & slice_group);

	/// Produces in "dest" the XORed value of "dest" and "src"

	/// \param[in,out] dest is the area where to write down the result
	/// \param[in] src points to vector or array of values to convert
	/// \param[in] n is the number of byte to convert from src to dest
	/// \note dest *must* be a valid pointer to an allocated memory area of at least n bytes
    extern void tools_memxor(void *dest, const void *src, U_I n);

	/// Produces a list of TLV from a constant type and a list of string

	/// \param[in,out] dialog for user interaction
	/// \param[in] type is the type each TLV will have
	/// \param[in] data is the list of string to convert into a list of TLV
	/// \return a tlv_list object. Each TLV in the list correspond to a string in the given list
    extern tlv_list tools_string2tlv_list(user_interaction & dialog, const U_16 & type, const std::vector<std::string> & data);



	/// Produces a pseudo random number x, where 0 <= x < max

	/// \param[in] max defines the range of the random number to return
	/// \return the returned value ranges from 0 (zero) up to max (= including max)
    extern U_I tools_pseudo_random(U_I max);


	/// Template for the decomposition of any number in any base (decimal, octal, hexa, etc.)

	/// \param[in] number is the number to decompose
	/// \param[in] base is the base to decompose the number into
	/// \return a vector of 'digit' int the specified base, the first beeing the less significative
	/// \note this template does not take care of the possibily existing optimized euclide division to speed up the operation
	/// like what exists for infinint. A specific overriden fonction for this type would be better.
	/// \note, the name "big_endian" is erroneous, it gives a little endian vector

    template <class N, class B> std::deque<B> tools_number_base_decomposition_in_big_endian(N number, const B & base)
    {
	std::deque<B> ret;

	if(base <= 0)
	    throw Erange("tools_number_decoupe_in_big_endian", "base must be strictly positive");

	while(number != 0)
	{
	    ret.push_back(number % base);
	    number /= base;
	}

	return ret;
    }

	/// convert a unsigned char into its hexa decima representation

	/// \param[in] x is the byte to convert
	/// \return the string representing the value of x written in hexadecimal
    extern std::string tools_unsigned_char_to_hexa(unsigned char x);

	/// convert a string into its hexadecima representation

	/// \param[in] input input string to convert
	/// \return a string containing an hexadecimal number corresponding to the bytes of the input string

    extern std::string tools_string_to_hexa(const std::string & input);

	/// Defines the CRC size to use for a given filesize

	/// \param[in] size is the size of the file to protect by CRC
	/// \return crc_size is the size of the crc to use
    extern infinint tools_file_size_to_crc_size(const infinint & size);

        /// get current working directory
    extern std::string tools_getcwd();

	/// return the string about compression ratio
    extern std::string tools_get_compression_ratio(const infinint & storage_size, const infinint & file_size, bool compressed);

	/// wrapper routine to strerror_r
    extern std::string tools_strerror_r(int errnum);

#ifdef GPGME_SUPPORT
	/// wrapper routint to gpgme_strerror_r
    extern std::string tools_gpgme_strerror_r(gpgme_error_t err);
#endif

#if HAVE_WCHAR_H
	/// convert a std::string to std::wstring (wide-string, aka string of wchar_t)
    extern std::wstring tools_string_to_wstring(const std::string & val);

	/// convert a std::wstring to std::string
    extern std::string tools_wstring_to_string(const std::wstring & val);
#endif

	/// allocate a new dirent structure for use with readdir_r
	///
	/// \param[in] path_name is the path of to the directory (and its underlying filesystem)
	/// where the resulting dirent will be used. Depending on fileystem, the size of the dirent
	/// structure may vary it is necessary to know the directory where the corresponding files
	/// resides
	/// \param[out] max_name_length is the maximum filename length allocated in the returned structure
	/// \return a pointer to the newly allocated dirent structure
    struct dirent *tools_allocate_struct_dirent(const std::string & path_name, U_64 & max_name_length);

	/// release a dirent structure as allocated by tools_allocate_struct_dirent
	///
	/// \param[in] ptr is the address of the structure to release
    extern void tools_release_struct_dirent(struct dirent *ptr);

	/// display the content of a secu_string, this function is only for trouble shooting!
    extern void tools_secu_string_show(user_interaction & dialog, const std::string & msg, const secu_string & key);

    template <class T> T tools_max(T a, T b) { return a > b ? a : b; }
    template <class T> T tools_min(T a, T b) { return a > b ? b : a; }

	/// remove from filesystem the file which path is given
    void tools_unlink(const std::string & filename);

	/// escape with a anti-slash character a set of chars found in the given string
    extern std::string tools_escape_chars_in_string(const std::string & val, const char *to_escape);

	/// convert an infinint to U_64 (aka "uint64_t" or yet "unsigned long long")

	/// \note: if the infinint is too large to fit in an U_64 it returns false
    extern bool tools_infinint2U_64(infinint val, U_64 & res);

        /// ascii to integer conversion

	/// \param[in] a is the ascii string to convert
	/// \param[out] val is the resulting value
	/// \return true if the conversion could be done false if the given string does not
	/// correspond to the decimal representation of an unsigned integer
	/// \note this call is now a warapper around line_tools_str2int
    extern bool tools_my_atoi(const char *a, U_I & val);

	/// convert a double (float) to infinint (integer) taking care of rounding it to the closest value
    extern infinint tools_double2infinint(double arg);

	/// check the value is not negative, and if asked set it to zero

	///\param[in,out] val variable which value to check
	///\param[in,out] ui for user interaction if necessary
	///\param[in] inode_path to the inode for message info
	///\param[in] nature type of the date/time (mtime,atime,ctime,birthtime,...)
	///\param[in] ask_before whether to just warn or ask user for confirmation
	///\param[in] silent if set, do not warn nor ask
    template <class T> void tools_check_negative_date(T & val,
						      user_interaction & ui,
						      const char *inode_path,
						      const char *nature,
						      bool ask_before,
						      bool silent)
    {
	if(val < 0)
	{
	    if(!silent)
	    {
		std::string msg = tools_printf(gettext("Found negative date (%s) for inode %s ."),
					       nature,
				      inode_path);
		if(ask_before)
		    ui.pause(tools_printf(gettext("%S Can we read it as if it was zero (1st January 1970 at 00:00:00 UTC)?"),
					  &msg));
		else // just warn
		    ui.message(msg + gettext("Considering date as if it was zero (Jan 1970)"));
	    }

	    val = 0;
	}
    }

} /// end of namespace

#endif
