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
// $Id: elastic.cpp,v 1.7.2.1 2007/07/22 16:34:59 edrusb Rel $
//
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#if HAVE_TIME_H
#include <time.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif
}

#include "elastic.hpp"
#include "infinint.hpp"

#define SINGLE_MARK 'X'
#define LOW_MARK '>'
#define HIGH_MARK '<'

//// elastic buffer structure ////
//
//   .......>OOOO<.....
//
//   where '.' are random bytes and
//   '>' and '<' are marks
//   OOOOO is a byte field giving the size in byte of the total
//   elastic buffer (big endian).
//
//   some trivial elastic buffer is 'X' for a buffer of size 1 (where 'X'
//     is the "single mark")
//   and '><' for an elastic buffer of size 2
////////////////////////////////////

using namespace std;

namespace libdar
{

    elastic::elastic(const char *buffer, U_32 size, elastic_direction dir)
    {
	U_32 pos = (dir == elastic_forward ? 0 : size - 1);
	const U_32 first_pos = pos;
	S_I step = (dir == elastic_forward ? +1 : -1);
	char first_mark = (dir == elastic_forward ? LOW_MARK : HIGH_MARK);
	char last_mark  = (dir == elastic_forward ? HIGH_MARK : LOW_MARK);

	while(pos < size && buffer[pos] != SINGLE_MARK && buffer[pos] != first_mark)
	    pos += step;

	if(pos >= size)
	    throw Erange("elastic::elastic", gettext("elastic buffer incoherent structure"));

	if(buffer[pos] == SINGLE_MARK)
	    if(first_pos == pos)
		taille = 1;
	    else
		throw Erange("elastic::elastic", gettext("elastic buffer incoherent structure"));
	else // elastic buffer size is greater than one
	{
	    U_32 power256 = 1;
	    const U_32 int_width = sizeof(U_32);
	    U_32 byte_counter = 0;
		// now reading total size

	    pos += step;
	    taille = 0;
	    while(pos < size && buffer[pos] != last_mark)
	    {
		if(dir != elastic_forward)
		{
		    taille *= 256;
		    taille += (unsigned char)(buffer[pos]);
		}
		else
		{
		    taille += power256 * (unsigned char)(buffer[pos]);
		    power256 *= 256;
		}

		pos += step;
		if(++byte_counter > int_width)
		    throw Erange("elastic::elastic", gettext("too large elastic buffer or elastic buffer incoherent structure"));
	    }

	    if(pos >= size)
		throw Erange("elastic::elastic", gettext("elastic buffer incoherent structure"));

	    if(taille == 0 && byte_counter == 0)
		taille = 2; // this is the trivial buffer of size 2
	    else
		if(taille < 3)
		    throw Erange("elastic::elastic", gettext("elastic buffer incoherent structure"));
	}
    }

    elastic::elastic(generic_file &f, elastic_direction dir)
    {
	U_32 count = 0;
	S_I (generic_file::*lecture)(char &a) = (dir == elastic_forward ? &generic_file::read_forward : &generic_file::read_back);
	char first_mark = (dir == elastic_forward ? LOW_MARK : HIGH_MARK);
	char last_mark  = (dir == elastic_forward ? HIGH_MARK : LOW_MARK);
	unsigned char a;

	while((f.*lecture)((char &)a) && a != SINGLE_MARK && a != first_mark)
	    ++count;

	if(a != SINGLE_MARK && a != first_mark)
	    throw Erange("elastic::elastic", gettext("elastic buffer incoherent structure"));
	else
	    ++count;

	if(a == SINGLE_MARK)
	    if(count == 1)
		taille = 1;
	    else
		throw Erange("elastic::elastic", gettext("elastic buffer incoherent structure"));
	else // elastic buffer size is greater than one
	{
	    U_32 power256 = 1;
	    const U_32 int_width = sizeof(U_32);
	    U_32 byte_counter = 0;
		// now reading total size

	    taille = 0;
	    while((f.*lecture)((char &)a) && a != last_mark)
	    {
		if(dir != elastic_forward)
		{
		    taille *= 256;
		    taille += a;
		}
		else
		{
		    taille += power256 * a;
		    power256 *= 256;
		}

		count++;
		if(++byte_counter > int_width)
		    throw Erange("elastic::elastic", gettext("too large elastic buffer or elastic buffer incoherent structure"));
	    }

	    if(a != last_mark)
		throw Erange("elastic::elastic", gettext("elastic buffer incoherent structure"));
	    else
		count++;

	    if(taille == 0 && byte_counter == 0)
		taille = 2; // this is the trivial buffer of size 2
	    else
		if(taille < 3)
		    throw Erange("elastic::elastic", gettext("elastic buffer incoherent structure"));

		// now skipping to the "end" of the elastic buffer
	    if(count < taille)
		if(dir == elastic_forward)
		    f.skip_relative(taille - count);
		else
		    f.skip_relative(count - taille);
	    else
		if(count > taille)
		    throw Erange("elastic::elastic", gettext("elastic buffer incoherent structure"));
	}
    }

    static void dummy_call(char *x)
    {
        static char id[]="$Id: elastic.cpp,v 1.7.2.1 2007/07/22 16:34:59 edrusb Rel $";
        dummy_call(id);
    }

    U_32 elastic::dump(char *buffer, U_32 size) const
    {
	if(size < taille)
	    throw Erange("elastic::dump", gettext("not enough space provided to dump the elastic buffer"));

	if(taille > 2)
	{
	    register U_32 pos;
	    register U_32 cur;
	    register U_32 len;
	    infinint infos = taille;
	    infinint tmp = infos.get_storage_size();

	    srand(time(NULL)+getpid());

		////
		// choosing the location of the first marks and following informations

	    len = 0;
	    tmp.unstack(len);
		// len is the place taken by the information (in bytes)
	    if(tmp != 0)
		throw SRC_BUG;
		// storage of a U_32 takes more bytes than the max value of U_32 !!!
	    if(len + 2 > taille) // +2 marks
		throw SRC_BUG;
		// a value takes more bytes to be stored as its own value !!!
		// (is only true for zero, which is an invalid "taille").

	    if(len + 2 < taille)
		cur = rand() % (taille - (len + 2)); // cur is the position of the first mark
	    else // len + 2 == taille
		cur = 0;

		// filling non information bytes with random values
	    for(pos = 0; pos < cur; ++pos)
		randomize(buffer+pos);

	    buffer[cur++] = LOW_MARK;
	    pos = 0;
	    while(pos < len)
		buffer[cur++] = (char)(infos[pos++]);
	    buffer[cur++] = HIGH_MARK;

		// filling the rest of the non information byte with random values
	    for(pos = cur; pos < taille; ++pos)
		randomize(buffer+pos);
	}
	else
	    if(taille == 1)
		buffer[0] = SINGLE_MARK;
	    else
		if(taille == 2)
		{
		    buffer[0] = LOW_MARK;
		    buffer[1] = HIGH_MARK;
		}
		else
		    throw SRC_BUG; // 'taille' is equal to zero (?)

	return taille;
    }


    void elastic::randomize(char *a) const
    {
	do
	{
	    *a = (unsigned char)(rand() % 256);
	}
	while(*a == SINGLE_MARK || *a == LOW_MARK || *a == HIGH_MARK);
    }


} // end of namespace


