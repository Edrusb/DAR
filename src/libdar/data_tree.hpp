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

    /// \file data_tree.hpp
    /// \brief two classes used to store tree information in dar_manager databases
    /// \ingroup Private


#ifndef DATA_TREE_HPP
#define DATA_TREE_HPP

#include "../my_config.h"

#include <map>
#include <string>
#include <list>
#include <set>
#include "infinint.hpp"
#include "generic_file.hpp"
#include "infinint.hpp"
#include "user_interaction.hpp"
#include "path.hpp"
#include "cat_directory.hpp"
#include "cat_inode.hpp"
#include "cat_detruit.hpp"

namespace libdar
{

	/// \ingroup Private
	/// @}

    typedef U_16 archive_num;
#define ARCHIVE_NUM_MAX  65534

	/// the data_tree class stores presence of a given file in a set of archives

	/// the data associated to a given file are the different modification dates
	/// that this file has been found in the archive the database has been feed by
	/// \ingroup Private
    class data_tree
    {
    public:
	enum lookup         //< the available status of a lookup
	{
	    found_present,  //< file data/EA has been found completely usable
	    found_removed,  //< file data/EA has been found as removed at that date
	    not_found,      //< no such file has been found in any archive of the base
	    not_restorable  //< file data/EA has been found existing at that date but not possible to restore (lack of data, missing archive in base, etc.)
	};

	enum etat
	{
	    et_saved,       //< data/EA present in the archive
	    et_patch,       //< data present as patch from the previous version
	    et_patch_unusable, //< data present as patch but base version not found in archive set
	    et_present,     //< file/EA present in the archive but data not saved (differential backup)
	    et_removed,     //< file/EA stored as deleted since archive of reference of file/EA not present in the archive
	    et_absent       //< file not even mentionned in the archive, This entry is equivalent to et_removed, but is required to be able to properly re-order the archive when user asks to do so. The dates associated to this state are computed from neighbor archives in the database
	};

	data_tree(const std::string &name);
	data_tree(generic_file & f, unsigned char db_version);
	data_tree(const data_tree & ref) = default;
	data_tree(data_tree && ref) noexcept = default;
	data_tree & operator = (const data_tree & ref) = default;
	data_tree & operator = (data_tree && ref) noexcept = default;
	virtual ~data_tree() = default;

	virtual void dump(generic_file & f) const;
	std::string get_name() const { return filename; };
	void set_name(const std::string & name) { filename = name; };

	    /// returns the archives to restore in order to obtain the data that was defined just before (or at) the given date
	    ///
	    /// \param[out] archive is the set of archive to restore in sequence to obtain the requested data
	    /// \param[in] date date above which to ignore data found in the database
	    /// \param[in] even_when_removed is true when user requested to restore the file in its latest state even if it has been removed afterward
	    /// \return the success of failure status of the requested lookup
	lookup get_data(std::set<archive_num> & archive, const datetime & date, bool even_when_removed) const;

	    /// if EA has been saved alone later, returns in which version for the state of the file at the given date.
	lookup get_EA(archive_num & archive, const datetime & date, bool even_when_removed) const;

	    /// return the date of file's last modification date within the give archive and whether the file has been saved or deleted
	bool read_data(archive_num num,
		       datetime & val,
		       etat & present) const;

	    /// return the date of last inode change and whether the EA has been saved or deleted
	bool read_EA(archive_num num, datetime & val, etat & present) const;

	void set_data(const archive_num & archive,
		      const datetime & date,
		      etat present) { set_data(archive, date, present, nullptr, nullptr); };

	void set_data(const archive_num & archive,
		      const datetime & date,
		      etat present,
		      const crc *base,
		      const crc *result) { last_mod[archive] = status_plus(date, present, base, result); (void) check_delta_validity(); };

	void set_EA(const archive_num & archive, const datetime & date, etat present) { status sta(date, present); last_change[archive] = sta; };

	    /// check date order between archives withing the database ; throw Erange if problem found with date order
	virtual bool check_order(user_interaction & dialog, const path & current_path, bool & initial_warn) const { return check_map_order(dialog, last_mod, current_path, "data", initial_warn) && check_map_order(dialog, last_change, current_path, "EA", initial_warn); };

	    /// add deleted entry if no object of the current archive exist and the entry of the previous archive is already present.

	    /// \param[in] archive is the number of the archive to finalize
	    /// \param[in] deleted_date date of deletion to use for inode removal when no
	    /// information can be grabbed from the archive (this date is taken from the
	    /// parent dir last modification date)
	    /// \param[in] ignore_archive_greater_or_equal ignore archives which number
	    /// is greater or equal than "ignore_archive_greater_or_equal" as if they were not
	    /// present in the database. If set to zero, no archive is ignored.
	virtual void finalize(const archive_num & archive,
			      const datetime & deleted_date,
			      const archive_num & ignore_archive_greater_or_equal);

	    /// return true if the corresponding file is no more located in any archive (thus, the object is no more usefull in the base)
	virtual bool remove_all_from(const archive_num & archive_to_remove, const archive_num & last_archive);

	    /// list where is saved this file
	void listing(user_interaction & dialog) const;
	virtual void apply_permutation(archive_num src, archive_num dst);

	    /// decrement archive numbers above num
	virtual void skip_out(archive_num num);
	virtual void compute_most_recent_stats(std::vector<infinint> & data,
					       std::vector<infinint> & ea,
					       std::vector<infinint> & total_data,
					       std::vector<infinint> & total_ea) const;

	virtual char obj_signature() const { return signature(); };
	static char signature() { return 't'; };

	    // fix corruption case that was brought by bug in release 2.4.0 to 2.4.9
	virtual bool fix_corruption(); // return true whether corruption could be fixed (meaning this entry can be safely removed from base)

    private:

	class status
	{
	public:
	    status(): date(0) { present = et_absent; };
	    status(const datetime & d, etat p) { date = d; present = p; };
	    status(const status & ref) = default;
	    status(status && ref) noexcept = default;
	    status & operator = (const status & ref) = default;
	    status & operator = (status && ref) noexcept = default;
	    virtual ~status() = default;

	    datetime date;                             //< date of the event
	    etat present;                              //< file's status in the archive

	    virtual void dump(generic_file & f) const; //< write the struct to file
	    virtual void read(generic_file &f,         //< set the struct from file
			      unsigned char db_version);
	};


	class status_plus : public status
	{
	public:
	    status_plus() { base = result = nullptr; };
	    status_plus(const datetime & d, etat p, const crc *xbase, const crc *xresult);
	    status_plus(const status_plus & ref): status(ref) { copy_from(ref); };
	    status_plus(status_plus && ref): status(std::move(ref)) { nullifyptr(); move_from(std::move(ref)); };
	    status_plus & operator = (const status_plus & ref) { detruit(); copy_from(ref); return *this; };
	    status_plus & operator = (status_plus && ref) noexcept { status_plus::operator = (std::move(ref)); move_from(std::move(ref)); return *this; };
	    ~status_plus() { detruit(); };

	    crc *base; //< only present for s_delta status, to have a link with the file to apply the patch to
	    crc *result; //< present for s_delta, s_saved, s_not_saved this is the crc of the data (or crc of the data resulting from the patch)

	    void dump(generic_file & f) const; //< write the struct to file
	    void read(generic_file &f,         //< set the struct from file
		      unsigned char db_version);

	private:
	    void nullifyptr() noexcept { base = result = nullptr; };
	    void copy_from(const status_plus & ref);
	    void move_from(status_plus && ref) noexcept;
	    void detruit();
	};


	std::string filename;
	std::map<archive_num, status_plus> last_mod; //< key is archive number ; value is last_mod time
	std::map<archive_num, status> last_change;   //< key is archive number ; value is last_change time


	    // when false is returned, this means that the user wants to ignore subsequent error of the same type
	    // else either no error yet met or user want to continue receiving the same type of error for other files
	    // in that later case initial_warn is set to false (first warning has been shown).
	template <class T> bool check_map_order(user_interaction & dialog,
						const std::map<archive_num, T> the_map,
						const path & current_path,
						const std::string & field_nature,
						bool & initial_warn) const;

	bool check_delta_validity(); // return true if no error has been met about delta patch (no delta is broken, missing its reference)
    };

	/// the data_dir class inherits from data_tree and holds the directory tree's parent relationship

	/// \ingroup Private
    class data_dir : public data_tree
    {
    public:
	data_dir(const std::string &name);
	data_dir(generic_file &f, unsigned char db_version);
	data_dir(const data_tree & ref);
	data_dir(const data_dir & ref);
	data_dir(data_dir && ref) noexcept = default;
	data_dir & operator = (const data_dir & ref) { rejetons.clear(); return *this; };
	data_dir & operator = (data_dir && ref) noexcept = default;
	~data_dir();

	virtual void dump(generic_file & f) const override;

	void add(const cat_inode *entry, const archive_num & archive);
	void add(const cat_detruit *entry, const archive_num & archive);
	const data_tree *read_child(const std::string & name) const;
	void read_all_children(std::vector<std::string> & fils) const;
	virtual void finalize_except_self(const archive_num & archive,
					  const datetime & deleted_date,
					  const archive_num & ignore_archives_greater_or_equal);

	    // inherited methods
	virtual bool check_order(user_interaction & dialog, const path & current_path, bool & initial_warn) const override;
	virtual void finalize(const archive_num & archive, const datetime & deleted_date, const archive_num & ignore_archives_greater_or_equal) override;
	virtual bool remove_all_from(const archive_num & archive_to_remove, const archive_num & last_archive) override;

	    /// list the most recent files owned by that archive (or by any archive if num == 0)
	void show(user_interaction & dialog, archive_num num, std::string marge = "") const;
	virtual void apply_permutation(archive_num src, archive_num dst) override;
	virtual void skip_out(archive_num num) override;
	void compute_most_recent_stats(std::vector<infinint> & data, std::vector<infinint> & ea,
				       std::vector<infinint> & total_data, std::vector<infinint> & total_ea) const;

	virtual char obj_signature() const override { return signature(); };
	static char signature() { return 'd'; };

	virtual bool fix_corruption() override; // inherited from data_tree


    private:
	std::list<data_tree *> rejetons;          //< subdir and subfiles of the current dir

	void add_child(data_tree *fils);          //< "this" is now responsible of "fils" disalocation
	void remove_child(const std::string & name);
	data_tree *find_or_addition(const std::string & name, bool is_dir, const archive_num & archive);
    };

    extern data_dir *data_tree_read(generic_file & f, unsigned char db_version);

	/// lookup routine to find a pointer to the dat_tree object corresponding to the given path

	/// \param[in] chemin is the path to look for
	/// \param[in] racine is the database to look into
	/// \param[out] ptr is a pointer to the looked node if found
	/// \return true if a node could be found in the database
    extern bool data_tree_find(path chemin, const data_dir & racine, const data_tree *& ptr);
    extern void data_tree_update_with(const cat_directory *dir, archive_num archive, data_dir *racine);
    extern archive_num data_tree_permutation(archive_num src, archive_num dst, archive_num x);

	/// @}

} // end of namespace

#endif
