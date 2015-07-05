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
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
#if HAVE_STRING_H
#include <string.h>
#endif
} // end extern "C"

#include "libdar.hpp"
#include "escape.hpp"
#include "cygwin_adapt.hpp"
#include "shell_interaction.hpp"
#include "fichier_local.hpp"

using namespace libdar;
using namespace std;

static user_interaction *ui = nullptr;

void f1();
void f2();

int main()
{
    U_I maj, med, min;

    get_version(maj, med, min);
    ui = new (nothrow) shell_interaction(&cout, &cerr, false);
    if(ui == nullptr)
	cout << "ERREUR !" << endl;

    f1();
    f2();
}

void f1()
{
    set<escape::sequence_type> nojump;
    fichier_local below = fichier_local(*ui, "escape_below", gf_write_only, 0666, false, true, false);
    escape tested = escape(&below, nojump);

    const char *seq1 = "bonjour les amis";
    const char *seq2 = "il fait beau il fait chaud";

    tested.write(seq1, strlen(seq1));
    tested.add_mark_at_current_position(escape::seqt_file);
    tested.write(seq2, strlen(seq2));

    const U_I buf_size = 6;
    const unsigned char buffer[] = { 0xAD, 0xFD, 0xEA, 0x77, 0x21, 0x19 };

    tested.write((const char *)buffer, buf_size);
    tested.write((const char *)buffer, 3);
    tested.write((const char *)buffer+3, 3);
    tested.add_mark_at_current_position(escape::seqt_ea);
}

void f2()
{
    set<escape::sequence_type> nojump;
    const U_I buf_size = 100;
    unsigned char buffer[buf_size];
    fichier_local below = fichier_local(*ui, "escape_below", gf_read_only, 0666, false, false, false);
    escape tested = escape(&below, nojump);
    S_I lu = 0;

    tested.skip(0);

    lu = tested.read((char *)buffer, buf_size);
    buffer[lu] = '\0';
    cout << "[" << buffer << "]" << endl;

    lu = tested.read((char *)buffer, buf_size);
    buffer[lu] = '\0';
    cout << "[" << buffer << "]" << endl;

    tested.skip_to_next_mark(escape::seqt_ea, true);

    lu = tested.read((char *)buffer, buf_size);
    buffer[lu] = '\0';
    cout << "[" << buffer << "]" << endl;

    tested.skip(0);
    tested.skip_to_next_mark(escape::seqt_file, true);

    lu = tested.read((char *)buffer, buf_size);
    buffer[lu] = '\0';
    cout << "[" << buffer << "]" << endl;

    lu = tested.read((char *)buffer, buf_size);
    buffer[lu] = '\0';
    cout << "[" << buffer << "]" << endl;

    tested.skip(0);
    if(tested.skip_to_next_mark(escape::seqt_ea, false))
	cout << "OK" << endl;
    else
	cout << "NOK" << endl;
    cout << libdar::deci(tested.get_position()).human() << endl;
}
