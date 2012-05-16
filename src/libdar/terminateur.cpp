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
// $Id: terminateur.cpp,v 1.12.2.1 2005/02/06 18:30:58 edrusb Rel $
//
/*********************************************************************/

#include "../my_config.h"
#include "terminateur.hpp"
#include "elastic.hpp"

#define BLOCK_SIZE 4

namespace libdar
{

    void terminateur::dump(generic_file & f)
    {
        infinint size = f.get_position(), nbbit, reste;
        S_I last_byte;
        unsigned char a;

        pos.dump(f);
        size = f.get_position() - size;

        euclide(size, BLOCK_SIZE, nbbit, reste);

        if(reste != 0)
        {
                // adding some non informational bytes to get a multiple of BLOCK_SIZE
            S_I bourrage = reste % BLOCK_SIZE;
            a = 0;
            for(S_I i = bourrage; i < BLOCK_SIZE; i++)
                f.write((char *)&a, 1);

                // one more for remaing bytes and non informational bytes.
            nbbit++;
        }

        last_byte = nbbit % 8;
        nbbit /= 8; // now, nbbit is the number of byte of terminator string (more or less 1)

        if(last_byte != 0)
        {                // making the last byte (starting eof) of the terminator string
            a = 0;
            for(S_I i = 0; i < last_byte; i++)
            {
                a >>= 1;
                a |= 0x80;
            }
            f.write((char *)&a, 1);
        }
	else // adding a terminal non 0xFF byte. (terminal when read down from end of file)
	{
	    a = 0;
	    f.write((char *)&a, 1);
	}

            // writing down all the other bytes of the terminator string
        a = 0xff;
        while(nbbit > 0)
        {
            f.write((char *)&a, 1);
            nbbit--;
        }
    }

    static void dummy_call(char *x)
    {
        static char id[]="$Id: terminateur.cpp,v 1.12.2.1 2005/02/06 18:30:58 edrusb Rel $";
        dummy_call(id);
    }

    void terminateur::read_catalogue(generic_file & f, bool with_elastic)
    {
        S_I offset = 0;
        unsigned char a;

        f.skip_to_eof();
	if(with_elastic)
	    (void)elastic(f, elastic_backward);
	    // temporary anomymous elastic skip backward in 'f'
	    // up to the other elastic buffer end

        try
        {
                // reading & counting the terminator string
            char b;
            do
            {
                if(f.read_back(b) != 1)
                    throw Erange("",""); // exception used locally
                a = (unsigned char)b;
                if(a == 0xFF)
                    offset++;
            }
            while(a == 0xFF);
            offset *= 8; // offset is now a number of bits

                // considering the first non 0xFF byte of the terminator string (backward reading)
            while(a != 0)
            {
                if((a & 0x80) == 0)
                    throw Erange("","");
                offset++;
                a <<= 1;
            }

            offset *= BLOCK_SIZE; // offset is now the byte offset of the position start
                // now we know where is located the position structure pointing to the start of the catalogue
            if(offset < 0)
                throw SRC_BUG; // signed int overflow

                // skipping the start of "location"
            if(! f.skip_relative(-offset))
                throw Erange("","");
        }
        catch(Erange &e)
        {
            throw Erange("terminateur::get_catalogue", gettext("Badly formated terminator, cannot extract catalogue location"));
        }

            // reading and returning the position
        pos = infinint(f.get_gf_ui(), NULL, &f);
    }

} // end of namespace
