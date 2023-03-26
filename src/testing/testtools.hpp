/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2023 Denis Corbin
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

#ifndef TESTTOOLS_HPP
#define TESTTOOLS_HPP

#include "../my_config.h"
#include "infinint.hpp"
#include "generic_file.hpp"

using namespace libdar;

extern void display(const infinint & x);
extern void display_read(user_interaction & dialog, generic_file & f);
extern void display_back_read(user_interaction & dialog, generic_file & f);

#endif
