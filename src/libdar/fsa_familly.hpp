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

    /// \file fsa_familly.hpp
    /// \brief filesystem specific attributes available famillies and fsa_scope definition
    /// \ingroup Private

#ifndef FSA_FAMILLY_HPP
#define FSA_FAMILLY_HPP

#include <string>
#include <set>

#include "integers.hpp"
#include "crc.hpp"

namespace libdar
{
	/// \addtogroup Private
	/// @{


	/// FSA familly

    enum fsa_familly { fsaf_hfs_plus, fsaf_linux_extX };
	// note: adding new fsa_familly need updating all_fsa_familly()

    enum fsa_nature { fsan_unset, fsan_creation_date, fsan_compressed, fsan_no_dump, fsan_immutable, fsan_undeletable };

    extern std::string fsa_familly_to_string(fsa_familly f);
    extern std::string fsa_nature_to_string(fsa_nature n);

    typedef std::set<fsa_familly> fsa_scope;

    extern fsa_scope all_fsa_famillies();
    extern infinint fsa_scope_to_infinint(const fsa_scope & val);
    extern fsa_scope infinint_to_fsa_scope(const infinint & ref);


	/// @}

} // end of namespace

#endif
