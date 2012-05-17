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
// $Id: header_version.hpp,v 1.12 2005/05/08 12:12:01 edrusb Rel $
//
/*********************************************************************/

    /// \file header_version.hpp
    /// \brief archive global header structure is defined here
    /// \ingroup Private

#ifndef HEADER_VERSION_HPP
#define HEADER_VERSION_HPP

#include "../my_config.h"
#include "infinint.hpp"
#include "generic_file.hpp"
#include "tools.hpp"

namespace libdar
{
    const int VERSION_FLAG_SAVED_EA_ROOT = 0x80;      // no more used since version "05"
    const int VERSION_FLAG_SAVED_EA_USER = 0x40;      // no more used since version "05"
    const int VERSION_FLAG_SCRAMBLED     = 0x20;      // scrambled or strong encryption used
    const int VERSION_SIZE = 3;
    typedef char dar_version[VERSION_SIZE];
    extern void version_copy(dar_version & dst, const dar_version & src);
    extern bool version_greater(const dar_version & left, const dar_version & right);
        // operation left > right

    struct header_version
    {
        dar_version edition;
        char algo_zip;
        std::string cmd_line;
        unsigned char flag; // added at edition 02

        void read(generic_file &f)
            {
                f.read(edition, sizeof(edition));
                f.read(&algo_zip, sizeof(algo_zip));
                tools_read_string(f, cmd_line);
                if(version_greater(edition, "01"))
                    f.read((char *)&flag, (size_t)1);
                else
                    flag = 0;
            };
        void write(generic_file &f)
            {
                f.write(edition, sizeof(edition));
                f.write(&algo_zip, sizeof(algo_zip));
                tools_write_string(f, cmd_line);
                f.write((char *)&flag, (size_t)1);
            };
    };

} // end of namespace

#endif
