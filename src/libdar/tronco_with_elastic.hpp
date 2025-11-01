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

    /// \file tronco_with_elastic.hpp
    /// \brief encryption layer with embedded elastic buffer

    /// it relies on tronconneuse or parallele_tronconneuse for the encryption
    /// and adds/fetches an elastic buffer at the beginning and at the end of the
    /// encrypted data to provide what's in between, which is the real encrypted data
    /// \ingroup Private
    ///

#ifndef TRONCO_WITH_ELASTIC_HPP
#define TRONCO_WITH_ELASTIC_HPP

#include "../my_config.h"
#include <string>

#include "tronconneuse.hpp"
#include "parallel_tronconneuse.hpp"
#include "elastic.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{


	/// this class abstracts encryption (tronconneuse & parallel_tronconneuse) adding elastic buffers a the extremities

    class tronco_with_elastic : public generic_file
    {
    public:
	    /// This is the constructor

	    /// \param[in] block_size is the size of block encryption (the size of clear data encrypted together).
	    /// \param[in] encrypted_side where encrypted data are read from or written to.
	    /// \param[in] reading_ver version of the archive format
	    /// \param[in] ptr pointer to a crypto_module object that will be passed to the tronco_with_elastic object
	    /// \note that encrypted_side is not owned and destroyed by tronco_with_elastic, it must exist during all the life of the
	    /// tronco_with_elastic object, and is not destroyed by the tronco_with_elastic's destructor
	tronco_with_elastic(U_32 block_size,
		     generic_file & encrypted_side,
		     const archive_version & reading_ver,
		     std::unique_ptr<crypto_module> & ptr);

	    /// copy constructor
	tronco_with_elastic(const tronco_with_elastic & ref);

	    /// move constructor
	tronco_with_elastic(tronco_with_elastic && ref) noexcept;

	    /// assignment operator
	tronco_with_elastic & operator = (const tronco_with_elastic & ref);

	    /// move operator
	tronco_with_elastic & operator = (tronco_with_elastic && ref);

	    /// destructor
	virtual ~tronco_with_elastic() noexcept override;

	    /// inherited from generic_file
	virtual bool skippable(skippability direction, const infinint & amount) override;
	    /// inherited from generic_file
	virtual bool skip(const infinint & pos) override;
	    /// inherited from generic_file
	virtual bool skip_to_eof() override;
	    /// inherited from generic_file
	virtual bool skip_relative(S_I x) override;
	    /// inherited from generic_file
	virtual bool truncatable(const infinint & pos) const override;
	    /// inherited from generic_file
	virtual infinint get_position() const override;

	    /// necessary before calling write,

	    /// \param[in] initial_shift defines the offset before which clear data is stored encrypted_side

	    /// \note this call adds an elastic buffer at the provided offset and shifts the writing
	    /// position right after it.
	void get_read_for_writing(const infinint & initial_shift);

	    /// in write_only mode indicate that end of file is reached

	    /// this call must be called in write mode to add final elastic buffer then purge the
	    /// internal cache before deleting the object (else some data may be lost)
	    /// no further write call is allowed
	    /// \note this call cannot be used from the destructor, because it relies on pure virtual methods
	virtual void write_end_of_file();

    private:
	enum { init, reading, writing, closed } status;

    };

	/// @}

} // end of namespace

#endif
