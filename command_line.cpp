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
// $Id: command_line.cpp,v 1.28 2002/03/25 22:02:38 denis Rel $
//
/*********************************************************************/

#include <unistd.h>
#include <stdio.h>
#include <string>
#include "deci.hpp"
#include "command_line.hpp"
#include "user_interaction.hpp"
#include "tools.hpp"
#include "dar.hpp"

using namespace std;

static infinint get_extended_size(string s);
static void show_license();
static void show_warranty();

bool get_args(int argc, char *argv[], operation &op, path * &fs_root, path * &sauv_root, path * &ref_root,
	      infinint &file_size, infinint &first_file_size, mask *&selection, mask *&subtree,
	      string &filename, string *&ref_filename,
	      bool &allow_over, bool &warn_over, bool &info_details,
	      compression &algo, bool &detruire, bool &pause, bool &beep)
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
	    
	    while((lu = getopt(argc, argv, "c:A:x:d:t:l:vzynwpkR:s:S:X:I:P:bhLW")) != EOF)
	    {
		switch(lu)
		{
		case 'c':
		case 'x':
		case 'd':
		case 't':
		case 'l':
		    if(optarg == NULL)
			throw Erange("get_args", " missing argument to -c");
		    if(filename != "" || sauv_root != NULL)
			throw Erange("get_args", " only one option of -c -d or -x is allowed");
		    sauv_root = new path(optarg);
		    if(sauv_root->degre() > 1)
		    {
			if(!sauv_root->pop(filename))
			    throw SRC_BUG; // sauv_root.degree > 1 but pop is not possible !!!
		    }
		    else
		    {
			delete sauv_root;
			filename = optarg;
			sauv_root = new path(".");
		    }
		    if(filename == "")
			throw Erange("get_args", string(" invalide argument for option -") + char(lu));
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
		    default:
			throw SRC_BUG;
		    }
		    break;
		case 'A':
		    if(ref_filename != NULL || ref_root != NULL)
			throw Erange("get_args", "only one -A option is allowed");
		    if(optarg == NULL)
			throw Erange("get_args", "missing argument to -A");
		    ref_root = new path(optarg);
		    if(ref_root == NULL)
			throw Ememory("get_args");
		    try
		    {
			if(ref_root->degre() > 1)
			{
			    ref_filename = new string();
			    if(ref_filename == NULL)
				throw Ememory("get_args");
			    if(!ref_root->pop(*ref_filename))
				throw SRC_BUG; // degree > 1 but can't pop !!!
			}
			else
			{
			    delete ref_root;
			    ref_root = new path(".");
			    if(ref_root == NULL)
				throw Ememory("get_args");
			    ref_filename = new string(optarg);
			    if(ref_filename == NULL)
				throw Ememory("get_args");
			}
		    }
		    catch(...)
		    {
			if(ref_root != NULL)
			    delete ref_root;
			if(ref_filename != NULL)
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
			    file_size = get_extended_size(optarg);
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
			throw Erange("get_args", "missiong argument to -S");
		    if(first_file_size == 0)
			first_file_size = get_extended_size(optarg);
		    else
			if(file_size == 0)
			    throw Erange("get_args", "only one -S option is allowed");
			else
			    if(file_size == first_file_size)
			    {
				try
				{
				    first_file_size = get_extended_size(optarg);
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
		case ':':
		    throw Erange("get_args", string("missing parameter to option ") + char(optopt));
		case '?':
		    user_interaction_warning(string("ignoring unknown option ") + char(optopt)); 
		    break;
		default:
		    user_interaction_warning(string("ignoring unknown option ") + char(lu)); 
		}
	    }

		//

	    if(filename == "" || sauv_root == NULL)
		throw Erange("get_args", string("missing -c -x -d -t -l option, see `") + path(argv[0]).basename() + " -h' for help");
	    if(fs_root == NULL)
		fs_root = new path(".");
	    if(ref_filename != NULL && op != create)
		user_interaction_warning("-A option is only useful with -c option\n");
	    if(algo != none && op != create)
		user_interaction_warning("-z or -y need only to be used with -c\n");
	    if(first_file_size != 0 && file_size == 0)
		throw Erange("get_args", "-S option requires the use of -s\n");

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

void usage(char *command_name)
{
    path commande = command_name;
    string tmp = commande.basename();
    char *name = tools_str2charptr(tmp);

    try
    {
	printf("usage :\t %s [-c|-x|-d|-t|-l] [<path>/]<basename> [options...] [list of paths]\n", name);
	printf("       \t %s -h\n", name);
	printf("\ncommands are :\n");
	printf("\t-c creates an archive\n");
	printf("\t-x extracts files from the archive\n");
	printf("\t-d compares filesystem with files stored in archive (NOT IMPLEMENTED)\n");
	printf("\t-t tests the archive integrity (NOT IMPLEMENTED)\n");
	printf("\t-l lists the content of the archive\n");
	printf("\t-h displays this help information\n");
	printf("\nall commands options :\n");
	printf("\t-v \t\t verbose output\n");
	printf("\t-R <path> \t filesystem root directory for saving or restoring (current dir by default)\n");
	printf("\t-X <mask> \t files to exclude from operation (none by default)\n");
	printf("\t-I <mask> \t files to include in the operation (all by default)\n");
	printf("\t\t\t -X and -I do not concern directories [see -P and list of paths for that]\n");
	printf("\t \t\t -X, -I and -P may be present several time on the command line\n");
	printf("\t-P <path> \t sub tree of filesystem root [-R] to exclude from operation\n");
	printf("\t-n \t\t don't overwrite files\n");
	printf("\t-w \t\t don't warn before overwritting files\n");
	printf("\t \t\t by default overwritting is allowed but a warning is issued before\n");
	printf("\t-b \t\t make terminal ringing when user action is required\n");
	printf("\t [list of paths] sub-trees of the filesystem root [-R] to consider (all by default) [see also -P]\n");
	printf("\nsaving options (to use with -c) :\n");
	printf("\t-A [<path>/]<basename> \t archive to take as reference and with which to save only the difference\n");
	printf("\t-z \t\t compress data in archive using gzip algorithm\n");
	printf("\t-y \t\t compress data in archive using bzip2 algorithm (NOT IMPLEMENTED)\n");
	printf("\t \t\t by default the archive is not compressed\n");
	printf("\t-s <integer> \t split the archive in several files of size <integer>\n");
	printf("\t-S <integer> \t first file size (if different from following ones) [requires -s]\n");
	printf("\t-p \t\t pause before writing to a new file [requires -s]\n");
	printf("\nrestoring options (to use with -x) :\n");
	printf("\t-k \t\t do not remove files stored as destroyed since the reference backup [see also -A]\n");
	printf("\t \t\t -k is useless with -n\n");
	printf("\n");
	printf("%s version %s, Copyright (C) 2002 Denis Corbin\n",  name, get_application_version());
	printf("%s comes with ABSOLUTELY NO WARRANTY; for details\n", name);
	printf("type `%s -W'.  This is free software, and you are welcome\n", name);
	printf("to redistribute it under certain conditions; type `%s -L | more'\n", name);
	printf("for details.\n\n");
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
    static char id[]="$Id: command_line.cpp,v 1.28 2002/03/25 22:02:38 denis Rel $";
    dummy_call(id);
}

static infinint get_extended_size(string s)
{
    unsigned int len = s.size();
    infinint factor = 1;

    if(len < 1)
	return false;
    switch(s[len-1])
    {
    case 'K':
    case 'k': // kilobyte
	factor = 1024;
	break;
    case 'M': // megabyte
	factor = infinint(1024)*infinint(1024);
	break;
    case 'G': // gigabyte
	factor = infinint(1024)*infinint(1024)*infinint(1024);
	break;
    case 'T': // terabyte
	factor = infinint(1024)*infinint(1024)*infinint(1024)*infinint(1024);
	break;
    case 'P': // petabyte
	factor = infinint(1024)*infinint(1024)*infinint(1024)*infinint(1024)*infinint(1024);
	break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
	break;
    default :
	throw Erange("command_line get_extended_size", string("unknown sufix in string : ")+s);
    }

    if(factor != 1)
	s = string(s.begin(), s.end()-1);

    deci tmp = s;
    factor *= tmp.computer();

    return factor;
}

static void show_warranty()
{
    printf("			    NO WARRANTY\n");
    printf("\n");
    printf("  11. BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY\n");
    printf("FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.  EXCEPT WHEN\n");
    printf("OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES\n");
    printf("PROVIDE THE PROGRAM \"AS IS\" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED\n");
    printf("OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF\n");
    printf("MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK AS\n");
    printf("TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU.  SHOULD THE\n");
    printf("PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING,\n");
    printf("REPAIR OR CORRECTION.\n");
    printf("\n");
    printf("  12. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING\n");
    printf("WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY AND/OR\n");
    printf("REDISTRIBUTE THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES,\n");
    printf("INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING\n");
    printf("OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED\n");
    printf("TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY\n");
    printf("YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER\n");
    printf("PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE\n");
    printf("POSSIBILITY OF SUCH DAMAGES.\n");
    printf("\n");
}

static void show_license()
{
    printf("		    GNU GENERAL PUBLIC LICENSE\n");
    printf("		       Version 2, June 1991\n");
    printf("\n");
    printf(" Copyright (C) 1989, 1991 Free Software Foundation, Inc.\n");
    printf("                       59 Temple Place, Suite 330, Boston, MA  02111-1307  USA\n");
    printf(" Everyone is permitted to copy and distribute verbatim copies\n");
    printf(" of this license document, but changing it is not allowed.\n");
    printf("\n");
    printf("			    Preamble\n");
    printf("\n");
    printf("  The licenses for most software are designed to take away your\n");
    printf("freedom to share and change it.  By contrast, the GNU General Public\n");
    printf("License is intended to guarantee your freedom to share and change free\n");
    printf("software--to make sure the software is free for all its users.  This\n");
    printf("General Public License applies to most of the Free Software\n");
    printf("Foundation's software and to any other program whose authors commit to\n");
    printf("using it.  (Some other Free Software Foundation software is covered by\n");
    printf("the GNU Library General Public License instead.)  You can apply it to\n");
    printf("your programs, too.\n");
    printf("\n");
    printf("  When we speak of free software, we are referring to freedom, not\n");
    printf("price.  Our General Public Licenses are designed to make sure that you\n");
    printf("have the freedom to distribute copies of free software (and charge for\n");
    printf("this service if you wish), that you receive source code or can get it\n");
    printf("if you want it, that you can change the software or use pieces of it\n");
    printf("in new free programs; and that you know you can do these things.\n");
    printf("\n");
    printf("  To protect your rights, we need to make restrictions that forbid\n");
    printf("anyone to deny you these rights or to ask you to surrender the rights.\n");
    printf("These restrictions translate to certain responsibilities for you if you\n");
    printf("distribute copies of the software, or if you modify it.\n");
    printf("\n");
    printf("  For example, if you distribute copies of such a program, whether\n");
    printf("gratis or for a fee, you must give the recipients all the rights that\n");
    printf("you have.  You must make sure that they, too, receive or can get the\n");
    printf("source code.  And you must show them these terms so they know their\n");
    printf("rights.\n");
    printf("\n");
    printf("  We protect your rights with two steps: (1) copyright the software, and\n");
    printf("(2) offer you this license which gives you legal permission to copy,\n");
    printf("distribute and/or modify the software.\n");
    printf("\n");
    printf("  Also, for each author's protection and ours, we want to make certain\n");
    printf("that everyone understands that there is no warranty for this free\n");
    printf("software.  If the software is modified by someone else and passed on, we\n");
    printf("want its recipients to know that what they have is not the original, so\n");
    printf("that any problems introduced by others will not reflect on the original\n");
    printf("authors' reputations.\n");
    printf("\n");
    printf("  Finally, any free program is threatened constantly by software\n");
    printf("patents.  We wish to avoid the danger that redistributors of a free\n");
    printf("program will individually obtain patent licenses, in effect making the\n");
    printf("program proprietary.  To prevent this, we have made it clear that any\n");
    printf("patent must be licensed for everyone's free use or not licensed at all.\n");
    printf("\n");
    printf("  The precise terms and conditions for copying, distribution and\n");
    printf("modification follow.\n");
    printf("\n");
    printf("		    GNU GENERAL PUBLIC LICENSE\n");
    printf("   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION\n");
    printf("\n");
    printf("  0. This License applies to any program or other work which contains\n");
    printf("a notice placed by the copyright holder saying it may be distributed\n");
    printf("under the terms of this General Public License.  The \"Program\", below,\n");
    printf("refers to any such program or work, and a \"work based on the Program\"\n");
    printf("means either the Program or any derivative work under copyright law:\n");
    printf("that is to say, a work containing the Program or a portion of it,\n");
    printf("either verbatim or with modifications and/or translated into another\n");
    printf("language.  (Hereinafter, translation is included without limitation in\n");
    printf("the term \"modification\".)  Each licensee is addressed as \"you\".\n");
    printf("\n");
    printf("Activities other than copying, distribution and modification are not\n");
    printf("covered by this License; they are outside its scope.  The act of\n");
    printf("running the Program is not restricted, and the output from the Program\n");
    printf("is covered only if its contents constitute a work based on the\n");
    printf("Program (independent of having been made by running the Program).\n");
    printf("Whether that is true depends on what the Program does.\n");
    printf("\n");
    printf("  1. You may copy and distribute verbatim copies of the Program's\n");
    printf("source code as you receive it, in any medium, provided that you\n");
    printf("conspicuously and appropriately publish on each copy an appropriate\n");
    printf("copyright notice and disclaimer of warranty; keep intact all the\n");
    printf("notices that refer to this License and to the absence of any warranty;\n");
    printf("and give any other recipients of the Program a copy of this License\n");
    printf("along with the Program.\n");
    printf("\n");
    printf("You may charge a fee for the physical act of transferring a copy, and\n");
    printf("you may at your option offer warranty protection in exchange for a fee.\n");
    printf("\n");
    printf("  2. You may modify your copy or copies of the Program or any portion\n");
    printf("of it, thus forming a work based on the Program, and copy and\n");
    printf("distribute such modifications or work under the terms of Section 1\n");
    printf("above, provided that you also meet all of these conditions:\n");
    printf("\n");
    printf("    a) You must cause the modified files to carry prominent notices\n");
    printf("    stating that you changed the files and the date of any change.\n");
    printf("\n");
    printf("    b) You must cause any work that you distribute or publish, that in\n");
    printf("    whole or in part contains or is derived from the Program or any\n");
    printf("    part thereof, to be licensed as a whole at no charge to all third\n");
    printf("    parties under the terms of this License.\n");
    printf("\n");
    printf("    c) If the modified program normally reads commands interactively\n");
    printf("    when run, you must cause it, when started running for such\n");
    printf("    interactive use in the most ordinary way, to print or display an\n");
    printf("    announcement including an appropriate copyright notice and a\n");
    printf("    notice that there is no warranty (or else, saying that you provide\n");
    printf("    a warranty) and that users may redistribute the program under\n");
    printf("    these conditions, and telling the user how to view a copy of this\n");
    printf("    License.  (Exception: if the Program itself is interactive but\n");
    printf("    does not normally print such an announcement, your work based on\n");
    printf("    the Program is not required to print an announcement.)\n");
    printf("\n");
    printf("These requirements apply to the modified work as a whole.  If\n");
    printf("identifiable sections of that work are not derived from the Program,\n");
    printf("and can be reasonably considered independent and separate works in\n");
    printf("themselves, then this License, and its terms, do not apply to those\n");
    printf("sections when you distribute them as separate works.  But when you\n");
    printf("distribute the same sections as part of a whole which is a work based\n");
    printf("on the Program, the distribution of the whole must be on the terms of\n");
    printf("this License, whose permissions for other licensees extend to the\n");
    printf("entire whole, and thus to each and every part regardless of who wrote it.\n");
    printf("\n");
    printf("Thus, it is not the intent of this section to claim rights or contest\n");
    printf("your rights to work written entirely by you; rather, the intent is to\n");
    printf("exercise the right to control the distribution of derivative or\n");
    printf("collective works based on the Program.\n");
    printf("\n");
    printf("In addition, mere aggregation of another work not based on the Program\n");
    printf("with the Program (or with a work based on the Program) on a volume of\n");
    printf("a storage or distribution medium does not bring the other work under\n");
    printf("the scope of this License.\n");
    printf("\n");
    printf("  3. You may copy and distribute the Program (or a work based on it,\n");
    printf("under Section 2) in object code or executable form under the terms of\n");
    printf("Sections 1 and 2 above provided that you also do one of the following:\n");
    printf("\n");
    printf("    a) Accompany it with the complete corresponding machine-readable\n");
    printf("    source code, which must be distributed under the terms of Sections\n");
    printf("    1 and 2 above on a medium customarily used for software interchange; or,\n");
    printf("\n");
    printf("    b) Accompany it with a written offer, valid for at least three\n");
    printf("    years, to give any third party, for a charge no more than your\n");
    printf("    cost of physically performing source distribution, a complete\n");
    printf("    machine-readable copy of the corresponding source code, to be\n");
    printf("    distributed under the terms of Sections 1 and 2 above on a medium\n");
    printf("    customarily used for software interchange; or,\n");
    printf("\n");
    printf("    c) Accompany it with the information you received as to the offer\n");
    printf("    to distribute corresponding source code.  (This alternative is\n");
    printf("    allowed only for noncommercial distribution and only if you\n");
    printf("    received the program in object code or executable form with such\n");
    printf("    an offer, in accord with Subsection b above.)\n");
    printf("\n");
    printf("The source code for a work means the preferred form of the work for\n");
    printf("making modifications to it.  For an executable work, complete source\n");
    printf("code means all the source code for all modules it contains, plus any\n");
    printf("associated interface definition files, plus the scripts used to\n");
    printf("control compilation and installation of the executable.  However, as a\n");
    printf("special exception, the source code distributed need not include\n");
    printf("anything that is normally distributed (in either source or binary\n");
    printf("form) with the major components (compiler, kernel, and so on) of the\n");
    printf("operating system on which the executable runs, unless that component\n");
    printf("itself accompanies the executable.\n");
    printf("\n");
    printf("If distribution of executable or object code is made by offering\n");
    printf("access to copy from a designated place, then offering equivalent\n");
    printf("access to copy the source code from the same place counts as\n");
    printf("distribution of the source code, even though third parties are not\n");
    printf("compelled to copy the source along with the object code.\n");
    printf("\n");
    printf("  4. You may not copy, modify, sublicense, or distribute the Program\n");
    printf("except as expressly provided under this License.  Any attempt\n");
    printf("otherwise to copy, modify, sublicense or distribute the Program is\n");
    printf("void, and will automatically terminate your rights under this License.\n");
    printf("However, parties who have received copies, or rights, from you under\n");
    printf("this License will not have their licenses terminated so long as such\n");
    printf("parties remain in full compliance.\n");
    printf("\n");
    printf("  5. You are not required to accept this License, since you have not\n");
    printf("signed it.  However, nothing else grants you permission to modify or\n");
    printf("distribute the Program or its derivative works.  These actions are\n");
    printf("prohibited by law if you do not accept this License.  Therefore, by\n");
    printf("modifying or distributing the Program (or any work based on the\n");
    printf("Program), you indicate your acceptance of this License to do so, and\n");
    printf("all its terms and conditions for copying, distributing or modifying\n");
    printf("the Program or works based on it.\n");
    printf("\n");
    printf("  6. Each time you redistribute the Program (or any work based on the\n");
    printf("Program), the recipient automatically receives a license from the\n");
    printf("original licensor to copy, distribute or modify the Program subject to\n");
    printf("these terms and conditions.  You may not impose any further\n");
    printf("restrictions on the recipients' exercise of the rights granted herein.\n");
    printf("You are not responsible for enforcing compliance by third parties to\n");
    printf("this License.\n");
    printf("\n");
    printf("  7. If, as a consequence of a court judgment or allegation of patent\n");
    printf("infringement or for any other reason (not limited to patent issues),\n");
    printf("conditions are imposed on you (whether by court order, agreement or\n");
    printf("otherwise) that contradict the conditions of this License, they do not\n");
    printf("excuse you from the conditions of this License.  If you cannot\n");
    printf("distribute so as to satisfy simultaneously your obligations under this\n");
    printf("License and any other pertinent obligations, then as a consequence you\n");
    printf("may not distribute the Program at all.  For example, if a patent\n");
    printf("license would not permit royalty-free redistribution of the Program by\n");
    printf("all those who receive copies directly or indirectly through you, then\n");
    printf("the only way you could satisfy both it and this License would be to\n");
    printf("refrain entirely from distribution of the Program.\n");
    printf("\n");
    printf("If any portion of this section is held invalid or unenforceable under\n");
    printf("any particular circumstance, the balance of the section is intended to\n");
    printf("apply and the section as a whole is intended to apply in other\n");
    printf("circumstances.\n");
    printf("\n");
    printf("It is not the purpose of this section to induce you to infringe any\n");
    printf("patents or other property right claims or to contest validity of any\n");
    printf("such claims; this section has the sole purpose of protecting the\n");
    printf("integrity of the free software distribution system, which is\n");
    printf("implemented by public license practices.  Many people have made\n");
    printf("generous contributions to the wide range of software distributed\n");
    printf("through that system in reliance on consistent application of that\n");
    printf("system; it is up to the author/donor to decide if he or she is willing\n");
    printf("to distribute software through any other system and a licensee cannot\n");
    printf("impose that choice.\n");
    printf("\n");
    printf("This section is intended to make thoroughly clear what is believed to\n");
    printf("be a consequence of the rest of this License.\n");
    printf("\n");
    printf("  8. If the distribution and/or use of the Program is restricted in\n");
    printf("certain countries either by patents or by copyrighted interfaces, the\n");
    printf("original copyright holder who places the Program under this License\n");
    printf("may add an explicit geographical distribution limitation excluding\n");
    printf("those countries, so that distribution is permitted only in or among\n");
    printf("countries not thus excluded.  In such case, this License incorporates\n");
    printf("the limitation as if written in the body of this License.\n");
    printf("\n");
    printf("  9. The Free Software Foundation may publish revised and/or new versions\n");
    printf("of the General Public License from time to time.  Such new versions will\n");
    printf("be similar in spirit to the present version, but may differ in detail to\n");
    printf("address new problems or concerns.\n");
    printf("\n");
    printf("Each version is given a distinguishing version number.  If the Program\n");
    printf("specifies a version number of this License which applies to it and \"any\n");
    printf("later version\", you have the option of following the terms and conditions\n");
    printf("either of that version or of any later version published by the Free\n");
    printf("Software Foundation.  If the Program does not specify a version number of\n");
    printf("this License, you may choose any version ever published by the Free Software\n");
    printf("Foundation.\n");
    printf("\n");
    printf("  10. If you wish to incorporate parts of the Program into other free\n");
    printf("programs whose distribution conditions are different, write to the author\n");
    printf("to ask for permission.  For software which is copyrighted by the Free\n");
    printf("Software Foundation, write to the Free Software Foundation; we sometimes\n");
    printf("make exceptions for this.  Our decision will be guided by the two goals\n");
    printf("of preserving the free status of all derivatives of our free software and\n");
    printf("of promoting the sharing and reuse of software generally.\n");
    printf("\n");
    show_warranty();
    printf("		     END OF TERMS AND CONDITIONS\n");
    printf("\n");
    printf("	    How to Apply These Terms to Your New Programs\n");
    printf("\n");
    printf("  If you develop a new program, and you want it to be of the greatest\n");
    printf("possible use to the public, the best way to achieve this is to make it\n");
    printf("free software which everyone can redistribute and change under these terms.\n");
    printf("\n");
    printf("  To do so, attach the following notices to the program.  It is safest\n");
    printf("to attach them to the start of each source file to most effectively\n");
    printf("convey the exclusion of warranty; and each file should have at least\n");
    printf("the \"copyright\" line and a pointer to where the full notice is found.\n");
    printf("\n");
    printf("    <one line to give the program's name and a brief idea of what it does.>\n");
    printf("    Copyright (C) <year>  <name of author>\n");
    printf("\n");
    printf("    This program is free software; you can redistribute it and/or modify\n");
    printf("    it under the terms of the GNU General Public License as published by\n");
    printf("    the Free Software Foundation; either version 2 of the License, or\n");
    printf("    (at your option) any later version.\n");
    printf("\n");
    printf("    This program is distributed in the hope that it will be useful,\n");
    printf("    but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
    printf("    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
    printf("    GNU General Public License for more details.\n");
    printf("\n");
    printf("    You should have received a copy of the GNU General Public License\n");
    printf("    along with this program; if not, write to the Free Software\n");
    printf("    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA\n");
    printf("\n");
    printf("\n");
    printf("Also add information on how to contact you by electronic and paper mail.\n");
    printf("\n");
    printf("If the program is interactive, make it output a short notice like this\n");
    printf("when it starts in an interactive mode:\n");
    printf("\n");
    printf("    Gnomovision version 69, Copyright (C) year name of author\n");
    printf("    Gnomovision comes with ABSOLUTELY NO WARRANTY; for details type `show w'.\n");
    printf("    This is free software, and you are welcome to redistribute it\n");
    printf("    under certain conditions; type `show c' for details.\n");
    printf("\n");
    printf("The hypothetical commands `show w' and `show c' should show the appropriate\n");
    printf("parts of the General Public License.  Of course, the commands you use may\n");
    printf("be called something other than `show w' and `show c'; they could even be\n");
    printf("mouse-clicks or menu items--whatever suits your program.\n");
    printf("\n");
    printf("You should also get your employer (if you work as a programmer) or your\n");
    printf("school, if any, to sign a \"copyright disclaimer\" for the program, if\n");
    printf("necessary.  Here is a sample; alter the names:\n");
    printf("\n");
    printf("  Yoyodyne, Inc., hereby disclaims all copyright interest in the program\n");
    printf("  `Gnomovision' (which makes passes at compilers) written by James Hacker.\n");
    printf("\n");
    printf("  <signature of Ty Coon>, 1 April 1989\n");
    printf("  Ty Coon, President of Vice\n");
    printf("\n");
    printf("This General Public License does not permit incorporating your program into\n");
    printf("proprietary programs.  If your program is a subroutine library, you may\n");
    printf("consider it more useful to permit linking proprietary applications with the\n");
    printf("library.  If this is what you want to do, use the GNU Library General\n");
    printf("Public License instead of this License.\n");
    printf("\n");
}
