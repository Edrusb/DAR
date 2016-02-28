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

    /// \file delta_signature.hpp
    /// \brief class used to manage binary delta signature
    /// \ingroup Private

#ifndef DELTA_SIGNATURE_HPP
#define DELTA_SIGNATURE_HPP

#include "../my_config.h"

extern "C"
{
} // end extern "C"

#include "memory_file.hpp"
#include "cat_tools.hpp"
#include "on_pool.hpp"
#include "crc.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

	// Datastructure in archive
	//
	//    SEQUENTIAL MODE (all along the archive)
	// +------+------+---------------+----------+--------+
	// | base | sig  | sig data      | data CRC | result |
	// | CRC  | size | (if size > 0) |    if    |  CRC   |
	// |      |      |               | size > 0 |        |
	// +------+------+---------------+----------+--------+
	//
	//    DIRECT MODE (in catalogue at end of archive)
	// +------+------+---------------+--------+
	// | base | sig  | sig offset    | result |
	// | CRC  | size | (if size > 0) |  CRC   |
	// |      |      |               |        |
	// +------+------+---------------+--------+
	//
	// this structure is used for all cat_file inode that have
	// either a delta signature or contain a delta patch (s_delta status)
	// base_crc is used to check we apply the patch on the good file before proceeding
	// result_crc is used to store the crc of the file the signature has been compulted on
	// result_crc is also used once a patch has been applied to verify the correctness of the patch result


	/// the plain file class
    class delta_signature : public on_pool
    {
    public:
	    /// constructor reading the object from an archive
	    ///
	    /// \param[in] f where to read the data from
	    /// \param[in] sequential_read if true read the whole data as it was dropped in sequential read mode, else only read the metadata
	delta_signature(generic_file & f, bool sequential_read);

	    /// constructor creating a brand new object
	delta_signature() { init(); };

	    /// destructor
	~delta_signature() { destroy(); };

	    /// same action as the first constructor but on an existing object
	void read(generic_file & f, bool sequential_read);

	    /// write down the data either in sequential_read mode (data+metadata) or in
	    /// direct access mode (metadata only)
	void dump(generic_file & f, bool sequential_read) const;

	    /// the provided memory_file passes under the responsibility of this object it contains
	    /// the delta signature
	void attach_sig(memory_file *delta_sig);

	    /// provide read access to the delta signature stored by this object
	const memory_file & get_sig() const;

	    /// drop the attached memory file to save place. Once dropped the memory file cannot
	    /// be re-read from archive when in sequential_read mode
	void detach_sig();

	    /// tells whether an memory file is attached to this object. if not get_sig() will throw an exception
	bool is_sig_attached() const { return sig != nullptr; };

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

    private:
	generic_file *read_from;    //< where to read data from in direct access mode
	infinint delta_sig_offset;  //< where to read data from to setup "sig" (set to zero when read in sequential mode, sig is setup on-fly)
	infinint delta_sig_size;    //< size of the data to setup "sig" (set to zero when reading in sequential mode, sig is then setup on-fly)
	memory_file *sig;           //< the signature data, if set nullptr it will be fetched from f in direct access mode only
	crc *patch_base_check;      //< associated CRC for the file this signature has been computed on
	crc *patch_result_check;    //< associated CRC

	    // invalidating copy constructor and assignment operator
	delta_signature(const delta_signature & ref) { throw SRC_BUG; };
	const delta_signature & operator = (const delta_signature & ref) { throw SRC_BUG; };

	void init();
	void destroy();
	bool is_completed(bool sequential_mode) const;
	void fetch_signature_data(generic_file &fic);
    };

	/// @}

} // end of namespace

#endif
