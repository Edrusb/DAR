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
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/
// $Id: entrepot.hpp,v 1.1 2012/04/27 11:24:30 edrusb Exp $
//
/*********************************************************************/


    /// \file entrepot.hpp
    /// \brief defines the entrepot interface
    /// \ingroup Private

#ifndef ENTREPOT_HPP
#define ENTREPOT_HPP

#include "../my_config.h"

#include <string>
#include "infinint.hpp"
#include "generic_file.hpp"
#include "user_interaction.hpp"
#include "fichier.hpp"
#include "hash_fichier.hpp"
#include "etage.hpp"
#include "path.hpp"

namespace libdar
{
	/// \addtogroup Private
	/// @{

	/// the Entrepot interface

    class entrepot
    {
    public:
	enum io_errors
	{
	    io_ok,    //< operation succeeded
	    io_exist, //< file already exists (write mode)
	    io_absent //< file does not exist (read mode)
	};
	    /// constructor
	entrepot();

	    /// destructor
	virtual ~entrepot() {};

	    /// says if two operators points to the same directory
	bool operator == (const entrepot & ref) const { return get_url() == ref.get_url(); };


	    /// defines the directory where to proceed to future open() -- this is a "chdir" equivalent
	void set_location(const path & chemin);

	    /// defines the root to use if set_location is given a relative path
	void set_root(const path & p_root) { if(p_root.is_relative()) throw Erange("entrepot::set_root", std::string(gettext("root's entrepot must be an absolute path: ")) + p_root.display()); root = p_root; };

	    /// set default permission and ownership for files to create thanks to the open() method
	void set_user_ownership(const std::string & x_user) { user = x_user; };
	void set_group_ownership(const std::string & x_group) { group = x_group; };
	void set_permission(const std::string & x_perm) { perm = x_perm; };

	const path & get_location() const { return where; };
	const path & get_root() const { return root; };
	std::string get_full_path() const { return (get_root() + get_location()).display(); };
	virtual std::string get_url() const = 0; //< defines an URL-like normalized full location of slices
	const std::string & get_user_ownership() const { return user; };
	const std::string & get_group_ownership() const { return group; };
	const std::string & get_permission() const { return perm; };

	    /// defines the way to open a file and return a "class fichier_global" object as last argument upon success
	    ///
	    /// \param[in] dialog for user interaction
	    /// \param[in] filename is the full path+name of the file to open (read/create/write to)
	    /// \param[in] mode defines which way to open the file (read-only, read-write or write-only)
	    /// \param[in] fail_if_exists tells whether the underlying implementation have to fail throwing Erange("exists") if the file already exist when write access is required
	    /// \param[in] erase tells whether the underlying implementation will empty an existing file before writing to it
	    /// \param[in] algo defines the hash file to create, other value than hash_none are acceptes only in writeonly mode with erase or fail_if_exist set
	    /// \param[out] ret points to the newly allocated object. It is the duty of the caller to delete it when no more needed. Note that this argument meaninfull only if the return code is "io_errors::io_ok".
	    /// \return the status of the open operation, other problem have to be reported throwing an exception
	    /// by the caller when no more needed.
	io_errors open(user_interaction & dialog,
		       const std::string & filename,
		       gf_mode mode,
		       bool fail_if_exists,
		       bool erase,
		       hash_algo algo,
		       fichier_global * & ret) const;

	    /// routines to read existing files in the current directory
	virtual void read_dir_reset() = 0;
	virtual bool read_dir_next(std::string & filename) = 0;

	void unlink(const std::string & filename) const { inherited_unlink(filename); }; //< done this way for homogeneity with open/inherited_open


	virtual entrepot *clone() const = 0;

    protected:
	virtual io_errors inherited_open(user_interaction & dialog,     //< for user interaction
					 const std::string & filename,  //< filename to open
					 gf_mode mode,                  //< mode to use
					 bool fail_if_exists,           //< whether to fail if file exists (write mode)
					 bool erase,                    //< whether to erase file if file already exists (write mode)
					 fichier_global * & ret) const = 0; //< the created object upon successful operation

	virtual void inherited_unlink(const std::string & filename) const = 0;

	virtual void read_dir_flush() = 0; //< ends the read_dir_next, (no more entry available)

    private:
	path where;
        path root;
	std::string user;
	std::string group;
	std::string perm;
    };


	/// the local filesystem entrepot

    class entrepot_local : public entrepot
    {
    public:
	entrepot_local(const std::string & perm, const std::string & user, const std::string & group, bool x_furtive_mode);
	entrepot_local(const entrepot_local & ref): entrepot(ref) { copy_from(ref); };
	entrepot_local & operator = (const entrepot_local & ref);
	~entrepot_local() { detruit(); };

	std::string get_url() const { return std::string("file://") + get_full_path(); };

	void read_dir_reset();
	bool read_dir_next(std::string & filename);

	entrepot *clone() const { return new entrepot_local(*this); };

    protected:
	io_errors inherited_open(user_interaction & dialog,
				 const std::string & filename,
				 gf_mode mode,
				 bool fail_if_exists,
				 bool erase,
				 fichier_global * & ret) const;

	void inherited_unlink(const std::string & filename) const;
	void read_dir_flush() { detruit(); };

    private:
	bool furtive_mode;
	etage *contents;

	void copy_from(const entrepot_local & ref) { furtive_mode = ref.furtive_mode; contents = NULL; };
	void detruit() { if(contents != NULL) { delete contents; contents = NULL; } };
    };

	/// @}

} // end of namespace

#endif
