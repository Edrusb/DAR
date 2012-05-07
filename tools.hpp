/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002 Denis Corbin
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
// $Id: tools.hpp,v 1.10 2002/10/31 21:02:37 edrusb Rel $
//
/*********************************************************************/

#ifndef TOOLS_HPP
#define TOOLS_HPP

#include <string>
#include <vector>
#include "path.hpp"
#include "generic_file.hpp"
#include "tuyau.hpp"
#include "integers.hpp"

using namespace std;

extern char *tools_str2charptr(string x);
extern void tools_write_string(generic_file & f, const string & s); 
    // add a '\0' at the end
extern void tools_read_string(generic_file & f, string & s); 
    // read up to '\0' char
extern void tools_write_string_all(generic_file & f, const string & s); 
    // '\0' has no special meaning no '\0' at the end
extern void tools_read_string_size(generic_file & f, string & s, infinint taille); 
    // '\0' has no special meaning
extern infinint tools_get_filesize(const path &p);
extern infinint tools_get_extended_size(string s);
extern char *tools_extract_basename(const char *command_name); 
extern void tools_split_path_basename(const char *all, path * &chemin, string & base);
extern void tools_split_path_basename(const string &all, string & chemin, string & base);
extern void tools_open_pipes(const string &input, const string & output, tuyau *&in, tuyau *&out);
extern void tools_blocking_read(int fd, bool mode);
extern string tools_name_of_uid(U_16 uid);
extern string tools_name_of_gid(U_16 gid);
extern string tools_int2str(int x);
extern U_32 tools_str2int(const string & x);
extern string tools_addspacebefore(string s, unsigned int expected_size);
extern string tools_display_date(infinint date);
extern void tools_system(const vector<string> & argvector);
extern void tools_write_vector(generic_file & f, const vector<string> & x);
extern void tools_read_vector(generic_file & f, vector<string> & x);
extern string tools_concat_vector(const string & separator, const vector<string> & x);
vector<string> operator + (vector<string> a, vector<string> b);

template <class T> vector<T> operator +=(vector<T> & a, const vector<T> & b)
{
    a = a + b;
    return a;
}

#endif
