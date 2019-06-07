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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

    /// \file cat_all_entrees.hpp
    /// \brief include file gathering all entree found in a catalogue
    /// \ingroup Private

#ifndef CAT_ALL_ENTREE_HPP
#define CAT_ALL_ENTREE_HPP

#include "../my_config.h"

extern "C"
{
} // end extern "C"

#include "cat_detruit.hpp"
#include "cat_device.hpp"
#include "cat_directory.hpp"
#include "cat_door.hpp"
#include "cat_entree.hpp"
#include "cat_etoile.hpp"
#include "cat_file.hpp"
#include "cat_ignored_dir.hpp"
#include "cat_inode.hpp"
#include "cat_lien.hpp"
#include "cat_mirage.hpp"
#include "cat_nomme.hpp"
#include "cat_eod.hpp"
#include "cat_chardev.hpp"
#include "cat_blockdev.hpp"
#include "cat_tube.hpp"
#include "cat_prise.hpp"
#include "cat_ignored.hpp"

#endif
