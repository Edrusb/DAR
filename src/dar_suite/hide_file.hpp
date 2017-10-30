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

    /// \file hide_file.hpp
    /// \brief contains class of base to split files in words
    /// \ingroup CMDLINE

#ifndef HIDE_FILE_HPP
#define HIDE_FILE_HPP

#include "../my_config.h"
#include <vector>
#include "infinint.hpp"
#include "generic_file.hpp"

using namespace libdar;
using namespace std;

    /// \addtogroup CMDLINE
    /// @{

class hide_file : public generic_file
{
public:
    hide_file(generic_file &f);
    hide_file(const hide_file & ref) = default;
    hide_file & operator = (const hide_file & ref) = default;
    ~hide_file() = default;

    virtual bool skippable(skippability direction, const infinint & amount) override { return true; };
    virtual bool skip(const infinint & pos) override;
    virtual bool skip_to_eof() override;
    virtual bool skip_relative(S_I x) override;
    virtual infinint get_position() const override;

protected:
    struct partie
    {
        infinint debut, longueur; // debut is the offset in ref file
        infinint offset; // offset in the resulting file
    };

    vector <partie> morceau;
    generic_file *ref;

    virtual void inherited_read_ahead(const infinint & amount) override { ref->read_ahead(amount); };
    virtual U_I inherited_read(char *a, U_I size) override;
    virtual void inherited_write(const char *a, size_t size) override;
    virtual void inherited_sync_write() override {};
    virtual void inherited_flush_read() override {};
    virtual void inherited_terminate() override {};

    virtual void fill_morceau() = 0;
        // the inherited classes have with this method
        // to fill the "morceau" variable that defines
        // the portions

private:
    U_I pos_index;
    infinint pos_relicat;
    bool is_init;

    void init() const;
};

    /// @}


#endif
