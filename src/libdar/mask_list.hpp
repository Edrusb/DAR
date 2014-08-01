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

    /// \file mask_list.hpp
    /// \brief here lies a mask that selects files present in a given list
    /// \ingroup API
    ///
    /// The mask_list classes defined here is to be used for filtering files
    /// in the libdar API calls.

#ifndef MASK_LIST_HPP
#define MASK_LIST_HPP

#include "../my_config.h"

#include "mask.hpp"

#include <string>

namespace libdar
{

        /// \addtogroup API
        /// @{

        /// the mask_list class, matches string that are present in a given file
	///
        /// the given file must contain one entry per line (thus no carriage return
        /// is allowed in a given entry). Note that the file listed in the
        /// file may have a relative path or an absolute path.

    class mask_list : public mask
    {
    public:

            /// the constructor

            /// \param[in] filename_list_st is the path to the file listing the
            /// filename to select for the operation
            /// \param[in] case_sensit whether comparison is case sensitive or not
            /// \param[in] prefix add this prefix to relative paths of the list. The
	    /// prefix should be either absolute, or "<ROOT>" (in case of operations
	    /// on an existing archive)
	    /// \param[in] include whether the mask_list is used for file inclusion or file exclusion
        mask_list(const std::string & filename_list_st, bool case_sensit, const path & prefix, bool include);

            /// inherited from the mask class
        bool is_covered(const std::string & expression) const;
            /// inherited from the mask class
        mask *clone() const { return new (get_pool()) mask_list(*this); };

            /// routing only necessary for doing some testing
        U_I size() const { return contenu.size(); };

    private:

        std::vector <std::string> contenu;
        U_I taille;
        bool case_s;
        bool including;   // mask is used for including files (not for excluding files)
    };

        /// @}

} // end of namespace

#endif
