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

    /// \file crc.hpp
    /// \brief class crc definition, used to handle Cyclic Redundancy Checks
    /// \ingroup Private

#ifndef CRC_HPP
#define CRC_HPP

#include "../my_config.h"

#include <string>
#include <list>
#include "integers.hpp"
#include "storage.hpp"
#include "infinint.hpp"
#include "on_pool.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

    class crc : public on_pool
    {
    public:
	static const U_I OLD_CRC_SIZE = 2;

	crc() = default;
	crc(const crc & ref) = default;
	crc & operator = (const crc & ref) = default;
	virtual ~crc() throw(Ebug) {};

	virtual bool operator == (const crc & ref) const = 0;
	bool operator != (const crc & ref) const { return ! (*this == ref); };

	virtual void compute(const infinint & offset, const char *buffer, U_I length) = 0;
	virtual void compute(const char *buffer, U_I length) = 0; // for sequential read only
	virtual void clear() = 0;
	virtual void dump(generic_file & f) const = 0;
	virtual std::string crc2str() const = 0;
	virtual infinint get_size() const = 0;
	virtual crc *clone() const = 0;
    };

    extern crc *create_crc_from_file(generic_file & f, memory_pool *pool, bool old = false);
    extern crc *create_crc_from_size(infinint width, memory_pool *pool);

    class crc_i : public crc
    {
    public:
	crc_i(const infinint & width);
	crc_i(const infinint & width, generic_file & f);
	crc_i(const crc_i & ref) : size(ref.size), cyclic(ref.size) { copy_data_from(ref); pointer = cyclic.begin(); };
	crc_i & operator = (const crc_i & ref) { copy_from(ref); return *this; };
	~crc_i() = default;

	bool operator == (const crc & ref) const;

	virtual void compute(const infinint & offset, const char *buffer, U_I length) override;
	virtual void compute(const char *buffer, U_I length) override; // for sequential read only
	virtual void clear() override;
	virtual void dump(generic_file & f) const override;
	virtual std::string crc2str() const override;
	virtual infinint get_size() const override { return size; };

    protected:
	virtual crc *clone() const override { return new (get_pool()) crc_i(*this); };

    private:

	infinint size;                              //< size of the checksum
	storage::iterator pointer;                  //< points to the next byte to modify
	storage cyclic;                             //< the checksum storage

	void copy_from(const crc_i & ref);
	void copy_data_from(const crc_i & ref);
    };


    class crc_n : public crc
    {
    public:

	crc_n(U_I width);
	crc_n(U_I width, generic_file & f);
	crc_n(const crc_n & ref) { copy_from(ref); };
	crc_n & operator = (const crc_n & ref);
	~crc_n() { destroy(); };

	bool operator == (const crc & ref) const;

	virtual void compute(const infinint & offset, const char *buffer, U_I length) override;
	virtual void compute(const char *buffer, U_I length) override; // for sequential read only
	virtual void clear() override;
	virtual void dump(generic_file & f) const override;
	virtual std::string crc2str() const override;
	virtual infinint get_size() const override { return size; };

    protected:
	virtual crc *clone() const override { return new (get_pool()) crc_n(*this); };

    private:

	U_I size;                                   //< size of checksum (non infinint mode)
	unsigned char *pointer;                     //< points to the next byte to modify (non infinint mode)
	unsigned char *cyclic;                      //< the checksum storage (non infinint mode)

	void alloc(U_I width);
	void copy_from(const crc_n & ref);
	void copy_data_from(const crc_n & ref);
	void destroy();
    };


	/// @}

} // end of namespace


#endif
