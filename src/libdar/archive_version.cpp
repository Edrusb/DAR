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

#include "../my_config.h"

#include "archive_version.hpp"
#include "erreurs.hpp"
#include "tools.hpp"

using namespace std;

// value read from file when the file is full of '\0'
#define EMPTY_VALUE (48*256 + 48)

#define ARCHIVE_VER_SIZE 4
#define OLD_ARCHIVE_VER_SIZE 3

namespace libdar
{

    const archive_version empty_archive_version()
    {
	return archive_version(EMPTY_VALUE);
    }

    archive_version::archive_version(U_16 x, unsigned char x_fix)
    {
	if(x > EMPTY_VALUE)
	    throw Efeature(gettext("Archive version too high, use a more recent version of libdar"));
	else
	{
	    version = x;
	    fix = x_fix;
	}
    }

    void archive_version::dump(generic_file & f) const
    {
	unsigned char tmp[ARCHIVE_VER_SIZE];

	tmp[0] = version / 256;
	tmp[1] = version % 256;
	tmp[2] = fix;
	tmp[3] = '\0';

	for(U_I i = 0; i < 3; ++i)
	    tmp[i] = to_char(tmp[i]);

	(void)f.write((char *)tmp, sizeof(tmp));
    }


    void archive_version::read(generic_file & f)
    {
	unsigned char tmp[OLD_ARCHIVE_VER_SIZE];
	U_I lu = f.read((char *)tmp, sizeof(tmp));

	if(lu < sizeof(tmp))
	    throw Erange("archive_version::read", gettext("Reached End of File while reading archive version"));


	for(U_I i = 0; i < 2; ++i)
	    tmp[i] = to_digit(tmp[i]);

	version = tmp[0]*256+tmp[1]; // little endian used in file

	    // sanity checks

	if(version < 8)
	{
	    if(tmp[2] != '\0')
		throw Erange("archive_version::read", gettext("Unexpected value while reading archive version"));
	}
	else // version >= 8
	{
	    fix = to_digit(tmp[2]);
	    lu = f.read((char *)tmp, 1);
	    if(lu < 1)
		throw Erange("archive_version::read", gettext("Reached premature end of file while reading archive version"));
	    if(tmp[0] != '\0')
		throw Erange("archive_version::read", gettext("Unexpected value while reading archive version"));
	}
    }

    string archive_version::display() const
    {
	string ret = tools_uword2str(version);

	if(ret.size() < 2)
	    ret = string("0") + ret;

	if(fix > 0)
	{
	    ret += "." + tools_uword2str((U_16)(fix));
		// this is intentional that only few values may be supported, as the fix number
		// should not grow very much (fix < 10) between two major releases
		// so far it was needed only once for more 8 major released (well it did not
		// this feature did not exist at that time, thus I had to shift by one the
		// in development code some of the revision test, which took time and was error prone).
		// so, never again.

	}

	return ret;
    }

    unsigned char archive_version::to_digit(unsigned char val)
    {
	if(val < 48)
	    return val + 208;
	else
	    return val - 48;
    }

    unsigned char archive_version::to_char(unsigned char val)
    {
	if(val < 208)
	    return val + 48;
	else
	    return val - 208;
    }


} // end of namespace
