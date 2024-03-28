//*********************************************************************/
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
}

#include "tools.hpp"
#include "i_entrepot_libcurl.hpp"
#include "fichier_libcurl.hpp"
#include "cache_global.hpp"

using namespace std;

namespace libdar
{

#if defined ( LIBCURL_AVAILABLE ) && defined ( LIBTHREADAR_AVAILABLE )

    entrepot_libcurl::i_entrepot_libcurl::i_entrepot_libcurl(const shared_ptr<user_interaction> & dialog,         //< for user interaction
							     mycurl_protocol proto,             //< network protocol to use
							     const string & login,              //< user login on remote host
							     const secu_string & password,      //< user password on remote host (empty for file auth or user interaction)
							     const string & host,               //< the remote server to connect to
							     const string & port,               //< TCP/UDP port to connec to (empty string for default)
							     bool auth_from_file,               //< whether to check $HOME/.netrc for password
							     const string & sftp_pub_keyfile,   //< where to fetch the public key (sftp only)
							     const string & sftp_prv_keyfile,   //< where to fetch the private key (sftp only)
							     const string & sftp_known_hosts,   //< location of the known_hosts file (empty string to disable this security check)
							     U_I waiting_time,
							     bool verbose): mem_ui(dialog),
									    x_proto(proto),
									    base_URL(build_url_from(proto, host, port)),
									    wait_delay(waiting_time),
									    verbosity(verbose),
									    withdirinfo(false)
    {
	current_dir.clear();
	reading_dir_tmp.clear();
	temporary_list.clear();
	cur_dir_cursor = current_dir.begin();

	    // initializing the fields from parent class entrepot

	set_root(path("/"));
	set_location(path("/"));
	set_user_ownership(""); // not used for this type of entrepot
	set_group_ownership(""); // not used for this type of entrepot

	if(!mycurl_is_protocol_available(proto))
	{
	    string named_proto = mycurl_protocol2string(proto);
	    throw Erange("entrepot_libcurl::i_entrepot_libcurl::i_entrepot_libcurl",
			 tools_printf(gettext("protocol %S is not supported by libcurl, aborting"), & named_proto));
	}

	set_libcurl_authentication(*dialog,
				   host,
				   login,
				   password,
				   auth_from_file,
				   sftp_pub_keyfile,
				   sftp_prv_keyfile,
				   sftp_known_hosts);

	if(verbose)
	{
	    easyh.setopt_global(CURLOPT_VERBOSE,(long)1);
	    get_ui().printf(gettext("repository parameters passed to libcurl:"));
	    get_ui().printf(gettext("  hostname\t: %s\n  port   \t: %s\n  login   \t: %s\n  password\t: (hidden)"),
			    host.c_str(),
			    port.c_str(),
			    login.c_str());
	    get_ui().printf(gettext("  base URL\t: %s"),
			    base_URL.c_str());
	}
    }

    bool entrepot_libcurl::i_entrepot_libcurl::read_dir_next(string & filename) const
    {
	bool isdir;
	return read_dir_next_dirinfo(filename, isdir);
    }

    bool entrepot_libcurl::i_entrepot_libcurl::read_dir_next_dirinfo(std::string & filename, bool & isdir) const
    {
	if(cur_dir_cursor == current_dir.end())
	    return false;
	else
	{
	    filename = cur_dir_cursor->first;
	    isdir = cur_dir_cursor->second;
	    ++cur_dir_cursor;
	    return true;
	}
    }

    void entrepot_libcurl::i_entrepot_libcurl::create_dir(const std::string & dirname, U_I permission)
    {
	path cwd = get_full_path();
	string order;
	mycurl_slist headers;

	shared_ptr<mycurl_easyhandle_node> node = easyh.alloc_instance();
	if(!node)
	    throw SRC_BUG;

	if(verbosity)
	    get_ui().printf("Creating directory %s", dirname.c_str());

	try
	{
	    switch(x_proto)
	    {
	    case proto_ftp:
		set_location((cwd + dirname));
		try
		{
		    node->setopt(CURLOPT_NEW_DIRECTORY_PERMS, long(permission));
		    node->setopt(CURLOPT_FTP_CREATE_MISSING_DIRS, long(CURLFTP_CREATE_DIR_RETRY));
		    node->setopt(CURLOPT_URL, get_libcurl_URL());
		    node->setopt(CURLOPT_NOBODY, long(1));
		    node->apply(get_pointer(), wait_delay);
		}
		catch(...)
		{
		    set_location(cwd);
		    throw;
		}
		set_location(cwd);
		break;
	    case proto_sftp:
		node->setopt(CURLOPT_NEW_DIRECTORY_PERMS, long(permission));
		order = "mkdir " + dirname;
		headers.append(order);
		node->setopt(CURLOPT_QUOTE, headers);
		node->setopt(CURLOPT_URL, get_libcurl_URL());
		node->setopt(CURLOPT_NOBODY, long(1));
		node->apply(get_pointer(), wait_delay);
		break;
	    default:
		throw SRC_BUG;
	    }
	}
	catch(Ebug & e)
	{
	    throw;
	}
	catch(Egeneric & e)
	{
	    e.prepend_message(tools_printf(gettext("Error met while creating directory %s : "), dirname.c_str()));
	    throw;
	}
    }

    fichier_global* entrepot_libcurl::i_entrepot_libcurl::inherited_open(const shared_ptr<user_interaction> & dialog,
						       const string & filename,
						       gf_mode mode,
						       bool force_permission,
						       U_I permission,
						       bool fail_if_exists,
						       bool erase) const
    {
	fichier_global *ret = nullptr;
	cache_global *rw = nullptr;
	gf_mode hidden_mode = mode;

	if(fail_if_exists)
	{
	    string tmp;

	    read_dir_reset();
	    while(read_dir_next(tmp))
		if(tmp == filename)
		    throw Esystem("i_entrepot_libcurl::inherited_open", "File exists on remote repository" , Esystem::io_exist);
	}

	string chemin = (path(get_url(), true).append(filename)).display();

	if(verbosity)
	    get_ui().printf("Asking libcurl to open the file %s", chemin.c_str());

	if(hidden_mode == gf_read_write)
	    hidden_mode = gf_write_only;

	try
	{
	    ret = new (nothrow) fichier_libcurl(dialog,
						chemin,
						x_proto,
						easyh.alloc_instance(),
						hidden_mode,
						wait_delay,
						force_permission,
						permission,
						erase);

	    if(ret == nullptr)
		throw Ememory("entrepot_libcurl::i_entrepot_libcurl::inherited_open");

	    switch(mode)
	    {
	    case gf_read_write:
		rw = new (nothrow) cache_global(dialog, ret, true);
		if(rw != nullptr)
		{
			// the former object pointed to by ret is now managed by rw
		    ret = nullptr;
			// the next call my throw an exception, having ret set to nullptr
			// avoid deleting the object pointed to by ret, in that context.
		    rw->change_to_read_write();
			// OK, we can now have ret pointing to the cache_global object to
			// be returned.
		    ret = rw;
		    rw = nullptr;
		}
		else
		    throw Ememory("entrpot_libcurl::inherited_open");
		break;
	    case gf_read_only:
		rw = new (nothrow) cache_global(dialog, ret, false);
		if(rw != nullptr)
		{
			// the former object pointed to by ret is now managed by rw
		    ret = rw;
		    rw = nullptr;
		}
		break;
	    case gf_write_only:
		    // small cache to avoid trailer and header writes byte per byte
		    // (1000 bytes ~ payload of an classical ethernet non-jumbo frame)
		rw = new (nothrow) cache_global(dialog, ret, false, 1000);
		if(rw != nullptr)
		{
			// the former object pointed to by ret is now managed by rw
		    ret = rw;
		    rw = nullptr;
		}
		break;
	    default:
		throw SRC_BUG;
	    }
	}
	catch(...)
	{
	    if(ret != nullptr)
		delete ret;
	    if(rw != nullptr)
		delete rw;
	    throw;
	}

	return ret;
    }

    void entrepot_libcurl::i_entrepot_libcurl::inherited_unlink(const string & filename) const
    {
	mycurl_slist headers;
	shared_ptr<mycurl_easyhandle_node> node;
	string order;

	node = easyh.alloc_instance();
	if(!node)
	    throw SRC_BUG;

	if(verbosity)
	    get_ui().printf("Asking libcurl to delete file %s", filename.c_str());

	switch(x_proto)
	{
	case proto_ftp:
	case proto_sftp:
	    if(x_proto == proto_ftp)
		order = "DELE " + filename;
	    else
		order = "rm " + filename;
	    headers.append(order);
	    node->setopt(CURLOPT_QUOTE, headers);
	    node->setopt(CURLOPT_URL, get_libcurl_URL());
	    node->setopt(CURLOPT_NOBODY, (long)1);
	    node->apply(get_pointer(), wait_delay);
	    break;
	default:
	    throw SRC_BUG;
	}
    }

    void entrepot_libcurl::i_entrepot_libcurl::read_dir_flush()
    {
	current_dir.clear();
	cur_dir_cursor = current_dir.end();
    }

    string entrepot_libcurl::i_entrepot_libcurl::get_libcurl_URL() const
    {
	string target = get_url();

	if(target.size() == 0)
	    throw SRC_BUG;
	else
	    if(target[target.size() - 1] != '/')
		target += "/";

	return target;
    }

    void entrepot_libcurl::i_entrepot_libcurl::set_libcurl_authentication(user_interaction & dialog,
									  const string & location,
									  const string & login,
									  const secu_string & password,
									  bool auth_from_file,
									  const string & sftp_pub_keyfile,
									  const string & sftp_prv_keyfile,
									  const string & sftp_known_hosts)
    {
	secu_string real_pass = password;
	string real_login = login;


	    // checking server authentication

	switch(x_proto)
	{
	case proto_ftp:
	    break;
	case proto_sftp:
	    if(!sftp_known_hosts.empty())
		easyh.setopt_global(CURLOPT_SSH_KNOWNHOSTS, sftp_known_hosts);
	    else
		dialog.message("Warning: known_hosts file check has been disabled, connecting to remote host is subjet to man-in-the-middle attack and login/password credential for remote sftp server to be stolen");

	    easyh.setopt_global(CURLOPT_SSH_PUBLIC_KEYFILE, sftp_pub_keyfile);
	    easyh.setopt_global(CURLOPT_SSH_PRIVATE_KEYFILE, sftp_prv_keyfile);
	    easyh.setopt_global(CURLOPT_SSH_AUTH_TYPES, (long)(CURLSSH_AUTH_PUBLICKEY|CURLSSH_AUTH_PASSWORD));
	    break;
	default:
	    throw SRC_BUG;
	}

	    // checking for user credentials

	if(real_login.empty())
	    real_login = "anonymous";

	if(auth_from_file)
	{
	    easyh.setopt_global(CURLOPT_USERNAME, real_login);
	    easyh.setopt_global(CURLOPT_NETRC, (long)(CURL_NETRC_OPTIONAL));
	}
	else // login + password authentication
	{
	    if(password.empty())
	    {
		real_pass = get_ui().get_secu_string(tools_printf(gettext("Please provide the password for login %S at host %S: "),
								  &real_login,
								  &location),
						     false);
	    }

	    secu_string auth(real_login.size() + 1 + real_pass.get_size() + 1);

	    auth.append(real_login.c_str(), real_login.size());
	    auth.append(":", 1);
	    auth.append(real_pass.c_str(), real_pass.get_size());

	    easyh.setopt_global(CURLOPT_USERPWD, auth);
	}

    }

    void entrepot_libcurl::i_entrepot_libcurl::fill_temporary_list() const
    {
	std::map<string, bool>::iterator it = current_dir.begin();

	temporary_list.clear();
	while(it != current_dir.end())
	{
	    temporary_list.push_back(it->first);
	    ++it;
	}
    }

    void entrepot_libcurl::i_entrepot_libcurl::update_current_dir_with_line(const string & line) const
    {
	    /// we expect the provided line to contain the following fields, separated by spaces:
	    ///. drwxrwxrwx
	    ///. <num hard links>
	    ///. <uid/username>
	    ///. <gid/username>
	    ///. <filesize>
	    ///. <date and time>
	    ///. <filename>
	    /// The most complicated part is to isolate the date from the rest, as it may depends on the local
	    /// and as space can also be present in both <date and time> and <filename>

	    /// It seemed more reliable to first get a temporary list of filename and then run a second time
	    /// the directory listing but with full listing information. In that second time for each line received,
	    /// search for a match at the end of line from the temporary list:
	    /// - each time, retain all filenames that match the end of the line
	    /// - retain the one having the longuest match
	    /// - and update its information whether it is a directory or not lookiing at the first char of the line.
	    /// - remove the matched entry from the temporary list of existing files and wait for the next line

	    /// it is this assumed that both temporary_list and current_dir are filled with the directory content
	    /// but the current_dir<>is_dir has still to be determined.

	    ///////////////////////////
	    // looking for longuest filename match found at end of 'line' from content of temporary_list
	    //

	    // we will record the best_match using max_len and best_entry:
	unsigned int max_len = 0;
	deque<string>::iterator best_entry = temporary_list.end();

	    // let's search!

	for(deque<string>::iterator filename_ptr = temporary_list.begin();
	    filename_ptr != temporary_list.end();
	    ++filename_ptr)
	{
		// for each filename of the temporary_list
		// we look whether it can be found entirely at end of line

	    if(line.size() > filename_ptr->size())
	    {
		unsigned int matched_len = 0; // will hold how much characters of filename are found at end of line

		    // reading backward char by char:

		string::const_reverse_iterator line_it = line.rbegin();
		string::reverse_iterator file_it = filename_ptr->rbegin();

		while(line_it != line.rend()
		      &&
		      file_it != filename_ptr->rend()
		      &&
		      *line_it == *file_it)
		{
		    ++matched_len;
		    ++line_it;
		    ++file_it;
		}

		    // matched_len is now calculated


		if(file_it == filename_ptr->rend())
		{

			// we found the full filename at end of line

		    if(max_len < matched_len)
		    {
			max_len = matched_len;
			best_entry = filename_ptr;
		    }
		    else if(max_len == matched_len)
		    {
			if(best_entry == temporary_list.end())
			    throw SRC_BUG;
			if(*best_entry == *filename_ptr)
			    throw Erange("i_entrepot_libcurl::update_current_dir_with_line", tools_printf("duplicated entry %s in directory", filename_ptr->c_str()));
			else
			    throw SRC_BUG;
		    }
			// else we already found a better match
		}
		    // else this is not a full match, ignoring this filename
	    }
		// else filename is too long to hold on the line with the extra attributes we expect to find
	}

	    ///////////////////////////
	    // udating current_dir
	    //
	    // now we have max_len and best_entry set
	    // we update the current_dir with directory nature information

	if(max_len > 0)
	{
		// we have found a match

	    if(best_entry == temporary_list.end())
		throw SRC_BUG; // incoherence between max_len and best_entry

		// looking for best_entry in current_dir and updating its value

	    map<string, bool>::iterator found = current_dir.find(*best_entry);
	    if(found == current_dir.end())
		throw SRC_BUG; // entry found in temporary_list but not in current_dir??? what was temporary_list filled with then???

	    if(line.size() == 0)
		throw SRC_BUG;

		// now updating value associated with best_entry
		// directory if the line starts with a 'd'
		// symlink if the line starts with an 'l', this may be a directory... maybe not,
		// chdir on symlink pointing to nondir inode will fail, but at least this stay an
		// option, which would not be the case if we assume all symlinks not to point to
		// directories: chdir to them would then be forbidden by entrepot_libcurl...
	    found->second = (line[0] == 'd' || line[0] == 'l');

		// we now remove the best_entry from temporary_list to speed up future search

	    temporary_list.erase(best_entry);
	}
	    // else no entry found in current_dir/temporary_list
	    // probably a entry that has just been added to the current directory
    }

    void entrepot_libcurl::i_entrepot_libcurl::set_current_dir(bool details) const
    {
	shared_ptr<mycurl_easyhandle_node> node;

	    // when details are needed, we run a first time without dir_details
	    // to fill the current_dir list. Then we update temporary_list and run
	    // a second time to fill the dir information
	if(details)
	{
	    set_current_dir(false); // we call ourselves!
	    fill_temporary_list();
	    withdirinfo = true;
	}
	else
	{
	    withdirinfo = false;
	    current_dir.clear();
	}

	node = easyh.alloc_instance();
	if(!node)
	    throw SRC_BUG;

	if(verbosity)
	    get_ui().printf("Asking libcurl to read directory content at %s", get_libcurl_URL().c_str());

	reading_dir_tmp = "";

	switch(x_proto)
	{
	case proto_ftp:
	case proto_sftp:
	    node->setopt(CURLOPT_URL, get_libcurl_URL());
	    if(!details)
	    {
		long listonly = 1;
		node->setopt(CURLOPT_DIRLISTONLY, listonly);
	    }
	    else
	    {
		long listonly = 0;
		node->setopt(CURLOPT_DIRLISTONLY, listonly);
	    }
	    node->setopt(CURLOPT_WRITEFUNCTION, (void*)get_ftp_listing_callback);
	    node->setopt(CURLOPT_WRITEDATA, (void*)(this));
	    node->apply(get_pointer(), wait_delay);
		// libcurl will invoke our callback function 'get_ftp_listing_callback'
		// passed above as argument, which one will fill the directory content
		// in the std::map current_dir. The reading_dir_tmp is the current
		// or the latest entry name of the directory that was under the process of
		// being spelled...

	    if(!reading_dir_tmp.empty())
	    {
		if(!details)
		    current_dir[reading_dir_tmp] = false; // for now we assume it is not a directory
		reading_dir_tmp.clear();
	    }
	    break;
	default:
	    throw SRC_BUG;
	}

	cur_dir_cursor = current_dir.begin();
    }

    string entrepot_libcurl::i_entrepot_libcurl::mycurl_protocol2string(mycurl_protocol proto)
    {
	string ret;

	switch(proto)
	{
	case proto_ftp:
	    ret = "ftp";
	    break;
	case proto_sftp:
	    ret = "sftp";
	    break;
	default:
	    throw SRC_BUG;
	}

	return ret;
    }

    string entrepot_libcurl::i_entrepot_libcurl::build_url_from(mycurl_protocol proto, const string & host, const string & port)
    {
	string ret = mycurl_protocol2string(proto) + "://" + host;

	if(!port.empty())
	    ret += ":" + port;

	ret += "/";
	    // to have any path added in the future to refer to the root
	    // of the remote repository and not relative to the landing directory

	return ret;
    }

    size_t entrepot_libcurl::i_entrepot_libcurl::get_ftp_listing_callback(void *buffer, size_t size, size_t nmemb, void *userp)
    {
	i_entrepot_libcurl *me = (i_entrepot_libcurl *)(userp);
	char *ptr = (char *)buffer;

	if(me == nullptr)
	    return size > 0 ? 0 : 1; // report error to libcurl

	for(unsigned int mi = 0; mi < nmemb; ++mi)
	    for(unsigned int i = 0; i < size; ++i)
	    {
		switch(*ptr)
		{
		case '\n':
		    if(me->withdirinfo)
			me->update_current_dir_with_line(me->reading_dir_tmp);
		    else
			me->current_dir[me->reading_dir_tmp] = false; // for now assumed to be non directory entry
		    me->reading_dir_tmp.clear();
		    break;
		case '\r':
		    break; // just ignore it
		default: // normal character
		    me->reading_dir_tmp += *ptr;
		    break;
		}
		++ptr;
	    }
	return size*nmemb;
    }

    bool entrepot_libcurl::i_entrepot_libcurl::mycurl_is_protocol_available(mycurl_protocol proto)
    {
	curl_version_info_data *data = curl_version_info(CURLVERSION_NOW);
	const char **ptr = nullptr;
	string named_proto = mycurl_protocol2string(proto);

	if(data == nullptr)
	    throw SRC_BUG;

	ptr = const_cast<const char **>(data->protocols);
	if(ptr == nullptr)
	    throw SRC_BUG;

	while(*ptr != nullptr && strcmp(*ptr, named_proto.c_str()) != 0)
	    ++ptr;

	return *ptr != nullptr;
    }



#endif

} // end of namespace
