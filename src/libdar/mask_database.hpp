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

    /// \file mask_database.hpp
    /// \brief mask_database class is a dar_manager database based mask class
    ///
    /// This class let the restoration of a data spaws among different full
    /// and different backups loaded to a dar_manager database. The difference
    /// with using dar_manager is that you have two processes, dar_manager and
    /// dar processes, and in between a long list of mask to generate on one side
    /// and interpret on the other side (TLV format see dar's --pipe-fd option)
    ///
    /// The objective here is first to have a single process (dar for CLI) that
    /// get reads the dar_manager database and restores *all* the data and only
    /// the necessary data (not restoring data of a full backup if a more recent
    /// backup has it saved), this means the process will have to restore several
    /// backups (which this class will provide information about) and for each
    /// this mask class will select only the necesary data (latest version) to
    /// be restored
    ///
    /// \ingroup API

#ifndef MASK_DATABASE_HPP
#define MASK_DATABASE_HPP

#include "../my_config.h"

extern "C"
{
} // end extern "C"


#include "mask.hpp"
#include "data_dir.hpp"
#include "datetime.hpp"
#include "restore_tree.hpp"


    /// class mask_database

    /// given a relative path to is_covered(), returns whether this file should be restored or not

    /// the value returned by is_covered() depends on the dar_manager database it has been generated from
    /// and the current archive number under restoration process, archive number which is set calling set_focus()

namespace libdar
{

    class mask_database: public mask
    {
    public:
	    /// mask_database constructor

	    /// \param[in] racine, taken from a i_dabase object (thus this object is only usable from that class)
	    /// \param[in] fs_root, path where data will be restored and path used as prefix by the caller of is_covered()
	    /// \param[in] ignore_older_than_that mask takes into account that version more recent that this date will be ignored
	    /// \note, depending on the value given to set_focus() this mask's is_covered()
	    /// will select only the file which data is the most recent for the focused archive
	    /// this avoids restoring a given file's data for archive N when archive M (where M > N) has
	    /// more recent data that will be restored when the focus will be set to M.
	mask_database(const data_dir* racine,
		      const path & fs_root,
		      const datetime & ignore_older_than_that);
	mask_database(const mask_database & ref) = default;
	mask_database(mask_database && ref) noexcept = default;
	mask_database & operator = (const mask_database & ref) = default;
	mask_database & operator = (mask_database && ref) noexcept = default;
        virtual ~mask_database() = default; // base is not

	    /// condition the mask to return is_covered for this provided archive num
	void set_focus(archive_num focus) const { zoom = focus; };

	    /// inherited from class mask
        virtual bool is_covered(const std::string &expression) const override;

	    /// inherited from class mask

	    /// \note this class is only used inside database::restore() method
	    /// where the dump() method is not used
	virtual std::string dump(const std::string & prefix = "") const override { return "database mask"; };

	    /// inherited from class mask
	virtual mask_database *clone() const override { return new (std::nothrow) mask_database(*this); };

    private:

	std::string fs_racine;              ///< prefix to reach the root where restoration will take place
	mutable archive_num zoom;           ///< the archive to focus on
	std::shared_ptr<restore_tree> tree; ///< contains all info to define archive set to use to restore a given path/file

    };


} // end of namespace

#endif
