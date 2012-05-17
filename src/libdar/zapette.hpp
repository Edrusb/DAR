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
// $Id: zapette.hpp,v 1.25 2011/04/17 13:12:30 edrusb Rel $
//
/*********************************************************************/
//

    /// \file zapette.hpp
    /// \brief remote control between dar and dar_slave.
    /// \ingroup Private
    ///
    /// Two classes are defined in this module
    /// - zapette is the dar side master class
    /// - slave_zapette dar_slave side
    /// .
    /// these two classes communicate throw a pair pipes

#ifndef ZAPETTE_HPP
#define ZAPETTE_HPP

#include "../my_config.h"
#include "infinint.hpp"
#include "generic_file.hpp"
#include "integers.hpp"

namespace libdar
{


	/// \addtogroup Private
	/// @{

	/// zapette emulate a file that is remotely controlled by slave_zapette

	/// class zapette sends order to slave_zapette throw a
	/// a first pipe and receive informations or data in return
	/// from a second pipe from slave_zapette
	/// \ingroup Private
    class zapette : public generic_file, public contextual, protected mem_ui
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
        bool skip_to_eof() { if(is_terminated()) throw SRC_BUG; position = file_size; return true; };
        bool skip_relative(S_I x);
        infinint get_position() { if(is_terminated()) throw SRC_BUG; return position; };

	    // overwritten inherited methods from contextual
        void set_info_status(const std::string & s);
	bool is_an_old_start_end_archive() const;
	const label & get_data_name() const;

    protected:
        U_I inherited_read(char *a, U_I size);
        void inherited_write(const char *a, U_I size);
	void inherited_sync_write() {};
	void inherited_terminate();

    private:
        generic_file *in, *out;
        infinint position, file_size;
        char serial_counter;

	    /// wrapped formatted method to communicate with the slave_zapette located behind the pair of pipes (= tuyau)

	    /// \param[in] size is the size of the amount of data we want the zapette to send us
	    /// \param[in] offset is the byte offset of the portion of the data we want
	    /// \param[in,out] data is the location where to return the requested data
	    /// \param[in] info the new contextual string to set to the slave_zapette.
	    /// \param[out] lu the amount of byte wrote to '*data'
	    /// \param[out] arg infinint value return for special order (see note below).
	    /// \note with default parameters, this method permits the caller to get a portion of data from the
	    /// remote slave_zapette. In addition, it let the caller change the 'contextual' status of the remote object.
	    /// if size is set to REQUEST_SPECIAL_ORDER, the offset is used to transmit a special order to the
	    /// remote slave_zapette. Defined order are for example REQUEST_OFFSET_END_TRANSMIT , REQUEST_OFFSET_GET_FILESIZE,
	    /// and so on (see at the beginning of zapette.cpp file for more). Each of these order may expect a returned value
	    /// which may be an integer (provided by the "arg" argument of this call)  a boolean value (provided by the "arg"
	    /// argument where 0 means false and 1 means true) or a char * (first byte to put the answer to is given by 'data' and
	    /// allocated space for the reply must be given through 'lu' which at return given the effective length of the returned
	    /// string

	void make_transfert(U_16 size, const infinint &offset, char *data, const std::string & info, S_I & lu, infinint & arg) const;
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
	    /// \param[in] data is where the informations or data is taken from. Object must inherit from contextual
        slave_zapette(generic_file *input, generic_file *output, generic_file *data);
        ~slave_zapette();


	    /// main execution method for slave_zapette class

	    /// this method implement a loop waiting for orders and answering to them
	    /// the loop stops when a special order is received from the peer zapette object
        void action();

    private:
        generic_file *in;     //< where to read orders from
	generic_file *out;    //< where to send requested info or data to
	generic_file *src;    //< where to read data from
	contextual *src_ctxt; //< same as src but seen as contextual
    };

	/// @}

} // end of namespace

#endif
