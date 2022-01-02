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
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

    /// \file mem_ui.hpp
    /// \brief class mem_ui definition. This class is to be used as parent class to handle user_interaction object management
    /// \ingroup Private

#ifndef MEM_UI_HPP
#define MEM_UI_HPP

#include "../my_config.h"

#include "user_interaction.hpp"
#include <memory>

namespace libdar
{

	/// \addtogroup Private
	/// @{

	/// class mem_ui to keep a copy of a user_interaction object

	/// this class is targeted for inheritance (it is advised to use a "protected"
	/// inheritance, not a "public" one). Instead of having all
	/// the stuf of managing, cloning, releasing a pointer on user_interaction
	/// a class simply put itslef as inherited from mem_ui to take the benefit
	/// of this implementation, once and for all. Use this class with caution
	/// espetially for class which will generate a ton of objects, as this will
	/// duplicate the user_interaction object in the same number.
	/// sometimes it is more efficient to have the user_interaction object as
	/// parameter of the constructor, using it if necessary while constructing the
	/// object only. In that situation, if the user_interaction is not need any
	/// further after construction, no need to make the class inherit from mem_ui.

    class mem_ui
    {
    public:

	    /// constructor

	    /// \param[in] dialog the user_interaction object to clone and store
	    /// If you plan to use mem_ui, you should pass the user_interaction to its constructor
	    /// for you later be able to call get_ui() at any time from the inherited class
	mem_ui(const std::shared_ptr<user_interaction> & dialog);

	    /// the copy constructor

	    /// need to be called from the copy constructor of any inherited class that explicitely define one
	mem_ui(const mem_ui & ref) = default;

	    /// the move constructor
	mem_ui(mem_ui && ref) noexcept = default;

	    /// assignement operator

	    /// you need to call it from the inherited class assignement operator
	    /// if the inherited class explicitely defines its own one.
	mem_ui & operator = (const mem_ui & ref) = default;

	    /// move operator
	mem_ui & operator = (mem_ui && ref) noexcept = default;


	    /// destructor

	    /// it is declared as virtual, for precaution, as it may not be very frequent to
	    /// release an object having just a mem_ui pointer on it.
        virtual ~mem_ui() noexcept(false) {};


    protected:
	    /// get access to the user_interaction object

	    /// \return a reference to the clone object. This reference stays valid during all
	    /// the live of the object.
	user_interaction & get_ui() const { return *ui; };

	    /// get access to the shared_ptr pointing to the user_interaction
	std::shared_ptr<user_interaction> get_pointer() const { return ui; };

    private:
	std::shared_ptr<user_interaction> ui;
    };

	/// @}

} // end of namespace


#endif
