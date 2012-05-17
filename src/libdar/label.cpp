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
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/
// $Id: label.cpp,v 1.9 2011/06/02 13:17:37 edrusb Rel $
//
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif

} // end extern "C"

#include "label.hpp"
#include "infinint.hpp"
#include "tools.hpp"

namespace libdar
{

    label::label()
    {
	clear();
    }

    bool label::operator == (const label & ref) const
    {
	return memcmp(val, ref.val, LABEL_SIZE) == 0;
    }


    void label::clear()
    {
	bzero(val, LABEL_SIZE);
    }

    bool label::is_cleared() const
    {
	register U_I i = 0;
	while(i < LABEL_SIZE && val[i] == '\0')
	    i++;

	return i >= LABEL_SIZE;
    }

    static void dummy_call(char *x)
    {
        static char id[]="$Id: label.cpp,v 1.9 2011/06/02 13:17:37 edrusb Rel $";
        dummy_call(id);
    }

    void label::generate_internal_filename()
    {
	const time_t src1 = time(NULL);
	const pid_t src2 = getpid();
	const uid_t src3 = getuid();
	unsigned char *dest = (unsigned char *)(&val);
	unsigned char *src = (unsigned char *)(&src1);
	U_I s1 = sizeof(src1) < LABEL_SIZE ? sizeof(src1) : LABEL_SIZE;

	for(register U_I i = 0 ; i < s1; ++i)
	    dest[i] = src[i];

	if(s1 < LABEL_SIZE)
	{
	    s1 = LABEL_SIZE - s1; // number of byte left to fill in "ret"
	    s1 = s1 < sizeof(src2) ? s1 : sizeof(src2); // number of byte to copy
	    src = (unsigned char *)(&src2);

	    for(register U_I i = 0; i < s1; ++i)
		dest[sizeof(src1)+i] = src[i];
	}

	s1 = sizeof(src1) + sizeof(src2);

	if(s1 < LABEL_SIZE)
	{
	    U_I s2 = LABEL_SIZE - s1;
	    s2 = s2 < sizeof(src3) ? s2 : sizeof(src3);

	    src = (unsigned char *)(&src3);
	    for(register U_I i = 0; i < s2; ++i)
		dest[s1+i] = src[i];
	}

	for(s1 = s1 + sizeof(src3); s1 < LABEL_SIZE; ++s1)
	    dest[s1] = (U_I)tools_pseudo_random(256);
    }

    void label::read(generic_file & f)
    {
	if(f.read(val, LABEL_SIZE) != (S_I)LABEL_SIZE)
	    throw Erange("label::read", gettext("Incomplete label"));
    }

    void label::dump(generic_file & f) const
    {
	f.write(val, LABEL_SIZE);
    }

    void label::copy_from(const label & ref)
    {
	bcopy(ref.val, val, LABEL_SIZE);
    }

    const label label_zero;

} // end of namespace
