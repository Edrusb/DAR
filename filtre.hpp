/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002 Denis Corbin
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
// $Id: filtre.hpp,v 1.12 2002/03/18 11:00:54 denis Rel $
//
/*********************************************************************/

#ifndef FILTRE_HPP
#define FILTRE_HPP

#include <vector>
#include "mask.hpp"
#include "compressor.hpp"
#include "catalogue.hpp"
#include "path.hpp"

struct statistics
{
    infinint treated; // saved | restored
    infinint skipped; // not changed since last backup | file not restored because not saved in backup
    infinint ignored; // ignored due to filter
    infinint errored; // could not be saved | could not be restored (filesystem access wrights)
    infinint deleted; // deleted file seen | number of files deleted

    void clear() { treated = skipped = ignored = errored = deleted = 0; };
};

void filtre_restore(const mask &filtre, 
		    const mask & subtree,
		    compressor *stockage, 
		    catalogue & cat, 
		    bool detruire, 
		    const path & fs_racine,
		    bool fs_allow_overwrite,
		    bool fs_warn_overwrite,
		    bool info_details, statistics & st);

void filtre_sauvegarde(const mask &filtre,
		       const mask &subtree,
		       compressor *stockage, 
		       catalogue & cat,
		       catalogue &ref,
		       const path & fs_racine,
		       bool info_details, statistics & st);

void filtre_difference(const mask &filtre,
		       const mask &subtree,
		       compressor *stockage,
		       const catalogue & cat,
		       const path & fs_racine,
		       bool info_details, statistics & st);

#endif
