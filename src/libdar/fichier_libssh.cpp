//*********************************************************************/
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
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
}

#include "fichier_libssh.hpp"
#include "tools.hpp"

using namespace std;

namespace libdar
{

    fichier_libssh::fichier_libssh(const shared_ptr<user_interaction> & dialog,
				   const shared_ptr<libssh_connection> & ptr,
				   const string & chemin,
				   gf_mode mode,
				   U_I permission,
				   bool fail_if_exists,
				   bool erase):
	fichier_global(dialog, mode),
	connect(ptr),
	my_path(chemin),
	sfd(nullptr),
	rabuffer(nullptr),
	rallocated(0),
	rasize(0),
	ralu(0)
    {
	int access_type = 0;

	if(! connect)
	    throw SRC_BUG;

	switch(mode)
	{
	case gf_read_only:
	    access_type |= O_RDONLY;
	    break;
	case gf_read_write:
	    access_type |= O_RDWR;
	    break;
	case gf_write_only:
	    access_type |= O_WRONLY;
	    break;
	default:
	    throw SRC_BUG;
	}

	if(mode != gf_read_only)
	{
	    access_type |= O_CREAT;

	    if(fail_if_exists)
		access_type |= O_EXCL;

	    if(erase)
		access_type |= O_TRUNC;
	}

	sfd = sftp_open(connect->get_sftp_session(),
			    chemin.c_str(),
			    access_type,
			    permission);

	if(sfd == nullptr)
	    throw Erange("fichier_libssh::fichier_libssh",
			 tools_printf(gettext("Failed openning SFTP file %s: %s"),
				      chemin.c_str(),
				      connect->get_sftp_error_msg()));

	rallocated = connect->get_max_read();
	rabuffer = new char[rallocated];
	if(rabuffer == nullptr)
	    throw Ememory("fichier_libssh::fichier_libsssh");
    }


    fichier_libssh::~fichier_libssh()
    {
	myclose();
    }

    void fichier_libssh::change_permission(U_I perm)
    {
	if(!connect)
	    throw SRC_BUG;

	int code = sftp_chmod(connect->get_sftp_session(),
			      my_path.c_str(),
			      perm);

	if(code != SSH_OK)
	    throw Erange("fichier_libssh::change_permission",
			 tools_printf(gettext("Unable to change permission of SFTP file %s: %s"),
				      my_path.c_str(),
				      connect->get_sftp_error_msg()));
    }


    infinint fichier_libssh::get_size() const
    {
	sftp_attributes attr;

	if(!connect)
	    throw SRC_BUG;

	attr = sftp_stat(connect->get_sftp_session(),
			 my_path.c_str());

	if(attr == nullptr)
	    throw Erange("fichier_libssh::change_permission",
			 tools_printf(gettext("Unable to fetch SFTP file size of %s: %s"),
				      my_path.c_str(),
				      connect->get_sftp_error_msg()));

	return infinint(attr->size);
    }


    bool fichier_libssh::skippable(skippability direction, const infinint & amount)
    {
	switch(direction)
	{
	case skip_backward:
	    return amount <= get_position();
	case skip_forward:
	    return true;
	default:
	    throw SRC_BUG;
	}
    }

    bool fichier_libssh::skip(const infinint & pos)
    {
	uint64_t libpos = 0;
	infinint q = pos;
	int ret = 0;

	clear_readahead();
	q.unstack(libpos);
	if(!q.is_zero())
	    throw Elimitint();

	ret = sftp_seek64(sfd,
			  libpos);

	return ret >= 0;
    }

    bool fichier_libssh::skip_to_eof()
    {
	return skip(get_size());
    }

    bool fichier_libssh::skip_relative(S_I x)
    {
	infinint cur = get_position();

	if(x >= 0)
	    return skip(cur + infinint(x));
	else
	    if(cur >= -x) // x is negative, -x is positive
		return skip(cur - infinint(-x));
	    else
		return false;
    }

    bool fichier_libssh::truncatable(const infinint & pos) const
    {
	return false;
    }

    infinint fichier_libssh::get_position() const
    {
	return infinint(sftp_tell(sfd));
    }

    void fichier_libssh::inherited_truncate(const infinint & pos)
    {
	throw Erange("fichier_libssh::inherited_truncate", string(gettext("libcurl does not allow truncating at a given position while uploading files")));
    }

    void fichier_libssh::inherited_read_ahead(const infinint & amount)
    {
	infinint remains = amount;
	size_t step = 0;
	size_t microstep = 0;
	ssize_t ret;

	clear_readahead();

	while(remains > 0 || step > 0)
	{
	    rahead tmp;

	    if(step < rallocated)
		remains.unstack(step);
		// this decrements remains
		// possibly down to zero

	    if(step > rallocated)
		microstep = rallocated;
	    else
		microstep = step;

	    ret = sftp_aio_begin_read(sfd, microstep, & tmp.handle);
	    if(ret == SSH_ERROR)
		throw Erange("fichier_libssh::inherited_read_ahead",
			     tools_printf(gettext("SFTP read-ahead failed: %s"),
					  connect->get_sftp_error_msg()));
	    if(ret != microstep)
		throw SRC_BUG;
		// libssh should not request less than microstep
		// as microstep <= rallocated < limit->max_read_length

	    step -= ret;
	    rareq.push_back(std::move(tmp));
	}
    }

    U_I fichier_libssh::fichier_global_inherited_write(const char *a, U_I size)
    {
	U_I wrote = 0;
	ssize_t step = 0;

	if(!connect)
	    throw SRC_BUG;

	clear_readahead();
	do
	{
	    step = sftp_write(sfd,
			      a + wrote,
			      size - wrote);
	    if(step > 0)
		wrote += step;
	}
	while(wrote < size && step > 0);

	return wrote;
    }

    bool fichier_libssh::fichier_global_inherited_read(char *a,
						       U_I size,
						       U_I & read,
						       std::string & message)
    {
	size_t step = 0;
	read = 0; // third argument of this method

	if(!connect)
	    throw SRC_BUG;

	do
	{

		//////
		// 1 - fetch from rabuffer if available
		//

	    if(ralu < rasize)
	    {
		U_I avail = rasize - ralu;
		U_I to_read = size - read;
		U_I min = to_read > avail ? avail : to_read;

		(void)memcpy(a + read, rabuffer + ralu, min);
		ralu += min;
		read += min;
		step = 1; // to not break the loop if some data are yet to be read
		continue;
	    }

		//////
		// 2 - fetching the iao requests if any
		//

	    if(!rareq.empty())
	    {
		ssize_t ret = sftp_aio_wait_read(&(rareq.front().handle), rabuffer, rallocated);
		if(ret == SSH_AGAIN)
		    throw SRC_BUG;
		if(ret == SSH_ERROR)
		    throw Erange("fichier_libssh::fichier_global_inherited_read",
				 tools_printf(gettext("Error while fetching SFTP read-ahead data: %s"),
					      connect->get_sftp_error_msg()));

		rasize = ret;
		ralu = 0;
		rareq.pop_front();
		step = 1; // to not break the loop if some data are yet to be read
		continue;
	    }

		//////
		// 3 - fetching data in synchronous mode
		//

	    step = sftp_read(sfd,
			     a + read,
			     size - read);

	    if(step > 0)
		read += step;

	}
	while(read < size && step > 0);

	if(step < 0)
	{
	    message = tools_printf(gettext("Libssh error while reading file's data: %s"),
				   connect->get_sftp_error_msg());
	    return false;
	}
	else
	    return true;
    }

    void fichier_libssh::myclose()
    {
	clear_readahead();
	if(rabuffer != nullptr)
	{
	    delete [] rabuffer;
	    rabuffer = nullptr;
	}
	if(sfd != nullptr)
	{
	    sftp_close(sfd);
	    sfd = nullptr;
	}
    }

} // end of namespace
