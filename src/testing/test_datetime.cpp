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
// to contact the author, see the AUTHOR file
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
} // end extern "C"

#include <iostream>

#include "libdar.hpp"
#include "datetime.hpp"
#include "shell_interaction.hpp"

using namespace libdar;
using namespace std;

static shell_interaction ui(cout, cerr, false);

void f1();
void f2();
void show_val(const datetime & val, datetime::time_unit unit);

int main()
{
    U_I maj, med, min;

    get_version(maj, med, min);

    try
    {
	f1();
	f2();
	return 0;
    }
    catch(Egeneric &e)
    {
	ui.printf("Exception caught: %S", &(e.get_message()));
	cout << e.dump_str() << endl;
	return 1;
    }
}

void f1()
{
    datetime d1 = infinint(315360000); // 1er janvier 1980
    datetime d2(1000, 50, datetime::tu_microsecond);

    show_val(d1, datetime::tu_nanosecond);
    show_val(d2, datetime::tu_nanosecond);
    show_val(d2, datetime::tu_microsecond);

    if(d1 == d2)
	ui.printf("Error !");
    if(d1 < d2)
	ui.printf("Error !");
    if(d1 > d2)
	ui.printf("OK");

    datetime d3 = d1 + d2;
    show_val(d3, datetime::tu_nanosecond);

    d3 = d2 + d1;
    show_val(d3, datetime::tu_nanosecond);

    datetime d4 = d3 - d1;
    show_val(d4, datetime::tu_microsecond);
    show_val(d4, datetime::tu_second);

    d4 -= d2;
    show_val(d4, datetime::tu_nanosecond);

    d3 = datetime(1, 100, datetime::tu_nanosecond);
    d4 = datetime(2, 100, datetime::tu_nanosecond);
    d4 -= d3;
    show_val(d4, datetime::tu_nanosecond);

    datetime d5(2000, 500, datetime::tu_nanosecond);
    show_val(d5, datetime::tu_microsecond);
}


void f2()
{
}


void show_val(const datetime & val, datetime::time_unit unit)
{
    string subunit;
    infinint sec = val.get_second_value();
    infinint sub = val.get_subsecond_value(unit);

    switch(unit)
    {
    case datetime::tu_second:
	subunit = "s";
	break;
    case datetime::tu_microsecond:
	subunit = "us";
	break;
    case datetime::tu_nanosecond:
	subunit = "ns";
	break;
    default:
	throw SRC_BUG;
    }

    ui.printf("result: %i s + %i %S", &sec, &sub, &subunit);
}
