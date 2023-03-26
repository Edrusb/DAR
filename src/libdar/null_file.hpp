/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2023 Denis Corbin
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

    /// \file null_file.hpp
    /// \brief /dev/null type file implementation under the generic_file interface
    /// \ingroup Private
    ///
    /// this class is used in particular when doing dry-run execution

#ifndef NULL_FILE_HPP
#define NULL_FILE_HPP

#include "../my_config.h"
#include "generic_file.hpp"
#include "thread_cancellation.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

	/// the null_file class implements the /dev/null behavior

	/// this is a generic_file implementation that emulate the
	/// comportment of the /dev/null special file.
	/// all that is writen to is lost, and nothing can be read from
	/// it (empty file). This is a completed implementation all
	/// call are consistent.

    class null_file : public generic_file, public thread_cancellation
    {
    public :
        null_file(gf_mode m) : generic_file(m) { set_offset(0); };
	null_file(const null_file & ref) = default;
	null_file(null_file && ref) noexcept = default;
	null_file & operator = (const null_file & ref) = default;
	null_file & operator = (null_file && ref) noexcept = default;
	~null_file() = default;

	virtual bool skippable(skippability direction, const infinint & amount) override { return true; };
        virtual bool skip(const infinint &pos) override { set_offset(pos); return true; };
        virtual bool skip_to_eof() override { offset = max_offset; return true; };
        virtual bool skip_relative(signed int x) override { return set_rel_offset(x); };
	virtual bool truncatable(const infinint & pos) const override { return true; };
        virtual infinint get_position() const override { return offset; };

    protected :
	virtual void inherited_read_ahead(const infinint & amount) override {};

        virtual U_I inherited_read(char *a, U_I size) override
	{
#ifdef MUTEX_WORKS
	    check_self_cancellation();
#endif
	    return 0;
	};

        virtual void inherited_write(const char *a, U_I siz) override
	{
#ifdef MUTEX_WORKS
	    check_self_cancellation();
#endif
	    set_offset(offset + siz);
	};

	virtual void inherited_truncate(const infinint & pos) override { if(pos < offset) offset = pos; };
	virtual void inherited_sync_write() override {};
	virtual void inherited_flush_read() override {};
	virtual void inherited_terminate() override {};

    private:
	infinint offset;
	infinint max_offset;

	void set_offset(const infinint & x)
	{
	    if(x > max_offset)
		max_offset = x;
	    offset = x;
	}

	bool set_rel_offset(signed int x)
	{
	    if(x >= 0)
	    {
		set_offset(offset + x);
		return true;
	    }
	    else // x < 0
	    {
		infinint tmp = -x;
		if(tmp > offset)
		{
		    offset = 0;
		    return false;
		}
		else
		{
		    offset -= tmp;
		    return true;
		}
	    }
	}

    };

	/// @}

} // end of namespace

#endif
