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

    /// \file cat_signature.hpp
    /// \brief class used to write and read from archive the type and status of each entry in a catalogue
    /// \ingroup Private

#ifndef CAT_SIGNATURE_HPP
#define CAT_SIGNATURE_HPP

#include "../my_config.h"
#include "generic_file.hpp"
#include "archive_version.hpp"

extern "C"
{
} // end extern "C"


namespace libdar
{
	/// \addtogroup Private
	/// @{

    enum class saved_status
    {
	saved,      //< inode is saved in the archive
	fake,       //< inode is not saved in the archive but is in the archive of reference (isolation context) s_fake is no more used in archive format "08" and above: isolated catalogue do keep the data pointers and s_saved stays a valid status in isolated catalogues.
	not_saved,  //< inode is not saved in the archive
	delta       //< inode is saved but as delta binary from the content (delta signature) of what was found in the archive of reference
    };

	/// class cat_signature combines the cat_entree::signature() of the object with its saved_status and read and store this combinason

	/// historically these two fields were saved on a single byte, but as libdar received new feature it was to narrow
	/// where from this class to transparently manage this field evolution
    class cat_signature
    {
    public:
	    /// set a signature from running libdar
	cat_signature(unsigned char original = 0, saved_status status = saved_status::saved); // -> mk_signature()

	    /// read a signature from archive for an existing cat_signature object (overwrite its value)

	    /// \param[in] f where to read data from
	    /// \param[in] reading_ver which format to expect
	    /// \return true if data could be read, false else.
	    /// \note the validity of the read data is not checked at that time
	    /// but a the time get_base_and_status() is called
	bool read(generic_file & f, const archive_version & reading_ver);
	void write(generic_file &f);

	bool get_base_and_status(unsigned char & base, saved_status & saved) const;
	bool get_base_and_status_isolated(unsigned char & base, saved_status & state, bool isolated) const;
	unsigned char get_base() const;

	static bool compatible_signature(unsigned char a, unsigned char b);

    private:
	static constexpr U_8 SAVED_FAKE_BIT = 0x80;
	static constexpr U_8 SAVED_NON_DELTA_BIT = 0x40;
	    // in ASCII lower case letter have binary values 11xxxxx and higher case
	    // have binary values 10xxxxx The the 7th byte is always set to 1. We can
	    // thus play with it to store the delta diff flag:
	    // - when set to 1, this is a non delta binary file (backward compatibility)
	    //   and the signature byte is a letter (lower or higher case to tell whether
	    //   the inode is saved or not
	    // - when set to 0, this is a delta saved file, setting it back to 1 should
	    //   give a lower case letter corresponding to the inode type.

	unsigned char old_field;  // field present since archive format 1
	unsigned char field_v10;  // field appeared starting archive format 10
    };
	/// @}

} // end of namespace

#endif
