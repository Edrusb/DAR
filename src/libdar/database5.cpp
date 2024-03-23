/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2024 Denis Corbin
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
#if STDC_HEADERS
#include <stdlib.h>
#endif

#if HAVE_ERRNO_H
#include <errno.h>
#endif
} // end extern "C"

#include <iomanip>
#include <iostream>
#include <deque>
#include "database5.hpp"
#include "user_interaction.hpp"
#include "deci.hpp"
#include "tools.hpp"
#include "storage.hpp"
#include "database_header.hpp"
#include "path.hpp"
#include "generic_file.hpp"
#include "data_tree.hpp"
#include "erreurs.hpp"
#include "nls_swap.hpp"

using namespace libdar5;
using namespace std;

    // from data_tree.hpp
using libdar::data_tree;

	// from erreurs.hpp
    using libdar::Egeneric;
    using libdar::Ebug;

namespace libdar5
{

    void database::show_contents(user_interaction & dialog) const
    {
	NLS_SWAP_IN;
	try
	{
	    libdar::database_archives_list content = get_contents();

	    string opt = libdar::tools_concat_vector(" ", get_options());

	    if(!dialog.get_use_dar_manager_contents())
	    {
		string compr = compression2string(get_compression());
		string dar_path = get_dar_path();
		string db_version = get_database_version();
		dialog.warning("");
		dialog.printf(gettext("dar path        : %S"), &dar_path);
		dialog.printf(gettext("dar options     : %S"), &opt);
		dialog.printf(gettext("database version: %S"), &db_version);
		dialog.printf(gettext("compression used: %S"), &compr);
		dialog.warning("");
		dialog.printf(gettext("archive #   |    path      |    basename"));
		dialog.printf("------------+--------------+---------------");
	    }

	    string road, base;

	    for(archive_num i = 1; i < content.size(); ++i)
	    {
		road = content[i].get_path();
		base = content[i].get_basename();
		if(dialog.get_use_dar_manager_contents())
		    dialog.dar_manager_contents(i, road, base);
		else
		{
		    opt = (road == "") ? gettext("<empty>") : road;
		    dialog.printf(" \t%u\t%S\t%S", i, &opt, &base);
		}
	    }
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void database::show_files(user_interaction & dialog, archive_num num, const database_used_options & opt) const
    {
	NLS_SWAP_IN;
	try
	{
 	    get_files(show_files_callback, &dialog, num, opt);
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }


    void database::show_version(user_interaction & dialog, path chemin) const
    {
	NLS_SWAP_IN;
	try
	{
	    get_version(get_version_callback, &dialog, chemin);
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void database::show_most_recent_stats(user_interaction & dialog) const
    {
	NLS_SWAP_IN;
	try
	{
	    if(!dialog.get_use_dar_manager_statistics())
	    {
		dialog.printf(gettext("  archive #   |  most recent/total data |  most recent/total EA"));
		dialog.printf(gettext("--------------+-------------------------+-----------------------")); // having it with gettext let the translater adjust columns width
	    }
	    libdar::database::show_most_recent_stats(statistics_callback, &dialog);
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }


    void database::show_files_callback(void *tag,
				       const std::string & filename,
				       bool available_data,
				       bool available_ea)
    {
	user_interaction *dialog = (user_interaction *)(tag);

	if(dialog == nullptr)
	    throw SRC_BUG;

	if(dialog->get_use_dar_manager_show_files())
	    dialog->dar_manager_show_files(filename,
					   available_data,
					   available_ea);
	else
	{
	    string etat = "";

	    if(available_data)
		etat += gettext("[ Saved ]");
	    else
		etat += gettext("[       ]");

	    if(available_ea)
		etat += gettext("[  EA   ]");
	    else
		etat += gettext("[       ]");

	    dialog->printf("%S  %S", &etat, &filename);
	}
    }

    void database::get_version_callback(void *tag,
					archive_num num,
					db_etat data_presence,
					bool has_data_date,
					datetime data,
					db_etat ea_presence,
					bool has_ea_date,
					datetime ea)
    {
	const string REMOVED = gettext("removed ");
	const string PRESENT = gettext("present ");
	const string SAVED   = gettext("saved   ");
	const string ABSENT  = gettext("absent  ");
	const string PATCH   = gettext("patch   ");
	const string BROKEN  = gettext("BROKEN  ");
	const string INODE   = gettext("inode   ");
	const string NO_DATE = "                          ";
	string data_state;
	string ea_state;
	string data_date;
	string ea_date;
	user_interaction *dialog = (user_interaction *)(tag);

	if(dialog == nullptr)
	    throw SRC_BUG;

	switch(data_presence)
	{
	case db_etat::et_saved:
	    data_state = SAVED;
	    break;
	case db_etat::et_patch:
	    data_state = PATCH;
	    break;
	case db_etat::et_patch_unusable:
	    data_state = BROKEN;
	    break;
	case db_etat::et_inode:
	    data_state = INODE;
	    break;
	case db_etat::et_present:
	    data_state = PRESENT;
	    break;
	case db_etat::et_removed:
	    data_state = REMOVED;
	    break;
	case db_etat::et_absent:
	    data_state = ABSENT;
	    break;
	default:
	    throw SRC_BUG;
	}

	switch(ea_presence)
	{
	case db_etat::et_saved:
	    ea_state = SAVED;
	    break;
	case db_etat::et_present:
	    ea_state = PRESENT;
	    break;
	case db_etat::et_removed:
	    ea_state = REMOVED;
	    break;
	case db_etat::et_absent:
	    throw SRC_BUG; // state not used for EA
	case db_etat::et_patch:
	    throw SRC_BUG;
	case db_etat::et_patch_unusable:
	    throw SRC_BUG;
	default:
	    throw SRC_BUG;
	}

	if(!has_data_date)
	{
	    data_state = ABSENT;
	    data_date = NO_DATE;
	}
	else
	    data_date = tools_display_date(data);

	if(!has_ea_date)
	{
	    ea_state = ABSENT;
	    ea_date = NO_DATE;
	}
	else
	    ea_date = tools_display_date(ea);

	if(dialog->get_use_dar_manager_show_version())
	    dialog->dar_manager_show_version(num, data_date, data_state, ea_date, ea_state);
	else
	    dialog->printf(" \t%u\t%S  %S  %S  %S", num, &data_date, &data_state, &ea_date, &ea_state);
    }

    void database::statistics_callback(void *tag,
				       U_I number,
				       const infinint & data_count,
				       const infinint & total_data,
				       const infinint & ea_count,
				       const infinint & total_ea)
    {
	user_interaction *dialog = (user_interaction *)(tag);

	if(dialog == nullptr)
	    throw SRC_BUG;

	if(dialog->get_use_dar_manager_statistics())
	    dialog->dar_manager_statistics(number, data_count, total_data, ea_count, total_ea);
	else
	    dialog->printf("\t%u %i/%i \t\t\t %i/%i", number, &data_count, &total_data, &ea_count, &total_ea);
    }


} // end of namespace
