//*********************************************************************/
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
// $Id: archive.hpp,v 1.2.2.1 2003/12/29 22:40:19 edrusb Rel $
//
/*********************************************************************/
//

#ifndef ARCHIVE_H
#define ARCHIVE_H

#include "crypto.hpp"
#include "path.hpp"
#include "catalogue.hpp"
#include "scrambler.hpp"

namespace libdar
{

    class archive
    {
    public:
	archive(const path & chem, const std::string & basename, const std::string & extension,
		crypto_algo crypto, const std::string &pass,
		const std::string & input_pipe, const std::string & output_pipe,
		const std::string & execute, bool info_details);
	~archive() { free(); };

	catalogue & get_cat() { if(cat == NULL) throw SRC_BUG; else return *cat; };
	const header_version & get_header() const { return ver; };
	const path & get_path() { if(local_path == NULL) throw SRC_BUG; else return *local_path; };

	bool get_sar_param(infinint & sub_file_size, infinint & first_file_size, infinint & last_file_size,
			   infinint & total_file_number);
	infinint get_level2_size();
	infinint get_cat_size() const { return local_cat_size; };

    private:
	generic_file *level1;
	scrambler *scram;
	compressor *level2;
	header_version ver;
	bool restore_ea_root;
	bool restore_ea_user;
	catalogue *cat;
	infinint local_cat_size;
	path *local_path;

	void free();
    };

} // end of namespace

#endif
