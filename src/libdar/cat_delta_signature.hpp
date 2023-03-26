/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2023 Denis Corbin
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

    /// \file cat_delta_signature.hpp
    /// \brief class used to manage binary delta signature in catalogue and archive
    /// \ingroup Private

#ifndef CAT_DELTA_SIGNATURE_HPP
#define CAT_DELTA_SIGNATURE_HPP

#include "../my_config.h"

extern "C"
{
} // end extern "C"

#include "memory_file.hpp"
#include "crc.hpp"
#include "proto_compressor.hpp"
#include "archive_version.hpp"

#include <memory>

namespace libdar
{

	/// \addtogroup Private
	/// @{

	// Datastructure in archive (DATA+METADATA)
	//
	//    SEQUENTIAL MODE - in the core of the archive
	// +------+------+-----------+--------------+----------+--------+
	// | base | sig  | sig block |sig data      | data CRC | result |
	// | CRC  | size | len (if   |(if size > 0) |    if    |  CRC   |
	// | (*)  |      | size > 0) |              | size > 0 |        |
	// +------+------+-----------+--------------+----------+--------+
	//
	//    DIRECT MODE - in catalogue at end of archive (METADATA)
	// +------+------+---------------+--------+
	// | base | sig  | sig offset    | result |
	// | CRC  | size | (if size > 0) |  CRC   |
	// | (*)  |      |               |        |
	// +------+------+---------------+--------+
	//
	//    DIRECT MODE - in the core of the archive (DATA)
	// +-----------+---------------+----------+
	// | sig block | sig data      | data CRC |
	// | len (if   | (if size > 0) |    if    |
	// | size > 0) |               | size > 0 |
	// +-----------+---------------+----------+
	//
	// (*) base_CRC has been moved to cat_file structure since format 11,2
	//     in order to read this field before the delta patch in sequential read mode
	//
	// this structure is used for all cat_file inode that have
	// either a delta signature or contain a delta patch (s_delta status)
	// base_crc is used to check we apply the patch on the good file before proceeding
	// result_crc is used to store the crc of the file the signature has been computed on
	// result_crc is also used once a patch has been applied to verify the correctness of the patch result


	/// the cat_delta_signature file class

	/// this class works in two implicit modes
	/// - read mode
	/// read the metadata from an archive the caller having to know where to find it
	/// read the data and fill the provided memory_file (get_sig()) by the delta signature
	/// provide access to the associated CRC
	/// - write mode
	/// stores the associated CRC
	/// write down the given delta signature (with metadata in sequential mode)
	/// write down the metadata
	/// the signature is not stored
    class cat_delta_signature
    {
    public:
	    /// constructor to read an object (using read() later on) from filesystem/backup

	    /// \param[in] f where to read the data from, used when calling read() later on. f must not
	    /// be set to nullptr and the pointed to object must survive this cat_delta_signature object
	    /// \param[in] c points to the compressor layer in order to suspend compression when reading
	    /// data (metadata will be read compressed or not depending on the its location (in-lined or
	    /// in the catalogue at end of archive)
	cat_delta_signature(generic_file *f, proto_compressor* c);

	    /// constructor to write an object to filesytem/backup (using dump_* methods later on)
	cat_delta_signature() { init(); };

	    /// copy constructor
	cat_delta_signature(const cat_delta_signature & ref) { init(); copy_from(ref); };

	    /// move constructor
	cat_delta_signature(cat_delta_signature && ref) noexcept { init(); move_from(std::move(ref)); };

	    /// assignement operator
	cat_delta_signature & operator = (const cat_delta_signature & ref) { clear(); copy_from(ref); return *this; };

	    /// move assignment operator
	cat_delta_signature & operator = (cat_delta_signature && ref) noexcept { move_from(std::move(ref)); return *this; };

	    /// destructor
	~cat_delta_signature() { destroy(); };

	    /////////// method for read mode ///////////

	    /// tells whether the read() call has been invoked
	bool is_pending_read() const { return pending_read; };

	    /// read the metadata of the object from the generic_file given at construction time
	    /// \note in sequential read mode, the data is also read at that time and loaded into memory,
	    /// thing which is done transparently by obtain_sig() when in direct access mode
	void read(bool sequential_read, const archive_version & ver);

	    /// the cat_delta_signature structure can only hold CRC without delta_signature, this call gives the situation about that point
	bool can_obtain_sig() const { return !delta_sig_size.is_zero(); };

	    /// provide a memory_file object which the caller has the duty to destroy after use

	    /// \note while drop_sig has not been called, obtain_sig() can be called any number of time
	    /// \note in direct mode (not sequential_real mode) the first call to obtain_sig() fetches
	    /// the data from the archive and loads it to memory.
	std::shared_ptr<memory_file> obtain_sig(const archive_version & ver) const;

	    /// provide the block size used for delta signature
	U_I obtain_sig_block_size() const { return sig_block_len; };

	    /// drop signature but keep metadata available

	    /// \note there is a lot of chance that a call to obtain_sig() will fail after drop_sig() has been
	    /// called when in sequential read mode, due to the limited possibility to skip backward in that mode
	void drop_sig() const { sig.reset(); };

	    /////////// method for write mode ///////////

	    /// give the object where to fetch from the delta signature, object must exist up to the next call to dump_data

	    /// \note seg_sig_ref() must be called each time before invoking dump_data(), normally it is done once...

	    /// for can_obtain_sig() to return true before the signature is provided
	void will_have_signature() { delta_sig_size = 1; };

	    /// the object pointed to by ptr must stay available when calling dump_data()/dump_metadata() later on

	    /// \note sig_block_size is an additional information about the block size used to setup the signature,
	    /// this is not the size of the signature!
	void set_sig(const std::shared_ptr<memory_file> & ptr, U_I sig_block_size);

	    /// variante used when the delta_signature object will only contain CRCs (no delta signature)
	void set_sig() { delta_sig_size = 0; delta_sig_offset = 0; sig_block_len = 0; sig.reset(); };

	    /// write down the data eventually with sequential read mark followed by delta sig metadata

	    /// \note ver is only used to know which version to use for reading the data, but it is
	    /// always written following the most recent supported archive format version
	void dump_data(generic_file & f, bool sequential_mode, const archive_version & ver) const;

	    /// write down the delta_signature metadata for catalogue
	void dump_metadata(generic_file & f) const;


	    /////////// method for both read and write modes ///////////

	    /// returns whether the object has a base patch CRC (s_delta status objects)
	bool has_patch_base_crc() const { return patch_base_check != nullptr; };

	    /// returns the CRC of the file to base the patch on, for s_delta objects
	bool get_patch_base_crc(const crc * & c) const;

	    /// set the reference CRC of the file to base the patch on, for s_detla objects
	void set_patch_base_crc(const crc & c);

	    /// returns whether the object has a CRC corresponding to data (for s_saved, s_delta, and when delta signature is present)
	bool has_patch_result_crc() const { return patch_result_check != nullptr; };

	    /// returns the CRC the file will have once restored or patched (for s_saved, s_delta, and when delta signature is present)
	bool get_patch_result_crc(const crc * & c) const;

	    /// set the CRC the file will have once restored or patched (for s_saved, s_delta, and when delta signature is present)
	void set_patch_result_crc(const crc & c);

	    /// reset the object
	void clear() { destroy(); init(); };

    private:
	crc *patch_base_check;      ///< associated CRC for the file this signature has been computed on, moved to cat_file since format 11.2, still need for older formats
	infinint delta_sig_size;    ///< size of the data to setup "sig" (set to zero when reading in sequential mode, sig is then setup on-fly)
	infinint delta_sig_offset;  ///< where to read sig_block_len followed by delta_sig_size bytes of data from which to setup "sig"
	    ///\note delta_sig_offset is set to zero when read in sequential mode, sig is setup on-fly
	mutable std::shared_ptr<memory_file>sig; ///< the signature data, if set nullptr it will be fetched from f in direct access mode only
	crc *patch_result_check;    ///< associated CRC
	generic_file *src;          ///< where to read data from
	proto_compressor *zip;      ///< needed to disable compression when reading delta signature data from an archive
	mutable U_I sig_block_len;  ///< block length used within delta signature
	bool pending_read;          ///< when the object has been created for read but data not yet read from archive

	void init() noexcept;
	void copy_from(const cat_delta_signature & ref);
	void move_from(cat_delta_signature && ref) noexcept;
	void destroy() noexcept;

	    // reads sig_block_len + sig data
	void fetch_data(const archive_version & ver) const;
    };

	/// @}

} // end of namespace

#endif
