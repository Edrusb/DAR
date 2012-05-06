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
// $Id: command_line.cpp,v 1.52 2002/06/27 17:42:35 denis Rel $
//
/*********************************************************************/

#include <unistd.h>
#include <sys/types.h>
#include <getopt.h>
#include <string>
#include "deci.hpp"
#include "command_line.hpp"
#include "user_interaction.hpp"
#include "tools.hpp"
#include "dar.hpp"
#include "dar_suite.hpp"

using namespace std;

    // return a newly allocated memory (to be deleted by the caller)
static void show_license();
static void show_warranty();
static void show_version(const char *command_name);
static void usage(const char *command_name);
static const struct option *get_long_opt();

bool get_args(int argc, char *argv[], operation &op, path * &fs_root, path * &sauv_root, path * &ref_root,
	      infinint &file_size, infinint &first_file_size, mask *&selection, mask *&subtree,
	      string &filename, string *&ref_filename,
	      bool &allow_over, bool &warn_over, bool &info_details,
	      compression &algo, bool &detruire, bool &pause, bool &beep, bool &make_empty_dir, 
	      bool & only_more_recent, bool & ea_root, bool & ea_user,
	      string & input_pipe, string &output_pipe,
	      bool & ignore_owner)
{
    fs_root = NULL;
    sauv_root = NULL;
    ref_root = NULL;
    selection = NULL;
    subtree = NULL;
    ref_filename = NULL;
    file_size = 0;
    first_file_size = 0;
    filename = "";
    allow_over = true;
    warn_over = true;
    info_details = false;
    algo = none;
    detruire = true;
    pause = false;
    beep = false;
    make_empty_dir = false;
    only_more_recent = false;
    input_pipe = "";
    output_pipe = "";
    ignore_owner = false;
#ifdef EA_SUPPORT
    ea_user = true;
    ea_root = true;
#else
    ea_user = false;
    ea_root = false;
#endif
    int lu;
    ou_mask inclus, exclus;
    ou_mask path_inclus, path_exclus;
    vector<string> l_path_exclus;
    et_mask *tmp_mask;

    try
    {
	try
	{
	    opterr = 0;
	    
	    while((lu = getopt_long(argc, argv, "c:A:x:d:t:l:vzynwpkR:s:S:X:I:P:bhLWDruUVC:i:o:O", get_long_opt(), NULL)) != EOF)
	    {
		switch(lu)
		{
		case 'c':
		case 'x':
		case 'd':
		case 't':
		case 'l':
		case 'C':
		    if(optarg == NULL)
			throw Erange("get_args", string(" missing argument to -")+char(lu));
		    if(filename != "" || sauv_root != NULL)
			throw Erange("get_args", " only one option of -c -d -t -l -C or -x is allowed");
		    if(optarg != "")
			tools_split_path_basename(optarg, sauv_root, filename);
		    else
			throw Erange("get_args", string(" invalid argument for option -") + char(lu));
		    switch(lu)
		    {
		    case 'c':
			op = create;
			break;
		    case 'x':
			op = extract;
			break;
		    case 'd':
			op = diff;
			break;
		    case 't':
			op = test;
			break;
		    case 'l':
			op = listing;
			break;
		    case 'C':
			op = isolate;
			break;
		    default:
			throw SRC_BUG;
		    }
		    break;
		case 'A':
		    if(ref_filename != NULL || ref_root != NULL)
			throw Erange("get_args", "only one -A option is allowed");
		    if(optarg == NULL)
			throw Erange("get_args", "missing argument to -A");
		    if(optarg == "")
			throw Erange("get_args", "invalid argument for option -A");
		    if(optarg == "-")
			throw Erange("get_args", "- not allowed with -A option");
		    ref_filename = new string();
		    if(ref_filename == NULL)
			throw Ememory("get_args");
		    try
		    {
			tools_split_path_basename(optarg, ref_root, *ref_filename);
		    }
		    catch(...)
		    {
			delete ref_filename;
			throw;
		    }
		    break;
		case 'v':
		    info_details = true;
		    break;
		case 'z':
		    if(algo == none)
			algo = gzip;
		    else
			throw Erange("get_args", "choose either -z or -y not both");
		    break;
		case 'y':
		    if(algo == none)
			algo = bzip2;
		    else
			throw Erange("get_args", "choose either -z or -y not both");
		    break;
		case 'n':
		    allow_over = false;
		    if(!warn_over)
		    {
			user_interaction_warning("-w option is useless with -n");
			warn_over = false;
		    }
		    break;
		case 'w':
		    warn_over = false;
		    break;
		case 'p':
		    pause = true;
		    break;
		case 'k':
		    detruire = false;
		    break;
		case 'R':
		    if(fs_root != NULL)
			throw Erange("get_args", "only one -R option is allowed");
		    if(optarg == NULL)
			throw Erange("get_args", "missing argument to -R");
		    else
			fs_root = new path(optarg);
		    if(fs_root == NULL)
			throw Ememory("get_args");
		    break;
		case 's':
		    if(file_size != 0)
			throw Erange("get_args", "only one -s option is allowed");
		    if(optarg == NULL)
			throw Erange("get_args", "missing argument to -s");
		    else
		    {
			try
			{
			    file_size = tools_get_extended_size(optarg);
			    if(first_file_size == 0)
				first_file_size = file_size;
			}
			catch(Edeci &e)
			{
			    user_interaction_warning("invalid size for option -s");
			    return false;
			}
		    }
		    break;
		case 'S':
		    if(optarg == NULL)
			throw Erange("get_args", "missing argument to -S");
		    if(first_file_size == 0)
			first_file_size = tools_get_extended_size(optarg);
		    else
			if(file_size == 0)
			    throw Erange("get_args", "only one -S option is allowed");
			else
			    if(file_size == first_file_size)
			    {
				try
				{
				    first_file_size = tools_get_extended_size(optarg);
				    if(first_file_size == file_size)
					user_interaction_warning("specifying -S with the same value as the one given in -s is useless");
				}
				catch(Egeneric &e)
				{
				    user_interaction_warning("invalid size for option -S");
				    return false;
				}

			    }
			    else
				throw Erange("get_args", "only one -S option is allowed");
		    break;
		case 'X':
		    if(optarg == NULL)
			throw Erange("get_args", "missiong argument to -X");
		    exclus.add_mask(simple_mask(string(optarg)));
		    break;
		case 'I':
		    if(optarg == NULL)
			throw Erange("get_args", "missing argument to -X");
		    inclus.add_mask(simple_mask(string(optarg)));
		    break;
		case 'P':
		    if(optarg == NULL)
			throw Erange("get_args", "missing argument to -P");
		    l_path_exclus.push_back(optarg);
		    break;
		case 'b':
		    beep = true;
		    break;
		case 'h':
		    usage(argv[0]);
		    return false;
		case 'L':
		    show_license();
		    return false;
		case 'W':
		    show_warranty();
		    return false;
		case 'D':
		    if(make_empty_dir)
			user_interaction_warning("syntax error: -D option has not to be present only once, ignoring additional -D");
		    else
			make_empty_dir = true;
		    break;
		case 'r':
		    if(!allow_over)
			user_interaction_warning("NOTE : -r is useless with -n\n");
		    if(only_more_recent)
			user_interaction_warning("syntax error: -r is only necessary once, ignoring other occurences\n");
		    else
			only_more_recent = true;
		    break;
		case 'u':
#ifdef EA_SUPPORT
		    if(!ea_user)
			user_interaction_warning("syntax error: -u is only necessary once, ignoring other occurences\n");
		    else
			ea_user = false;
#else
		    user_interaction_warning("WARNING! Extended Attributs Support has not been activated at compilation time, thus -u option does nothing");
#endif 
		    break;
		case 'U':
#ifdef EA_SUPPORT
		    if(!ea_root)
			user_interaction_warning("syntax error : -U is only necessary once, ignoring other occurences\n");
		    else
			ea_root = false;
#else
		    user_interaction_warning("WARNING! Extended Attributs Support has not been activated at compilation time, thus -U option does nothing.");
#endif
		    break;
		case 'V':
		    show_version(argv[0]);
		    return false;
		case ':':
		    throw Erange("get_args", string("missing parameter to option ") + char(optopt));
		case 'i':
		    if(optarg == NULL)
			throw Erange("get_args", "missing argument to -i");
		    if(input_pipe == "")
			input_pipe = optarg;
		    else
			user_interaction_warning("only one -i option is allowed, considering the first one");
		    break;
		case 'o':
		    if(optarg == NULL)
			throw Erange("get_args", "missing argument to -o");
		    if(output_pipe == "")
			output_pipe = optarg;
		    else
			user_interaction_warning("only one -o option is allowed, considering the first one");
		    break;
		case 'O':
		    if(ignore_owner)
			user_interaction_warning("only one -O option is necessary, ignoring other instances");
		    else
			ignore_owner = true;
		    break;
		case '?':
		    user_interaction_warning(string("ignoring unknown option ") + char(optopt)); 
		    break;
		default:
		    user_interaction_warning(string("ignoring unknown option ") + char(lu)); 
		}
	    }

		// some sanity checks

	    if(filename == "" || sauv_root == NULL)
		throw Erange("get_args", string("missing -c -x -d -t -l -C option, see `") + path(argv[0]).basename() + " -h' for help");
	    if(filename == "-" && file_size != 0)
		throw Erange("get_args", "slicing (-s option), is not compatible with archive on standart output (\"-\" as filename)");
	    if(fs_root == NULL)
		fs_root = new path(".");
	    if(ref_filename != NULL && op != create && op != isolate)
		user_interaction_warning("-A option is only useful with -c option\n");
	    if(op == isolate && ref_filename == NULL)
		throw Erange("get_args", "with -C option, -A option is mandatory");
	    if(filename == "-" && ref_filename != NULL && *ref_filename == "-"
	       && output_pipe == "")
		throw Erange("get_args", "-o is mandatory when using \"-A -\" with \"-c -\" or \"-C -\"");
	    if(algo != none && op != create && op != isolate)
		user_interaction_warning("-z or -y need only to be used with -c\n");
	    if(first_file_size != 0 && file_size == 0)
		throw Erange("get_args", "-S option requires the use of -s\n");
	    if(ignore_owner && (op == isolate || (op == create && ref_root == NULL) || op == test || op == listing))
		user_interaction_warning("ignoring -O option, as it is useless in this situation");
	    if(getuid() != 0 && op == extract && !ignore_owner) // uid == 0 for root
	    {
		char *name = tools_extract_basename(argv[0]);

		try
		{
		    ignore_owner = true;
		    string msg = string("file ownership will not be restored as ") + name + " is not run as root. to avoid this message use -O option";
		    user_interaction_pause(msg);
		}
		catch(...)
		{
		    delete name;
		    throw;
		}
		delete name;
	    }

		// generating masks
		// for filenames

	    tmp_mask = new et_mask();
	    selection = tmp_mask;
	    if(tmp_mask == NULL)
		throw Ememory("get_args");
	    if(inclus.size() > 0)
		tmp_mask->add_mask(inclus);
	    else
		tmp_mask->add_mask(bool_mask(true));
	    if(exclus.size() > 0)
		tmp_mask->add_mask(not_mask(exclus));

		// generating masks for
		// directory tree

		// reading arguments remaining on the command line

	    if(optind < argc)
		for(int i = optind; i < argc; i++)
		    path_inclus.add_mask(simple_path_mask((*fs_root + path(argv[i])).display()));
	    else
		path_inclus.add_mask(bool_mask(true));

		// making exclude mask from file

	    while(l_path_exclus.size() > 0)
	    {
		path_exclus.add_mask(same_path_mask((*fs_root + path(l_path_exclus.back())).display()));
		l_path_exclus.pop_back();
	    }

	    tmp_mask = new et_mask();
	    subtree = tmp_mask;
	    if(tmp_mask == NULL)
		throw Ememory("get_args");
	    if(path_inclus.size() > 0)
		tmp_mask->add_mask(path_inclus);
	    else
		tmp_mask->add_mask(bool_mask(true));
	    if(path_exclus.size() > 0)
		tmp_mask->add_mask(not_mask(path_exclus));
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
	    if(ref_filename != NULL)
		delete ref_filename;
	    throw;
	}
    }
    catch(Erange & e)
    {
	user_interaction_warning(string("parse error on command line : ") + e.get_message());
	return false;
    }
    return true;
}

static void usage(const char *command_name)
{
    char *name = tools_extract_basename(command_name);

    try
    {
	ui_printf("usage :\t %s [-c|-x|-d|-t|-l|-C] [<path>/]<basename> [options...] [list of paths]\n", name);
	ui_printf("       \t %s -h\n", name);
	ui_printf("       \t %s -V\n", name);
	ui_printf("\ncommands are:\n");
	ui_printf("\t-c creates an archive\n");
	ui_printf("\t-x extracts files from the archive\n");
	ui_printf("\t-d compares archive with existing filesystem\n");
	ui_printf("\t-t tests the archive integrity\n");
	ui_printf("\t-l lists the contents of the archive\n");
	ui_printf("\t-C isolates the catalogue from an archive\n");
	ui_printf("\n");
	ui_printf("\t-h displays this help information\n");
	ui_printf("\t-V displays version information\n");
	ui_printf("\ncommon options:\n");
	ui_printf("\t-v \t\t verbose output\n");
	ui_printf("\t-R <path> \t filesystem root directory for saving or restoring (current dir by default)\n");
	ui_printf("\t-X <mask> \t files to exclude from operation (none by default)\n");
	ui_printf("\t-I <mask> \t files to include in the operation (all by default)\n");
	ui_printf("\t\t\t -X and -I do not concern directories [see -P and list of paths for that]\n");
	ui_printf("\t \t\t -X, -I and -P may be present several time on the command line\n");
	ui_printf("\t-P <path> \t sub tree of filesystem root [-R] to exclude from operation\n");
	ui_printf("\t-n \t\t don't overwrite files\n");
	ui_printf("\t-w \t\t don't warn before overwriting files\n");
	ui_printf("\t \t\t by default overwriting is allowed but a warning is issued before\n");
	ui_printf("\t-b \t\t make terminal ringing when user action is required\n");
	ui_printf("\t-O \t\t do not consider user and group ownership\n");
#ifdef EA_SUPPORT
	ui_printf("\t-u \t\t do not save/restore Extended Attributes of the user namespace\n");
	ui_printf("\t-U \t\t do not save/restore Extended Attributes of the root namespace (need root priviledges\n");
#endif
	ui_printf("\t [list of paths] sub-trees of the filesystem root [-R] to consider (all by default) [see also -P]\n");
	ui_printf("\nsaving or isolation options (to use with -c or -C) :\n");
	ui_printf("\t-A [<path>/]<basename> archive to take as reference (mandatory with -C)\n");
	ui_printf("\t-z \t\t compress data in archive using gzip algorithm\n");
	ui_printf("\t-y \t\t compress data in archive using bzip2 algorithm (NOT IMPLEMENTED)\n");
	ui_printf("\t \t\t by default the archive is not compressed\n");
	ui_printf("\t-s <integer> \t split the archive in several files of size <integer>\n");
	ui_printf("\t-S <integer> \t first file size (if different from following ones) [requires -s]\n");
	ui_printf("\t-p \t\t pause before writing to a new file [requires -s]\n");
	ui_printf("\t-D \t\t store as empty directory those that have been excluded by filters (using -P option)\n");
	ui_printf("\nrestoring options (to use with -x) :\n");
	ui_printf("\t-k \t\t do not remove files stored as destroyed since the reference backup [see also -A]\n");
	ui_printf("\t \t\t -k is useless with -n\n");
	ui_printf("\t-r \t\t restore files only if they are absent or more recent than thoses on filesystem\n");
	ui_printf("\nreading options (to use with -x, -d, -t, -l, -C, -c)\n");
	ui_printf("\t-i <filename>\t use the given filename to read data from dar_slave instead of standart input\n");
	ui_printf("\t-o <filename>\t use the given filename to write orders to dar_slave instead of standart output\n");
	ui_printf("\t\t\t theses two options are only available if the basename of the archive to read is \"-\", which means\n");
	ui_printf("\t\t\t reading the archive from a pipe. For -C and -c options the archive to read is the one\n");
	ui_printf("\t\t\t given with -A option: the archive of reference\n");
	ui_printf("\n");
	ui_printf("see man page for more details, not all common option are available for each option\n\n"); 
    }
    catch(...)
    {
	delete name;
	throw;
    }
    delete name;
}

static void dummy_call(char *x)
{
    static char id[]="$Id: command_line.cpp,v 1.52 2002/06/27 17:42:35 denis Rel $";
    dummy_call(id);
}

static void show_warranty()
{
    ui_printf("			    NO WARRANTY\n");
    ui_printf("\n");
    ui_printf("  11. BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY\n");
    ui_printf("FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.  EXCEPT WHEN\n");
    ui_printf("OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES\n");
    ui_printf("PROVIDE THE PROGRAM \"AS IS\" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED\n");
    ui_printf("OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF\n");
    ui_printf("MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK AS\n");
    ui_printf("TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU.  SHOULD THE\n");
    ui_printf("PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING,\n");
    ui_printf("REPAIR OR CORRECTION.\n");
    ui_printf("\n");
    ui_printf("  12. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING\n");
    ui_printf("WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY AND/OR\n");
    ui_printf("REDISTRIBUTE THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES,\n");
    ui_printf("INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING\n");
    ui_printf("OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED\n");
    ui_printf("TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY\n");
    ui_printf("YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER\n");
    ui_printf("PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE\n");
    ui_printf("POSSIBILITY OF SUCH DAMAGES.\n");
    ui_printf("\n");
}

static void show_license()
{
    ui_printf("		    GNU GENERAL PUBLIC LICENSE\n");
    ui_printf("		       Version 2, June 1991\n");
    ui_printf("\n");
    ui_printf(" Copyright (C) 1989, 1991 Free Software Foundation, Inc.\n");
    ui_printf("                       59 Temple Place, Suite 330, Boston, MA  02111-1307  USA\n");
    ui_printf(" Everyone is permitted to copy and distribute verbatim copies\n");
    ui_printf(" of this license document, but changing it is not allowed.\n");
    ui_printf("\n");
    ui_printf("			    Preamble\n");
    ui_printf("\n");
    ui_printf("  The licenses for most software are designed to take away your\n");
    ui_printf("freedom to share and change it.  By contrast, the GNU General Public\n");
    ui_printf("License is intended to guarantee your freedom to share and change free\n");
    ui_printf("software--to make sure the software is free for all its users.  This\n");
    ui_printf("General Public License applies to most of the Free Software\n");
    ui_printf("Foundation's software and to any other program whose authors commit to\n");
    ui_printf("using it.  (Some other Free Software Foundation software is covered by\n");
    ui_printf("the GNU Library General Public License instead.)  You can apply it to\n");
    ui_printf("your programs, too.\n");
    ui_printf("\n");
    ui_printf("  When we speak of free software, we are referring to freedom, not\n");
    ui_printf("price.  Our General Public Licenses are designed to make sure that you\n");
    ui_printf("have the freedom to distribute copies of free software (and charge for\n");
    ui_printf("this service if you wish), that you receive source code or can get it\n");
    ui_printf("if you want it, that you can change the software or use pieces of it\n");
    ui_printf("in new free programs; and that you know you can do these things.\n");
    ui_printf("\n");
    ui_printf("  To protect your rights, we need to make restrictions that forbid\n");
    ui_printf("anyone to deny you these rights or to ask you to surrender the rights.\n");
    ui_printf("These restrictions translate to certain responsibilities for you if you\n");
    ui_printf("distribute copies of the software, or if you modify it.\n");
    ui_printf("\n");
    ui_printf("  For example, if you distribute copies of such a program, whether\n");
    ui_printf("gratis or for a fee, you must give the recipients all the rights that\n");
    ui_printf("you have.  You must make sure that they, too, receive or can get the\n");
    ui_printf("source code.  And you must show them these terms so they know their\n");
    ui_printf("rights.\n");
    ui_printf("\n");
    ui_printf("  We protect your rights with two steps: (1) copyright the software, and\n");
    ui_printf("(2) offer you this license which gives you legal permission to copy,\n");
    ui_printf("distribute and/or modify the software.\n");
    ui_printf("\n");
    ui_printf("  Also, for each author's protection and ours, we want to make certain\n");
    ui_printf("that everyone understands that there is no warranty for this free\n");
    ui_printf("software.  If the software is modified by someone else and passed on, we\n");
    ui_printf("want its recipients to know that what they have is not the original, so\n");
    ui_printf("that any problems introduced by others will not reflect on the original\n");
    ui_printf("authors' reputations.\n");
    ui_printf("\n");
    ui_printf("  Finally, any free program is threatened constantly by software\n");
    ui_printf("patents.  We wish to avoid the danger that redistributors of a free\n");
    ui_printf("program will individually obtain patent licenses, in effect making the\n");
    ui_printf("program proprietary.  To prevent this, we have made it clear that any\n");
    ui_printf("patent must be licensed for everyone's free use or not licensed at all.\n");
    ui_printf("\n");
    ui_printf("  The precise terms and conditions for copying, distribution and\n");
    ui_printf("modification follow.\n");
    ui_printf("\n");
    ui_printf("		    GNU GENERAL PUBLIC LICENSE\n");
    ui_printf("   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION\n");
    ui_printf("\n");
    ui_printf("  0. This License applies to any program or other work which contains\n");
    ui_printf("a notice placed by the copyright holder saying it may be distributed\n");
    ui_printf("under the terms of this General Public License.  The \"Program\", below,\n");
    ui_printf("refers to any such program or work, and a \"work based on the Program\"\n");
    ui_printf("means either the Program or any derivative work under copyright law:\n");
    ui_printf("that is to say, a work containing the Program or a portion of it,\n");
    ui_printf("either verbatim or with modifications and/or translated into another\n");
    ui_printf("language.  (Hereinafter, translation is included without limitation in\n");
    ui_printf("the term \"modification\".)  Each licensee is addressed as \"you\".\n");
    ui_printf("\n");
    ui_printf("Activities other than copying, distribution and modification are not\n");
    ui_printf("covered by this License; they are outside its scope.  The act of\n");
    ui_printf("running the Program is not restricted, and the output from the Program\n");
    ui_printf("is covered only if its contents constitute a work based on the\n");
    ui_printf("Program (independent of having been made by running the Program).\n");
    ui_printf("Whether that is true depends on what the Program does.\n");
    ui_printf("\n");
    ui_printf("  1. You may copy and distribute verbatim copies of the Program's\n");
    ui_printf("source code as you receive it, in any medium, provided that you\n");
    ui_printf("conspicuously and appropriately publish on each copy an appropriate\n");
    ui_printf("copyright notice and disclaimer of warranty; keep intact all the\n");
    ui_printf("notices that refer to this License and to the absence of any warranty;\n");
    ui_printf("and give any other recipients of the Program a copy of this License\n");
    ui_printf("along with the Program.\n");
    ui_printf("\n");
    ui_printf("You may charge a fee for the physical act of transferring a copy, and\n");
    ui_printf("you may at your option offer warranty protection in exchange for a fee.\n");
    ui_printf("\n");
    ui_printf("  2. You may modify your copy or copies of the Program or any portion\n");
    ui_printf("of it, thus forming a work based on the Program, and copy and\n");
    ui_printf("distribute such modifications or work under the terms of Section 1\n");
    ui_printf("above, provided that you also meet all of these conditions:\n");
    ui_printf("\n");
    ui_printf("    a) You must cause the modified files to carry prominent notices\n");
    ui_printf("    stating that you changed the files and the date of any change.\n");
    ui_printf("\n");
    ui_printf("    b) You must cause any work that you distribute or publish, that in\n");
    ui_printf("    whole or in part contains or is derived from the Program or any\n");
    ui_printf("    part thereof, to be licensed as a whole at no charge to all third\n");
    ui_printf("    parties under the terms of this License.\n");
    ui_printf("\n");
    ui_printf("    c) If the modified program normally reads commands interactively\n");
    ui_printf("    when run, you must cause it, when started running for such\n");
    ui_printf("    interactive use in the most ordinary way, to print or display an\n");
    ui_printf("    announcement including an appropriate copyright notice and a\n");
    ui_printf("    notice that there is no warranty (or else, saying that you provide\n");
    ui_printf("    a warranty) and that users may redistribute the program under\n");
    ui_printf("    these conditions, and telling the user how to view a copy of this\n");
    ui_printf("    License.  (Exception: if the Program itself is interactive but\n");
    ui_printf("    does not normally print such an announcement, your work based on\n");
    ui_printf("    the Program is not required to print an announcement.)\n");
    ui_printf("\n");
    ui_printf("These requirements apply to the modified work as a whole.  If\n");
    ui_printf("identifiable sections of that work are not derived from the Program,\n");
    ui_printf("and can be reasonably considered independent and separate works in\n");
    ui_printf("themselves, then this License, and its terms, do not apply to those\n");
    ui_printf("sections when you distribute them as separate works.  But when you\n");
    ui_printf("distribute the same sections as part of a whole which is a work based\n");
    ui_printf("on the Program, the distribution of the whole must be on the terms of\n");
    ui_printf("this License, whose permissions for other licensees extend to the\n");
    ui_printf("entire whole, and thus to each and every part regardless of who wrote it.\n");
    ui_printf("\n");
    ui_printf("Thus, it is not the intent of this section to claim rights or contest\n");
    ui_printf("your rights to work written entirely by you; rather, the intent is to\n");
    ui_printf("exercise the right to control the distribution of derivative or\n");
    ui_printf("collective works based on the Program.\n");
    ui_printf("\n");
    ui_printf("In addition, mere aggregation of another work not based on the Program\n");
    ui_printf("with the Program (or with a work based on the Program) on a volume of\n");
    ui_printf("a storage or distribution medium does not bring the other work under\n");
    ui_printf("the scope of this License.\n");
    ui_printf("\n");
    ui_printf("  3. You may copy and distribute the Program (or a work based on it,\n");
    ui_printf("under Section 2) in object code or executable form under the terms of\n");
    ui_printf("Sections 1 and 2 above provided that you also do one of the following:\n");
    ui_printf("\n");
    ui_printf("    a) Accompany it with the complete corresponding machine-readable\n");
    ui_printf("    source code, which must be distributed under the terms of Sections\n");
    ui_printf("    1 and 2 above on a medium customarily used for software interchange; or,\n");
    ui_printf("\n");
    ui_printf("    b) Accompany it with a written offer, valid for at least three\n");
    ui_printf("    years, to give any third party, for a charge no more than your\n");
    ui_printf("    cost of physically performing source distribution, a complete\n");
    ui_printf("    machine-readable copy of the corresponding source code, to be\n");
    ui_printf("    distributed under the terms of Sections 1 and 2 above on a medium\n");
    ui_printf("    customarily used for software interchange; or,\n");
    ui_printf("\n");
    ui_printf("    c) Accompany it with the information you received as to the offer\n");
    ui_printf("    to distribute corresponding source code.  (This alternative is\n");
    ui_printf("    allowed only for noncommercial distribution and only if you\n");
    ui_printf("    received the program in object code or executable form with such\n");
    ui_printf("    an offer, in accord with Subsection b above.)\n");
    ui_printf("\n");
    ui_printf("The source code for a work means the preferred form of the work for\n");
    ui_printf("making modifications to it.  For an executable work, complete source\n");
    ui_printf("code means all the source code for all modules it contains, plus any\n");
    ui_printf("associated interface definition files, plus the scripts used to\n");
    ui_printf("control compilation and installation of the executable.  However, as a\n");
    ui_printf("special exception, the source code distributed need not include\n");
    ui_printf("anything that is normally distributed (in either source or binary\n");
    ui_printf("form) with the major components (compiler, kernel, and so on) of the\n");
    ui_printf("operating system on which the executable runs, unless that component\n");
    ui_printf("itself accompanies the executable.\n");
    ui_printf("\n");
    ui_printf("If distribution of executable or object code is made by offering\n");
    ui_printf("access to copy from a designated place, then offering equivalent\n");
    ui_printf("access to copy the source code from the same place counts as\n");
    ui_printf("distribution of the source code, even though third parties are not\n");
    ui_printf("compelled to copy the source along with the object code.\n");
    ui_printf("\n");
    ui_printf("  4. You may not copy, modify, sublicense, or distribute the Program\n");
    ui_printf("except as expressly provided under this License.  Any attempt\n");
    ui_printf("otherwise to copy, modify, sublicense or distribute the Program is\n");
    ui_printf("void, and will automatically terminate your rights under this License.\n");
    ui_printf("However, parties who have received copies, or rights, from you under\n");
    ui_printf("this License will not have their licenses terminated so long as such\n");
    ui_printf("parties remain in full compliance.\n");
    ui_printf("\n");
    ui_printf("  5. You are not required to accept this License, since you have not\n");
    ui_printf("signed it.  However, nothing else grants you permission to modify or\n");
    ui_printf("distribute the Program or its derivative works.  These actions are\n");
    ui_printf("prohibited by law if you do not accept this License.  Therefore, by\n");
    ui_printf("modifying or distributing the Program (or any work based on the\n");
    ui_printf("Program), you indicate your acceptance of this License to do so, and\n");
    ui_printf("all its terms and conditions for copying, distributing or modifying\n");
    ui_printf("the Program or works based on it.\n");
    ui_printf("\n");
    ui_printf("  6. Each time you redistribute the Program (or any work based on the\n");
    ui_printf("Program), the recipient automatically receives a license from the\n");
    ui_printf("original licensor to copy, distribute or modify the Program subject to\n");
    ui_printf("these terms and conditions.  You may not impose any further\n");
    ui_printf("restrictions on the recipients' exercise of the rights granted herein.\n");
    ui_printf("You are not responsible for enforcing compliance by third parties to\n");
    ui_printf("this License.\n");
    ui_printf("\n");
    ui_printf("  7. If, as a consequence of a court judgment or allegation of patent\n");
    ui_printf("infringement or for any other reason (not limited to patent issues),\n");
    ui_printf("conditions are imposed on you (whether by court order, agreement or\n");
    ui_printf("otherwise) that contradict the conditions of this License, they do not\n");
    ui_printf("excuse you from the conditions of this License.  If you cannot\n");
    ui_printf("distribute so as to satisfy simultaneously your obligations under this\n");
    ui_printf("License and any other pertinent obligations, then as a consequence you\n");
    ui_printf("may not distribute the Program at all.  For example, if a patent\n");
    ui_printf("license would not permit royalty-free redistribution of the Program by\n");
    ui_printf("all those who receive copies directly or indirectly through you, then\n");
    ui_printf("the only way you could satisfy both it and this License would be to\n");
    ui_printf("refrain entirely from distribution of the Program.\n");
    ui_printf("\n");
    ui_printf("If any portion of this section is held invalid or unenforceable under\n");
    ui_printf("any particular circumstance, the balance of the section is intended to\n");
    ui_printf("apply and the section as a whole is intended to apply in other\n");
    ui_printf("circumstances.\n");
    ui_printf("\n");
    ui_printf("It is not the purpose of this section to induce you to infringe any\n");
    ui_printf("patents or other property right claims or to contest validity of any\n");
    ui_printf("such claims; this section has the sole purpose of protecting the\n");
    ui_printf("integrity of the free software distribution system, which is\n");
    ui_printf("implemented by public license practices.  Many people have made\n");
    ui_printf("generous contributions to the wide range of software distributed\n");
    ui_printf("through that system in reliance on consistent application of that\n");
    ui_printf("system; it is up to the author/donor to decide if he or she is willing\n");
    ui_printf("to distribute software through any other system and a licensee cannot\n");
    ui_printf("impose that choice.\n");
    ui_printf("\n");
    ui_printf("This section is intended to make thoroughly clear what is believed to\n");
    ui_printf("be a consequence of the rest of this License.\n");
    ui_printf("\n");
    ui_printf("  8. If the distribution and/or use of the Program is restricted in\n");
    ui_printf("certain countries either by patents or by copyrighted interfaces, the\n");
    ui_printf("original copyright holder who places the Program under this License\n");
    ui_printf("may add an explicit geographical distribution limitation excluding\n");
    ui_printf("those countries, so that distribution is permitted only in or among\n");
    ui_printf("countries not thus excluded.  In such case, this License incorporates\n");
    ui_printf("the limitation as if written in the body of this License.\n");
    ui_printf("\n");
    ui_printf("  9. The Free Software Foundation may publish revised and/or new versions\n");
    ui_printf("of the General Public License from time to time.  Such new versions will\n");
    ui_printf("be similar in spirit to the present version, but may differ in detail to\n");
    ui_printf("address new problems or concerns.\n");
    ui_printf("\n");
    ui_printf("Each version is given a distinguishing version number.  If the Program\n");
    ui_printf("specifies a version number of this License which applies to it and \"any\n");
    ui_printf("later version\", you have the option of following the terms and conditions\n");
    ui_printf("either of that version or of any later version published by the Free\n");
    ui_printf("Software Foundation.  If the Program does not specify a version number of\n");
    ui_printf("this License, you may choose any version ever published by the Free Software\n");
    ui_printf("Foundation.\n");
    ui_printf("\n");
    ui_printf("  10. If you wish to incorporate parts of the Program into other free\n");
    ui_printf("programs whose distribution conditions are different, write to the author\n");
    ui_printf("to ask for permission.  For software which is copyrighted by the Free\n");
    ui_printf("Software Foundation, write to the Free Software Foundation; we sometimes\n");
    ui_printf("make exceptions for this.  Our decision will be guided by the two goals\n");
    ui_printf("of preserving the free status of all derivatives of our free software and\n");
    ui_printf("of promoting the sharing and reuse of software generally.\n");
    ui_printf("\n");
    show_warranty();
    ui_printf("		     END OF TERMS AND CONDITIONS\n");
    ui_printf("\n");
    ui_printf("	    How to Apply These Terms to Your New Programs\n");
    ui_printf("\n");
    ui_printf("  If you develop a new program, and you want it to be of the greatest\n");
    ui_printf("possible use to the public, the best way to achieve this is to make it\n");
    ui_printf("free software which everyone can redistribute and change under these terms.\n");
    ui_printf("\n");
    ui_printf("  To do so, attach the following notices to the program.  It is safest\n");
    ui_printf("to attach them to the start of each source file to most effectively\n");
    ui_printf("convey the exclusion of warranty; and each file should have at least\n");
    ui_printf("the \"copyright\" line and a pointer to where the full notice is found.\n");
    ui_printf("\n");
    ui_printf("    <one line to give the program's name and a brief idea of what it does.>\n");
    ui_printf("    Copyright (C) <year>  <name of author>\n");
    ui_printf("\n");
    ui_printf("    This program is free software; you can redistribute it and/or modify\n");
    ui_printf("    it under the terms of the GNU General Public License as published by\n");
    ui_printf("    the Free Software Foundation; either version 2 of the License, or\n");
    ui_printf("    (at your option) any later version.\n");
    ui_printf("\n");
    ui_printf("    This program is distributed in the hope that it will be useful,\n");
    ui_printf("    but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
    ui_printf("    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
    ui_printf("    GNU General Public License for more details.\n");
    ui_printf("\n");
    ui_printf("    You should have received a copy of the GNU General Public License\n");
    ui_printf("    along with this program; if not, write to the Free Software\n");
    ui_printf("    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA\n");
    ui_printf("\n");
    ui_printf("\n");
    ui_printf("Also add information on how to contact you by electronic and paper mail.\n");
    ui_printf("\n");
    ui_printf("If the program is interactive, make it output a short notice like this\n");
    ui_printf("when it starts in an interactive mode:\n");
    ui_printf("\n");
    ui_printf("    Gnomovision version 69, Copyright (C) year name of author\n");
    ui_printf("    Gnomovision comes with ABSOLUTELY NO WARRANTY; for details type `show w'.\n");
    ui_printf("    This is free software, and you are welcome to redistribute it\n");
    ui_printf("    under certain conditions; type `show c' for details.\n");
    ui_printf("\n");
    ui_printf("The hypothetical commands `show w' and `show c' should show the appropriate\n");
    ui_printf("parts of the General Public License.  Of course, the commands you use may\n");
    ui_printf("be called something other than `show w' and `show c'; they could even be\n");
    ui_printf("mouse-clicks or menu items--whatever suits your program.\n");
    ui_printf("\n");
    ui_printf("You should also get your employer (if you work as a programmer) or your\n");
    ui_printf("school, if any, to sign a \"copyright disclaimer\" for the program, if\n");
    ui_printf("necessary.  Here is a sample; alter the names:\n");
    ui_printf("\n");
    ui_printf("  Yoyodyne, Inc., hereby disclaims all copyright interest in the program\n");
    ui_printf("  `Gnomovision' (which makes passes at compilers) written by James Hacker.\n");
    ui_printf("\n");
    ui_printf("  <signature of Ty Coon>, 1 April 1989\n");
    ui_printf("  Ty Coon, President of Vice\n");
    ui_printf("\n");
    ui_printf("This General Public License does not permit incorporating your program into\n");
    ui_printf("proprietary programs.  If your program is a subroutine library, you may\n");
    ui_printf("consider it more useful to permit linking proprietary applications with the\n");
    ui_printf("library.  If this is what you want to do, use the GNU Library General\n");
    ui_printf("Public License instead of this License.\n");
    ui_printf("\n");
}

static void show_version(const char *command_name)
{
    char *name = tools_extract_basename(command_name);
    try
    {
	ui_printf("\n %s version %s, Copyright (C) 2002 Denis Corbin\n\n",  name, dar_suite_version());
#ifdef EA_SUPPORT
	ui_printf(" compiled with Extended Attributes support\n");
#else
	ui_printf(" WARNING! EA support has NOT been activated at compilation time\n");
#endif
	ui_printf(" %s with %s version %s\n", __DATE__, CC_NAT, __VERSION__);
	ui_printf(" %s comes with ABSOLUTELY NO WARRANTY; for details\n", name);
	ui_printf(" type `%s -W'.  This is free software, and you are welcome\n", name);
	ui_printf(" to redistribute it under certain conditions; type `%s -L | more'\n", name);
	ui_printf(" for details.\n\n");
    }
    catch(...)
    {
	delete name;
	throw;
    }
    delete name;
}

static const struct option *get_long_opt()
{
    static const struct option ret[] = {
	{"beep", no_argument, NULL, 'b' },
	{"create", required_argument, NULL, 'c'},
	{"diff", required_argument, NULL, 'd'},
	{"help", no_argument, NULL, 'h'},
	{"input", required_argument, NULL, 'i'},
	{"no-deleted", no_argument, NULL, 'k'},
	{"list", required_argument, NULL, 'l'},
	{"no-overwrite", no_argument, NULL, 'n'},
	{"output", required_argument, NULL, 'o'},
	{"pause", no_argument, NULL, 'p'},
	{"recent", no_argument, NULL, 'r'},
	{"slice", required_argument, NULL, 's'},
	{"test", required_argument, NULL, 't'},
	{"no-user-EA", no_argument, NULL, 'u'},
	{"verbose", no_argument, NULL, 'v'},
	{"no-warn", no_argument, NULL, 'w'},
	{"extract", required_argument, NULL, 'x'},
	{"bzip2", no_argument, NULL, 'y'},
	{"gzip", no_argument, NULL, 'z'},
	{"ref", required_argument, NULL, 'A'},
	{"isolate", required_argument, NULL, 'C'},
	{"empty-dir", no_argument, NULL, 'D'},
	{"include", required_argument, NULL, 'I'},
	{"prune", required_argument, NULL, 'P'},
	{"fs-root", required_argument, NULL, 'R'},
	{"first-slice", required_argument, NULL, 'S'},
	{"no-system-EA", no_argument, NULL, 'U'},
	{"version", no_argument, NULL, 'V'},
	{"exclude", required_argument, NULL, 'X'},
	{"ignore-owner", no_argument, NULL, 'O'},
	{ NULL, 0, NULL, 0 }
    };

    return ret;
}
