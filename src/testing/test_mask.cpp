/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2022 Denis Corbin
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
#include <iostream>

#include "mask.hpp"
#include "integers.hpp"

using namespace libdar;
using namespace std;

static void display_res(mask *m, string s)
{
    cout << s << " : " << (m->is_covered(s) ? "OUI" : "non") << endl;
}

int main()
{
    simple_mask m1 = simple_mask(string("*.toto"), true);
    simple_mask m2 = simple_mask(string("a?.toto"), true);
    simple_mask m3 = simple_mask(string("a?.toto"), true);
    simple_mask m4 = simple_mask(string("*.toto"), true);
    simple_mask m5 = simple_mask(string("*.toto"), false);
    display_res(&m1, "tutu.toto");
    display_res(&m2, "a1.toto");
    display_res(&m3, "b1.toto");
    display_res(&m4, "toto");
    display_res(&m4, "a.TOTO");
    display_res(&m5, "a.TOTO");

    bool_mask m6 = true;
    bool_mask m7 = false;
    display_res(&m6, "totot");
    display_res(&m7, "totot");

    regular_mask m8 = regular_mask("^toto", true);
    regular_mask m9 = regular_mask("titi$", false);
    display_res(&m8, "totola");
    display_res(&m8, "tOTOla");
    display_res(&m8, "ttotola");
    display_res(&m9, "latiti");
    display_res(&m9, "laTiTI");
    m8 = regular_mask(".+\\.[1-9][0-9]*\\.dar", true);
    display_res(&m8, "acapulco");
    display_res(&m8, ".1928.dar");
    display_res(&m8, "toto.182dar");
    display_res(&m8, "toto..dar");
    display_res(&m8, "tutu.1.dar");
    display_res(&m8, "tutu.0.dar");
    display_res(&m8, "t.1928.dar");
    m8 = regular_mask("^etc/rc.d/.*\\.", true);
    display_res(&m8, "etc/rc.d/toto.aiai");
    display_res(&m8, "etc/rc.d/totoaiai");
    display_res(&m8, "etc/rc.d/toto/titi.du");

    same_path_mask m10 = same_path_mask("Zorro", true);
    same_path_mask m11 = same_path_mask("Zorro", false);
    display_res(&m10, "Zorro");
    display_res(&m10, "zorro");
    display_res(&m10, "toto");
    display_res(&m11, "Zorro");
    display_res(&m11, "zorro");
    display_res(&m11, "toto");

    exclude_dir_mask m12 = exclude_dir_mask("/tmp/crotte", true);
    exclude_dir_mask m13 = exclude_dir_mask("/tmp/croTte", false);
    display_res(&m12, "/tmp/crotte");
    display_res(&m12, "/tmp/crotte/de/bique");
    display_res(&m12, "/tmp");
    display_res(&m12, "/toto/crotte");
    display_res(&m13, "/tmp/crotte");
    display_res(&m13, "/tmp/CrOtte");
    display_res(&m13, "/tMp/cRoTTE/seche");

    simple_path_mask m14 = simple_path_mask(path("/tmp/crotte"), true);
    simple_path_mask m15 = simple_path_mask(path("/tmp/croTte"), false);
    display_res(&m14, "/tmp/crotte");
    display_res(&m14, "/tmp/crotte/de/bique");
    display_res(&m14, "/tmp");
    display_res(&m14, "/toto/crotte");
    display_res(&m15, "/tmp/crotte");
    display_res(&m15, "/tmp/CrOtte");
    display_res(&m15, "/tMp/cRoTTE/seche");
    display_res(&m15, "/tMp");

}

