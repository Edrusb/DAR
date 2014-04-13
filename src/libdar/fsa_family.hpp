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

    /// \file fsa_family.hpp
    /// \brief filesystem specific attributes available families and fsa_scope definition
    /// \ingroup API

#ifndef FSA_FAMILY_HPP
#define FSA_FAMILY_HPP

#include <string>
#include <set>

#include "integers.hpp"
#include "crc.hpp"

namespace libdar
{
	/// \addtogroup API
	/// @{


	/// FSA family
    enum fsa_family { fsaf_hfs_plus, fsaf_linux_extX };
	// note: adding new fsa_family need updating all_fsa_family()

	/// FSA nature
    enum fsa_nature { fsan_unset,
		      fsan_creation_date,
		      fsan_append_only,
		      fsan_compressed,
		      fsan_no_dump,
		      fsan_immutable,
		      fsan_data_journaling,
		      fsan_secure_deletion,
		      fsan_no_tail_merging,
		      fsan_undeletable,
		      fsan_noatime_update,
		      fsan_synchronous_directory,
		      fsan_synchronous_update,
		      fsan_top_of_dir_hierarchy };

    extern std::string fsa_family_to_string(fsa_family f);
    extern std::string fsa_nature_to_string(fsa_nature n);

    typedef std::set<fsa_family> fsa_scope;

	/// provides a scope containing all FSA families
    extern fsa_scope all_fsa_families();

    extern infinint fsa_scope_to_infinint(const fsa_scope & val);
    extern fsa_scope infinint_to_fsa_scope(const infinint & ref);
    extern std::string fsa_scope_to_string(bool saved, const fsa_scope & scope);

	/// @}

} // end of namespace

#endif
