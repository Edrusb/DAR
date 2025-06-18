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

    /// \file proto_tronco.hpp
    /// \brief defines common interface for tronconneuse and parallel_tronconneuse
    /// \ingroup Private
    ///

#ifndef PROTO_TRONCO_HPP
#define PROTO_TRONCO_HPP

#include "../my_config.h"
#include <string>

#include "infinint.hpp"
#include "integers.hpp"
#include "generic_file.hpp"
#include "archive_version.hpp"
#include "generic_file.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{


	/// the trailing_clear_data_callback call back is a mean by which the upper layer cat tell when encrypted data ends

	/// \param[in] below is the generic_file containing the encrypted data that the proto_tronco inherited
	/// class is willing to decipher.
	/// \param[in] reading_ver is the archive format version of the archive under operation
	/// \return the callback returns the offset of the first non encrypted data at the end of the provided generic_file.
	/// \exception if the offset of the first non encrypted data cannot be found in the provided generic_file object, the
	/// callback should throw an Erange() exception.
	/// \note this method should be invoked when decryption failed at or near end of file.
	/// \note the libdar archive format is ended by a clear trailier which is expected
	/// to be read backward, by the end of the archive. The last part of this trailer (see terminateur class) records the offset of the
	/// beginning of this trailier (which are all clear clear), it is thus possible outside of the
	/// encrypted layer to tail the clear data from the encrypted one and avoid trying to decipher
	/// data that is not encrypted. This is the reason of existence for this callback.
    typedef infinint (*trailing_clear_data_callback)(generic_file & below, const archive_version & reading_ver);

    class proto_tronco: public generic_file
    {
    public:
	using generic_file::generic_file;

	virtual void set_initial_shift(const infinint & x) = 0;
	virtual void set_callback_trailing_clear_data(trailing_clear_data_callback) = 0;
	virtual U_32 get_clear_block_size() const = 0;
	virtual	void write_end_of_file() = 0;

    };

	/// @}

} // end of namespace

#endif
