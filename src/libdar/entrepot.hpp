/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2019 Denis Corbin
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

    /// \file entrepot.hpp
    /// \brief defines the entrepot interface.
    /// Entrepot interface defines a generic way to interact with files (slices)
    /// on a filesystem. It is used to instanciate file-like objects (from class inherited
    /// from class fichier_global, in order to read or write data to such file.
    /// The entrepot_local and fichier_local classes are the only one classes
    /// available from libdar to implement the entrepot and fichier classes interfaces
    /// respectively. External applications like webdar can implement entrepot_ftp
    /// and fichier_ftp classes to provide transparent access to dar backup localted on a
    /// remote ftp server. More can follow in the future.
    /// \ingroup API

#ifndef ENTREPOT_HPP
#define ENTREPOT_HPP

#include "../my_config.h"

#include <string>
#include <memory>
#include "user_interaction.hpp"
#include "path.hpp"
#include "archive_aux.hpp"
#include "gf_mode.hpp"

namespace libdar
{
	/// \addtogroup API
	/// @{

	// no need to dig into this from API header
    class fichier_global;

	/// the Entrepot interface

    class entrepot
    {
    public:

	    /// constructor
	entrepot();

	    /// copy constructor
	entrepot(const entrepot & ref) = default;

	    /// move constructor
	entrepot(entrepot && ref) noexcept = default;

	    /// assignment operator
	entrepot & operator = (const entrepot & ref) = default;

	    /// move operator
	entrepot & operator = (entrepot && ref) noexcept = default;

	    /// destructor
	virtual ~entrepot() = default;

	    /// says whether two entrepot objects points to the same location
	bool operator == (const entrepot & ref) const { return get_url() == ref.get_url(); };


	    /// defines the directory where to proceed to future open() -- this is a "chdir" semantics
	virtual void set_location(const path & chemin);

	    /// defines the root to use if set_location is given a relative path
	virtual void set_root(const path & p_root);

	    /// returns the full path of location

	    /// \note this is equivalent to get_root()/get_location() if get_location() is relative path else get_location()
	virtual path get_full_path() const;

	    /// full path of current directory + anything necessary to provide URL formated information
	virtual std::string get_url() const = 0;

	    /// set default ownership for files to be created thanks to the open() methods
	void set_user_ownership(const std::string & x_user) { user = x_user; };
	void set_group_ownership(const std::string & x_group) { group = x_group; };

	virtual const path & get_location() const { return where; }; //< retreives relative to root path the current location points to
	virtual const path & get_root() const { return root; };      //< retrieves the given root location

	const std::string & get_user_ownership() const { return user; };
	const std::string & get_group_ownership() const { return group; };

	    /// defines the way to open a file and return a "class fichier_global" object as last argument upon success

	    /// \param[in] dialog for user interaction
	    /// \param[in] filename is the full path+name of the file to open (read/create/write to)
	    /// \param[in] mode defines which way to open the file (read-only, read-write or write-only)
	    /// \param[in] force_permission whether to set the file permission to the value given in the permission argument
	    /// \param[in] permission if force_permission is set, change the file permission to that value
	    /// \param[in] fail_if_exists tells whether the underlying implementation have to fail throwing Erange("exists") if the file already exist when write access is required
	    /// \param[in] erase tells whether the underlying implementation will empty an existing file before writing to it
	    /// \param[in] algo defines the hash file to create, other value than hash_none are accepted only in writeonly mode with erase or fail_if_exist set
	    /// \return upon success returns an object from a class inherited from fichier_global that the caller has the duty to delete, else an exception is thrown (most of the time it should be a Esystem object)
	    /// by the called inherited class
	fichier_global *open(const std::shared_ptr<user_interaction> & dialog,
			     const std::string & filename,
			     gf_mode mode,
			     bool force_permission,
			     U_I permission,
			     bool fail_if_exists,
			     bool erase,
			     hash_algo algo) const;

	    /// routines to read existing files in the current directory (see set_location() / set_root() methods)
	virtual void read_dir_reset() const = 0;
	virtual bool read_dir_next(std::string & filename) const = 0;

	void unlink(const std::string & filename) const { inherited_unlink(filename); }; //< done this way for homogeneity with open/inherited_open

	    /// generate a clone of "this"

	    /// \deprecated this method will disapear in the future it is only kept
	    /// there to allow the APIv5 adaptation layer to work over APIv6
	virtual entrepot *clone() const = 0;

    protected:
	virtual fichier_global *inherited_open(const std::shared_ptr<user_interaction> & dialog,     //< for user interaction
					       const std::string & filename,  //< filename to open
					       gf_mode mode,                  //< mode to use
					       bool force_permission,         //< set the permission of the file to open
					       U_I permission,                //< value of the permission to assign when force_permission is true
					       bool fail_if_exists,           //< whether to fail if file exists (write mode)
					       bool erase) const = 0;         //< whether to erase file if file already exists (write mode)

	virtual void inherited_unlink(const std::string & filename) const = 0;

	virtual void read_dir_flush() = 0; //< ends the read_dir_next, (no more entry available)

    private:
	path where;
        path root;
	std::string user;
	std::string group;
    };

	/// @}

} // end of namespace

#endif
