/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2018 Denis Corbin
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

#include <memory>

namespace libdar
{

	/// \addtogroup Private
	/// @{

	// Datastructure in archive (DATA+METADATA)
	//
	//    SEQUENTIAL MODE - in the core of the archive
	// +------+------+---------------+----------+--------+
	// | base | sig  | sig data      | data CRC | result |
	// | CRC  | size | (if size > 0) |    if    |  CRC   |
	// |      |      |               | size > 0 |        |
	// +------+------+---------------+----------+--------+
	//
	//    DIRECT MODE - in catalogue at end of archive (METADATA)
	// +------+------+---------------+--------+
	// | base | sig  | sig offset    | result |
	// | CRC  | size | (if size > 0) |  CRC   |
	// |      |      |               |        |
	// +------+------+---------------+--------+
	//
	//    DIRECT MODE - in the core of the archive (DATA)
	// +---------------+----------+
	// | sig data      | data CRC |
	// | (if size > 0) |    if    |
	// |               | size > 0 |
	// +---------------+----------+
	//
	//
	// this structure is used for all cat_file inode that have
	// either a delta signature or contain a delta patch (s_delta status)
	// base_crc is used to check we apply the patch on the good file before proceeding
	// result_crc is used to store the crc of the file the signature has been computed on
	// result_crc is also used once a patch has been applied to verify the correctness of the patch result


	/// the cat_delta_signature file class

	/// this class works in to implicit modes
	/// - read mode
	/// read the metadata from an archive the caller having knowing where to find it
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
	    /// constructor reading the object METADATA (and also its DATA when in sequential mode) from an archive

	    /// \param[in] f where to read the data from
	    /// \param[in] sequential_read if true read the whole metadata+data as it was dropped in sequential read mode, else only read the metadata (found in a catalogue)
	cat_delta_signature(generic_file & f, bool sequential_read);

	    /// constructor creating a brand new empty object
	cat_delta_signature() { init(); };

	    /// copy constructor
	cat_delta_signature(const cat_delta_signature & ref) { init(); copy_from(ref); };

	    /// move constructor
	cat_delta_signature(cat_delta_signature && ref) noexcept { init(); move_from(std::move(ref)); };

	    /// assignement operator
	cat_delta_signature & operator = (const cat_delta_signature & ref) { destroy(); init(); copy_from(ref); return *this; };

	    /// move assignment operator
	cat_delta_signature & operator = (cat_delta_signature && ref) noexcept { move_from(std::move(ref)); return *this; };

	    /// destructor
	~cat_delta_signature() { destroy(); };

	    /////////// method for read mode ///////////

	    /// same action as the first constructor but on an existing object

	    /// \note in sequential_read mode the data is also read
	void read(generic_file & f, bool sequential_read);

	    /// the cat_delta_signature structure can only hold CRC without delta_signature, this call gives the situation about that point
	bool can_obtain_sig() { return !just_crc; };

	    /// provide a memory_file object which the caller has the duty to destroy after use

	    /// \note to obtain the sig data a second time, one must call read_data() again, then obtain_sig() should succeed
	std::shared_ptr<memory_file> obtain_sig();

	    /// drop signature but keep metadata available
	void drop_sig() { sig.reset(); };

	    /////////// method for write mode ///////////

	    /// give the object where to fetch from the delta signature, object must exist up to the next call to dump_data

	    /// \note seg_sig_ref() must be called each time before invoking dump_data(), normally it is done once...

	    /// for can_obtain_sig() to return true before the signature is provided
	void will_have_signature() { just_crc = false; };

	    /// the object pointed to by ptr must stay available when calling dump_data()/dump_metadata() later on
	void set_sig(const std::shared_ptr<memory_file> & ptr);

	    /// variante used when the delta_signature object will only contain CRCs (no delta signature)
	void set_sig() { just_crc = true; delta_sig_size = 0; delta_sig_offset = 0; sig.reset(); };

	    /// write down the data eventually with sequential read mark followed by delta sig metadata
	void dump_data(generic_file & f, bool sequential_mode) const;

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
	crc *patch_base_check;      ///< associated CRC for the file this signature has been computed on
	infinint delta_sig_size;    ///< size of the data to setup "sig" (set to zero when reading in sequential mode, sig is then setup on-fly)
	infinint delta_sig_offset;  ///< where to read data from to setup "sig" (set to zero when read in sequential mode, sig is setup on-fly)
	mutable std::shared_ptr<memory_file>sig; ///< the signature data, if set nullptr it will be fetched from f in direct access mode only
	crc *patch_result_check;    ///< associated CRC
	bool just_crc;              ///< whether a delta signature is present or just checksum are stored
	generic_file *src;          ///< where to read data from

	void init() noexcept;
	void copy_from(const cat_delta_signature & ref);
	void move_from(cat_delta_signature && ref) noexcept;
	void destroy() noexcept;
	void fetch_data(generic_file & f) const;
    };

	/// @}

} // end of namespace

#endif
