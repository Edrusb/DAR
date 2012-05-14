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
// $Id: test_sar.cpp,v 1.5 2003/10/18 14:43:07 edrusb Rel $
//
/*********************************************************************/

#include "../my_config.h"
#include <iostream>

#include "sar.hpp"
#include "deci.hpp"
#include "testtools.hpp"
#include "user_interaction.hpp"
#include "test_memory.hpp"
#include "integers.hpp"
#include "shell_interaction.hpp"

using namespace libdar;

static void f1();
static void f2();
static void f3();

int main()
{
    MEM_BEGIN;
    MEM_IN;
    shell_interaction_init(&cout, &cerr);
    MEM_OUT;
    f1();
    MEM_OUT;
    f2();
    MEM_OUT;
    f3();
    MEM_OUT;
    MEM_END;
}

static void f1()
{
    try
    {
        sar sar1 = sar("destination", "txt", 100, 110, SAR_OPT_DEFAULT/*&~SAR_OPT_DONT_ERASE&~SAR_OPT_WARN_OVERWRITE*/, path("./test"));
        fichier src = fichier("./test/source.txt", gf_read_only);
        src.copy_to(sar1);
    }
    catch(Egeneric &e)
    {
        e.dump();
    }
}

static void f2()
{
    try
    {
        sar sar2 = sar("destination", "txt", SAR_OPT_DEFAULT, path("./test"));
        fichier dst = fichier("./test/destination.txt", gf_write_only);

        sar2.copy_to(dst);
    }
    catch(Egeneric &e)
    {
        e.dump();
    }
}

static void f3()
{
    try
    {
        sar sar3 = sar("destination", "txt", SAR_OPT_DEFAULT, path("./test"));
        fichier src = fichier("./test/source.txt", gf_read_only);
        
        display(sar3.get_position());
        display(src.get_position());

        sar3.skip(210);
        src.skip(210);
        display(sar3.get_position());
        display(src.get_position());
        display_read(sar3);
        display_read(src);
        display(sar3.get_position());
        display(src.get_position());

        sar3.skip_to_eof();
        src.skip_to_eof();
        display(sar3.get_position());
        display(src.get_position());
        display_read(sar3);
        display_read(src);
        display_back_read(sar3);
        display_back_read(src);

        display(sar3.get_position());
        display(src.get_position());
        sar3.skip_relative(-20);
        src.skip_relative(-20);
        display(sar3.get_position());
        display(src.get_position());
        display_read(sar3);
        display_read(src);
        display_back_read(sar3);
        display_back_read(src);
        display(sar3.get_position());
        display(src.get_position());

        sar3.skip(3);
        src.skip(3);
        display_back_read(sar3);
        display_back_read(src);
        display(sar3.get_position());
        display(src.get_position());
        display_read(sar3);
        display_read(src);
        display(sar3.get_position());
        display(src.get_position());

        sar3.skip(0);
        src.skip(0);
        display_back_read(sar3);
        display_back_read(src);
        display_read(sar3);
        display_read(src);
    }
    catch(Egeneric & e)
    {
        e.dump();
    }
    shell_interaction_close();
}
