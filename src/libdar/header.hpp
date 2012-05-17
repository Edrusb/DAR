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
// $Id: header.hpp,v 1.9 2006/01/08 16:33:42 edrusb Rel $
//
/*********************************************************************/

    /// \file header.hpp
    /// \brief slice header structure is defined here
    /// \ingroup Private


#ifndef HEADER_HPP
#define HEADER_HPP

#include "../my_config.h"
#include "infinint.hpp"
#include "generic_file.hpp"
#include "user_interaction.hpp"

namespace libdar
{
    const unsigned int LABEL_SIZE = 10;

    const char FLAG_NON_TERMINAL = 'N';
    const char FLAG_TERMINAL = 'T';
    const char EXTENSION_NO = 'N';
    const char EXTENSION_SIZE = 'S';

    const U_32 SAUV_MAGIC_NUMBER = 123;

    typedef U_32 magic_number;
    typedef char label[LABEL_SIZE];

    extern void label_copy(label & left, const label & right);
    extern bool header_label_is_equal(const label &a, const label &b);
    extern void header_generate_internal_filename(label & ret);

    struct header
    {
        magic_number magic;
        label internal_name;
        char flag;
        char extension; // extension rules the use of the following fields
        infinint size_ext; // if EXTENSION_SIZE

        header();
        header(const header & ref) { copy_from(ref); };
        struct header & operator = (const header & ref) { copy_from(ref); return *this; };

        void read(generic_file & f);
        void write(generic_file & f);
        void read(user_interaction & dialog, S_I fd);
        void write(user_interaction & dialog, S_I fd);

        static U_I size() { return sizeof(magic_number) + sizeof(label) + 2*sizeof(char); };
        void copy_from(const header & ref);
    };

} // end of namespace

#endif
