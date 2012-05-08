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
// $Id: macro_tools.cpp,v 1.5 2003/02/11 22:01:54 edrusb Rel $
//
/*********************************************************************/

#include "macro_tools.hpp"
#include "terminateur.hpp"
#include "user_interaction.hpp"
#include "zapette.hpp"
#include "sar.hpp"
#include "dar_suite.hpp"

const version macro_tools_supported_version = "03";

static void version_check(const header_version & ver);

catalogue *macro_tools_get_catalogue_from(generic_file & f, compressor & zip, bool info_details, infinint &cat_size)
{
    terminateur term;
    catalogue *ret;
	
    if(info_details)
	user_interaction_warning("Extracting contents of the archive...");

    term.read_catalogue(f);
    if(zip.skip(term.get_catalogue_start()))
    {
	try
	{
	    ret = new catalogue(zip);
	}
	catch(Egeneric & e)
	{
	    throw Erange("get_catalogue_from", string("cannot open catalogue: ") + e.get_message());
	}
	cat_size = zip.get_position() - term.get_catalogue_start();
    }
    else
	throw Erange("get_catalogue_from", "missing catalogue in file.");

    if(ret == NULL)
	throw Ememory("get_catalogue_from");

    return ret;
}

void macro_tools_open_archive(const path &sauv_path, 
			      const string &basename, 
			      const string &extension, 
			      S_I options, 
			      const string & pass, 
			      generic_file *&ret1, 
			      scrambler *&scram, 
			      compressor *&ret2, 
			      header_version &ver, 
			      const string &input_pipe, 
			      const string &output_pipe, 
			      const string & execute)
{
    generic_file *zip_base = NULL;
    
    if(basename == "-")
    {
	tuyau *in = NULL;
	tuyau *out = NULL;
	
	try
	{
	    tools_open_pipes(input_pipe, output_pipe, in, out);
	    ret1 = new zapette(in, out);
	    if(ret1 == NULL)
	    {
		delete in;
		delete out;
	    }
	    else
		in = out = NULL; // now managed by the zapette
	}
	catch(...)
	{
	    if(in != NULL)
		delete in;
	    if(out != NULL)
		delete out;
	    throw;
	}
    }
    else
	ret1 = new sar(basename, extension, options, sauv_path, execute);

    if(ret1 == NULL)
	throw Ememory("open_archive");

    if(pass != "")
    {
	scram = new scrambler(pass, *ret1);
	if(scram == NULL)
	    throw Ememory("open_archive");
	zip_base = scram;
    }
    else
    {
	scram = NULL;
	zip_base = ret1;
    }

    ver.read(*ret1);
    version_check(ver);
    catalogue_set_reading_version(ver.edition);
    file::set_compression_algo_used(char2compression(ver.algo_zip));
    file::set_archive_localisation(zip_base);
    ret2 = new compressor(char2compression(ver.algo_zip), *zip_base);
    if((ver.flag & VERSION_FLAG_SCRAMBLED) != 0)
	user_interaction_warning("Warning, this archive has been \"scrambled\" (-K option). A wrong key is not possible to detect, it will cause DAR to report the archive as corrupted\n");

    if(ret2 == NULL)
    {
	delete ret1;
	throw Ememory("open_archive");
    }
}

catalogue *macro_tools_get_catalogue_from(const string &basename)
{
    generic_file *ret1 = NULL;
    scrambler *scram = NULL;
    compressor *ret2 = NULL;
    header_version ver;
    string input_pipe, output_pipe, execute;
    catalogue *ret = NULL;
    infinint size;
    string chemin, base;

    input_pipe = output_pipe = execute = "";
    tools_split_path_basename(basename, chemin, base);
    if(chemin == "")
	chemin = ".";
    try
    {
	path where = chemin;
	macro_tools_open_archive(where, base, EXTENSION, SAR_OPT_DONT_ERASE, "", 
				 ret1, scram, ret2, ver, input_pipe, output_pipe, execute);
	
	ret = macro_tools_get_catalogue_from(*ret1, *ret2, false, size);
    }
    catch(...)
    {
	if(ret1 != NULL)
	    delete ret1;
	if(ret2 != NULL)
	    delete ret2;
	if(scram != NULL)
	    delete scram;
	if(ret != NULL)
	    delete ret;
	throw;
    }
    if(ret1 != NULL)
	delete ret1;
    if(ret2 != NULL)
	delete ret2;
    if(scram != NULL)
	delete scram;
    
    return ret;
}

static void dummy_call(char *x)
{
    static char id[]="$Id: macro_tools.cpp,v 1.5 2003/02/11 22:01:54 edrusb Rel $";
    dummy_call(id);
}

static void version_check(const header_version & ver)
{
    if(atoi(ver.edition) > atoi(macro_tools_supported_version))
	user_interaction_pause("The format version of the archive is too high for that software version, try reading anyway?");
}
