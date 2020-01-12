/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2020 Denis Corbin
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

    /// \file my_config.h
    /// \brief include macro defined by the configure script and some specific additional ones
    /// \ingroup Private

#ifndef MY_CONFIG_H
#define MY_CONFIG_H

#if HAVE_CONFIG_H
#define NODUMP_LINUX 0
#define NODUMP_EXT2FS 1
#include "../config.h"

    // workaround for autoconf 2.59
#undef mbstate_t

#endif

#include "gettext.h"

#endif
