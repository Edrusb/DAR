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
// to contact the author : dar.linux@free.fr
/*********************************************************************/
// $Id: zapette.hpp,v 1.10 2004/11/07 18:21:39 edrusb Rel $
//
/*********************************************************************/
//

    /// \file zapette.hpp
    /// \brief remote control between dar and dar_slave.
    ///
    /// Two classes are defined in this module
    /// - zapette is the dar side master class
    /// - slave_zapette dar_slave side
    /// .
    /// theses two classes communicate throw a pair pipes

#ifndef ZAPETTE_HPP
#define ZAPETTE_HPP

#include "../my_config.h"
#include "infinint.hpp"
#include "generic_file.hpp"
#include "integers.hpp"

namespace libdar
{

	/// zapette emulate a file that is remotely controlled by slave_zapette

	/// class zapette sends order to slave_zapette throw a
	/// a first pipe and receive informations or data in return
	/// from a second pipe from slave_zapette
	/// \ingroup Private
    class zapette : public contextual
    {
    public:

	    /// zapette constructor

	    /// \param[in] dialog is used to return status information to the user
	    /// \param[in] input is the pipe (see class tuyau) from which is received the information or data
	    /// \param[in] output is used to send orders to slave_zapette
        zapette(user_interaction & dialog, generic_file *input, generic_file *output);
        ~zapette();

            // inherited methods from generic_file
        bool skip(const infinint &pos);
        bool skip_to_eof() { position = file_size; return true; };
        bool skip_relative(S_I x);
        infinint get_position() { return position; };

	    // inherited methods from contextual
        void set_info_status(const std::string & s);
        std::string get_info_status() const { return info; };

    protected:
        S_I inherited_read(char *a, size_t size);
        S_I inherited_write(char *a, size_t size);

    private:
        generic_file *in, *out;
        infinint position, file_size;
        char serial_counter;
	std::string info;

	void make_transfert(U_16 size, const infinint &offset, char *data, const std::string & info, S_I & lu, infinint & arg);
    };

	/// this class answers to order given by a zapette object

	/// through a pair of pipes slave_zapette return information about
	/// a given local archive (single or multi slices).
	/// \ingroup Private
    class slave_zapette
    {
    public:

	    /// slave_zapette constructor

	    /// \param[in] input is used to receive orders from an zapette object
	    /// \param[in] output is used to return informations or data in answer to received orders
	    /// \param[in] data is where the informations or data is taken from
        slave_zapette(generic_file *input, generic_file *output, contextual *data);
        ~slave_zapette();


	    /// main execution method for slave_zapette class

	    /// this method implement a loop waiting for orders and answering to them
	    /// the loop stops when a special order is received from the peer zapette object
        void action();

    private:
        generic_file *in, *out;
	contextual *src;
    };

} // end of namespace

#endif
