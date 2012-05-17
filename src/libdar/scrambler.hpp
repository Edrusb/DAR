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
// $Id: scrambler.hpp,v 1.21.2.1 2012/02/19 17:25:09 edrusb Exp $
//
/*********************************************************************/

    /// \file scrambler.hpp
    /// \brief contains the definition of the scrambler class, a very weak encryption scheme
    /// \ingroup Private

#ifndef SCRAMBLER_HPP
#define SCRAMBLER_HPP

#include "../my_config.h"
#include <string>
#include "infinint.hpp"
#include "generic_file.hpp"
#include "erreurs.hpp"
#include "infinint.hpp"
#include "secu_string.hpp"

namespace libdar
{

	/// \brief scrambler is a very weak encryption scheme
	/// \ingroup Private

    class scrambler : public generic_file
    {
    public:
        scrambler(const secu_string & pass, generic_file & hidden_side);
	scrambler(const scrambler & ref) : generic_file(ref) { throw SRC_BUG; };
        ~scrambler() { if(buffer != NULL) delete [] buffer; };

	const scrambler & operator = (const scrambler & ref) { throw SRC_BUG; };

        bool skip(const infinint & pos) { if(ref == NULL) throw SRC_BUG; return ref->skip(pos); };
        bool skip_to_eof() { if(ref==NULL) throw SRC_BUG; return ref->skip_to_eof(); };
        bool skip_relative(S_I x) { if(ref == NULL) throw SRC_BUG; return ref->skip_relative(x); };
        infinint get_position() { if(ref == NULL) throw SRC_BUG; return ref->get_position(); };

#ifdef LIBDAR_SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(scrambler);
#endif
    protected:
        U_I inherited_read(char *a, U_I size);
        void inherited_write(const char *a, U_I size);
	void inherited_sync_write() {}; // nothing to do
	void inherited_terminate() {};  // nothing to do

    private:
        std::string key;
        U_32 len;
        generic_file *ref;
        unsigned char *buffer;
        U_I buf_size;
    };

} // end of namespace

#endif
