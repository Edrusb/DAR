/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2025 Denis Corbin
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
} // extern "C"

#include "archive_summary.hpp"

using namespace std;

namespace libdar
{
    void archive_summary::clear()
    {
	slice_size = 0;
	first_slice_size = 0;
	last_slice_size = 0;
	slice_number = 0;
	archive_size = 0;
	catalog_size = 0;
	storage_size = 0;
	data_size = 0;
	contents.clear();
	edition.clear();
	algo_zip.clear();
	user_comment.clear();
	cipher.clear();
	asym.clear();
	is_signed = false;
	tape_marks = false;
	compr_block_size = 0;
	salt.clear();
	kdf_hash.clear();
    }

} // end of namespace
