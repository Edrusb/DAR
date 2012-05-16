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
// $Id: statistics.hpp,v 1.6 2004/11/12 21:58:18 edrusb Rel $
//
/*********************************************************************/
//

    /// \file statistics.hpp
    /// \brief handle the statistic structure that gives a summary of treated files after each operation

#ifndef STATISTICS_HPP
#define STATISTICS_HPP

#include "../my_config.h"

namespace libdar
{

	/// structure returned by libdar call to give a summary of the operation done in term of file treated

	/// the different fields are used for backup, restoration and other operation
	/// their meaning changes a bit depending on the operation. Some operation may
	/// not use all fields. To have a detailed view of what fields get used and what
	// are their meaning see the archive class constructor and methods documentation
	/// \ingroup API
    struct statistics
    {
        infinint treated;       ///< number of file treated (saved, restored, etc.) [all operations]
        infinint hard_links;    ///< number of hard linked
        infinint skipped;       ///< files not changed since last backup / file not restored because not saved in backup
        infinint ignored;       ///< ignored files due to filter
        infinint tooold;        ///< ignored files because less recent than the filesystem entry [restoration]
        infinint errored;       ///< files that could not be saved / files that could not be restored (filesystem access right)
        infinint deleted;       ///< deleted file seen / number of files deleted during the operation [restoration]
        infinint ea_treated;    ///< number of EA saved / number of EA restored

	    /// reset counters to zero
        void clear() { treated = hard_links = skipped = ignored = tooold = errored = deleted = ea_treated = 0; };

	    /// total number of file treated
        infinint total() const
            {
                return treated+skipped+ignored+tooold+errored+deleted;
                    // hard_link are also counted in other counters
            };
    };

} // end of namespace

#endif
