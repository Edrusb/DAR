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
// $Id: config_file.hpp,v 1.1 2003/02/11 22:01:19 edrusb Rel $
//
/*********************************************************************/

#ifndef CONFIG_FILE_HPP
#define CONFIG_FILE_HPP


#include "hide_file.hpp"
#include <vector>

class config_file : public hide_file
{
public:	
    config_file(const vector<string> & target, generic_file &f) : hide_file(f) { cibles = target; };

protected:
    void fill_morceau();

private:
    vector<string> cibles;

    bool find_next_target(generic_file &f, infinint & debut, string & nature, infinint & fin);
};


#endif
