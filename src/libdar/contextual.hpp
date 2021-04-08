/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2021 Denis Corbin
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

    /// \file contextual.hpp
    /// \brief class contextual adds the information of phases in the generic_file
    /// \ingroup Private


#ifndef CONTEXTUAL_HPP
#define CONTEXTUAL_HPP


#include "../my_config.h"

extern "C"
{
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
} // end extern "C"

#include "erreurs.hpp"
#include "label.hpp"

#include <string>

namespace libdar
{

#define CONTEXT_INIT "init"
#define CONTEXT_OP   "operation"
#define CONTEXT_LAST_SLICE  "last_slice"


	/// \addtogroup Private
	/// @{

	/// the contextual class adds the information of phases in the generic_file

	/// several phases are defined like for example
	/// - INIT phase
	/// - OPERATIONAL phase
	/// - LAST SLICE phase
	/// .
	/// these are used to help the command launched between slices to
	/// decide the action to do depending on the context when reading an archive
	/// (first slice / last slice read, ...)
	/// the context must also be transfered to dar_slave through the pair of tuyau objects
	///
	/// this class also support some additional informations common to all 'level1' layer of
	/// archive, such as:
	/// - the data_name information
	///

    class contextual
    {
    public :
	contextual() { status = ""; };
	contextual(const contextual & ref) = default;
	contextual(contextual && ref) noexcept = default;
	contextual & operator = (const contextual & ref) = default;
	contextual & operator = (contextual && ref) noexcept = default;
        virtual ~contextual() noexcept(false) {};

	    /// defines the new contextual value

	    /// \note inherited class may redefine this call but
	    /// but must call the parent method to set the value
	    /// contextual:set_info_status()
	virtual void set_info_status(const std::string & s) { status = s; };

	    /// get the current contextual value
        virtual std::string get_info_status() const { return status; };

	    /// returns whether the archive is a old archive (format < 8)
	virtual bool is_an_old_start_end_archive() const = 0;

	    /// obtain the data_name of the archive (label associated with the archive's data)

	    /// \note label are conserved with dar_xform and archive isolation, but are
	    /// not with archive merging or archive creation (full or differential backup)
	virtual const label & get_data_name() const = 0;

    private:
	std::string status;
    };

	/// @}

} // end of namespace

#endif
