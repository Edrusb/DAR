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
// $Id: getopt_decision.h,v 1.3.4.1 2010/12/21 20:54:08 edrusb Rel $
//
/*********************************************************************/

#ifndef GETOPT_DECISION_H
#define GETOPT_DECISION_H

#include "../my_config.h"

#if HAVE_GETOPT_LONG
 #if HAVE_GETOPT_IN_UNISTD_H
  #if HAVE_GETOPT_LONG_IN_UNISTD_H
    // we do not need to include <getopt.h>
  #else // we miss getopt_long but have getopt
    #include "my_getopt_long.h"
  #endif
 #else // no getopt() in unistd.h
    // we assume that if unistd.h does not know getopt() it does nor know getopt_long()
  #include <getopt.h>
 #endif
#endif

#ifndef EOF
#define EOF -1
#endif

#endif
