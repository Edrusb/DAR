/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2019 Denis Corbin
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

    /// \file cygwin_adapt.h
    /// \brief thin C adaptation layer to Cygwin specifities
    /// \ingroup Private

#ifndef CYGWIN_ADAPT_H
#define CYGWIN_ADAPT_H

#include "../my_config.h"

    /// \addtogroup Private
    /// @{

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
	// if fcntl.h does not define O_TEXT nor O_BINARY (which is a Cygwin
	// speciality), we define them as neutral ORed values : zero

#ifndef O_TEXT
// zero is neutral in ORed expression where it is expected to be used
#define O_TEXT 0
#endif

#ifndef O_BINARY
// zero is neutral in ORed expression where it is expected to be used
#define O_BINARY 0
#else
#define CYGWIN_BUILD 1
// if O_BINARY is defined we are compiling on or for a cygwin plateform
#endif

    /// @}

#endif
