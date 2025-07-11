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
#include "cat_delta_signature.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{


	/// the plain file class

    class cat_file : public cat_inode
    {
    public :

	    /// how to get data from archive
	enum get_data_mode
	{
	    keep_compressed, ///< provide access to compressed data
	    keep_hole,       ///< provide access to uncompressed data but sparse_file datastructure
	    normal,          ///< provide access to full data (uncompressed, uses skip() to restore holes)
	    plain            ///< provide access to plain data, no skip to restore holes, provide instead zeroed bytes
	};

	static constexpr U_8 FILE_DATA_WITH_HOLE = 0x01;     ///< file's data contains hole datastructure
	static constexpr U_8 FILE_DATA_IS_DIRTY = 0x02;      ///< data modified while being saved
	static constexpr U_8 FILE_DATA_HAS_DELTA_SIG = 0x04; ///< delta signature is present

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
        cat_file(const std::shared_ptr<user_interaction> & dialog,
		 const smart_pointer<pile_descriptor> & pdesc,
		 const archive_version & reading_ver,
		 saved_status saved,
		 compression default_algo,
		 bool small);
	cat_file(const cat_file & ref);
	cat_file(cat_file && ref) = delete;
	cat_file & operator = (const cat_file & ref) = delete;
	cat_file & operator = (cat_file && ref) = delete;
        ~cat_file() { detruit(); };

        virtual bool has_changed_since(const cat_inode & ref,
				       const infinint & hourshift,
				       comparison_fields what_to_check) const override;
        infinint get_size() const { return status == from_patch ? (*in_place).get_size() : *size; };
	void change_size(const infinint & s) const { *size = s; };
        infinint get_storage_size() const { return *storage_size; };
        void set_storage_size(const infinint & s) { *storage_size = s; };

	    /// check whether the object will be able to provide a object using get_data() method
	bool can_get_data() const { return get_saved_status() == saved_status::saved || get_saved_status() == saved_status::delta || status == from_path; };

	    /// returns a newly allocated object in read_only mode

	    /// \param[in] mode whether to return compressed, with hole or plain file
	    /// \param[in,out] delta_sig_mem if not nullptr, write to that file the delta signature of the file
	    /// \param[in] signature_block_size is the block size to use to build the signature (passed to librsync as is)
	    /// \param[in] delta_ref if not nullptr, use the provided signature to generate a delta binary patch
	    /// \param[in] checksum if not null will set *checsum to the address of a newly allocated crc object
	    /// that the caller has the duty to release when no more needed but *not before* the returned generic_file
	    /// object has been destroyed first. The computed crc is against the
	    /// real data found on disk not the one of the delta diff that could be generated from get_data()
	    /// \note the object pointed to by delta_sig must exist during the whole life of the returned
	    /// object, as well as the object pointed to by delta_ref if provided.
	    /// \note when both delta_sig and delta_ref are provided, the delta signature is computed on the
	    /// file data, then the delta binary is computed.
        virtual generic_file *get_data(get_data_mode mode,
				       std::shared_ptr<memory_file> delta_sig_mem,
				       U_I signature_block_size,
				       std::shared_ptr<memory_file> delta_ref,
				       const crc **checksum = nullptr) const;

        void clean_data() const; // partially free memory (but get_data() becomes disabled)

	    /// used while merging, chages the behavior of our get_data() to provide the patched version of the provided file data

	    /// \return true if and only if:
	    /// - we are a binary patch
	    /// - the provided cat_file has data saved
	    /// - crc of the provided file match our base CRC
	    /// Else, the call does nothing to this object, get_data() is unchanged.
	    /// \note if this mode is set successfully, get_data() must not be called with keep_compressed or keep_hole or normal, but in "plain" mode
	bool set_data_from_binary_patch(cat_file* in_place_addr);

	    /// return the save value as what set_data_from_binary_patch has provided
	bool applying_binary_patch() const { return status == from_patch && in_place != nullptr; };

        void set_offset(const infinint & r);
	const infinint & get_offset() const;
        virtual unsigned char signature() const override { return 'f'; };
	virtual std::string get_description() const override { return "file"; };

        void set_crc(const crc &c);
        bool get_crc(const crc * & c) const; ///< the argument is set to an allocated crc object owned by the "cat_file" object, its stay valid while this "cat_file" object exists and MUST NOT be deleted by the caller in any case
	bool has_crc() const { return check != nullptr; };
	bool get_crc_size(infinint & val) const; ///< returns true if crc is know and puts its width in argument
	void drop_crc() { if(check != nullptr) { delete check; check = nullptr; } };

	    // whether the plain file has to detect sparse file
	void set_sparse_file_detection_read(bool val) { if(status == from_cat) throw SRC_BUG; if(val) file_data_status_read |= FILE_DATA_WITH_HOLE; else file_data_status_read &= ~FILE_DATA_WITH_HOLE; };

	void set_sparse_file_detection_write(bool val) { if(val) file_data_status_write |= FILE_DATA_WITH_HOLE; else file_data_status_write &= ~FILE_DATA_WITH_HOLE; };

	    // whether the plain file is stored with a sparse_file datastructure in the archive
	bool get_sparse_file_detection_read() const { return (file_data_status_read & FILE_DATA_WITH_HOLE) != 0; };
	bool get_sparse_file_detection_write() const { return (file_data_status_write & FILE_DATA_WITH_HOLE) != 0; };

        virtual cat_entree *clone() const override { return new (std::nothrow) cat_file(*this); };

        compression get_compression_algo_read() const { return algo_read; };

	compression get_compression_algo_write() const { return algo_write; };

	    // object migration methods (merging)
	void change_compression_algo_write(compression x) { algo_write = x; };

	    // dirtiness

	bool is_dirty() const { return dirty; };
	void set_dirty(bool value) { dirty = value; };


	    /// return whether the object has an associated delta signature structure
	bool has_delta_signature_structure() const { return delta_sig != nullptr; };

	    /// return whether the object has an associated delta signature structure including a delta signature data (not just CRC)

	    /// \note when reading file from archive/generic_file if the metadata is not loaded to memory calling
	    /// either read_delta_signature_metadata() or read_delta_signature() *and* if sequential read mode is used
	    /// this call will always report false, even if delta signature can be available from filesystem/archive
	bool has_delta_signature_available() const { return delta_sig != nullptr && delta_sig->can_obtain_sig(); };


	    /// returns whether the object has a base patch CRC (s_delta status objects)
	bool has_patch_base_crc() const;

	    /// returns the CRC of the file to base the patch on, for s_delta objects
	bool get_patch_base_crc(const crc * & c) const;

	    /// set the reference CRC of the file to base the patch on, for s_detla objects
	void set_patch_base_crc(const crc & c);



	    /// returns whether the object has a CRC corresponding to data (for s_saved, s_delta, and when delta signature is present)
	bool has_patch_result_crc() const;

	    /// returns the CRC the file will have once restored or patched (for s_saved, s_delta, and when delta signature is present)
	bool get_patch_result_crc(const crc * & c) const;

	    /// set the CRC the file will have once restored or patched (for s_saved, s_delta, and when delta signature is present)
	void set_patch_result_crc(const crc & c);

	    /// prepare the object to receive a delta signature structure
	void will_have_delta_signature_structure();

	    /// prepare the object to receive a delta signature structure including delta signature

	    /// this calls will lead an to error if the delta_signature is written to archive or used while only CRC info
	    /// has been set (= metadata of delta signature) but no delta signature data has read from the archive or
	    /// has been provided (by mean of a memory_file when calling dump_delta_signature() method)
	void will_have_delta_signature_available();

	    /// write down to archive the given delta signature

	    /// \param[in] sig is the signature to dump
	    /// \param[in] sign_block_size block size to used to build the delta signature
	    /// \param[in] where is the location where to write down the signature
	    /// \param[in] small if set to true drop down additional information to allow sequential reading mode
	void dump_delta_signature(std::shared_ptr<memory_file> & sig, U_I sign_block_size, generic_file & where, bool small) const;

	    /// variant of dump_delta_signature when just CRC have to be dumped
	void dump_delta_signature(generic_file & where, bool small) const;


	    /// load metadata (and delta signature when in sequential mode) into memory

	    /// \note call drop_delta_signature_data() subsequently if only the metada is needed (when un sequential read mode or not, it does not hurt)
	void read_delta_signature_metadata() const;

	    /// fetch the delta signature from the archive

	    /// \param[out] delta_sig is either nullptr or points to a shared memory_file
	    /// containing the delta signature.
	    /// \param[out] block_len is the block size that has been used to build the signature
	    /// \note nullptr is returned if the delta_signature only contains CRCs
	void read_delta_signature(std::shared_ptr<memory_file> & delta_sig,
				  U_I & block_len) const;

	    /// drop the delta signature from memory (will not more be posible to be read, using read_delta_signature)
	void drop_delta_signature_data() const;

	    /// return true if ref and "this" have both equal delta signatures
	bool has_same_delta_signature(const cat_file & ref) const;

	    /// remove information about delta signature also associated CRCs if status is not s_delta
	void clear_delta_signature_only();

	    /// remove any information about delta signature
	void clear_delta_signature_structure();

	    /// not used
	virtual bool operator == (const cat_entree & ref) const override { return true; };

	    /// compare just data not inode information EA nor FSA
	bool same_data_as(const cat_file & other,
			  bool check_data,
			  const infinint & hourshift,
			  bool me_or_other_read_in_seq_mode);

	    /// expose the archive format the object of the backup this object comes from
	const archive_version & get_archive_version() const { return read_ver; };

    protected:
        virtual void sub_compare(const cat_inode & other,
				 bool isolated_mode,
				 bool seq_read_mode) const override;
        virtual void inherited_dump(const pile_descriptor & pdesc, bool small) const override;
	virtual void post_constructor(const pile_descriptor & pdesc) override;

        enum { from_path, from_cat, from_patch } status;

    private:
	std::string chemin;     ///< path to the data (when read from filesystem)
        infinint *offset;       ///< start location of the data in 'loc'
        infinint *size;         ///< size of the data (uncompressed)
        infinint *storage_size; ///< how much data used in archive (after compression)
        crc *check;             ///< crc computed on the data
	bool dirty;             ///< true when a file has been modified at the time it was saved
        compression algo_read;  ///< which compression algorithm to use to read the file's data
	compression algo_write; ///< which compression algorithm to use to write down (merging) the file's data
	bool furtive_read_mode; ///< used only when status equals "from_path"
	char file_data_status_read;  ///< defines the datastructure to use when reading the data
	char file_data_status_write; ///< defines the datastructure to apply when writing down the data
	crc *patch_base_check;       ///< when data contains a delta patch, moved from delta_sig since format 10.2
	cat_delta_signature *delta_sig; ///< delta signature and associated CRC
	mutable bool delta_sig_read; ///< whether delta sig has been read/initialized from filesystem
	archive_version read_ver; ///< archive format used/to use
	std::unique_ptr<cat_file> in_place; ///< when data is build from patch (merging context only) this is the object to apply the patch on (while we store the patch data)

	void sub_compare_internal(const cat_inode & other,
				  bool can_read_my_data,
				  bool can_read_other_data,
				  const infinint & hourshift,
				  bool seq_read_mode) const;

	void clean_patch_base_crc();

	void detruit();

	cat_file* clone_as_file() const;

    };

	/// @}

} // end of namespace

#endif
