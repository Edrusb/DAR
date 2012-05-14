//*********************************************************************/
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
// $Id: statistics.hpp,v 1.4 2003/10/18 14:43:07 edrusb Rel $
//
/*********************************************************************/
//

#ifndef STATISTICS_HPP
#define STATISTICS_HPP

#include "../my_config.h"

namespace libdar
{

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

} // end of namespace
    
#endif
