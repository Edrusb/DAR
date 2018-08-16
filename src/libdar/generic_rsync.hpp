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

    /// \file generic_rsync.hpp
    /// \brief class generic_rsync provides a generic_file interface to librsync
    /// \ingroup Private


#ifndef GENERIC_RSYNC_HPP
#define GENERIC_RSYNC_HPP

#include "../my_config.h"

extern "C"
{
#if HAVE_LIBRSYNC_H
#include <librsync.h>
#endif
}

#include "generic_file.hpp"
#include "erreurs.hpp"

namespace libdar
{

	/// \addtogroup Private
        /// @{

	/// generic_file interface to librsync

    class generic_rsync : public generic_file
    {
    public:
	    /// constructor for "signature" operation

	    /// in this mode the generic_rsync object is read only, all data
	    /// read from it is fetched unchanged from "below" while the signature is
	    /// computed. The file signature is output to signature_storage
	    /// \param[in] signature_storage is write only mode generic_file
	    /// \param[in] below is read only to fetch data from
	generic_rsync(generic_file *signature_storage,
		      generic_file *below);

	    /// constructor for "delta" operation

	    /// in this mode the generic_rsync object is also read only, all data
	    /// read from it is the resulting delta of the data read from "below" based
	    /// the given base_signature.
	    /// \param[in] base_signature is read only
	    /// \param[in] below is the plain file to read from and for which to compute the
	    /// delta based on base_signature
	    /// \param[in] crc_size is the size of the crc to create if checksum is not nullptr
	    /// \param[in] checksum if not null, the *checksum will be set to the address of a newly
	    /// allocated crc that will receive the calculated crc of the below object, this
	    /// CRC is calcuated for the data of "below". Caller has the duty to release this object
	    /// when no more needed but never before this generic_rsync object has been destroyed.
	generic_rsync(generic_file *base_signature,
		      generic_file *below,
		      const infinint & crc_size,
		      const crc **checksum);

	    /// constructor for "patch" operation

	    /// in this mode the generic_rsync object is read only, the data read from
	    /// it is built from the current file's data and the delta signature
	    /// as a first step the on current data CRC is computed CRC and compared
	    /// to the original CRC given in argument. If they do not match, an exception
	    /// Edata is thrown and nothing is modified on filesystem.
	    /// \param[in] current_data is a read_only object that contains the data to be used
	    /// as base for the patch (this data is not modified)
	    /// \param[in] delta is read only and contains the patch to apply
	    /// \param[in] not_used is not used but present to make a distinction between this
	    /// constructor and the constructor used to generate delta signature
	generic_rsync(generic_file *current_data,
		      generic_file *delta,
		      bool not_used);

	generic_rsync(const generic_rsync & ref) = delete;
	generic_rsync(generic_rsync && ref) noexcept = delete;
	generic_rsync & operator = (const generic_rsync & ref) = delete;
	generic_rsync & operator = (generic_rsync && ref) noexcept = delete;

	~generic_rsync() noexcept(false);

	    // inherited from generic_file

	virtual bool skippable(skippability direction, const infinint & amount) override { return false; };
	virtual bool skip(const infinint & pos) override {if(pos != 0 || !initial) throw SRC_BUG; else return true; };
	virtual bool skip_to_eof() override { throw SRC_BUG; };
	virtual bool skip_relative(S_I x) override { if(x != 0) throw SRC_BUG; else return true; };
	virtual infinint get_position() const override { return x_below->get_position(); };

    protected:
	virtual void inherited_read_ahead(const infinint & amount) override {};
	virtual U_I inherited_read(char *a, U_I size) override;
	virtual void inherited_write(const char *a, U_I size) override;
	virtual void inherited_sync_write() override {};
	virtual void inherited_flush_read() override {};
	virtual void inherited_terminate() override;

    private:
	enum { sign, delta, patch } status;

	generic_file *x_below;    ///< underlying layer to read from / write to
	generic_file *x_input;
	generic_file *x_output;
	bool initial;
	char *working_buffer;
	U_I working_size;
	bool patching_completed;
	crc *data_crc;

#if LIBRSYNC_AVAILABLE
	rs_job_t *job;
	rs_signature_t *sumset;

	    /// opaque is pointer to a generic_rsync object
	static rs_result patch_callback(void *opaque,
					rs_long_t pos,
					size_t *len,
					void **buf);
#endif

	    /// feed librsync using rs_job_iter

	    /// \param[in] buffer_in bytes of data to give to librsync
	    /// \param[in,out] avail_in is the amount of byte available, and after the call the amount of not yet read bytes
	    /// remaining at the beginning of the buffer_in buffer (when shift_input is set to true) or at the end of buffer_in if shift is set
	    /// to false.
	    /// \param[in] shift_input
	    /// \param[out] buffer_out where to drop the data from librsync
	    /// \param[in,out] avail_out is the size of the allocated memory pointed to by buffer_out and after the call the amount
	    /// of byte that has been dropped to the buffer_out buffer.
	    /// \return true if librsync returned RS_DONE else false is return if RS_BLOCKED is returned. For any other errors an exception is thrown
	bool step_forward(const char *buffer_in,
			  U_I & avail_in,
			  bool shift_input,
			  char *buffer_out,
			  U_I & avail_out);
	void free_job();
	void send_eof();
    };

	/// @}

} // end of namespace

#endif
