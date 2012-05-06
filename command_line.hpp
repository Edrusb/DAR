/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002 Denis Corbin
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
// $Id: command_line.hpp,v 1.19 2002/06/18 21:16:06 denis Rel $
//
/*********************************************************************/

#ifndef COMMAND_LINE_HPP
#define COMMAND_LINE_HPP

#include <string>
#include <vector>
#include "infinint.hpp"
#include "compressor.hpp"
#include "mask.hpp"
#include "path.hpp"

using namespace std;

enum operation { extract, create, diff, test, listing, isolate };

extern bool get_args(int argc, char *argv[], operation &op, path * &fs_root, path *&sauv_root, path *&ref_root, 
		     infinint &file_size, infinint &first_file_size, 
		     mask *&selection, mask *&subtree,
		     string &filename, string *&ref_filename,
		     bool &allow_over, bool &warn_over, bool &info_details,
		     compression &algo, bool &detruire, 
		     bool &pause, bool &beep,
		     bool & make_empty_dir, bool & only_more_recent, 
		     bool & ea_root, bool & ea_user,
		     string & input_pipe, string &output_pipe,
		     bool &ignore_owner);
    
#endif
