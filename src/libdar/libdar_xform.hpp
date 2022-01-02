/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2022 Denis Corbin
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

    /// \file libdar_xform.hpp
    /// \brief API for dar_xform functionnality
    /// \ingroup API

#ifndef LIBDAR_XFORM_HPP
#define LIBDAR_XFORM_HPP

#include "../my_config.h"
#include "infinint.hpp"
#include "user_interaction.hpp"
#include "archive_aux.hpp"

#include <memory>

namespace libdar
{

	/// \addtogroup API
	/// @{

	/// class implementing the dar_xform feature

    class libdar_xform
    {
    public:
	    /// the archive to transform is read from a set of slices

	    /// \param[in] ui for user interaction, may be set to std::nullptr
	    /// \param[in] chem the path where resides the archive
	    /// \param[in] basename the basename of the archive
	    /// \param[in] extension should be set to "dar" as always
	    /// \param[in] min_digits the way slice number is written in files,
	    /// use 0 if this feature was not used at creation time
	    /// \param[in] execute command to execute before each new slice
	    /// same substitution is available as archive_options_create::set_execute()
	libdar_xform(const std::shared_ptr<user_interaction> & ui,
		     const std::string & chem,
		     const std::string & basename,
		     const std::string & extension,
		     const infinint & min_digits,
		     const std::string & execute);

	    /// the archive to transform is read from a named pipe

	    /// \param[in] dialog for user interaction, may be set to std::nullptr
	    /// \param[in] pipename named pipe where to read the archive from (single sliced one)
	libdar_xform(const std::shared_ptr<user_interaction> & dialog,
		     const std::string & pipename); /// < if pipename is set to "-" reading from standard input

	    /// the archive to transform is read from a file descriptor open in read mode

	    /// \param[in] dialog for user interaction, may be set to std::nullptr
	    /// \param[in] filedescriptor the filedescriptor to reading the archive from
	libdar_xform(const std::shared_ptr<user_interaction> & dialog,
		     int filedescriptor);

	    /// copy constructor is not allowed
	libdar_xform(const libdar_xform & ref) = delete;

	    /// move constructor
	libdar_xform(libdar_xform && ref) noexcept;

	    /// assignment operator is not allowed
	libdar_xform & operator = (const libdar_xform & ref) = delete;

	    /// move assignment operator
	libdar_xform & operator = (libdar_xform && ref) noexcept;

	    /// destructor
	~libdar_xform();

	    /// the resulting archive is a written to disk possibly multi-sliced

	    /// \param[in] path directory where to write the new archive to
	    /// \param[in] basename archive base name to create
	    /// \param[in] extension should be set to "dar" as always
	    /// \param[in] allow_over whether to allow slice overwriting
	    /// \param[in] warn_over whether to warn before overwriting a slice
	    /// \param[in] pause the number of slice to pause asking the user for continuation. Set
	    /// to zero to disable pausing
	    /// \param[in] first_slice_size size of the first slice if different of the other. Set
	    /// to zero to have the first slice having the same size as others
	    /// \param[in] slice_size size of slices (except the first slice which may be different).
	    /// set to zero if slicing is not wanted
	    /// \param[in] slice_perm number written in octal corresponding to the permission of slices
	    /// to create. if set to an empty string the slice permission will not be overriden and will
	    /// follow the umask() value
	    /// \param[in] slice_user user name or UID to assign slice to. Assuming the process has
	    /// the permission/capabilities to change UID of files
	    /// \param[in] slice_group group name or GID to assign slice to. Assuming the process has
	    /// the permission/capabilities to change GID of files
	    /// \param[in] hash hashing algorithm to rely on generate hash file for each slice. Use
	    /// libdar::hash_algo::hash_none to disable this feature
	    /// \param[in] min_digits numbering of slices in filename should be padded by as much zero
	    /// to have no less than this number of digit. Use zero to disable this feature
	    /// \param[in] execute command to execute after each slice has been completed. Same
	    /// substitution is available as archive_options_create::set_execute
	void xform_to(const std::string & path,
		      const std::string & basename,
		      const std::string & extension,
		      bool allow_over,
		      bool warn_over,
		      const infinint & pause,
		      const infinint & first_slice_size,
		      const infinint & slice_size,
		      const std::string & slice_perm,
		      const std::string & slice_user,
		      const std::string & slice_group,
		      hash_algo hash,
		      const infinint & min_digits,
		      const std::string & execute);

	    /// the resulting archive is a single sliced archive sent to a filedescriptor

	    /// \param[in] filedescriptor file descriptor open in write mode to write the archive to
	    /// \param[in] execute command to execute after the archive has been completed.
	    /// same string substitution available as described in other xform_to method
	void xform_to(int filedescriptor,
		      const std::string & execute);

    private:
	class i_libdar_xform;
	std::unique_ptr<i_libdar_xform> pimpl;

    };

	/// @}

} // end of namespace

#endif
