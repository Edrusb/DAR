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
// $Id: archive.cpp,v 1.3 2003/11/19 00:42:49 edrusb Rel $
//
/*********************************************************************/
//

#include "archive.hpp"
#include "sar.hpp"
#include "macro_tools.hpp"

namespace libdar
{
    
    archive::archive(const path & chem, const std::string & basename, const std::string & extension,
		     crypto_algo crypto, const std::string &pass,
		     const std::string & input_pipe, const std::string & output_pipe,
		     const std::string & execute, bool info_details)
    {
	level1 = NULL;
	scram = NULL;
	level2 = NULL;
	cat = NULL;
	local_path = NULL;

	try
	{
	    macro_tools_open_archive(chem, basename, extension, SAR_OPT_DEFAULT, crypto, pass, level1, scram, level2, ver,
				    input_pipe, output_pipe, execute);
	    if((ver.flag & VERSION_FLAG_SAVED_EA_ROOT) == 0)
		restore_ea_root = false; // not restoring something not saved
	    if((ver.flag & VERSION_FLAG_SAVED_EA_USER) == 0)
		restore_ea_user = false; // not restoring something not saved
	    
	    cat = macro_tools_get_catalogue_from(*level1, *level2, info_details, local_cat_size);
	    local_path = new path(chem);
	    if(local_path == NULL)
		throw Ememory("archive::archive");
	}
	catch(...)
	{
	    free();
	    throw;
	}
	    
    }


    bool archive::get_sar_param(infinint & sub_file_size, infinint & first_file_size, infinint & last_file_size, 
				infinint & total_file_number)
    {
	sar *real_decoupe = dynamic_cast<sar *>(level1);
	if(real_decoupe != NULL)
	{
	    sub_file_size = real_decoupe->get_sub_file_size();
	    first_file_size = real_decoupe->get_first_sub_file_size();
	    if(real_decoupe->get_total_file_number(total_file_number)
	       && real_decoupe->get_last_file_size(last_file_size))
		return true;
	    else // could not read size parameters
		throw Erange("archive::get_sar_param", "Sorry, file size is unknown at this step of the program.\n");
	}
	else
	    return false;
    }
    

    infinint archive::get_level2_size()
    {
	level2->skip_to_eof();
	return level2->get_position();
    }

    static void dummy_call(char *x)
    {
        static char id[]="$Id: archive.cpp,v 1.3 2003/11/19 00:42:49 edrusb Rel $";
        dummy_call(id);
    }

    void archive::free()
    {
	if(cat != NULL)
	    delete cat;
	if(level2 != NULL)
	    delete level2;
	if(scram != NULL)
	    delete scram;
	if(level1 != NULL)
	    delete level1;
	if(local_path != NULL)
	    delete local_path;
    }


} // end of namespace
