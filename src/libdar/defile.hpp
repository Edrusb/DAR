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
// to contact the author : dar.linux@free.fr
/*********************************************************************/
// $Id: defile.hpp,v 1.7 2004/11/07 18:21:37 edrusb Rel $
//
/*********************************************************************/

    /// \file defile.hpp
    /// \brief here is defined the defile class
    /// \ingroup Private

#ifndef DEFILE_HPP
#define DEFILE_HPP

#include "../my_config.h"
#include "catalogue.hpp"
#include "path.hpp"

namespace libdar
{
	/// the defile class keep trace of the real path of files while the flow in the filter routines

	/// the filter routines manipulates flow of inode, where their relative order
	/// represent the directory structure. To be able to know what is the real path
	/// of the current inode, all previously passed inode must be known.
	/// this class is used to display the progression of the filtering routing,
	/// and the file on which the filtering routine operates
	/// \ingroup Private
    class defile
    {
    public :
        defile(const path &racine) : chemin(racine) { init = true; };

        void enfile(const entree *e);
        path get_path() const { return chemin; };
        std::string get_string() const { return chemin.display(); };

    private :
        path chemin;
        bool init;
    };

} // end of namespace

#endif
