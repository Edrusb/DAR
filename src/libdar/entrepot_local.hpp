/*********************************************************************/
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

    /// \file entrepot_local.hpp
    /// \brief defines the implementation for local filesystem entrepot
    /// The entrepot_local correspond to the local filesystems.
    /// \ingroup Private

#ifndef ENTREPOT_LOCAL_HPP
#define ENTREPOT_LOCAL_HPP

#include "../my_config.h"

#include <string>
#include "user_interaction.hpp"
#include "entrepot.hpp"
#include "fichier_global.hpp"
#include "etage.hpp"

namespace libdar
{
	/// \addtogroup Private
	/// @{

	/// implementation for entrepot to access to local filesystem
	///
	/// entrepot_local generates objects of class "fichier_local" inherited class of fichier_global

    class entrepot_local : public entrepot
    {
    public:
	entrepot_local(const std::string & user, const std::string & group, bool x_furtive_mode);
	entrepot_local(const entrepot_local & ref): entrepot(ref) { copy_from(ref); };
	entrepot_local(entrepot_local && ref) noexcept: entrepot(std::move(ref)) { nullifyptr(); move_from(std::move(ref)); };
	entrepot_local & operator = (const entrepot_local & ref);
	entrepot_local & operator = (entrepot_local && ref) noexcept { entrepot::operator = (std::move(ref)); move_from(std::move(ref)); return *this; };
	~entrepot_local() { detruit(); };

	virtual std::string get_url() const override { return std::string("file://") + get_full_path().display(); };

	virtual void read_dir_reset() const override;
	virtual bool read_dir_next(std::string & filename) const override;

	virtual entrepot *clone() const override { return new (std::nothrow) entrepot_local(*this); };

    protected:
	virtual fichier_global *inherited_open(const std::shared_ptr<user_interaction> & dialog,
					       const std::string & filename,
					       gf_mode mode,
					       bool force_permission,
					       U_I permission,
					       bool fail_if_exists,
					       bool erase) const override;

	virtual void inherited_unlink(const std::string & filename) const override;
	virtual void read_dir_flush() override { detruit(); };

    private:
	bool furtive_mode;
	etage *contents;

	void nullifyptr() noexcept { contents = nullptr; };
	void copy_from(const entrepot_local & ref) { furtive_mode = ref.furtive_mode; contents = nullptr; };
	void move_from(entrepot_local && ref) noexcept { std::swap(contents, ref.contents), std::swap(furtive_mode, ref.furtive_mode); };
	void detruit() { if(contents != nullptr) { delete contents; contents = nullptr; } };
    };

	/// @}

} // end of namespace

#endif
