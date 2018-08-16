/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2018 Denis Corbin
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

    /// \file config_file.hpp
    /// \brief specific routines to manages included files's targets
    /// \ingroup CMDLINE


#ifndef CONFIG_FILE_HPP
#define CONFIG_FILE_HPP

#include "../my_config.h"
#include <deque>
#include "hide_file.hpp"

using namespace libdar;

    /// \addtogroup CMDLINE
    /// @{

class config_file : public hide_file
{
public:
    config_file(const deque<string> & target, generic_file &f);
    config_file(const config_file & ref) = default;
    config_file & operator = (const config_file & ref) = default;
    ~config_file() = default;

    deque<string> get_read_targets() const;

protected:
    virtual void fill_morceau() override;

private:
    struct t_cible
    {
	string target;
	bool seen;

	t_cible(const string & val) { target = val; seen = false; };
	t_cible() { target = ""; seen = false; };
    };

    deque<t_cible> cibles;

    bool is_a_target(const string & val);
    bool find_next_target(generic_file &f, infinint & debut, string & nature, infinint & fin);

};

    /// @}

#endif
