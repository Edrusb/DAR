/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2020 Denis Corbin
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
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

    /// \file catalogue.hpp
    /// \brief here is defined the classe used to manage catalogue of archives
    /// \ingroup Private

#ifndef CATALOGUE_HPP
#define CATALOGUE_HPP

#include "../my_config.h"

extern "C"
{
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
} // end extern "C"

#include "infinint.hpp"
#include "path.hpp"
#include "integers.hpp"
#include "mask.hpp"
#include "user_interaction.hpp"
#include "label.hpp"
#include "escape.hpp"
#include "datetime.hpp"
#include "slice_layout.hpp"
#include "cat_entree.hpp"
#include "cat_nomme.hpp"
#include "cat_directory.hpp"
#include "mem_ui.hpp"
#include "delta_sig_block_size.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

	/// the catalogue class which gather all objects contained in a give archive
    class catalogue: public mem_ui
    {
    public :
        catalogue(const std::shared_ptr<user_interaction> & ui,
		  const datetime & root_last_modif,
		  const label & data_name);
        catalogue(const std::shared_ptr<user_interaction> & ui,
		  const pile_descriptor & pdesc,
		  const archive_version & reading_ver,
		  compression default_algo,
		  bool lax,
		  const label & lax_layer1_data_name, // ignored unless in lax mode, in lax mode unless it is a cleared label, forces the catalogue label to be equal to the lax_layer1_data_name for it be considered a plain internal catalogue, even in case of corruption
		  bool only_detruit = false); // if set to true, only directories and detruit objects are read from the archive
        catalogue(const catalogue & ref) : mem_ui(ref), out_compare(ref.out_compare) { partial_copy_from(ref); };
	catalogue(catalogue && ref) = delete;
        catalogue & operator = (const catalogue &ref);
	catalogue & operator = (catalogue && ref) = delete;
        virtual ~catalogue() { detruire(); };


	    // reading methods. The reading is iterative and uses the current_read cat_directory pointer

        virtual void reset_read() const; // set the reading cursor to the beginning of the catalogue
	virtual void end_read() const; // set the reading cursor to the end of the catalogue
        virtual void skip_read_to_parent_dir() const;
            // skip all items of the current dir and of any subdir, the next call will return
            // next item of the parent dir (no cat_eod to exit from the current dir !)
        virtual bool read(const cat_entree * & ref) const;
            // sequential read (generates cat_eod) and return false when all files have been read
        virtual bool read_if_present(std::string *name, const cat_nomme * & ref) const;
            // pseudo-sequential read (reading a directory still
            // implies that following read are located in this subdirectory up to the next EOD) but
            // it returns false if no entry of this name are present in the current directory
            // a call with nullptr as first argument means to set the current dir the parent directory
	void remove_read_entry(std::string & name);
	    // in the currently read directory, removes the entry which name is given in argument
	const cat_directory & get_current_reading_dir() const { if(current_read == nullptr) throw SRC_BUG; return *current_read; };
	    // remove from the catalogue all the entries that have not yet been read
	    // by read().
	virtual void tail_catalogue_to_current_read();


        void reset_sub_read(const path &sub); // initialise sub_read to the given directory
        bool sub_read(user_interaction & ui, const cat_entree * &ref); // sequential read of the catalogue, ignoring all that
            // is not part of the subdirectory specified with reset_sub_read
            // the read include the inode leading to the sub_tree as well as the pending cat_eod

	    // return true if the last read entry has already been read
	    // and has not to be counted again. This is never the case for catalogue but may occur
	    // with escape_catalogue (where from the 'virtual').
	    // last this method gives a valid result only if the last read() entry is a directory as
	    // only directory may be read() twice.
	virtual bool read_second_time_dir() const { return false; };


	    // Additions methods. The addition is also iterative but uses its specific current_add directory pointer

        void reset_add();

	    /// catalogue extension routines for escape sequence
	    // real implementation is only needed in escape_catalogue class, here there nothing to be done
	virtual void pre_add(const cat_entree *ref, const pile_descriptor* dest = nullptr) const {};
	virtual void pre_add_ea(const cat_entree *ref, const pile_descriptor* dest = nullptr) const {};
	virtual void pre_add_crc(const cat_entree *ref, const pile_descriptor* dest = nullptr) const {};
	virtual void pre_add_dirty( const pile_descriptor* dest = nullptr) const {};
	virtual void pre_add_ea_crc(const cat_entree *ref, const pile_descriptor* dest = nullptr) const {};
	virtual void pre_add_waste_mark(const pile_descriptor* dest = nullptr) const {};
	virtual void pre_add_failed_mark(const pile_descriptor* dest = nullptr) const {};
	virtual void pre_add_fsa(const cat_entree *ref, const pile_descriptor* dest = nullptr) const {};
	virtual void pre_add_fsa_crc(const cat_entree *ref, const pile_descriptor* dest = nullptr) const {};
	virtual void pre_add_delta_sig(const pile_descriptor* dest = nullptr) const {};
	virtual escape *get_escape_layer() const { return nullptr; };
	virtual void drop_escape_layer() {};

        void add(cat_entree *ref); // add at end of catalogue (sequential point of view)
	void re_add_in(const std::string &subdirname); // return into an already existing subdirectory for further addition
	void re_add_in_replace(const cat_directory &dir); // same as re_add_in but also set the properties of the existing directory to those of the given argument
        void add_in_current_read(cat_nomme *ref); // add in currently read directory
	const cat_directory & get_current_add_dir() const { if(current_add == nullptr) throw SRC_BUG; return *current_add; };



	    // Comparison methods. The comparision is here also iterative and uses its specific current_compare directory pointer

        void reset_compare() const;
        bool compare(const cat_entree * name, const cat_entree * & extracted) const;
            // returns true if the ref exists, and gives it back in second argument as it is in the current catalogue.
            // returns false is no entry of that nature exists in the catalogue (in the current directory)
            // if ref is a directory, the operation is normaly relative to the directory itself, but
            // such a call implies a chdir to that directory. thus, a call with an EOD is necessary to
            // change to the parent directory.
            // note :
            // if a directory is not present, returns false, but records the inexistant subdirectory
            // structure defined by the following calls to this routine, this to be able to know when
            // the last available directory is back the current one when changing to parent directory,
            // and then proceed with normal comparison of inode. In this laps of time, the call will
            // always return false, while it temporary stores the missing directory structure



	    // non interative methods


	    /// add into "this" detruit object corresponding to object of ref absent in "this"

	    ///\note ref must have the same directory tree "this", else the operation generates an exception
        infinint update_destroyed_with(const catalogue & ref);


	    /// copy from ref missing files in "this" and mark then as "not_saved" (no change since reference)

	    /// in case of abortion, completes missing files as if what could not be
	    /// inspected had not changed since the reference was done
	    /// aborting_last_etoile is the highest etoile reference withing "this" current object.
	void update_absent_with(const catalogue & ref, infinint aborting_next_etoile);


	    /// remove/destroy from "this" all objects that are neither directory nor detruit objects
	void drop_all_non_detruits();

	    /// check whether all inode existing in the "this" and ref have the same attributes

	    /// \note stops at the first inode found in both catalogue that do not match for at least one attribute
	    /// including CRC for DATA, EA or FSA if present, then return false.
	bool is_subset_of(const catalogue & ref) const;

	    /// before dumping the catalogue, need to set all hardlinked inode they have not been saved once
	void reset_dump() const;

	    /// write down the whole catalogue to file
        void dump(const pile_descriptor & pdesc) const;

        entree_stats get_stats() const { return stats; };

	    /// whether the catalogue is empty or not
	bool is_empty() const { if(contenu == nullptr) throw SRC_BUG; return contenu->is_empty(); };

        const cat_directory *get_contenu() const { return contenu; }; // used by data_tree

	const label & get_data_name() const { return ref_data_name; };
	void set_data_name(const label & val) { ref_data_name = val; };

	datetime get_root_dir_last_modif() const { return contenu->get_last_modif(); };

	    /// recursive evaluation of directories that have changed (make the cat_directory::get_recurisve_has_changed() method of entry in this catalogue meaningful)
	void launch_recursive_has_changed_update() const { contenu->recursive_has_changed_update(); };

	    /// recursive setting of mirage inode_wrote flag
	void set_all_mirage_s_inode_wrote_field_to(bool val) const { const_cast<cat_directory *>(contenu)->set_all_mirage_s_inode_wrote_field_to(val); };

	datetime get_root_mtime() const { return contenu->get_last_modif(); };

	    /// reset all pointers to the root (a bit better than reset_add() + reset_read() + reset_compare() + reset_sub_read())
	void reset_all();

	void set_to_unsaved_data_and_FSA() { if(contenu == nullptr) throw SRC_BUG; contenu->recursively_set_to_unsaved_data_and_FSA(); };

	    /// change location where to find EA, FSA and DATA for all the objects of the catalogue
	void change_location(const pile_descriptor & pdesc);

	    /// copy delta signatures to the given stack and update the cat_file objects accordingly

	    /// \param[in] destination where to drop delta signatures
	    /// \param[in] sequential_read whether we read the archive in sequential mode
	    /// \param[in] build if set and delta signature is not present but data is available for a file, calculate the delta sig
	    /// \param[in] delta_mask defines what files to calculate delta signature for when build is set to true
	    /// \param[in] delta_sig_min_size minimum size below which to never calculate delta signatures
	    /// \param[in] signature_block_size block size to use for computing delta signatures
	    /// \note this method relies on reset_read() and read()
	void transfer_delta_signatures(const pile_descriptor & destination,
				       bool sequential_read,
				       bool build,
				       const mask & delta_mask,
				       const infinint & delta_sig_min_size,
				       const delta_sig_block_size & signature_block_size);

	    /// remove delta signature from the catalogue object as if they had never been calculated
	void drop_delta_signatures();

    protected:
	entree_stats & access_stats() { return stats; };
	void copy_detruits_from(const catalogue & ref); // needed for escape_catalogue implementation only.

	const cat_eod * get_r_eod_address() const { return & r_eod; }; // cat_eod are never stored in the catalogue
	    // however it is sometimes required to return such a reference to a valid object
	    // owned by the catalogue.


	    /// invert the data tree memory management responsibility pointed to by "contenu" pointers between the current
	    /// catalogue and the catalogue given in argument.
	void swap_stuff(catalogue & ref);

    private :
        cat_directory *contenu;                   ///< catalogue contents
        mutable path out_compare;                 ///< stores the missing directory structure, when extracting
        mutable cat_directory *current_compare;   ///< points to the current directory when extracting
        mutable cat_directory *current_add;       ///< points to the directory where to add the next file with add_file;
        mutable cat_directory *current_read;      ///< points to the directory where the next item will be read
        path *sub_tree;                           ///< path to sub_tree
        mutable signed int sub_count;             ///< count the depth in of read routine in the sub_tree
        entree_stats stats;                       ///< statistics catalogue contents
	label ref_data_name;                      ///< name of the archive where is located the data

        void partial_copy_from(const catalogue &ref);
        void detruire();

        static const cat_eod r_eod;           ///< needed to return eod reference, without taking risk of saturating memory
	static const U_I CAT_CRC_SIZE;
    };



	/// @}

} // end of namespace

#endif
