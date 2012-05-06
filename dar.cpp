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
// to contact the author by email : dar.linux@free.fr
/*********************************************************************/
// $Id: dar.cpp,v 1.34.1.4 2002/05/28 20:17:29 denis Rel $
//
/*********************************************************************/
//
#include <iostream.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "erreurs.hpp"
#include "generic_file.hpp"
#include "infinint.hpp"
#include "path.hpp"
#include "mask.hpp"
#include "user_interaction.hpp"
#include "terminateur.hpp"
#include "compressor.hpp"
#include "command_line.hpp"
#include "catalogue.hpp"
#include "sar.hpp"
#include "filtre.hpp"
#include "tools.hpp"
#include "header.hpp"
#include "defile.hpp"
#include "deci.hpp"
#include "test_memory.hpp"
#include "dar.hpp"

using namespace std;

#define EXIT_OK 0
#define EXIT_BUG -1
#define EXIT_SYNTAX -2
#define EXIT_ERROR -3

#define EXTENSION "dar"

const version supported_version = "01";
const char *application_version = "1.0.3";

static void op_create(const path &fs_root, const path &sauv_path, const path *ref_path,
		      const mask &selection, const mask &subtree, 
		      string filename, string *ref_filename,
		      bool allow_over, bool warn_over, bool info_details, bool pause,
		      compression algo, const infinint &file_size, 
		      const infinint &first_file_size,
		      int argc, char *argv[]);
static void op_extract(const path &fs_root, const path &sauv_path,
		       const mask &selection, const mask &subtree, 
		       string filename, bool allow_over, bool warn_over, 
		       bool info_details, bool detruire);
static void op_listing(const path &sauv_path, const string &filename, bool info_details);
static catalogue *get_catalogue_from(generic_file & decoupe, compressor &zip, bool info_details, infinint &cat_size);
static fichier *open_archive_fichier(const string &filename, bool allow_over, bool warn_over);
static void open_archive(const path &sauv_path, const string &basename, const string &extension, int options, sar *&ret1, compressor *&ret2, header_version &ver);
static string make_string_from(int argc, char *argv[]);
static void display_sauv_stat(const statistics & st);
static void display_rest_stat(const statistics & st);
static void version_check(const header_version & ver);

const char *get_application_version()
{
    return application_version;
}

int main(int argc, char *argv[])
{
    int ret = EXIT_OK;

    MEM_IN;
    try
    {
	operation op;
	path *fs_root;
	path *sauv_root;
	path *ref_root;
	infinint file_size;
	infinint first_file_size;
	mask *selection, *subtree;
	string filename, *ref_filename;
	bool allow_over, warn_over, info_details, detruire, pause, beep;
	compression algo;

	user_interaction_init(0, cerr);
	if(! get_args(argc, argv, op, fs_root, sauv_root, ref_root,
		      file_size, first_file_size, selection, 
		      subtree, filename, ref_filename, 
		      allow_over, warn_over, info_details, algo, detruire, pause, beep))
	    ret = EXIT_SYNTAX;
	else
	{
	    user_interaction_set_beep(beep);
	    
	    try
	    {
		switch(op)
		{
		case create:
		    op_create(*fs_root, *sauv_root, ref_root, *selection, *subtree, filename, ref_filename, 
			      allow_over, warn_over, info_details, pause, algo, file_size, 
			      first_file_size, argc, argv);
		    break;
		case extract:
		    op_extract(*fs_root, *sauv_root, *selection, *subtree, filename, allow_over, warn_over,
			       info_details, detruire);
		    break;
		case diff:
		case test:
		    throw Efeature("archive testing is not yet implemented");
		case listing:
		    op_listing(*sauv_root, filename, info_details);
		    break;
		default:
		    throw SRC_BUG;
		}
	    }
	    catch(...)
	    {
		if(fs_root != NULL)
		    delete fs_root;
		if(sauv_root != NULL)
		    delete sauv_root;
		if(selection != NULL)
		    delete selection;
		if(subtree != NULL)
		    delete subtree;
		if(ref_root != NULL)
		    delete ref_root;
		if(ref_filename != NULL)
		    delete ref_filename;
		throw;
	    }
	    if(fs_root != NULL)
		delete fs_root;
	    if(sauv_root != NULL)
		delete sauv_root;
	    if(selection != NULL)
		delete selection;
	    if(subtree != NULL)
	    delete subtree;
	    if(ref_root != NULL)
		delete ref_root;
	    if(ref_filename != NULL)
		delete ref_filename;
	}
    }
    catch(Efeature &e)
    {
	user_interaction_warning(string("NOT YET IMPLEMENTED FEATURE has been used : ") + e.get_message());
	user_interaction_warning("please check documentation or upgrade your software if available");
	ret = EXIT_SYNTAX;
    }
    catch(Ehardware & e)
    {
	user_interaction_warning(string("SEEMS TO BE A HARDWARE PROBLEM"));
	e.dump();
	user_interaction_warning("please check your hardware");
	ret = EXIT_ERROR;
    }
    catch(Ememory & e)
    {
	user_interaction_warning("lack of memory to achieve the operation, aborting operation");
	ret = EXIT_ERROR;
    }
    catch(Erange & e)
    {
	user_interaction_warning("FATAL error, aborting operation");
	user_interaction_warning(e.get_message());
	ret = EXIT_SYNTAX;
    }
    catch(Euser_abort & e)
    {
	user_interaction_warning(string("aborting program. User refused to continue while asking : ") + e.get_message());
	ret = EXIT_ERROR;
    }
    catch(Egeneric & e)
    {
	e.dump();
	user_interaction_warning("INTERNAL ERROR, PLEASE REPORT THE PREVIOUS OUTPUT TO MAINTAINER");
	ret = EXIT_BUG;
    }
    
    user_interaction_close();
    MEM_OUT;
    MEM_END;
    return ret;
}

static void op_create(const path &fs_root, const path &sauv_path, const path *ref_path,
		      const mask &selection, const mask &subtree, 
		      string filename, string *ref_filename,
		      bool allow_over, bool warn_over, bool info_details, bool pause,
		      compression algo, const infinint &file_size, 
		      const infinint &first_file_size, 
		      int argc, char *argv[])
{
    MEM_IN;
    try
    {
	terminateur coord;
	catalogue current;
	catalogue *ref = NULL;
	generic_file *decoupe = NULL;
	compressor *zip = NULL;
	int sar_opt = 0;

	if(sauv_path.is_subdir_of(fs_root)
	   && selection.is_covered(filename+".1."+EXTENSION)
	   && subtree.is_covered((sauv_path + (filename+".1."+EXTENSION)).display()))
	    user_interaction_pause(string("WARNING ! the archive is located in the directory to backup, this may create endless loop when the archive will try to save itself. You can either add -X \"")+ filename + ".*." + EXTENSION +"\" on the command line, or change the location of the archive (see -h for help). Do you really want to continue ?");

	    // building the reference catalogue
	if(ref_filename != NULL) // from a existing archive
	{
	    sar *tmp;
	    header_version v;

	    if(ref_path == NULL)
		throw SRC_BUG; // ref_filename != NULL but ref_path == NULL;
	    open_archive(*ref_path, *ref_filename, EXTENSION, SAR_OPT_DONT_ERASE, tmp, zip, v);
	    try
	    {
		infinint tmp2;

		ref = get_catalogue_from(*tmp, *zip, info_details, tmp2);
	    }
	    catch(...)
	    {
		delete zip;
		delete tmp;
		throw;
	    }
	    delete zip;
	    delete tmp;
	    zip = NULL;
	}
	else // building reference catalogue from scratch (empty catalogue)
	    ref = new catalogue();

	if(ref == NULL)
	    throw Ememory("op_create");
	try
	{
	    statistics st;
	    header_version ver;

	    if(allow_over)
		sar_opt &= ~SAR_OPT_DONT_ERASE;
	    else
		sar_opt |= SAR_OPT_DONT_ERASE;
	    if(warn_over)
		sar_opt |= SAR_OPT_WARN_OVERWRITE;
	    else
		sar_opt &= ~SAR_OPT_WARN_OVERWRITE;
	    if(pause)
		sar_opt |= SAR_OPT_PAUSE;
	    else
		sar_opt &= ~SAR_OPT_PAUSE;
	    if(pause && ref_path != NULL && *ref_path == sauv_path)
		user_interaction_pause("ready to start writing archive ? ");

	    if(file_size == 0)
		decoupe = open_archive_fichier((sauv_path + sar_make_filename(filename, 1, EXTENSION)).display(), allow_over, warn_over);
	    else
		decoupe = new sar(filename, EXTENSION, file_size, first_file_size, sar_opt, sauv_path);
	    if(decoupe == NULL)
		throw Ememory("op_create");

	    version_copy(ver.edition, supported_version);
	    ver.algo_zip = compression2char(algo);
	    ver.cmd_line = make_string_from(argc-1, argv+1); // argv[0] is the command itself, we don't need it (where from `+1')
	    ver.write(*decoupe);
	    
	    zip = new compressor(algo, *decoupe);
	    if(zip == NULL)
		throw Ememory("op_create");
	    filtre_sauvegarde(selection, subtree, zip, current, *ref, fs_root, info_details, st);
	    zip->flush_write();
	    coord.set_catalogue_start(zip->get_position());
	    if(info_details && ref_filename != NULL)
		user_interaction_warning("adding reference to files that has been destroyed since reference backup");
	    st.deleted = current.update_destroyed_with(*ref);
	    if(info_details)
		user_interaction_warning("writting archive content");
	    current.dump(*zip);
	    zip->flush_write();
	    delete zip; 
	    zip = NULL;
	    coord.dump(*decoupe);
	    delete decoupe;
	    display_sauv_stat(st);
	}
	catch(...)
	{
	    if(ref != NULL)
		delete ref;
	    if(zip != NULL)
	    {
		zip->clean_write();
		delete zip;
	    }
	    if(decoupe != NULL)
		delete decoupe;
	    throw;
	}
	delete ref;
    }
    catch(Erange &e)
    {
	user_interaction_warning(string("Error while saving data : ") + e.get_message());
    }
    MEM_OUT;
}

static void op_extract(const path &fs_root, const path &sauv_path,
		       const mask &selection, const mask &subtree, 
		       string filename, bool allow_over, bool warn_over, 
		       bool info_details,  bool detruire)
{
    try
    {
	sar *decoupe;
	compressor *zip;
	header_version ver;

	open_archive(sauv_path, filename, EXTENSION, SAR_OPT_DEFAULT, decoupe, zip, ver);
	try
	{
	    infinint tmp;

	    catalogue *cat = get_catalogue_from(*decoupe, *zip, info_details, tmp);
	    try
	    {
		statistics st;
		filtre_restore(selection, subtree, zip, *cat, detruire, fs_root, allow_over, warn_over, info_details, st);
		display_rest_stat(st);
	    }
	    catch(...)
	    {
		delete cat;
		throw;
	    }
	    delete cat;
	}
	catch(...)
	{
	    delete zip;
	    delete decoupe;
	    throw;
	}
	delete zip;
	delete decoupe;
    }
    catch(Erange &e)
    {
	user_interaction_warning(string("Error while restoring data : ") + e.get_message());
    }
}

static void op_listing(const path &sauv_path, const string &filename, bool info_details)
{
    try
    {
	sar *decoupe;
	compressor *zip;
	header_version ver;
	
	open_archive(sauv_path, filename, EXTENSION, SAR_OPT_DEFAULT, decoupe, zip, ver);
	try
	{
	    infinint cat_size;
 
	    catalogue *cat = get_catalogue_from(*decoupe, *zip, info_details, cat_size);
	    try
	    {
		infinint sub_file_size = decoupe->get_sub_file_size();
		infinint first_file_size = decoupe->get_first_sub_file_size();
		infinint last_file_size, file_number;
		cout << "archive version format               : " << ver.edition << endl;
		cout << "compression algorithm used           : " << compression2string(char2compression(ver.algo_zip)) << endl;
		cout << "catalogue size in archive            : " << deci(cat_size).human() << " bytes" << endl;
		cout << "command line options used for backup : " << ver.cmd_line << endl;
		if(decoupe->get_total_file_number(file_number) 
		   && decoupe->get_last_file_size(last_file_size))
		{
		    cout << "archive is composed of " << deci(file_number).human() << " file" <<endl;
		    if(file_number == 1)
			cout << "file size : " <<  deci(last_file_size).human() + " bytes" << endl;
		    else
		    {
			if(first_file_size != sub_file_size)
			    cout << "first file size       : " << deci(first_file_size).human() + " bytes" << endl;
			cout << "file size             : " << deci(sub_file_size).human() + " bytes" << endl;
			cout << "last file size        : " << deci(last_file_size).human() + " bytes" << endl;
		    }
		    if(file_number > 1)
			cout << "archive total size is : " << deci( first_file_size + (file_number-2)*sub_file_size + last_file_size ).human() << " bytes" << endl;
		}
		else
		    cout << "Sorry, file size is unknown at this step of the program" << endl; 
		user_interaction_pause("continue listing archive contents ?");
		cat->listing(cout);
	    }
	    catch(...)
	    {
		delete cat;
		throw;
	    }
	    delete cat;
	}
	catch(...)
	{
	    delete zip;
	    delete decoupe;
	    throw;
	}
	delete zip;
	delete decoupe;
    }
    catch(Erange &e)
    {
	user_interaction_warning(string("Error while listing archive contents : ") + e.get_message());
    }
}

static void dummy_call(char *x)
{
    static char id[]="$Id: dar.cpp,v 1.34.1.4 2002/05/28 20:17:29 denis Rel $";
    dummy_call(id);
}

static void open_archive(const path &sauv_path, const string &basename, const string &extension, int options, sar *&ret1, compressor *&ret2, header_version &ver)
{
    ret1 = new sar(basename, extension, options, sauv_path);
    if(ret1 == NULL)
	throw Ememory("open_archive");
    ver.read(*ret1);
    version_check(ver);
    ret2 = new compressor(char2compression(ver.algo_zip), *ret1);
    if(ret2 == NULL)
    {
	delete ret1;
	throw Ememory("open_archive");
    }
}

static catalogue *get_catalogue_from(generic_file & f, compressor & zip, bool info_details, infinint &cat_size)
{
    terminateur term;
    catalogue *ret;
	
    if(info_details)
	user_interaction_warning("extracting contents of the archive");

    term.read_catalogue(f);
    if(zip.skip(term.get_catalogue_start()))
    {
	ret = new catalogue(zip);
	cat_size = zip.get_position() - term.get_catalogue_start();
    }
    else
	throw Erange("get_catalogue_from", "missing catalogue in file");

    if(ret == NULL)
	throw Ememory("get_catalogue_from");

    return ret;
}

static fichier *open_archive_fichier(const string &filename, bool allow_over, bool warn_over)
{
    char *name = tools_str2charptr(filename);
    fichier *ret = NULL;
    
    try
    {
	int fd;

	if(!allow_over || warn_over)
	{
	    struct stat buf;
	    if(lstat(name, &buf) < 0)
	    {
		if(errno != ENOENT)
		    throw Erange("open_archive_fichier", strerror(errno));
	    }
	    else
	    {
		if(!allow_over)
		    throw Erange("open_archive_fichier", filename + " already exists, and overwritten is forbidden, aborting");
		if(warn_over)
		    user_interaction_pause(filename + " is about to be overwritten, continue ?");
	    }
	}
	    
	fd = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if(fd < 0)
	    throw Erange("open_archive_fichier", strerror(errno));

	ret = new trivial_sar(fd);
	if(ret == NULL) 
	    throw Ememory("open_archive_fichier");
    }
    catch(...)
    {
	delete name;
	if(ret != NULL)
	    delete ret;
	throw;
    }
    delete name;
    return ret;
}

static string make_string_from(int argc, char *argv[])
{
    string ret = "";
    int i;

    for(i = 0; i < argc; i++)
    {
	ret += argv[i];
	ret += " ";
    }

    return ret;
}

static void display_sauv_stat(const statistics & st)
{
    cout << endl << endl << " -------------------------------------------- " << endl;
    cout << deci(st.treated).human() << " file(s) saved" << endl;
    cout << deci(st.skipped).human() << " file(s) not saved (no file change)" << endl;
    cout << deci(st.ignored).human() << " file(s) ignored filed (excluded by filters)" << endl;
    cout << deci(st.errored).human() << " file(s) failed to save (filesystem error)" << endl;
    cout << deci(st.deleted).human() << " file(s) recorded as deleted from reference backup" << endl;
    cout << " -------------------------------------------- " << endl;
    cout << deci(st.treated+st.skipped+st.ignored+st.errored+st.deleted).human() << " total number of file considered" << endl;
}

static void display_rest_stat(const statistics & st)
{

    cout << endl << endl << " -------------------------------------------- " << endl;
    cout << deci(st.treated).human() << " file(s) restored" << endl;
    cout << deci(st.skipped).human() << " file(s) not restored (not saved in archive)" << endl;
    cout << deci(st.ignored).human() << " file(s) ignored (excluded by filters)" << endl;
    cout << deci(st.errored).human() << " file(s) failed to restore (filesystem error)" << endl;
    cout << deci(st.deleted).human() << " file(s) deleted" << endl;
    cout << " -------------------------------------------- " << endl;
    cout << deci(st.treated+st.skipped+st.ignored+st.errored+st.deleted).human() << " total number of file considered" << endl;
}

static void version_check(const header_version & ver)
{
    if(atoi(ver.edition) > atoi(supported_version))
	user_interaction_pause("the file version of the archive is too high for that software version, try reading anyway ?");
}
