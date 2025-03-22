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

    /// \file cat_signature.hpp
    /// \brief class used to write and read from archive the type and status of each entry in a catalogue
    /// \ingroup Private

#ifndef CAT_SIGNATURE_HPP
#define CAT_SIGNATURE_HPP

#include "../my_config.h"
#include "generic_file.hpp"
#include "archive_version.hpp"
#include "cat_status.hpp"

extern "C"
{
} // end extern "C"


namespace libdar
{
	/// \addtogroup Private
	/// @{

	/// class cat_signature combines the cat_entree::signature() of the object with its saved_status and read and store this combinason

	/// historically these two fields were saved on a single byte, but as libdar received new feature it was to narrow
	/// where from this class to transparently manage this field evolution
    class cat_signature
    {
    public:
	    /// set a signature from running libdar
	cat_signature(unsigned char original, saved_status status); // -> mk_signature()

	    /// set a signature from an disk archive
	cat_signature(generic_file & f, const archive_version & reading_ver);

	    /// read a signature from archive for an existing cat_signature object (overwrite its value)

	    /// \param[in] f where to read data from
	    /// \param[in] reading_ver which format to expect
	    /// \return true if data could be read, false else.
	    /// \note the validity of the read data is not checked at that time
	    /// but a the time get_base_and_status() is called
	bool read(generic_file & f, const archive_version & reading_ver);
	void write(generic_file &f);

	    /// provide typ and status as read from the archive

	    /// \param[out] base the signature() of the entry
	    /// \param[out] saved the get_saved_status() of the entry
	    /// \return false if the field read from the archive is malformed, in which case the returned argument are meaningless
	bool get_base_and_status(unsigned char & base, saved_status & saved) const;

	static bool compatible_signature(unsigned char a, unsigned char b);

    private:
	static constexpr U_8 SAVED_FAKE_BIT = 0x80;
	static constexpr U_8 SAVED_NON_DELTA_BIT = 0x40;

	    /// stores file type and status information

	    /// \note this field "field" stores two types of information:
	    //. type of inode: and the nature of the object that has to be build from the following bytes
	    //. status of the info: on which depends whether some or all field should be read in the following data
	    ///
	    /// Historically, the type was stored as different letters, then the differential backup
	    /// has been added and the saved/not_saved status appeared as
	    /// uppercase for not_saved status and lowercase for saved status (backward compatibility)
	    ///
	    /// adding features after features, the FAKE status used in isolated catalogues used
	    /// the bit 8 (ASCII used 7 lower bits only).
	    ///
	    /// but while adding the delta-diff feature it has been realized that 3 bits could be used beside
	    /// information type to store the status thanks to the way ASCI encodes letters:
	    //. lower case letters have binary values 011xxxxx
	    //. and higher case have binary values 010xxxxx
	    /// the 5 lower bits encode the letter nature, remains the bits 6, 7 and 8 to encode
	    /// the status of the inode:
	    //. 011----- (3) status is "saved" (backward compatibility) this is a lowercase letter
	    //. 010----- (2) status is "not_saved" (backward compatibily) this is an uppercase letter
	    //. 111----- (7) status is "fake" (backward compatibility) used only with lowercase letters
	    //. 001----- (1) status is "delta" (setting back the byte 6 to 1 gives the letter value)
	    //. 100----- (4) status is "inode_only (setting back the bytes as 011 gives the letter value)
	    //. 101----- (5) status is unused (setting back the high bytes to 011 gives the letter value)
	    //. 110----- (6) status is unused (setting back the high bytes to 011 gives the letter value)
	    //. 000----- (0) status is unused (setting back the high bytes to 011 gives the letter value)
	unsigned char field;
    };
	/// @}

} // end of namespace

#endif
