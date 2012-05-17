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
// $Id: zapette.hpp,v 1.4 2003/10/18 14:43:07 edrusb Rel $
//
/*********************************************************************/
//

#ifndef ZAPETTE_HPP
#define ZAPETTE_HPP

#pragma interface

#include "../my_config.h"
#include "generic_file.hpp"
#include "integers.hpp"

namespace libdar
{

    class zapette: public generic_file
    {
    public:

        zapette(generic_file *input, generic_file *output);
        ~zapette();

            // inherited methods
        bool skip(const infinint &pos);
        bool skip_to_eof() { position = file_size; return true; };
        bool skip_relative(S_I x);
        infinint get_position() { return position; };

    protected:
        S_I inherited_read(char *a, size_t size);
        S_I inherited_write(const char *a, size_t size);

    private:
        generic_file *in, *out;
        infinint position, file_size;
        char serial_counter;

        void make_transfert(U_16 size, const infinint &offset, char *data, S_I & lu, infinint & arg);
    };

    class slave_zapette
    {
    public:
        slave_zapette(generic_file *input, generic_file *output, generic_file *data);
        ~slave_zapette();

        void action();

    private:
        generic_file *in, *out, *src;
    };

} // end of namespace

#endif
