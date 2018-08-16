/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2018 Denis Corbin
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

    /// \file my_getopt_long.h
    /// \brief may lead to the definition of getopt_long to solve declaration conflicts in <unistd.h> and <getopt.h>
    /// \ingroup CMDLINE


#ifndef MY_GETOPT_LONG_H
#define MY_GETOPT_LONG_H

// getopt may be declated in <unistd.h> on systems like FreeBSD.
// if you want to use libgnugetopt you need to include <getopt.h>
// on this system. Thus a conflict appear because the getopt is
// declared twice. To avoid you either have not to include <unistd.h>
// or not to include <getopt.h>. But neither the first nor the
// second is acceptable, because we need other stuf declared in
// <unistd.h> (open() & so on) and stuf declared in <getopt.h>
// (like getopt_long which is gnu typical).
//
// to solve this problem, I have extracted the gnu getopt_long
// as declared under Linux in the present file. When getopt is
// declared in <unistd.h> it is still possible to include the
// current file in place of <getopt.h>, to get getopt_long available
//
// at linking time, if libgnugetopt is available we use it
//
// see getopt_decision.hpp

# define no_argument		0
# define required_argument	1
# define optional_argument	2

struct option
{
# if defined __STDC__ && __STDC__
  const char *name;
# else
  char *name;
# endif
  /* has_arg can't be an enum because some compilers complain about
     type mismatches in all the code that assumes it is an int.  */
  int has_arg;
  int *flag;
  int val;
};

extern int getopt_long (int __argc, char *const *__argv, const char *__shortopts,
		        const struct option *__longopts, int *__longind);


#endif
