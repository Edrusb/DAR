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

    /// \file tronc.hpp
    /// \brief defines a limited segment over another generic_file.
    /// \ingroup Private
    ///
    /// This is used to read a part of a file as if it was a real file generating
    /// end of file behavior when reaching the given length.

#ifndef TRONC_HPP
#define TRONC_HPP

#include "../my_config.h"
#include "infinint.hpp"
#include "generic_file.hpp"

namespace libdar
{
	/// \addtogroup Private
	/// @{

	/// makes a segment of a generic_file appear like a real generic_file

    class tronc : public generic_file
    {
    public :
	    /// constructor

	    /// \param f is the file to take the segment from
	    /// \param offset is the position of the beginning of the segment
	    /// \param size is the size of the segment
	    /// \param own_f is true if this object has to own and must destroy the 'f' object at tronc's destruction time
        tronc(generic_file *f, const infinint &offset, const infinint &size, bool own_f = false);
        tronc(generic_file *f, const infinint &offset, const infinint &size, gf_mode mode, bool own_f = false);

	    /// other constructor, the end of the segment is the end of the underlying generic_file
	    /// only data before offset is inaccessible
        tronc(generic_file *f, const infinint &offset, bool own_f = false);
        tronc(generic_file *f, const infinint &offset, gf_mode mode, bool own_f = false);

	tronc(const tronc & ref) = delete;
	tronc(tronc && ref) noexcept = delete;
	tronc & operator = (const tronc & ref) = delete;
	tronc & operator = (tronc && ref) = delete;

	    /// destructor
	~tronc() { detruit(); };

	    /// modify the tronc object to zoom on another (size limited) portion of the underlying object
	void modify(const infinint & new_offset, const infinint & new_size);
	    /// modify the tronc object to zoom on another (size unlimited) portion of the underlying object
	void modify(const infinint & new_offset);
	    /// modify the tronc object to become transparent and allow unrestricted access to the underlyuing object
	void modify() { modify(0); };

	    /// inherited from generic_file
	virtual bool skippable(skippability direction, const infinint & amount) override;
	    /// inherited from generic_file
        virtual bool skip(const infinint & pos) override;
	    /// inherited from generic_file
        virtual bool skip_to_eof() override;
	    /// inherited from generic_file
        virtual bool skip_relative(S_I x) override;
	    /// inherited from generic_file
	virtual bool truncatable(const infinint & pos) const override { return ref->truncatable(start + pos); };
        virtual infinint get_position() const override { return current; };

	    /// when a tronc is used over a compressor, it becomes necessary to disable position check
	    ///
	    /// \note by default, before each read or write, the tronc object checks that the underlying
	    /// object is at adhoc position in regard to where the cursor is currently in the tronc. Disabling
	    /// that check let ignore possible position mismatch (which are normal when a compressor is found below)
	    /// while reading or writing but keep seeking the underlying object to the requested position upon any call
	    /// to tronc::skip_* familly methods.
	void check_underlying_position_while_reading_or_writing(bool mode) { check_pos = mode ; };


    protected :
	    /// inherited from generic_file
	virtual void inherited_read_ahead(const infinint & amount) override;
	    /// inherited from generic_file
        virtual U_I inherited_read(char *a, U_I size) override;
	    /// inherited from generic_file
        virtual void inherited_write(const char *a, U_I size) override;
	virtual void inherited_truncate(const infinint & pos) override;
	virtual void inherited_sync_write() override { ref->sync_write(); }
	virtual void inherited_flush_read() override {};
	virtual void inherited_terminate() override {if(own_ref) ref->terminate(); };

    private :
        infinint start;    ///< offset in the global generic file to start at
	infinint sz;       ///< length of the portion to consider
        generic_file *ref; ///< global generic file of which to take a piece
        infinint current;  ///< inside position of the next read or write
	bool own_ref;      ///< whether we own ref (and must destroy it when no more needed)
	bool limited;      ///< whether the sz argument is to be considered
	bool check_pos;    ///< whether to check and eventually adjust (seek) the position of the underlying layer at each read or write

	void set_back_current_position();
	void detruit() noexcept { if(own_ref && ref != nullptr) delete ref; };
    };

	/// @}

} // end of namespace

#endif
