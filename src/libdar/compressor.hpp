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

    /// \file compressor.hpp
    /// \brief compression engine implementation
    /// \ingroup Private

#ifndef COMPRESSOR_HPP
#define COMPRESSOR_HPP

#include "../my_config.h"

#include "infinint.hpp"
#include "integers.hpp"
#include "wrapperlib.hpp"
#include "proto_compressor.hpp"

namespace libdar
{


	/// \addtogroup Private
	/// @{

	/// compression class for gzip and bzip2 algorithms
    class compressor : public proto_compressor
    {
    public :
        compressor(compression algo, generic_file & compressed_side, U_I compression_level = 9);
            // compressed_side is not owned by the object and will remains (must survive)
            // after the objet destruction

	compressor(const compressor & ref) = delete;
	compressor(compressor && ref) noexcept = delete;
	compressor & operator = (const compressor & ref) = delete;
	compressor & operator = (compressor && ref) noexcept = delete;
        ~compressor();

        virtual compression get_algo() const override;
	virtual void suspend_compression() override;
	virtual void resume_compression() override;
	virtual bool is_compression_suspended() const override { return suspended; };


            // inherited from generic file
	virtual bool skippable(skippability direction, const infinint & amount) override { return compressed->skippable(direction, amount); };
        virtual bool skip(const infinint & pos) override { inherited_sync_write(); inherited_flush_read(); return compressed->skip(pos); };
        virtual bool skip_to_eof() override { inherited_sync_write(); inherited_flush_read(); return compressed->skip_to_eof(); };
        virtual bool skip_relative(S_I x) override { inherited_sync_write(); inherited_flush_read(); return compressed->skip_relative(x); };
	virtual bool truncatable(const infinint & pos) const override { return compressed->truncatable(pos); };
        virtual infinint get_position() const override { return compressed->get_position(); };

    protected :
	virtual void inherited_read_ahead(const infinint & amount) override { compressed->read_ahead(amount); };
        virtual U_I inherited_read(char *a, U_I size) override;
        virtual void inherited_write(const char *a, U_I size) override;
	virtual void inherited_truncate(const infinint & pos) override;
	virtual void inherited_sync_write() override;
	virtual void inherited_flush_read() override;
	virtual void inherited_terminate() override;

    private :
        struct xfer
        {
            wrapperlib wrap;
            char *buffer;
            U_I size;

            xfer(U_I sz, wrapperlib_mode mode);
            ~xfer();
        };

        xfer *compr, *decompr;     ///< datastructure for bzip2, gzip and zx compression

        generic_file *compressed;
        compression current_algo;
	bool suspended;
	U_I current_level;

        void init(compression algo, generic_file *compressed_side, U_I compression_level);
	void reset_compr_engine();    ///< reset the compression engine ready for use
        void clean_write(); // discard any byte buffered and not yet wrote to compressed_side;
    };

	/// @}

} // end of namespace

#endif
