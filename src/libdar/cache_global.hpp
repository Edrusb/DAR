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

    /// \file cache_global.hpp
    /// \brief adaptation of the cache class to the fichier_global interface
    /// \ingroup Private

#ifndef CACHE_GLOBAL_HPP
#define CACHE_GLOBAL_HPP

#include "../my_config.h"
#include "cache.hpp"
#include "fichier_global.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

	/// the cache_global cache is an adaptation of the cache class to the fichier_global interface

    class cache_global: public fichier_global
    {
    public:
	static const U_I default_cache_size = 102400;

	    /// constructor

	    /// \param[in] dialog for user interaction requested by fichier_global
	    /// \param[in] x_ptr the hidden/underlying generic_file to provide caching feature for
	    /// \param[in] shift_mode see cache class constructor for details
	    /// \param[in] size cache size
	    /// \note the object pointed to by x_ptr passed under the responsibility of the cache_global object,
	    /// it will be automatically deleted when no more needed
	cache_global(const std::shared_ptr<user_interaction> & dialog, fichier_global *x_ptr, bool shift_mode, U_I size = default_cache_size);

	    /// copy constructor
	cache_global(cache_global & ref) = delete;

	    /// move constructor
	cache_global(cache_global && ref) = delete;

	    /// assignment operator
	cache_global & operator = (const cache_global & ref) = delete;

	    /// move assignment operator
	cache_global & operator = (cache_global && ref) = delete;

	    /// destructor
	~cache_global() { detruit(); };

	    // inherited from fichier_global

	virtual void change_ownership(const std::string & user, const std::string & group) override { ptr->change_ownership(user, group); };
	virtual void change_permission(U_I perm) override { ptr->change_permission(perm); };
        virtual infinint get_size() const override { return ptr->get_size(); };
	virtual void fadvise(advise adv) const override { ptr->fadvise(adv); };


	    // inherited from generic_file grand-parent class

	virtual bool skippable(skippability direction, const infinint & amount) override { return buffer->skippable(direction, amount); };
        virtual bool skip(const infinint & pos) override { return buffer->skip(pos); };
        virtual bool skip_to_eof() override { return buffer->skip_to_eof(); };
        virtual bool skip_relative(S_I x) override { return buffer->skip_relative(x); };
	virtual bool truncatable(const infinint & pos) const override { return buffer->truncatable(pos); };
        virtual infinint get_position() const override { return buffer->get_position(); };

	    // expose cache specific methods

	void change_to_read_write() { buffer->change_to_read_write(); };

    protected:

	    // inherited from fichier_global

	virtual U_I fichier_global_inherited_write(const char *a, U_I size) override { buffer->write(a, size); return size; };
	virtual bool fichier_global_inherited_read(char *a, U_I size, U_I & read, std::string & message) override { read = buffer->read(a, size); return true; };

	    // inherted from generic_file
	virtual void inherited_read_ahead(const infinint & amount) override { buffer->read_ahead(amount); };
	virtual void inherited_truncate(const infinint & pos) override { buffer->truncate(pos); };
	virtual void inherited_sync_write() override { buffer->sync_write(); ptr->sync_write(); };
	virtual void inherited_flush_read() override { buffer->flush_read();  };
	virtual void inherited_terminate() override { buffer->terminate(); ptr->terminate(); };


    private:
	cache *buffer;
	fichier_global *ptr;

	void detruit();
    };

	/// @}

} // end of namespace

#endif

