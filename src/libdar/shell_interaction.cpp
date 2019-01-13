/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2018 Denis Corbin
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
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
#if HAVE_ERRNO_H
#include <errno.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_STRINGS_H
#include <strings.h>
#endif

#if STDC_HEADERS
# include <string.h>
#else
# if !HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
    char *strchr (), *strrchr ();
# if !HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

#if HAVE_STDIO_H
#include <stdio.h>
#endif

#if HAVE_LIMITS_H
#include <limits.h>
#endif

#if HAVE_ERRNO_H
#include <errno.h>
#endif

} // end extern "C"

    // L_ctermid should be defined from stdio.h but this is not
    // the case under cygwin! So we define it ourself with a
    // somehow arbitrarily large value taking the risk it to be
    // too short and having ctermid() writing over the reserved buffer
    // space
#ifndef L_ctermid
#define L_ctermid 200
#endif

#include "tools.hpp"
#include "integers.hpp"
#include "cygwin_adapt.hpp"
#include "shell_interaction.hpp"
#include "nls_swap.hpp"

#ifndef L_ctermid
// cygwin does not always define L_ctermid in stdio.h
#define L_ctermid 10240
// this value should be large enough for most cases
// if it is smaller than system expects, ctermid() may lead
// to program crash
#endif

using namespace std;
using namespace libdar;

namespace libdar
{

#ifdef SSIZE_MAX
    const U_I shell_interaction::bufsize = 1024 > SSIZE_MAX ? SSIZE_MAX : 1024;
#else
    const U_I shell_interaction::bufsize = 1024;
#endif



    shell_interaction::shell_interaction(ostream & out,
					 ostream & interact,
					 bool silent): output(&out), inter(&interact)
    {
	NLS_SWAP_IN;
        try
        {
	    has_terminal = false;
	    beep = false;
	    at_once = 0;
	    count = 0;

		// looking for an input terminal
		//
		// we do not use anymore standart input but open a new descriptor
		// from the controlling terminal. This allow in some case to keep use
		// standart input for piping data while still having user interaction
		// possible.


		// terminal settings
	    try
	    {
		char tty[L_ctermid+1];
		struct termios term;

		(void)ctermid(tty);
		tty[L_ctermid] = '\0';

		input = ::open(tty, O_RDONLY|O_TEXT);
		if(input < 0)
		    throw Erange("",""); // used locally
		else
		{
		    if(silent)
			has_terminal = false; // force non interactive mode
		    else
			    // preparing input for swaping between char mode and line mode (terminal settings)
			if(tcgetattr(input, &term) >= 0)
			{
			    initial = term;
			    initial_noecho = term;
			    initial_noecho.c_lflag &= ~ECHO;
			    term.c_lflag &= ~ICANON;
			    term.c_lflag &= ~ECHO;
			    term.c_cc[VTIME] = 0;
			    term.c_cc[VMIN] = 1;
			    interaction = term;

				// checking now that we can change to character mode
			    set_term_mod(m_inter);
			    set_term_mod(m_initial);
				// but we don't need it right now, so swapping back to line mode
			    has_terminal = true;
			}
			else // failed to retrieve parameters from tty
			    throw Erange("",""); // used locally
		}
	    }
	    catch(Erange & e)
	    {
		if(e.get_message() == "")
		{
		    if(!silent)
			message(gettext("No terminal found for user interaction. All questions will be assumed a negative answer (less destructive choice), which most of the time will abort the program."));
		}
		else
		    throw;
	    }
	}
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
    }

	/// copy constructor

    shell_interaction::shell_interaction(const shell_interaction & ref): user_interaction(ref), output(ref.output), inter(ref.inter)
    {
	if(ref.input >= 0)
	{
	    input = dup(ref.input);
	    if(input < 0)
		throw Erange("shell_interaction::shell_interaction", string("Failed dup()-licating file descriptor: ") + tools_strerror_r(errno));
	}
	else
	    input = ref.input;
	beep = ref.beep;
	initial = ref.initial;
	interaction = ref.interaction;
	initial_noecho = ref.initial_noecho;
	has_terminal = ref.has_terminal;
    }


    shell_interaction::~shell_interaction()
    {
	if(has_terminal)
	    set_term_mod(m_initial);

	if(input >= 0)
	{
	    close(input);
	    input = -1;
	}
    }

    void shell_interaction::change_non_interactive_output(ostream & out)
    {
	output = &out;
    }

    void shell_interaction::read_char(char & a)
    {
	NLS_SWAP_IN;
        try
        {
	    sigset_t old_mask;

	    if(input < 0)
		throw SRC_BUG;

	    tools_block_all_signals(old_mask);
	    set_term_mod(m_inter);
	    if(read(input, &a, 1) < 0)
		throw Erange("shell_interaction_read_char", string(gettext("Error reading character: ")) + strerror(errno));
	    tools_blocking_read(input, true);
	    set_term_mod(m_initial);
	    tools_set_back_blocked_signals(old_mask);
	}
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
    }

    void shell_interaction::archive_show_contents(const archive & ref, const archive_options_listing_shell & options)
    {
	NLS_SWAP_IN;
        try
        {
	    archive_listing_sizes_in_bytes = options.get_sizes_in_bytes();
	    archive_listing_display_ea = options.get_display_ea();
	    all_slices.clear();
	    marge = "";

	    switch(options.get_list_mode())
	    {
	    case archive_options_listing_shell::normal:
		printf(gettext("[Data ][D][ EA  ][FSA][Compr][S]| Permission | User  | Group | Size    |          Date                 |    filename"));
		printf(        "--------------------------------+------------+-------+-------+---------+-------------------------------+------------");
		ref.op_listing(archive_listing_callback_tar, this, options);
		break;
	    case archive_options_listing_shell::tree:
		printf(gettext("Access mode    | User | Group | Size   |          Date                 |[Data ][D][ EA  ][FSA][Compr][S]|   Filename"));
		printf(        "---------------+------+-------+--------+-------------------------------+--------------------------------+-----------");
		ref.op_listing(archive_listing_callback_tree, this, options);
		break;
	    case archive_options_listing_shell::xml:
		message("<?xml version=\"1.0\" ?>");
		message("<!DOCTYPE Catalog SYSTEM \"dar-catalog.dtd\">");
		message("<Catalog format=\"1.2\">");
		ref.op_listing(archive_listing_callback_xml, this, options);
		message("</Catalog>");
		break;
	    case archive_options_listing_shell::slicing:
		message("Slice(s)|[Data ][D][ EA  ][FSA][Compr][S]|Permission| Filemane");
		message("--------+--------------------------------+----------+-----------------------------");
		ref.op_listing(archive_listing_callback_slicing, this, options);
		message("-----");
		message(tools_printf("All displayed files have their data in slice range [%s]", all_slices.display().c_str()));
		message("-----");
		break;
	    default:
		throw SRC_BUG;
	    }
	}
        catch(...)
        {
            NLS_SWAP_OUT;
            throw;
        }
        NLS_SWAP_OUT;
    }

    void shell_interaction::database_show_contents(const database & ref)
    {
	NLS_SWAP_IN;
	try
	{
	    database_archives_list content = ref.get_contents();

	    string opt = tools_concat_vector(" ", ref.get_options());
	    string road, base;
	    string compr = compression2string(ref.get_compression());
	    string dar_path = ref.get_dar_path();
	    string db_version = ref.get_database_version();

	    message("");
	    printf(gettext("dar path        : %S"), &dar_path);
	    printf(gettext("dar options     : %S"), &opt);
	    printf(gettext("database version: %S"), &db_version);
	    printf(gettext("compression used: %S"), &compr);
	    message("");
	    printf(gettext("archive #   |    path      |    basename"));
	    printf("------------+--------------+---------------");


	    for(archive_num i = 1; i < content.size(); ++i)
	    {
		road = content[i].get_path();
		base = content[i].get_basename();
		opt = (road == "") ? gettext("<empty>") : road;
		printf(" \t%u\t%S\t%S", i, &opt, &base);
	    }
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void shell_interaction::database_show_files(const database & ref,
						archive_num num,
						const database_used_options & opt)
    {
	NLS_SWAP_IN;
	try
	{
 	    ref.get_files(show_files_callback, this, num, opt);
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }


    void shell_interaction::database_show_version(const database & ref, const path & chemin)
    {
	NLS_SWAP_IN;
	try
	{
	    ref.get_version(get_version_callback, this, chemin);
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void shell_interaction::database_show_statistics(const database & ref)
    {
	NLS_SWAP_IN;
	try
	{
	    printf(gettext("  archive #   |  most recent/total data |  most recent/total EA"));
	    printf(gettext("--------------+-------------------------+-----------------------")); // having it with gettext let the translater adjust columns width
	    ref.show_most_recent_stats(statistics_callback, this);
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void shell_interaction::inherited_message(const string & message)
    {
	if(at_once > 0)
	{
 	    U_I c = 0, max = message.size();
	    while(c < max)
	    {
		if(message[c] == '\n')
		    count++;
		c++;
	    }
	    count++; // for the implicit \n at end of message
	    if(count >= at_once)
	    {
		count = 0;
		pause(libdar::dar_gettext("Continue? "));
	    }
	}
	my_message(message);
    }

    bool shell_interaction::inherited_pause(const string &message)
    {
	char buffer[bufsize];
	char & a = buffer[0];
	char & b = buffer[1];
	bool ret;

	if(!has_terminal)
	    return false;

	if(input < 0)
	    throw SRC_BUG;

	set_term_mod(shell_interaction::m_inter);
	try
	{
	    sigset_t old_mask;
	    S_I tmp_ret, errno_bk, tmp_sup, errno_sup;


	    do
	    {
		    // flushing any character remaining in the input stream

		tools_blocking_read(input, false);
		while(read(input, buffer, bufsize) >= 0)
		    ;
		tools_blocking_read(input, true);

		    // now asking the user

		*(inter) << message << gettext(" [return = YES | Esc = NO]") << (beep ? "\007\007\007" : "") << endl;
		tools_block_all_signals(old_mask);
		tmp_ret = read(input, &a, 1);
		errno_bk = errno;

		    // checking if another character is available in the pipe

		tools_blocking_read(input, false);
		errno_sup = EAGAIN+1; // = something different from EAGAIN, whatever it is...
		usleep(10000); // let a change for any other typed character to reach the input device
		tmp_sup = read(input, &b, 1);
		errno_sup = errno;
		tools_blocking_read(input, true);

		    // checking error conditions

		tools_set_back_blocked_signals(old_mask);
		if(tmp_ret < 0)
		    if(errno_bk != EINTR)
			throw Erange("shell_interaction:interaction_pause", string(gettext("Error while reading user answer from terminal: ")) + strerror(errno_bk));
	    }
	    while((a != 27 && a != '\n') || tmp_sup != -1 || errno_sup != EAGAIN);

	    if(a != 27)
		*(inter) << gettext("Continuing...") << endl;
	    else
		*(inter) << gettext("Escaping...") << endl;

	    ret = a != 27; // 27 is escape key
	}
	catch(...)
	{
	    set_term_mod(shell_interaction::m_initial);
	    throw;
	}
	set_term_mod(shell_interaction::m_initial);

	return ret;
    }

    string shell_interaction::inherited_get_string(const string & message, bool echo)
    {
	string ret;
	const U_I expected_taille = 100;
#ifdef SSIZE_MAX
	const U_I taille = expected_taille > SSIZE_MAX ? SSIZE_MAX : expected_taille;
#else
	const U_I taille = expected_taille;
#endif
	U_I lu, i;
	char buffer[taille+1];
	bool fin = false;

	if(!echo)
	    set_term_mod(shell_interaction::m_initial);

	if(output == nullptr || input < 0)
	    throw SRC_BUG;  // shell_interaction has not been properly initialized
	*(inter) << message;
	do
	{
	    lu = ::read(input, buffer, taille);
	    i = 0;
	    while(i < lu && buffer[i] != '\n')
		++i;
	    if(i < lu)
		fin = true;
	    buffer[i] = '\0';
	    ret += string(buffer);
	}
	while(!fin);
	if(!echo)
	    *(inter) << endl;
	set_term_mod(shell_interaction::m_initial);

	return ret;
    }

    secu_string shell_interaction::inherited_get_secu_string(const string & message, bool echo)
    {
	const U_I expected_taille = 1000;
#ifdef SSIZE_MAX
	const U_I taille = expected_taille > SSIZE_MAX ? SSIZE_MAX : expected_taille;
#else
	const U_I taille = expected_taille;
#endif
	secu_string ret = taille;
	bool fin = false;
	U_I last = 0, i = 0;

	if(!has_terminal)
	    throw Erange("shell_interaction::interaction_secu_string", gettext("Secured string can only be read from a terminal"));

	if(!echo)
	    set_term_mod(shell_interaction::m_noecho);

	try
	{
	    if(output == nullptr || input < 0)
		throw SRC_BUG;  // shell_interaction has not been properly initialized
	    *(inter) << message;
	    do
	    {
		ret.append(input, taille - ret.get_size());
		i = last;
		while(i < ret.get_size() && ret.c_str()[i] != '\n')
		    ++i;
		if(i < ret.get_size()) // '\n' found so we stop here but remove this last char
		{
		    fin = true;
		    ret.reduce_string_size_to(i);
		}
		else
		    last = i;

		if(ret.get_size() == taille && !fin)
		    throw Erange("interaction_secu_string", gettext("provided password is too long for the allocated memory"));
	    }
	    while(!fin);

	    if(!echo)
		*(inter) << endl;
	}
	catch(...)
	{
	    set_term_mod(shell_interaction::m_initial);
	    throw;
	}
	set_term_mod(shell_interaction::m_initial);

	return ret;
    }

    void shell_interaction::set_term_mod(shell_interaction::mode m)
    {
	termios *ptr = nullptr;
	switch(m)
	{
	case m_initial:
	    ptr = &initial;
	    break;
	case m_inter:
	    ptr = &interaction;
	    break;
	case m_noecho:
	    ptr = &initial_noecho;
	    break;
	default:
	    throw SRC_BUG;
	}

	if(tcsetattr(input, TCSANOW, ptr) < 0)
	    throw Erange("shell_interaction : set_term_mod", string(gettext("Error while changing user terminal properties: ")) + strerror(errno));
    }

    void shell_interaction::my_message(const std::string & mesg)
    {
	if(output == nullptr)
	    throw SRC_BUG; // shell_interaction has not been properly initialized
	*output << mesg;
	if(mesg.rbegin() == mesg.rend() || *(mesg.rbegin()) != '\n')
	    *output << endl;
    }

    void shell_interaction::archive_listing_callback_tree(const std::string & the_path,
							  const list_entry & entry,
							  void *context)
    {
	static const string marge_plus = " |  ";
	static const U_I marge_plus_length = marge_plus.size();

	shell_interaction *me = (shell_interaction *)(context);
	if(me == nullptr)
	    throw SRC_BUG;

	if(entry.is_eod())
	{
	    U_I length = me->marge.size();

	    if(length >= marge_plus_length)
		me->marge.erase(length - marge_plus_length, marge_plus_length);
	    else
		throw SRC_BUG;
	    me->printf("%S +---", &(me->marge));

	}
	else
	{
	    string nom = entry.get_name();

	    if(entry.is_removed_entry())
	    {
		string tmp_date = entry.get_removal_date();
		char type = tools_cast_type_to_unix_type(entry.get_removed_type());
		me->message(tools_printf(gettext("%S [%c] [ REMOVED ENTRY ] (%S)  %S"), &me->marge, type, &tmp_date, &nom));
	    }
	    else // not a removed entry
	    {
		string a = entry.get_perm();
		string b = entry.get_uid(true);
		string c = entry.get_gid(true);
		string d = entry.get_file_size(me->archive_listing_sizes_in_bytes);
		string e = entry.get_last_modif();
		string f =
		    entry.get_data_flag()
		    + entry.get_delta_flag()
		    + entry.get_ea_flag()
		    + entry.get_fsa_flag()
		    + entry.get_compression_ratio_flag()
		    + entry.get_sparse_flag();

		if(me->archive_listing_display_ea && entry.is_hard_linked())
		{
		    string tiq = entry.get_etiquette();
		    nom += tools_printf(" [%S] ", &tiq);
		}

		me->printf("%S%S\t%S\t%S\t%S\t%S\t%S %S", &me->marge, &a, &b, &c, &d, &e, &f, &nom);
		if(me->archive_listing_display_ea)
		{
		    string key;

		    entry.get_ea_reset_read();
		    while(entry.get_ea_read_next(key))
			me->message(me->marge + gettext("      Extended Attribute: [") + key + "]");
		}

		if(entry.is_dir())
		    me->marge += marge_plus;
	    }
	}
    }

    void shell_interaction::archive_listing_callback_tar(const std::string & the_path,
						 const list_entry & entry,
						 void *context)
    {
	shell_interaction *me = (shell_interaction *)(context);
	if(me == nullptr)
	    throw SRC_BUG;

	if(entry.is_eod())
	    return;

	if(entry.is_removed_entry())
	{
	    string tmp_date = entry.get_removal_date();
	    char type = tools_cast_type_to_unix_type(entry.get_removed_type());
	    me->printf("%s (%S) [%c] %S", gettext(REMOVE_TAG), &tmp_date, type, &the_path);
	}
	else
	{
	    string a = entry.get_perm();
	    string b = entry.get_uid(true);
	    string c = entry.get_gid(true);
	    string d = entry.get_file_size(me->archive_listing_sizes_in_bytes);
	    string e = entry.get_last_modif();
	    string f =
		entry.get_data_flag()
		+ entry.get_delta_flag()
		+ entry.get_ea_flag()
		+ entry.get_fsa_flag()
		+ entry.get_compression_ratio_flag()
		+ entry.get_sparse_flag();
	    string tiq = "";

	    if(me->archive_listing_display_ea && entry.is_hard_linked())
		tiq = tools_printf(" [%s]", entry.get_etiquette().c_str());

	    me->printf("%S %S   %S\t%S\t%S\t%S\t%S%S", &f, &a, &b, &c, &d, &e, &the_path, &tiq);
	    if(me->archive_listing_display_ea)
	    {
		string key;

		entry.get_ea_reset_read();
		while(entry.get_ea_read_next(key))
		    me->message(gettext("      Extended Attribute: [") + key + "]");
	    }
	}
    }

    void shell_interaction::archive_listing_callback_xml(const std::string & the_path,
							 const list_entry & entry,
							 void *context)
    {
	shell_interaction *me = (shell_interaction *)(context);
	if(me == nullptr)
	    throw SRC_BUG;

	if(entry.is_eod())
	{
	    U_I length = me->marge.size();

	    if(length > 0)
		me->marge.erase(length - 1, 1); // removing the last tab character
	    else
		throw SRC_BUG;
	    me->printf("%S</Directory>", &me->marge);
	}
	else
	{
	    string name = tools_output2xml(entry.get_name());
	    unsigned char sig = entry.get_type();

	    if(entry.is_removed_entry())
	    {
		me->xml_listing_attributes(entry);
		switch(sig)
		{
		case 'd':
		    me->printf("%S<Directory name=\"%S\">", &(me->marge), &name);
		    me->xml_listing_attributes(entry);
		    me->printf("%S</Directory>", &(me->marge));
		    break;
		case 'f':
		case 'h':
		case 'e':
		    me->printf("%S<File name=\"%S\">", &(me->marge), &name);
		    me->xml_listing_attributes(entry);
		    me->printf("%S</File>", &(me->marge));
		    break;
		case 'l':
		    me->printf("%S<Symlink name=\"%S\">", &(me->marge), &name);
		    me->xml_listing_attributes(entry);
		    me->printf("%S</Symlink>", &(me->marge));
		    break;
		case 'c':
		    me->printf("%S<Device name=\"%S\" type=\"character\">", &(me->marge), &name);
		    me->xml_listing_attributes(entry);
		    me->printf("%S</Device>", &(me->marge));
		    break;
		case 'b':
		    me->printf("%S<Device name=\"%S\" type=\"block\">", &(me->marge), &name);
		    me->xml_listing_attributes(entry);
		    me->printf("%S</Device>", &(me->marge));
		    break;
		case 'p':
		    me->printf("%S<Pipe name=\"%S\">", &(me->marge), &name);
		    me->xml_listing_attributes(entry);
		    me->printf("%S</Pipe>", &(me->marge));
		    break;
		case 's':
		    me->printf("%S<Socket name=\"%S\">", &(me->marge), &name);
		    me->xml_listing_attributes(entry);
		    me->printf("%S</Socket>", &(me->marge));
		    break;
		default:
		    throw SRC_BUG;
		}
	    }
	    else // other than cat_detruit object
	    {

		string maj, min, chksum, chkbasesum, chkresultsum, target, delta_sig;
		string dirty, sparse;
		string size = entry.get_file_size(me->archive_listing_sizes_in_bytes);
		string stored = entry.get_storage_size_for_data(me->archive_listing_sizes_in_bytes);
		string crc_tmp;
		string crc_patch_base;
		string crc_patch_result;

		if(!entry.is_file() || !entry.is_sparse())
		{
		    infinint stor;
		    entry.get_storage_size_for_data(stor);

		    if(stor.is_zero())
			stored = size;
		}

		    // building entry for each type of cat_inode

		switch(sig)
		{
		case 'd': // directories
		    me->printf("%S<Directory name=\"%S\">", &(me->marge), &name);
		    me->xml_listing_attributes(entry);
		    me->marge += "\t";
		    break;
		case 'h': // hard linked files
		case 'e': // hard linked files
		    throw SRC_BUG; // no more used in dynamic data
		case 'f': // plain files
		    dirty = yes_no(entry.is_dirty());
		    sparse = yes_no(entry.is_sparse());
		    if(!entry.has_data_present_in_the_archive())
			stored = "";

		    chksum = entry.get_data_crc();
		    delta_sig = yes_no(entry.has_delta_signature());
		    chkbasesum = entry.get_delta_patch_base_crc();
		    chkresultsum = entry.get_delta_patch_result_crc();

		    me->printf("%S<File name=\"%S\" size=\"%S\" stored=\"%S\" crc=\"%S\" dirty=\"%S\" sparse=\"%S\" delta_sig=\"%S\" patch_base_crc=\"%S\" patch_result_crc=\"%S\">",
			       &(me->marge), &name, &size, &stored, &chksum, &dirty, &sparse, &delta_sig, &chkbasesum, &chkresultsum);
		    me->xml_listing_attributes(entry);
		    me->printf("%S</File>", &(me->marge));
		    break;
		case 'l': // soft link
		    target = tools_output2xml(entry.get_link_target());
		    me->printf("%S<Symlink name=\"%S\" target=\"%S\">",
			       &(me->marge), &name, &target);
		    me->xml_listing_attributes(entry);
		    me->printf("%S</Symlink>", &(me->marge));
		    break;
		case 'c':
		case 'b':
			// this is maybe less performant, to have both 'c' and 'b' here and
			// make an additional test, but this has the advantage to not duplicate
			// very similar code, which would obviously evoluate the same way.
			// Experience shows that two identical codes even when driven by the same need
			// are an important source of bugs, as one may forget to update both of them, the
			// same way...

		    if(sig == 'c')
			target = "character";
		    else
			target = "block";
			// we re-used target variable which is not used for the current cat_inode

		    if(entry.has_data_present_in_the_archive())
		    {
			maj = entry.get_major();
			min = entry.get_minor();
		    }
		    else
			maj = min = "";

		    me->printf("%S<Device name=\"%S\" type=\"%S\" major=\"%S\" minor=\"%S\">",
			       &(me->marge), &name, &target, &maj, &min);
		    me->xml_listing_attributes(entry);
		    me->printf("%S</Device>", &(me->marge));
		    break;
		case 'p':
		    me->printf("%S<Pipe name=\"%S\">", &(me->marge), &name);
		    me->xml_listing_attributes(entry);
		    me->printf("%S</Pipe>", &(me->marge));
		    break;
		case 's':
		    me->printf("%S<Socket name=\"%S\">", &(me->marge), &name);
		    me->xml_listing_attributes(entry);
		    me->printf("%S</Socket>", &(me->marge));
		    break;
		default:
		    throw SRC_BUG;
		}
	    }
	}
    }

    void shell_interaction::archive_listing_callback_slicing(const std::string & the_path,
							     const list_entry & entry,
							     void *context)
    {
	shell_interaction *me = (shell_interaction *)(context);
	if(me == nullptr)
	    throw SRC_BUG;

	if(entry.is_eod())
	    return;

	me->all_slices += entry.get_slices();

	if(entry.is_removed_entry())
	    me->message(tools_printf("%s\t %s%S", entry.get_slices().display().c_str(), gettext(REMOVE_TAG), &the_path));
	else
	{
	    string a = entry.get_perm();
	    string f = entry.get_data_flag()
		+ entry.get_delta_flag()
		+ entry.get_ea_flag()
		+ entry.get_fsa_flag()
		+ entry.get_compression_ratio_flag()
		+ entry.get_sparse_flag();

	    me->printf("%s\t %S%S %S", entry.get_slices().display().c_str(), &f, &a, &the_path);
	}
    }


    void shell_interaction::show_files_callback(void *tag,
						const std::string & filename,
						bool available_data,
						bool available_ea)
    {
	shell_interaction *dialog = (shell_interaction *)(tag);
	string etat = "";

	if(dialog == nullptr)
	    throw SRC_BUG;

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

    void shell_interaction::get_version_callback(void *tag,
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
	shell_interaction *dialog = (shell_interaction *)(tag);

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

	dialog->printf(" \t%u\t%S  %S  %S  %S", num, &data_date, &data_state, &ea_date, &ea_state);
    }

    void shell_interaction::statistics_callback(void *tag,
						U_I number,
						const infinint & data_count,
						const infinint & total_data,
						const infinint & ea_count,
						const infinint & total_ea)
    {
	shell_interaction *dialog = (shell_interaction *)(tag);

	if(dialog == nullptr)
	    throw SRC_BUG;

	dialog->printf("\t%u %i/%i \t\t\t %i/%i", number, &data_count, &total_data, &ea_count, &total_ea);
    }


    void shell_interaction::xml_listing_attributes(const list_entry & entry)
    {
	string user = entry.get_uid(true);
	string group = entry.get_gid(true);
	string permissions = entry.get_perm();
	string atime = tools_uint2str(entry.get_last_access_s());
	string mtime = tools_uint2str(entry.get_last_modif_s());
	string ctime = tools_uint2str(entry.get_last_change_s());
	string data;
	string metadata;
	string ending_data;

	    // defining "data" string

	switch(entry.get_data_status())
	{
	case saved_status::saved:
	    data = "saved";
	    break;
	case saved_status::fake:
	case saved_status::not_saved:
	    data = "referenced";
	    break;
	case saved_status::delta:
	    data = "patch";
	    break;
	case saved_status::inode_only:
	    data = "inode-only";
	    break;
	default:
	    throw SRC_BUG;
	}

	    // defining "metadata" string

	switch(entry.get_ea_status())
	{
	case ea_saved_status::full:
	    metadata = "saved";
	    break;
	case ea_saved_status::partial:
	case ea_saved_status::fake:
	    metadata = "referenced";
	    break;
	case ea_saved_status::none:
	case ea_saved_status::removed:
	    metadata = "absent";
	    break;
	default:
	    throw SRC_BUG;
	}

	if(entry.is_removed_entry())
	    metadata = "absent";

	bool go_ea = archive_listing_display_ea  && entry.has_EA_saved_in_the_archive() && !entry.is_removed_entry();
	string end_tag = go_ea ? ">" : " />";

	printf("%S<Attributes data=\"%S\" metadata=\"%S\" user=\"%S\" group=\"%S\" permissions=\"%S\" atime=\"%S\" mtime=\"%S\" ctime=\"%S\"%S",
	       &marge, &data, &metadata, &user, &group, &permissions, &atime, &mtime, &ctime, &end_tag);

	if(go_ea)
	{
	    string new_begin = marge + "\t";
	    string key;

	    entry.get_ea_reset_read();
	    while(entry.get_ea_read_next(key))
		message(new_begin + "<EA_entry ea_name=\"" + key + "\" />");
	    printf("%S</Attributes>", &marge);
	}
    }
} // end of namespace

