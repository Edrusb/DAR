/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2020 Denis Corbin
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
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

    /// \file slave_zapette.hpp
    /// \brief remote control for dar_slave.
    /// \ingroup Private
    ///
    /// Two objects communicates through a pair of pipes:
    /// - a zapette object is the dar side master class
    /// - a slave_zapette object is dar_slave side
    /// .
    /// they form a communication channel between dar/libdar (zapette side)
    /// and dar_slave/libdar (dar_slave side)


#ifndef SLAVE_ZAPETTE_HPP
#define SLAVE_ZAPETTE_HPP

#include "../my_config.h"
#include "generic_file.hpp"
#include "contextual.hpp"

namespace libdar
{


	/// \addtogroup Private
	/// @{

	/// this class answers to order given by a zapette object

	/// through a pair of pipes slave_zapette return information about
	/// a given local archive (single or multi slices).

    class slave_zapette
    {
    public:

	    /// slave_zapette constructor

	    /// \param[in] input is used to receive orders from an zapette object
	    /// \param[in] output is used to return informations or data in answer to received orders
	    /// \param[in] data is where the informations or data is taken from. Object must inherit from contextual
        slave_zapette(generic_file *input, generic_file *output, generic_file *data);
	slave_zapette(const slave_zapette & ref) = delete;
	slave_zapette(slave_zapette && ref) noexcept = delete;
	slave_zapette & operator = (const slave_zapette & ref) = delete;
	slave_zapette & operator = (slave_zapette && ref) noexcept = delete;
        ~slave_zapette();

	    /// main execution method for slave_zapette class

	    /// this method implement a loop waiting for orders and answering to them
	    /// the loop stops when a special order is received from the peer zapette object
        void action();

    private:
        generic_file *in;     ///< where to read orders from
	generic_file *out;    ///< where to send requested info or data to
	generic_file *src;    ///< where to read data from
	contextual *src_ctxt; ///< same as src but seen as contextual
    };

	/// @}

} // end of namespace

#endif
