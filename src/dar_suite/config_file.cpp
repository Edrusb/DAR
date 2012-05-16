//*********************************************************************/
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
// $Id: config_file.cpp,v 1.6.4.1 2003/12/20 23:05:34 edrusb Rel $
//
/*********************************************************************/
//

#include "../my_config.h"

extern "C"
{
#if STDC_HEADERS
#include <ctype.h>
#endif
} // end extern "C"

#include "config_file.hpp"
#include "tools.hpp"

using namespace libdar;

void config_file::fill_morceau()
{
    string read_target;
    partie tmp;
    infinint last_offset = 0;
    infinint first, last;
    enum { debut, fin } status = fin;

        // we read any text put before the first condition
    tmp.debut = 0;
    tmp.offset = 0;
    morceau.clear();

    if(ref == NULL)
        throw SRC_BUG;
    ref->skip(0);

    while(find_next_target(*ref, first, read_target, last))
    {
        switch(status)
        {
        case fin:
            if(first < tmp.debut)
                throw SRC_BUG; // first byte of the line where next target
                // resides is before where we started looking for new target !
            tmp.longueur = first - tmp.debut;
            last_offset += tmp.longueur;
            if(tmp.longueur > 0)
                morceau.push_back(tmp);
            status = debut;
                // no break !
        case debut:
            if(tools_is_member(read_target, cibles))
            {
                tmp.debut = last;
                tmp.offset = last_offset;
                status = fin;
            }
            break;
        default:
            throw SRC_BUG;
        }
    }

    if(status == fin)
    {
        if(ref->get_position() < tmp.debut)
            throw SRC_BUG;
        tmp.longueur = ref->get_position() - tmp.debut;
        if(tmp.longueur > 0)
            morceau.push_back(tmp);
    }
}


bool config_file::find_next_target(generic_file &f, infinint & debut, string & nature, infinint & fin)
{
    char a;
    bool found = false;
    enum { init, search, end, purge } status = init;
    debut = f.get_position();


    while(f.read(&a, 1) == 1 && !found)
    {
        switch(status)
        {
        case init:  // looking for next word beginning
            switch(a)
            {
            case ' ':
            case '\t':
                break;
            case '\n':
                debut = f.get_position();
                break;
            default:
                if(isalpha(a))
                {
                    nature = a;
                    status = search;
                }
                else
                    status = purge;
            }
            break;
        case search: // reading the current word
            if(isalpha(a))
                nature += a;
            else
                if(a == ':')
                    status = end;
                else
		    if(a == '\n')
		    {
			status = init;
			debut = f.get_position();
		    }
		    else
			status = purge;
            break;
        case end: // got a target word (= word + `:' )
            switch(a)
            {
            case ' ':
            case '\t':
            case '\r':
                break;
            case '\n':
                fin = f.get_position();
                found = true;
                break;
            default:
                status = purge;
            }
            break;
        case purge: // just read word is not a target, resetting to the next line
            if(a == '\n')
            {
                status = init;
                debut = f.get_position();
            }
            break;
        default:
            throw SRC_BUG;
        }
    }

    return found;
}

static void dummy_call(char *x)
{
    static char id[]="$Id: config_file.cpp,v 1.6.4.1 2003/12/20 23:05:34 edrusb Rel $";
    dummy_call(id);
}
