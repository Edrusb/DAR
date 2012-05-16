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
// $Id: hide_file.hpp,v 1.10 2004/10/01 22:05:34 edrusb Rel $
//
/*********************************************************************/

#ifndef HIDE_FILE_HPP
#define HIDE_FILE_HPP

#include "../my_config.h"
#include <vector>
#include "infinint.hpp"
#include "generic_file.hpp"

using namespace libdar;
using namespace std;

class hide_file : public generic_file
{
public:
    hide_file(user_interaction & dialog, generic_file &f);

    bool skip(const infinint & pos);
    bool skip_to_eof();
    bool skip_relative(S_I x);
    infinint get_position();

protected:
    struct partie
    {
        infinint debut, longueur; // debut is the offset in ref file
        infinint offset; // offset in the resulting file
    };

    vector <partie> morceau;
    generic_file *ref;

    S_I inherited_read(char *a, size_t size);
    S_I inherited_write(char *a, size_t size);

    virtual void fill_morceau() = 0;
        // the inherited classes have with this method
        // to fill the "morceau" variable that defines
        // the portions

private:
    U_I pos_index;
    infinint pos_relicat;
    bool is_init;

    void init();
};


#endif
