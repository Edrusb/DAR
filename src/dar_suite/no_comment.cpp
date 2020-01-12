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
#include "no_comment.hpp"

using namespace libdar;

void no_comment::fill_morceau()
{
    partie tmp;
    infinint last_offset = 0;
    char a = ' ';
    enum { st_unknown, st_command, st_comment } status = st_unknown;
    bool stop = false;
    bool last_block_is_comment = true;

    morceau.clear();
    if(ref == nullptr)
        throw SRC_BUG;
    ref->skip(0);
    tmp.longueur = 0;

    while(!stop)
    {
        stop = ref->read(&a, 1) != 1;
        switch(status)
        {
        case st_unknown:
            switch(a)
            {
            case ' ':
            case '\t':
                ++tmp.longueur;
                break;
	    case '\n':  // cannot treat empty lines as command, because too short to fit in loop/switch
		tmp.longueur = 0;
		break;
            case '#':
                status = st_comment;
                break;
            default:
                status = st_command;
                tmp.debut = ref->get_position() - 1;
                tmp.offset = last_offset;
		tmp.longueur = 1;
            }
            break;
        case st_comment:
            if(a == '\n')
            {
                status = st_unknown;
                last_block_is_comment = true;
                tmp.longueur = 0;
            }
            break;
        case st_command:
            if(!stop)
                ++tmp.longueur;
            if(a == '\n' || stop)
            {
                status = st_unknown;

                if(last_block_is_comment)
                {
                    morceau.push_back(tmp);
                    last_offset = tmp.offset+tmp.longueur;
                }
                else
                {
                    if(morceau.empty())
                        throw SRC_BUG;
                    morceau.back().longueur = ref->get_position() - morceau.back().debut;
                    last_offset = morceau.back().offset+morceau.back().longueur;
                }
                last_block_is_comment = false;
                tmp.longueur = 0;
            }
            break;
        default:
            throw SRC_BUG;
        }
    }
}
