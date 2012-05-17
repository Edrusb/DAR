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
// $Id: tools.hpp,v 1.12.2.1 2003/11/29 08:49:58 edrusb Rel $
//
/*********************************************************************/

#ifndef TOOLS_HPP
#define TOOLS_HPP

#include "../my_config.h"
#include <string>
#include <vector>
#include "path.hpp"
#include "generic_file.hpp"
#include "tuyau.hpp"
#include "integers.hpp"

namespace libdar
{

    extern char *tools_str2charptr(std::string x);
    extern void tools_write_string(generic_file & f, const std::string & s);
        // add a '\0' at the end
    extern void tools_read_string(generic_file & f, std::string & s);
        // read up to '\0' char
    extern void tools_write_string_all(generic_file & f, const std::string & s);
        // '\0' has no special meaning no '\0' at the end
    extern void tools_read_string_size(generic_file & f, std::string & s, infinint taille);
        // '\0' has no special meaning
    extern infinint tools_get_filesize(const path &p);
    extern infinint tools_get_extended_size(std::string s);
    extern char *tools_extract_basename(const char *command_name);
    extern void tools_split_path_basename(const char *all, path * &chemin, std::string & base);
    extern void tools_split_path_basename(const std::string &all, std::string & chemin, std::string & base);
    extern void tools_open_pipes(const std::string &input, const std::string & output, tuyau *&in, tuyau *&out);
    extern void tools_blocking_read(int fd, bool mode);
    extern std::string tools_name_of_uid(U_16 uid);
    extern std::string tools_name_of_gid(U_16 gid);
    extern std::string tools_int2str(S_I x);
    extern U_32 tools_str2int(const std::string & x);
    extern std::string tools_addspacebefore(std::string s, unsigned int expected_size);
    extern std::string tools_display_date(infinint date);
    extern void tools_system(const std::vector<std::string> & argvector);
    extern void tools_write_vector(generic_file & f, const std::vector<std::string> & x);
    extern void tools_read_vector(generic_file & f, std::vector<std::string> & x);
    extern std::string tools_concat_vector(const std::string & separator,
					   const std::vector<std::string> & x);
    std::vector<std::string> operator + (std::vector<std::string> a, std::vector<std::string> b);
    extern bool tools_is_member(const std::string & val, const std::vector<std::string> & liste);
    extern void tools_display_features(bool ea, bool largefile, bool nodump, bool special_alloc, U_I bits);
    extern bool is_equal_with_hourshift(const infinint & hourshift, const infinint & date1, const infinint & date2);
    extern bool tools_my_atoi(char *a, U_I & val);

    template <class T> std::vector<T> operator +=(std::vector<T> & a, const std::vector<T> & b)
    {
        a = a + b;
        return a;
    }
    extern const char *tools_get_from_env(const char **env, const char *clef);

        // base is the filename a slice
    extern void tools_check_basename(const path & loc, std::string & base, const std::string & extension);

	// get current working directory
    extern std::string tools_getcwd();

        // integer (agregates) manipulations
        // argument must be a regular interger (a bit field).
    template <class T> extern T tools_rotate_right_one_bit(T v)
    {
        bool retenue = (v & 1) != 0;

        v >>= 1;
        if(retenue)
            v |= T(1) << (sizeof(v)*8 - 1);

        return v;
    }

    template <class T> extern T tools_maxof_agregate(T unused) { return ~T(0) > 0 ? ~T(0) : ~tools_rotate_right_one_bit(T(1)); }

} // end of namespace

#endif
