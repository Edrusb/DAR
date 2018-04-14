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

    /// \file libdar_xform.hpp
    /// \brief API for dar_xform functionnality
    /// \ingroup API

#ifndef LIBDAR_XFORM_HPP
#define LIBDAR_XFORM_HPP

#include "../my_config.h"
#include "tuyau.hpp"
#include "entrepot_local.hpp"
#include "sar.hpp"
#include "slave_zapette.hpp"
#include "infinint.hpp"
#include "mem_ui.hpp"

namespace libdar
{

	/// \addtogroup API
	/// @{

    class libdar_xform : public mem_ui
    {
    public:
	libdar_xform(const std::shared_ptr<user_interaction> & ui,
		     const std::string & chem,
		     const std::string & basename,
		     const std::string & extension,
		     const infinint & min_digits,
		     const std::string & execute);
	libdar_xform(const std::shared_ptr<user_interaction> & dialog,
		     const std::string & pipename); //< if pipename is set to "-" reading from standard input
	libdar_xform(const std::shared_ptr<user_interaction> & dialog,
		     int filedescriptor);
	libdar_xform(const libdar_xform & ref) = delete;
	libdar_xform(libdar_xform && ref) noexcept = default;
	libdar_xform & operator = (const libdar_xform & ref) = delete;
	libdar_xform & operator = (libdar_xform && ref) noexcept = default;
	~libdar_xform() = default;

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
	void xform_to(int filedescriptor,
		      const std::string & execute);

    private:
	bool can_xform;
	std::unique_ptr<generic_file> source;
	std::unique_ptr<path> src_path;        //< may be null when reading from a pipe
	std::unique_ptr<entrepot_local> entrep;
	bool format_07_compatible;
	label dataname;

	void init_entrep();
	void xform_to(generic_file *dst);
    };

	/// @}

} // end of namespace

#endif
