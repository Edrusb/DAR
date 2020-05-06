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

extern "C"
{

} // end extern "C"

#include "zstd.hpp"

using namespace std;

namespace libdar
{

    zstd::zstd(gf_mode mode,
	       U_I compression_level,
	       generic_file & compressed,
	       U_I workers)
    {
#if LIBZSTD_AVAILABLE

#else
	throw Ecompilation(gettext("zstd compression"));
#endif
    }

    zstd::~zstd()
    {
#if LIBZSTD_AVAILABLE

#endif
    }

    U_I zstd::read(char *a, U_I size)
    {
#if LIBZSTD_AVAILABLE

#else
	throw Ecompilation(gettext("zstd compression"));
#endif
    }

    void zstd::write(const char *a, U_I size)
    {
#if LIBZSTD_AVAILABLE

#else
	throw Ecompilation(gettext("zstd compression"));
#endif
    }

    void zstd::write_eof_and_flush()
    {
#if LIBZSTD_AVAILABLE

#else
	throw Ecompilation(gettext("zstd compression"));
#endif
    }

    void zstd::reset()
    {
#if LIBZSTD_AVAILABLE

#else
	throw Ecompilation(gettext("zstd compression"));
#endif
    }



} // end of namespace
