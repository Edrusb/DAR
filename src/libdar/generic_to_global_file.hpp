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

    /// \file generic_to_global_file.hpp
    /// \brief this class provides an fichier_global interface for any type of generic_file object
    ///
    /// the main use is to be able to have hash_fichier working on other type of objects
    /// than fichier_global. The drawback is that all file specific operation is ignored
    /// (file ownership, permission...). The other point is the get_size() method which returns
    /// the current position of the read or write cursor, not the total available amount of data
    /// \ingroup Private

#ifndef GENERIC_TO_GLOBAL_FILE_HPP
#define GENERIC_TO_GLOBAL_FILE_HPP

#include "fichier_global.hpp"
#include "memory_file.hpp"
#include "storage.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

    class generic_to_global_file : public fichier_global
    {
    public:

	    /// Constructors & Destructor
	    ///
	    /// \param[in] dialog for user interaction
	    /// \param[in] d pointer to the generic_file object to provide a fichier_global interface to.
	    /// \note the pointed to generic_file object must survive the whole live of the generic_to_gloabl_file. This generic_file is not owned nor deleted by the generic_to_global_file object that points to it.
	generic_to_global_file(user_interaction & dialog, generic_file *d, gf_mode mode): fichier_global(dialog, mode) { if(d == nullptr) throw SRC_BUG; if(d->get_mode() != gf_read_write && d->get_mode() != mode) throw SRC_BUG; data = d; };

	generic_to_global_file(const generic_to_global_file & ref) = default;
	generic_to_global_file & operator = (const generic_to_global_file & ref) = default;
	~generic_to_global_file() = default;

	    // virtual method inherited from generic_file
	bool skippable(skippability direction, const infinint & amount) { return data->skippable(direction, amount); }
	bool skip(const infinint & pos) { return data->skip(pos); };
	bool skip_to_eof() { return data->skip_to_eof(); };
	bool skip_relative(S_I x) { return data->skip_relative(x); };
	infinint get_position() const { return data->get_position(); };


	    // virtual method inherited from fichier_global
	void change_ownership(const std::string & user, const std::string & group) {};
	void change_permission(U_I perm) {};
	infinint get_size() const { return data->get_position(); }; //< yes, this is the drawback of this template class convertion, get_size() does not return the real size of the object
	void fadvise(advise adv) const {};


    protected:

	    // virtual method inherited from generic_file
	void inherited_read_ahead(const infinint & amount) {}; // no optimization can be done here, we rely on the OS here
	void inherited_sync_write() {};
	void inherited_flush_read() {};
	void inherited_terminate() {};

	    // inherited from fichier_global
	U_I fichier_global_inherited_write(const char *a, U_I size) { data->write(a, size); return size; };
	bool fichier_global_inherited_read(char *a, U_I size, U_I & read, std::string & message) { read = data->read(a, size); message = "THIS IS A BUG IN GENERIC_TO_GLOBAL_FILE, PLEASE REPORT TO THE MAINTAINER!"; return true; };

    private:
	generic_file *data;
    };

	/// @}

} // end of namespace

#endif
