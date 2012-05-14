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
// $Id: command_line.hpp,v 1.14 2003/03/13 20:48:11 edrusb Rel $
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

enum operation { noop, extract, create, diff, test, listing, isolate };
    // noop stands for no-operation. get_args() never returns such value,
    // it is just necessary within the command_line module

extern bool get_args(const char *home,
		     S_I argc, char *argv[], operation &op, path * &fs_root, 
		     path *&sauv_root, path *&ref_root, 
		     infinint &file_size, infinint &first_file_size, 
		     mask *&selection, mask *&subtree,
		     string &filename, string *&ref_filename,
		     bool &allow_over, bool &warn_over, bool &info_details,
		     compression &algo, U_I & compression_level,
		     bool &detruire, 
		     bool &pause, bool &beep,
		     bool & make_empty_dir, bool & only_more_recent, 
		     bool & ea_root, bool & ea_user,
		     string & input_pipe, string &output_pipe,
		     bool &ignore_owner,
		     string & execute, string & execute_ref,
		     string & pass, string & pass_ref,
		     mask *&compress_mask,
		     bool & flat,
		     infinint & min_compr_size,
		     bool & nodump);
    
#endif
