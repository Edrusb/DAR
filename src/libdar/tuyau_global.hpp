/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2022 Denis Corbin
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

    /// \file tuyau_global.hpp
    /// \brief seekable pipe on top of another fichier_global
    ///
    /// objects of this class provide the mean to give position and seek forward
    /// features on a underlying system object that only support sequential reads
    /// like pipes and some special devices. The class tuyau does the same but
    /// is a mix of many features and very bundled to the system calls. It would
    /// worth in the future revising class tuyau to remove from it the features now
    /// carried by tuyau_global (to be considered as class zapette also need these
    /// skip()/get_position() emulated features, while it is completely independent
    /// from entrepot class hierarchy).
    /// \ingroup Private

#ifndef TUYAU_GLOBAL_HPP
#define TUYAU_GLOBAL_HPP

#include "../my_config.h"
#include "tuyau.hpp"
#include "fichier_global.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

	/// the tuyau_global provides skip()/get_position() features on top of pipe-like object

    class tuyau_global: public fichier_global
    {
    public:
	    /// constructor

	    /// \param[in] dialog for user interaction requested by fichier_global
	    /// \param[in] x_ptr the hidden/underlying generic_file to provide seekable feature for
	    /// \note the object pointed to by x_ptr passes under the responsibility of the tuyau_global object,
	    /// it will be automatically deleted when no more needed by the tuyau_global object.
	tuyau_global(const std::shared_ptr<user_interaction> & dialog,
		     fichier_global *x_ptr);

	    /// copy constructor
	tuyau_global(tuyau_global & ref) = delete;

	    /// move constructor
	tuyau_global(tuyau_global && ref) = delete;

	    /// assignment operator
	tuyau_global & operator = (const tuyau_global & ref) = delete;

	    /// move assignment operator
	tuyau_global & operator = (tuyau_global && ref) = delete;

	    /// destructor
	~tuyau_global() { detruit(); };

	    // inherited from fichier_global

	virtual void change_ownership(const std::string & user, const std::string & group) override { ptr->change_ownership(user, group); };
	virtual void change_permission(U_I perm) override { ptr->change_permission(perm); };
        virtual infinint get_size() const override { return ptr->get_size(); };
	virtual void fadvise(advise adv) const override { ptr->fadvise(adv); };


	    // inherited from generic_file grand-parent class

	virtual bool skippable(skippability direction, const infinint & amount) override { return ptr->skippable(direction, amount); };
        virtual bool skip(const infinint & pos) override;
        virtual bool skip_to_eof() override;
        virtual bool skip_relative(S_I x) override;
	virtual bool truncatable(const infinint & pos) const override { return ptr->truncatable(pos); };
        virtual infinint get_position() const override { return current_pos; };


    protected:

	    // inherited from fichier_global

	virtual U_I fichier_global_inherited_write(const char *a, U_I size) override;
	virtual bool fichier_global_inherited_read(char *a, U_I size, U_I & read, std::string & message) override;

	    // inherted from generic_file
	virtual void inherited_read_ahead(const infinint & amount) override { ptr->read_ahead(amount); };
	virtual void inherited_truncate(const infinint & pos) override { ptr->truncate(pos); };
	virtual void inherited_sync_write() override { ptr->sync_write(); };
	virtual void inherited_flush_read() override { ptr->flush_read(); };
	virtual void inherited_terminate() override { ptr->terminate(); };


    private:
	static const U_I buffer_size = 102400;

	fichier_global *ptr;             ///< points to the underlying object
	infinint current_pos;            ///< record the current offset
	char buffer[buffer_size];        ///< to skip emulation done by reading data

	void detruit();
	U_I read_and_drop(U_I bytes);
    };

	/// @}

} // end of namespace

#endif

