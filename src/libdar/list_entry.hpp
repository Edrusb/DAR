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

    /// \file list_entry.hpp
    /// \brief class of objects describing an entry in the archive, used by archive::get_children_in_table
    /// \ingroup API


#ifndef LIST_ENTRY_HPP
#define LIST_ENTRY_HPP

#include <string>

#include "../my_config.h"
#include "infinint.hpp"
#include "deci.hpp"
#include "catalogue.hpp"
#include "tools.hpp"
#include "compressor.hpp"
#include "integers.hpp"

namespace libdar
{

	/// the list_entry class provides mean to get information about a particular entry in the archive
	///
	/// it provides methods for libdar to fill up such object and methods for API user
	/// to read the information. Each information uses its own method, thus it will require
	/// several call to different method to get the full description of the object.
	/// This has the advantage to let the possiblity to add new fields in the future
	/// without breaking anything in API user programs.
	/// \ingroup API
    class list_entry
    {
    public:
	list_entry();

	    // methods for API users
	    // field that are not set are returned as empty string

	const std::string & get_name() const { return my_name; };
	unsigned char get_type() const { return type; };
	bool is_dir() const { return type == 'd'; };
	bool is_file() const { return type == 'f'; };
	bool is_symlink() const { return type == 'l'; };
	bool is_char_device() const { return type == 'c'; };
	bool is_block_device() const { return type == 'b'; };
	bool is_unix_socket() const { return type == 's'; };
	bool is_named_pipe() const { return type == 'p'; };
	bool is_hard_linked() const { return hard_link; };
	bool is_removed_entry() const { return type == 'x'; };
	bool is_door_inode() const { return type == 'o'; };

	bool has_data_present_in_the_archive() const { return data_status == s_saved; };
	bool has_EA() const { return ea_status != inode::ea_none && ea_status != inode::ea_removed; };
	bool has_EA_saved_in_the_archive() const { return ea_status == inode::ea_full; };

	std::string get_uid() const { return deci(uid).human(); };
	std::string get_gid() const { return deci(gid).human(); };
	std::string get_perm() const { return tools_int2str(perm); };
	std::string get_last_access() const { return last_access != 0 ? tools_display_date(last_access) : ""; };
	std::string get_last_modif() const { return last_modif != 0 ? tools_display_date(last_modif) : ""; };
	std::string get_last_change() const { return last_change != 0 ? tools_display_date(last_change) : ""; };
	std::string get_file_size() const { return deci(file_size).human(); };
	std::string get_compression_ratio() const { return tools_get_compression_ratio(storage_size, file_size); };
	bool is_sparse() const { return sparse_file; };
	std::string get_compression_algo() const { return compression2string(compression_algo); };
	bool is_dirty() const { return dirty; };
	std::string get_link_target() const { return target; };
	std::string get_major() const { return tools_int2str(major); };
	std::string get_minor() const { return tools_int2str(minor); };

	    // methods for libdar to setup the object

	void set_name(const std::string & val) { my_name = val; };
	void set_type(unsigned char val) { type = val; };
	void set_hard_link(bool val) { hard_link = val; };
	void set_uid(const infinint & val) { uid = val; };
	void set_gid(const infinint & val) { gid = val; };
	void set_perm(U_16 val) { perm = val; };
	void set_last_access(const infinint & val) { last_access = val; };
	void set_last_modif(const infinint & val) { last_modif = val; };
	void set_saved_status(saved_status val) { data_status = val; };
	void set_ea_status(inode::ea_status val) { ea_status = val; };
	void set_last_change(const infinint & val) { last_change = val; };
	void set_file_size(const infinint & val) { file_size = val; };
	void set_storage_size(const infinint & val) { storage_size = val; };
	void set_is_sparse_file(bool val) { sparse_file = val; };
	void set_compression_algo(compression val) { compression_algo = val; };
	void set_dirtiness(bool val) { dirty = val; };
	void set_link_target(const std::string & val) { target = val; };
	void set_major(int val) { major = val; };
	void set_minor(int val) { minor = val; };

    private:
	std::string my_name;
	bool hard_link;
	unsigned char type;
	infinint uid;
	infinint gid;
	U_16 perm;
	infinint last_access;
	infinint last_modif;
	saved_status data_status;
	inode::ea_status ea_status;
	infinint last_change;
	infinint file_size;
	infinint storage_size;
	bool sparse_file;
	compression compression_algo;
	bool dirty;
	std::string target;
	int major;
	int minor;
    };

} // end of namespace

#endif
