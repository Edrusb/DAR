/*********************************************************************/
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

    /// \file entrepot.hpp
    /// \brief defines the entrepot interface.
    ///
    /// Entrepot interface defines a generic way to interact with files (slices)
    /// on a filesystem. It is used to instantiate file-like objects (from classes inherited
    /// from class fichier_global), in order to read or write data to such files.
    /// \note only the constructors of class entrepot are part of libdar API
    /// the other methods may change without notice and are not expected to be used
    /// from external applications.
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
#include "entrepot_aux.hpp"

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

	    /// set default ownership for files to be created thanks to the open() or create_dir() methods
	void set_user_ownership(const std::string & x_user) { user = x_user; };
	void set_group_ownership(const std::string & x_group) { group = x_group; };

	virtual const path & get_location() const { return where; }; ///< retreives relative to root path the current location points to
	virtual const path & get_root() const { return root; };      ///< retrieves the given root location

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
	    /// \param[in] provide_a_plain_file if true a plainly seekable object is returned which mimics the available features of a plain file, else a pipe-like object which has limited seek features but which can still record the offset position is returned and is suitable to be used on real named/anonymous pipes (if the provided filename is expected to be of such type for example)
	    /// \return upon success returns an object from a class inherited from fichier_global that the caller has the duty to delete, else an exception is thrown (most of the time it should be a Esystem object)
	    /// by the called inherited class
	fichier_global *open(const std::shared_ptr<user_interaction> & dialog,
			     const std::string & filename,
			     gf_mode mode,
			     bool force_permission,
			     U_I permission,
			     bool fail_if_exists,
			     bool erase,
			     hash_algo algo,
			     bool provide_a_plain_file = true) const;

	    /// change user_interaction if the implementation recorded it (at construction time for example)

	    /// \note method open() just above uses the specified user_interaction provided as its first argument
	virtual void change_user_interaction(const std::shared_ptr<user_interaction> & new_dialog) {};

	    /// get the current user_interaction if the implementation reocrded it at construction time (may be nullptr if not)
	virtual std::shared_ptr<user_interaction> get_current_user_interaction() const { return std::shared_ptr<user_interaction>(); };

	    /// routines to read existing files in the current directory (see set_location() / set_root() methods)

	    /// \param[in] dir_details, if set to true, use read_dir_next() with the isdir argument, else
	    /// use the read_dir_next() with a single argument. By default and for backward compatibility
	    /// dir_details is set to false.
	virtual void read_dir_reset() const = 0;

	    /// read the next filename of the current directory

	    /// \param[out] filename name of the next entry in the directory, (valid only if this method returned true)
	    /// \returns false if no more filename could be fould
	    /// \note either use read_dir_reset() followed by read_dir_next() calls, or call
	    /// read_dir_reset_dirinfo() followed by read_dir_next_dirinfo() calls
	virtual bool read_dir_next(std::string & filename) const = 0;


	    /// routines to read existing files with dir information

	    /// to be used before calling read_dir_next_dirinfo().
	    /// \note after calling read_dir_reset_dirinfo() calling read_dir_next() gives undefined result,
	    /// you first need to call read_dir_reset() or continue using read_dir_next_dirinfo()
	virtual void read_dir_reset_dirinfo() const = 0;


	    /// alternative to the method read_dir_next, should be implemented also

	    /// \param[out] filename name of the next entry in the directory, (valid only if this method returned true)
	    /// \param[out] tp gives the nature of the entry
	    /// \note a call to read_dir_reset_dirinfo() should be done before the first call to this method
	    /// \note either use read_dir_reset() followed by read_dir_next() calls, or call
	    /// read_dir_reset_dirinfo() followed by read_dir_next_dirinfo() calls
	virtual bool read_dir_next_dirinfo(std::string & filename, inode_type & tp) const = 0;

	    /// create a new directory in the current directory

	    /// \param[in] dirname is the name of the subdirectory to create (not its path!) It is created in as sub-directory of the directory given to set_location()
	    /// \param[in] permission is the usual POSIX user/group/other permission bits to set to the directory to create
	    /// \note the operation fails if an entry of that name already exists
	    /// \note the implementation should set the user and group ownership according to the
	    /// argument provided to set_user_ownership() and set_group_ownership(), if this feature
	    /// is supported in the underlying implementation
	virtual void create_dir(const std::string & dirname, U_I permission) = 0;

	    /// remove the target file from the entrepot
	void unlink(const std::string & filename) const { inherited_unlink(filename); }; ///< done this way for homogeneity with open/inherited_open

	    /// generate a clone of "this"

	    /// \deprecated this method will disapear in the future it is only kept
	    /// there to allow the APIv5 adaptation layer to work over APIv6
	virtual entrepot *clone() const = 0;

    protected:
	virtual fichier_global *inherited_open(const std::shared_ptr<user_interaction> & dialog,     ///< for user interaction
					       const std::string & filename,  ///< filename to open
					       gf_mode mode,                  ///< mode to use
					       bool force_permission,         ///< set the permission of the file to open
					       U_I permission,                ///< value of the permission to assign when force_permission is true
					       bool fail_if_exists,           ///< whether to fail if file exists (write mode)
					       bool erase                     ///< whether to erase file if file already exists (write mode)
	    ) const = 0;

	virtual void inherited_unlink(const std::string & filename) const = 0;

	virtual void read_dir_flush() const = 0; ///< ends the read_dir_next, (no more entry available)

    private:
	path where;
        path root;
	std::string user;
	std::string group;
    };

	/// @}

} // end of namespace

#endif
