/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2023 Denis Corbin
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

    /// \file terminateur.hpp
    /// \brief the terminateur class which defines the position of the catalogue
    /// \ingroup Private
    ///
    /// the terminateur is a byte sequence present as the last bytes of an archive
    /// which indicates how much byte backward libdar must skip back to find the
    /// beginning of the catalogue.

#ifndef TERMINATEUR_HPP
#define TERMINATEUR_HPP

#include "../my_config.h"
#include "infinint.hpp"
#include "generic_file.hpp"
#include "archive_version.hpp"

namespace libdar
{
	/// \addtogroup Private
        /// @{

	/// terminateur class indicates the location of the beginning of the catalogue

	/// it is the last bytes sequence of an archive.
    class terminateur
    {
    public :
	terminateur() = default;
	terminateur(const terminateur & ref) = default;
	terminateur(terminateur && ref) noexcept = default;
	terminateur & operator = (const terminateur & ref) = default;
	terminateur & operator = (terminateur && ref) noexcept = default;
	~terminateur() = default;

        void set_catalogue_start(infinint xpos) { pos = xpos; };
        void dump(generic_file &f);
        void read_catalogue(generic_file &f, bool with_elastic, const archive_version & reading_ver, const infinint & where_from = 0);
        infinint get_catalogue_start() const { return pos; };
	infinint get_terminateur_start() const { return t_start; };

    private :
        infinint pos;
	infinint t_start;
    };

	/// @}

} // end of namespace

#endif
