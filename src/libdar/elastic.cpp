/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2022 Denis Corbin
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
// to contact the author, see the AUTHOR file
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

#if HAVE_GCRYPT_H
#ifndef GCRYPT_NO_DEPRECATED
#define GCRYPT_NO_DEPRECATED
#endif
#include <gcrypt.h>
#endif
}

#include "elastic.hpp"
#include "infinint.hpp"
#include "header_version.hpp"
#include "tools.hpp"

#define SINGLE_MARK 'X'

//// elastic buffer structure ////
//
//   .......LOOOOH.....
//
//   where '.' are random bytes and
//   'L' and 'H' are LOW marks and HIGH mark respectively
//   OOOOO is a byte field giving the size in byte of the total
//   elastic buffer (big endian), written in base 254 (values corresponding to LOW and HIGH marks cannot be used here)
//
// before archive format "07" the size was written in base 256 which sometimes
// lead this field to containe a low or high mark. At reading time it was not
// possible to determin the effective size of the elastic buffer due to confusion
// with this mark value inside the size field.
//
//   some trivial elastic buffer is 'X' for a buffer of size 1 (where 'X'
//     is the "single mark")
//   and 'LH' for an elastic buffer of size 2
////////////////////////////////////

using namespace std;

namespace libdar
{

    elastic::elastic(U_32 size)
    {
	if(size == 0)
	    throw Erange("elastic::elastic", gettext("Zero is not a valid size for an elastic buffer"));
	if(size >  max_length())
	    throw Erange("elastic::elastic", gettext("Size too large for an elastic buffer"));

	taille = size;
    }

    elastic::elastic(const unsigned char *buffer, U_32 size, elastic_direction dir, const archive_version & reading_ver)
    {
	U_32 pos = (dir == elastic_forward ? 0 : size - 1);
	const U_32 first_pos = pos;
	S_I step = (dir == elastic_forward ? +1 : -1);
	unsigned char first_mark = (dir == elastic_forward ? get_low_mark(reading_ver) : get_high_mark(reading_ver));
	unsigned char last_mark  = (dir == elastic_forward ? get_high_mark(reading_ver) : get_low_mark(reading_ver));

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
	    U_32 power_base = 1;
	    U_I base = base_from_version(reading_ver);
	    const U_32 int_width = sizeof(U_32);
	    U_32 byte_counter = 0;
		// now reading total size

	    pos += step;
	    taille = 0;
	    while(pos < size && buffer[pos] != last_mark)
	    {
		if(dir != elastic_forward)
		{
		    taille *= base;
		    taille += buffer[pos];
		}
		else
		{
		    taille += power_base * buffer[pos];
		    power_base *= base;
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

    elastic::elastic(generic_file &f, elastic_direction dir,  const archive_version & reading_ver)
    {
	U_32 count = 0;
	S_I (generic_file::*lecture)(char &a) = (dir == elastic_forward ? &generic_file::read_forward : &generic_file::read_back);
	unsigned char first_mark = (dir == elastic_forward ? get_low_mark(reading_ver) : get_high_mark(reading_ver));
	unsigned char last_mark  = (dir == elastic_forward ? get_high_mark(reading_ver) : get_low_mark(reading_ver));
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
	    U_32 power_base = 1;
	    U_I base = base_from_version(reading_ver);
	    const U_32 int_width = sizeof(U_32);
	    U_32 byte_counter = 0;
		// now reading total size

	    taille = 0;
	    while((f.*lecture)((char &)a) && a != last_mark)
	    {
		if(dir != elastic_forward)
		{
		    taille *= base;
		    taille += a;
		}
		else
		{
		    taille += power_base * a;
		    power_base *= base;
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

    U_32 elastic::dump(unsigned char *buffer, U_32 size) const
    {
	if(size < taille)
	    throw Erange("elastic::dump", gettext("not enough space provided to dump the elastic buffer"));

	if(taille > 2)
	{
	    U_32 pos;
	    U_32 cur;
	    U_32 len;
	    static const unsigned char base = 254;
	    deque <unsigned char> digits = tools_number_base_decomposition_in_big_endian(taille, (unsigned char)(base));
	    U_I seed = ::time(nullptr)+getpid();

#if CRYPTO_AVAILABLE
	    U_I stronger;
	    gcry_create_nonce((unsigned char*) &stronger, sizeof(stronger));
	    seed ^= stronger;
#endif
		// let's randomize a little more
	    srand(seed);

		////
		// choosing the location of the first marks and following informations

	    len = digits.size();
		// len is the number of byte taken by the size information to encode

	    if(len + 2 > taille) // +2 marks
		throw SRC_BUG;
		// a value takes more bytes to be stored than its own value !!!
		// (is only true for zero, which is an invalid "taille").

	    if(len + 2 < taille)
		cur = rand() % (taille - (len + 2)); // cur is the position of the first mark
	    else // len + 2 == taille
		cur = 0;

		// filling non information bytes with random values
	    for(pos = 0; pos < cur; ++pos)
		randomize(buffer+pos);

	    buffer[cur++] = get_low_mark();
	    pos = 0;
	    while(pos < len)
		buffer[cur++] = digits[pos++];
	    buffer[cur++] = get_high_mark();

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
		    buffer[0] = get_low_mark();
		    buffer[1] = get_high_mark();
		}
		else
		    throw SRC_BUG; // 'taille' is equal to zero (?)

	return taille;
    }


    void elastic::randomize(unsigned char *a) const
    {
	do
	{
	    *a = (unsigned char)(rand() % 256);
	}
	while(*a == SINGLE_MARK || *a == get_low_mark() || *a == get_high_mark());
    }

    U_I elastic::base_from_version(const archive_version & reading_ver) const
    {
	if(reading_ver > 6)
	    return 254;
	else
	    return 256;
    }

    unsigned char elastic::get_low_mark(const archive_version & reading_ver) const
    {
	if(reading_ver > 6)
	    return get_low_mark();
	else
	    return '>';
    }

    unsigned char elastic::get_high_mark(const archive_version & reading_ver) const
    {
	if(reading_ver > 6)
	    return get_high_mark();
	else
	    return '<';
    }


} // end of namespace
