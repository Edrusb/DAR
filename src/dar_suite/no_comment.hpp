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
// $Id: no_comment.hpp,v 1.10 2011/01/09 17:25:58 edrusb Rel $
//
/*********************************************************************/

    /// \file no_comment.hpp
    /// \brief contains class that transparently strips out the comments from configuration file
    /// \ingroup CMDLINE



#ifndef NO_COMMENT_HPP
#define NO_COMMENT_HPP

#include "../my_config.h"
#include "hide_file.hpp"

using namespace libdar;

    /// \addtogroup CMDLINE
    /// @{

class no_comment : public hide_file
{
public:
    no_comment(generic_file &f) : hide_file(f) {};

protected:
    void fill_morceau();
};

    // @}

#endif
