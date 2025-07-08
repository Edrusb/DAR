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

    /// \file cat_inode.hpp
    /// \brief base object for all inode types, managed EA and FSA, dates, permissions, ownership, ...
    /// \ingroup Private

#ifndef CAT_INODE_HPP
#define CAT_INODE_HPP

#include "../my_config.h"

extern "C"
{
} // end extern "C"

#include "infinint.hpp"
#include "ea.hpp"
#include "integers.hpp"
#include "mask.hpp"
#include "user_interaction.hpp"
#include "filesystem_specific_attribute.hpp"
#include "datetime.hpp"
#include "cat_nomme.hpp"
#include "archive_aux.hpp"

namespace libdar
{
	/// \addtogroup Private
	/// @{

	/// the root class for all cat_inode

    class cat_inode : public cat_nomme
    {
    public:

	    /// flag used to only consider certain fields when comparing/restoring inodes

        cat_inode(const infinint & xuid,
		  const infinint & xgid,
		  U_16 xperm,
		  const datetime & last_access,
		  const datetime & last_modif,
		  const datetime & last_change,
		  const std::string & xname,
		  const infinint & device);
        cat_inode(const std::shared_ptr<user_interaction> & dialog,
		  const smart_pointer<pile_descriptor> & pdesc,
		  const archive_version & reading_ver,
		  saved_status saved,
		  bool small);
        cat_inode(const cat_inode & ref);
	cat_inode(cat_inode && ref) noexcept: cat_nomme(std::move(ref)) { nullifyptr(); move_from(std::move(ref)); };
	cat_inode & operator = (const cat_inode & ref);
	cat_inode & operator = (cat_inode && ref) { cat_nomme::operator = (std::move(ref)); move_from(std::move(ref)); return *this; };
        ~cat_inode() noexcept(false);

        const infinint & get_uid() const { return uid; };
        const infinint & get_gid() const { return gid; };
        U_16 get_perm() const { return perm; };
        datetime get_last_access() const { return last_acc; };
        datetime get_last_modif() const { return last_mod; };
        void set_last_access(const datetime & x_time) { last_acc = x_time; };
        void set_last_modif(const datetime & x_time) { last_mod = x_time; };
	infinint get_device() const { if(fs_dev == nullptr) throw SRC_BUG; return *fs_dev; };

        bool same_as(const cat_inode & ref) const;
        bool is_more_recent_than(const cat_inode & ref, const infinint & hourshift) const;
	    // used for RESTORATION
        virtual bool has_changed_since(const cat_inode & ref, const infinint & hourshift, comparison_fields what_to_check) const;
            // signature() left as an abstract method
            // clone is abstract too
	    // used for INCREMENTAL BACKUP

        void compare(const cat_inode &other,
		     const mask & ea_mask,
		     comparison_fields what_to_check,
		     const infinint & hourshift,
		     bool symlink_date,
		     const fsa_scope & scope,
		     bool isolated_mode,  ///< do not try to compare pointed to data, EA of FSA (suitable for isolated catalogue)
		     bool seq_read_mode   ///< *this* is read in seq mode => do not try to fetch delta sig when both isolated and seq_read mode are used
	    ) const;
            // throw Erange exception if a difference has been detected
            // this is not a symetrical comparison, but all what is present
            // in the current object is compared against the argument
            // which may contain supplementary informations
	    // used for DIFFERENCE



            //////////////////////////////////
            // EXTENDED ATTRIBUTES Methods
            //

            // I : to know whether EA data is present or not for this object
        void ea_set_saved_status(ea_saved_status status);
        ea_saved_status ea_get_saved_status() const { return ea_saved; };

            // II : to associate EA list to an cat_inode object (mainly for backup operation) #EA_FULL only#
        void ea_attach(ea_attributs *ref);

	    /// the returned value is the address of an existing file of the cat_inode object and shall not be deleted by the caller
        const ea_attributs *get_ea() const;              //   #<-- EA_FULL *and* EA_REMOVED# for this call only
        void ea_detach() const; //discards any future call to get_ea() !
	infinint ea_get_size() const; //returns the size of EA (still valid if ea have been detached) mainly used to define CRC width

            // III : to record where is dump the EA in the archive #EA_FULL only#
        void ea_set_offset(const infinint & pos);
	bool ea_get_offset(infinint & pos) const;
        void ea_set_crc(const crc & val);
	void ea_get_crc(const crc * & ptr) const; ///< the argument is set to point to an allocated crc object owned by this "cat_inode" object, this reference stays valid while the "cat_inode" object exists and MUST NOT be deleted by the caller in any case
	bool ea_get_crc_size(infinint & val) const; ///< returns true if crc is know and puts its width in argument

            // IV : to know/record if EA and FSA have been modified # any EA status# and FSA status #
        datetime get_last_change() const { return last_cha; };
        void set_last_change(const datetime & x_time) { last_cha = x_time; };
	bool has_last_change() const { return !last_cha.is_null(); };
	    // old format did provide last_change only when EA were present, since archive
	    // format 8, this field is always present even in absence of EA. Thus it is
	    // still necessary to check if the cat_inode has a last_change() before
	    // using get_last_change() (depends on the version of the archive read).


            //////////////////////////////////
            // FILESYSTEM SPECIFIC ATTRIBUTES Methods
            //

	    // I : which FSA are present
	void fsa_set_saved_status(fsa_saved_status status);
	fsa_saved_status fsa_get_saved_status() const { return fsa_saved; };
	    /// gives the set of FSA family recorded for that inode
	fsa_scope fsa_get_families() const { if(fsa_families == nullptr) throw SRC_BUG; return infinint_to_fsa_scope(*fsa_families); };



	    // II : add or drop FSA list to the cat_inode
	void fsa_attach(filesystem_specific_attribute_list *ref);
	void fsa_partial_attach(const fsa_scope & val);            // #<-- FSA_PARTIAL only
	const filesystem_specific_attribute_list *get_fsa() const; // #<-- FSA_FULL only
	void fsa_detach() const; // discard any future call to get_fsa() !
	infinint fsa_get_size() const; // returns the size of FSA (still valid if fsal has been detached) / mainly used to define CRC size

	    // III : to record where FSA are dumped in the archive (only if fsa_status not empty !)
	void fsa_set_offset(const infinint & pos);
	bool fsa_get_offset(infinint & pos) const;
	void fsa_set_crc(const crc & val);
	void fsa_get_crc(const crc * & ptr) const;
	bool fsa_get_crc_size(infinint & val) const;

    protected:
        virtual void sub_compare(const cat_inode & other,
				 bool isolated_mode,
				 bool seq_read_mode) const {};
	bool get_small_read() const { return small_read; }; ///< true if object has been created by sequential reading of an archive

	    // inherited from cat_entree
        virtual void inherited_dump(const pile_descriptor & pdesc, bool small) const override;


    private :
        infinint uid;               ///< inode owner's user ID
        infinint gid;               ///< inode owner's group ID
        U_16 perm;                  ///< inode's permission
        datetime last_acc;          ///< last access time (atime)
	datetime last_mod;          ///< last modification time (mtime)
        datetime last_cha;          ///< last inode meta data change (ctime)
        ea_saved_status ea_saved;   ///< inode Extended Attribute status
	fsa_saved_status fsa_saved; ///< inode Filesystem Specific Attribute status

	bool small_read;         ///< whether we the object has been built with sequential-reading

            //  the following is used only if ea_saved == full
        infinint *ea_offset;     ///< offset in archive where to find EA
        ea_attributs *ea;        ///< Extended Attributes read or to be written down
	infinint *ea_size;       ///< storage size required by EA
            // the following is used if ea_saved == full or ea_saved == partial or
        crc *ea_crc;             ///< CRC computed on EA

	infinint *fsa_families; ///< list of FSA families present for that inode (set to nullptr in fsa_none mode)
	infinint *fsa_offset;    ///< offset in archive where to find FSA  # always allocated (to be reviewed)
	filesystem_specific_attribute_list *fsal; ///< Filesystem Specific Attributes read or to be written down # only allocated if fsa_saved if set to FULL
	infinint *fsa_size;      ///< storage size required for FSA
	crc *fsa_crc;            ///< CRC computed on FSA
	    //
	infinint *fs_dev;        ///< filesystem ID on which resides the inode (only used when read from filesystem)
	archive_version edit;    ///< need to know EA and FSA format used in archive file


	void nullifyptr() noexcept;
	void destroy() noexcept;
	void copy_from(const cat_inode & ref);
	void move_from(cat_inode && ref) noexcept;

	static const ea_attributs empty_ea;
    };

	/// @}

} // end of namespace

#endif
