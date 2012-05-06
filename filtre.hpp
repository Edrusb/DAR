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
// $Id: filtre.hpp,v 1.21 2002/05/16 21:50:51 denis Rel $
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
    infinint hard_links; // hard linked stored
    infinint skipped; // not changed since last backup | file not restored because not saved in backup
    infinint ignored; // ignored due to filter
    infinint tooold; // ignored because less recent than the filesystem entry
    infinint errored; // could not be saved | could not be restored (filesystem access wrights)
    infinint deleted; // deleted file seen | number of files deleted
    infinint ea_treated; // number of EA saved | number of EA restored

    void clear() { treated = hard_links = skipped = ignored = tooold = errored = deleted = ea_treated = 0; };
    infinint total() const
	{ 
	    return treated+skipped+ignored+tooold+errored+deleted; 
		// hard_link are also counted in other counters 
	};
};

extern void filtre_restore(const mask &filtre, 
			   const mask & subtree,
			   catalogue & cat, 
			   bool detruire, 
			   const path & fs_racine,
			   bool fs_allow_overwrite,
			   bool fs_warn_overwrite,
			   bool info_details, statistics & st,
			   bool only_if_more_recent,
			   bool restore_ea_root,
			   bool restore_ea_user);

extern void filtre_sauvegarde(const mask &filtre,
			      const mask &subtree,
			      compressor *stockage, 
			      catalogue & cat,
			      catalogue &ref,
			      const path & fs_racine,
			      bool info_details, 
			      statistics & st,
			      bool make_empty_dir,
			      bool save_ea_root,
			      bool save_ea_user);

extern void filtre_difference(const mask &filtre,
			      const mask &subtree,
			      catalogue & cat,
			      const path & fs_racine,
			      bool info_details, statistics & st,
			      bool check_ea_root,
			      bool check_ea_user);

extern void filtre_test(const mask &filtre,
			const mask &subtree,
			catalogue & cat,
			bool info_details,
			statistics & st);

extern void filtre_isolate(catalogue & cat,
			   catalogue & ref,
			   bool info_details);

#endif
