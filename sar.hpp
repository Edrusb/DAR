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
// $Id: sar.hpp,v 1.22 2002/05/17 16:17:48 denis Rel $
//
/*********************************************************************/

#ifndef SAR_HPP
#define SAR_HPP

#pragma interface

#include <string>
#include "generic_file.hpp"
#include "header.hpp"
#include "path.hpp"

#define SAR_OPT_DEFAULT (SAR_OPT_WARN_OVERWRITE)
#define SAR_OPT_WARN_OVERWRITE 0x01
#define SAR_OPT_DONT_ERASE 0x02
#define SAR_OPT_PAUSE 0x04

class sar : public generic_file
{
public:
    sar(const string & base_name, const string & extension, int options, const path & dir);
    sar(const string & base_name, const string & extension, const infinint & file_size, const infinint & first_file_size, int options, const path & dir);
    ~sar();

	// inherited from generic_file
    bool skip(infinint pos);
    bool skip_to_eof();
    bool skip_relative(signed int x);
    infinint get_position();

	// informational routines
    infinint get_sub_file_size() const { return size; };
    infinint get_first_sub_file_size() const { return first_size; };
    bool get_total_file_number(infinint &num) const { num = of_last_file_num; return of_last_file_known; };
    bool get_last_file_size(infinint &num) const { num = of_last_file_size; return of_last_file_known; };

protected :
    int inherited_read(char *a, size_t sz);
    int inherited_write(char *a, size_t sz);

private :
    path archive_dir;
    string base, ext;
    infinint size;
    infinint first_size;
    infinint first_file_offset;
    infinint file_offset;

	// theses following variables are modified by open_file
	// else the are used only for reading
    infinint of_current;
    infinint of_max_seen;
    bool of_last_file_known;
    infinint of_last_file_num;
    infinint of_last_file_size;
    label of_internal_name;
    fichier *of_fd;
    char of_flag;
    bool initial;

	// theses are the option flags
    bool opt_warn_overwrite;
    bool opt_dont_erase;
    bool opt_pause;

    bool skip_forward(unsigned int x);
    bool skip_backward(unsigned int x);
    void close_file();
    void open_readonly(char *fic, const infinint &num);
    void open_writeonly(char *fic, const infinint &num);
    void open_file_init();
    void open_file(infinint num);
    void set_options(int opt);
    void set_offset(infinint offset);
    void open_last_file();
    header make_write_header(const infinint &num, char flag);
};


class trivial_sar : public generic_file
{
public:
    trivial_sar(generic_file *ref); // trivial_sar own the argument
    ~trivial_sar() { if(reference != NULL) delete reference; };

    bool skip(infinint pos) { return reference->skip(pos + offset); };
    bool skip_to_eof() { return reference->skip_to_eof(); };
    bool skip_relative(signed int x);
    infinint get_position();

protected:
    int inherited_read(char *a, size_t size) { return reference->read(a, size); };
    int inherited_write(char *a, size_t size) { return reference->write(a, size); };
    
private:
    generic_file *reference;
    infinint offset;
};

extern string sar_make_filename(string base_name, infinint num, string ext);

#endif
