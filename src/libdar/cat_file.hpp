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

    /// \file cat_file.hpp
    /// \brief class used to record plain files in a catalogue
    /// \ingroup Private

#ifndef CAT_FILE_HPP
#define CAT_FILE_HPP

#include "../my_config.h"

extern "C"
{
} // end extern "C"

#include "cat_inode.hpp"
#include "memory_file.hpp"
#include "cat_tools.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{


	/// the plain file class
    class cat_file : public cat_inode
    {
    public :
	enum get_data_mode
	{
	    keep_compressed, //< provide access to compressed data
	    keep_hole,       //< provide access to uncompressed data but sparse_file datastructure
	    normal,          //< provide access to full data (uncompressed, uses skip() to restore holes)
	    plain            //< provide access to plain data, no skip to restore holes, provide instead zeroed bytes
	};

	static const U_8 FILE_DATA_WITH_HOLE = 0x01; //< file's data contains hole datastructure
	static const U_8 FILE_DATA_IS_DIRTY = 0x02;  //< data modified while being saved
	static const U_8 FILE_DATA_HAS_DELTA_SIG = 0x04; //< delta signature is present

        cat_file(const infinint & xuid,
		 const infinint & xgid,
		 U_16 xperm,
		 const datetime & last_access,
		 const datetime & last_modif,
		 const datetime & last_change,
		 const std::string & src,
		 const path & che,
		 const infinint & taille,
		 const infinint & fs_device,
		 bool x_furtive_read_mode);
        cat_file(const cat_file & ref);
        cat_file(user_interaction & dialog,
		 const pile_descriptor & pdesc,
		 const archive_version & reading_ver,
		 saved_status saved,
		 compression default_algo,
		 bool small);
        ~cat_file() { detruit(); };

        bool has_changed_since(const cat_inode & ref, const infinint & hourshift, cat_inode::comparison_fields what_to_check) const;
        infinint get_size() const { return *size; };
	void change_size(const infinint & s) const { *size = s; };
        infinint get_storage_size() const { return *storage_size; };
        void set_storage_size(const infinint & s) { *storage_size = s; };

	    /// returns a newly allocated object in read_only mode
	    ///
	    /// \param[in] mode whether to return compressed, with hole or plain file
	    /// \param[in,out] delta_sig if not nullptr, write to that file the delta signature of the file
	    /// \param[in] delta_ref if not nullptr, use the provided signature to generate a delta binary
	    /// \note the object pointed to by delta_sig must exist during the whole life of the returned
	    /// object, as well as the object pointed to by delta_ref if provided.
	    /// \note when both delta_sig and delta_ref are provided, the delta signature is computed on the
	    /// file data, then the delta binary is computed.
        virtual generic_file *get_data(get_data_mode mode,
				       memory_file *delta_sig,
				       generic_file *delta_ref) const;
        void clean_data(); // partially free memory (but get_data() becomes disabled)
        void set_offset(const infinint & r);
	const infinint & get_offset() const;
        unsigned char signature() const { return mk_signature('f', get_saved_status()); };

        void set_crc(const crc &c);
        bool get_crc(const crc * & c) const; //< the argument is set the an allocated crc object the owned by the "cat_file" object, its stay valid while this "cat_file" object exists and MUST NOT be deleted by the caller in any case
	bool has_crc() const { return check != nullptr; };
	bool get_crc_size(infinint & val) const; //< returns true if crc is know and puts its width in argument
	void drop_crc() { if(check != nullptr) { delete check; check = nullptr; } };

	    // whether the plain file has to detect sparse file
	void set_sparse_file_detection_read(bool val) { if(status == from_cat) throw SRC_BUG; if(val) file_data_status_read |= FILE_DATA_WITH_HOLE; else file_data_status_read &= ~FILE_DATA_WITH_HOLE; };

	void set_sparse_file_detection_write(bool val) { if(val) file_data_status_write |= FILE_DATA_WITH_HOLE; else file_data_status_write &= ~FILE_DATA_WITH_HOLE; };

	    // whether the plain file is stored with a sparse_file datastructure in the archive
	bool get_sparse_file_detection_read() const { return (file_data_status_read & FILE_DATA_WITH_HOLE) != 0; };
	bool get_sparse_file_detection_write() const { return (file_data_status_write & FILE_DATA_WITH_HOLE) != 0; };

        cat_entree *clone() const { return new (get_pool()) cat_file(*this); };

        compression get_compression_algo_read() const { return algo_read; };

	compression get_compression_algo_write() const { return algo_write; };

	    // object migration methods (merging)
	void change_compression_algo_write(compression x) { algo_write = x; };

	    // dirtiness

	bool is_dirty() const { return dirty; };
	void set_dirty(bool value) { dirty = value; };


	    /// return whether the object has an associated delta signature
	bool has_delta_signature() const { return will_have_delta_sig; };


	    /// returns whether the object has a base patch CRC (s_delta status objects)
	bool has_patch_base_crc() const { return patch_base_check != nullptr; };

	    /// returns the CRC of the file to base the patch on, for s_delta objects
	bool get_patch_base_crc(const crc * & c) const;

	    /// set the reference CRC of the file to base the patch on, for s_detla objects
	void set_patch_base_crc(const crc & c);



	    /// returns whether the object has a CRC corresponding to data (for s_saved, s_delta, and when delta signature is present)
	bool has_patch_result_crc() const { return patch_result_check != nullptr || (get_saved_status() == s_saved && check != nullptr); };

	    /// returns the CRC the file will have once restored or patched (for s_saved, s_delta, and when delta signature is present)
	bool get_patch_result_crc(const crc * & c) const;

	    /// set the CRC the file will have once restored or patched (for s_saved, s_delta, and when delta signature is present)
	void set_patch_result_crc(const crc & c);

	    /// for small dump to have the delta_sig set
	    ///
	    /// \note at the time of the small dump the delta_signature is not even calculated so we
	    /// cannot use dump_delta_signature() to set this flag
	void will_have_delta_signature() { will_have_delta_sig = true; };

	    /// write down to archive the given delta signature
	    ///
	    /// \param[in] sig is the signature to dump
	    /// \param[in] where is the location where to write down the signature
	    /// \param[in] small if set to true drop down additional information to allow sequential reading mode
	void dump_delta_signature(memory_file & sig, generic_file & where, bool small);

	    /// fetch the delta signature from the archive
	    ///
	    /// \param[in] sig is the object where to copy to the delta signature
	void read_delta_signature(memory_file & sig) const;

	    /// return true if ref and "this" have both equal delta signatures
	bool has_same_delta_signature(const cat_file & ref) const;

	    /// remove any information about delta signature
	void clear_delta_signature();

    protected:
        void sub_compare(const cat_inode & other, bool isolated_mode) const;
        void inherited_dump(const pile_descriptor & pdesc, bool small) const;
	void post_constructor(const pile_descriptor & pdesc);

        enum { empty, from_path, from_cat } status;

    private:
	std::string chemin;     //< path to the data (when read from filesystem)
        infinint *offset;       //< start location of the data in 'loc'
        infinint *size;         //< size of the data (uncompressed)
        infinint *storage_size; //< how much data used in archive (after compression)
        crc *check;             //< crc computed on the data
	crc *patch_base_check;  //< stored crc of the file the delta diff has been based on (used for restoration)
	crc *patch_result_check;//< stored crc of the file once the patch has been successfully applied (used for incremental delta patch backup)
	bool dirty;             //< true when a file has been modified at the time it was saved
        compression algo_read;  //< which compression algorithm to use to read the file's data
	compression algo_write; //< which compression algorithm to use to write down (merging) the file's data
	bool furtive_read_mode; //< used only when status equals "from_path"
	char file_data_status_read;  //< defines the datastructure to use when reading the data
	char file_data_status_write; //< defines the datastructure to apply when writing down the data
	infinint delta_sig_offset;   //< if not nullptr, is the offset where to find the delta signature of the file
	infinint delta_sig_size;     //< storage size of the delta signature
	crc *delta_sig_crc;          //< delta_signature CRC
	bool will_have_delta_sig;    //< delta_signature will be calculated for this inode or is already present

        void detruit();
    };

	/// @}

} // end of namespace

#endif
