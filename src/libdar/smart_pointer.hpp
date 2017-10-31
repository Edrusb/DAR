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

    /// \file smart_pointer.hpp
    /// \brief template class implementing memory efficient smart pointer
    /// \ingroup Private
    /// \note Why not using std::shared_ptr? because I needed to have a warranty on the
    /// scalability in term of max number of smart_pointers that can be bound to an object

#ifndef SMART_POINTER_HPP
#define SMART_POINTER_HPP

#include "../my_config.h"

#include "infinint.hpp"
#include "erreurs.hpp"

namespace libdar
{

	/// class which holds the address of the allocated memory for many smart_pointers
	///
	/// \note it should not be used directly, rather see below the smart_pointer class template

    template <class T> class smart_node
    {
    public:
	    /// \note the given pointed to object passes under the responsibility of the smart_node
	smart_node(T *arg): ptr(arg), count_ref(0) { if(arg == nullptr) throw SRC_BUG; };
	smart_node(const smart_node & ref) = delete;
	smart_node & operator = (const smart_node & ref) = delete;
	~smart_node() { if(ptr != nullptr) delete ptr; if(!count_ref.is_zero()) throw SRC_BUG; };

	void add_ref() { ++count_ref; };
	void del_ref() { if(count_ref.is_zero()) throw SRC_BUG; --count_ref; if(count_ref.is_zero()) delete this; };
	T & get_val() { return *ptr; };

    private:
	T *ptr;
	infinint count_ref;

    };


	/// smart pointer class to be used to automagically manage multiple time pointed to address
	///
	/// this class tend to mimic normal pointer with the additional feature of automatically releasing
	/// the pointed to object when no more smart_pointer point to it. In consequence:
	/// - it must not be used to point to non dynamically allocated memory using the "new" operator,
	/// - pointed to memory must never be deleted manually, the last smart_pointer will do it at its
	///   destruction time

    template <class T> class smart_pointer
    {
    public:
	    /// creates a smart_pointer equivalent to a pointer to NULL
	smart_pointer() { ptr = nullptr; };

	    /// creates a smart_pointer pointing to an allocated memory
	    ///
	    /// \param[in] arg is the address of the allocated memory the smart_pointer must manage,
	    /// nullptr is allowed and lead to the same behavior as the constructor without argument
	    /// \note the given pointed to object, passes under the responsibility of the smart_pointer
	    /// and must not be deleted any further
	smart_pointer(T *arg)
	{
	    if(arg != nullptr)
	    {
		ptr = new (std::nothrow) smart_node<T>(arg);
		if(ptr == nullptr)
		    throw Ememory("smart_pointer::smart_pointer");
		ptr->add_ref();
	    }
	    else
		ptr = nullptr;
	};

	    /// copy constructor
	smart_pointer(const smart_pointer & ref) { ptr = ref.ptr; if(ptr != nullptr) ptr->add_ref(); };

	    /// move constructor
	smart_pointer(smart_pointer && ref) { ptr = ref.ptr; ref.ptr = nullptr; };

	    /// destructor
	~smart_pointer() { if(ptr != nullptr) ptr->del_ref(); };

	    /// assignment operator
	smart_pointer & operator = (const smart_pointer & ref)
	{
	    if(ref.ptr != ptr)
	    {
		if(ref.ptr != nullptr)
		{
		    if(ptr != nullptr)
			ptr->del_ref();
		    ptr = ref.ptr;
		    ptr->add_ref();
		}
		else
		{
		    ptr->del_ref(); // ptr is no nullptr because ref.ptr != ptr
		    ptr = nullptr;
		}
	    }
	    return *this;
	};

	    /// move assignment operator
	smart_pointer & operator = (smart_pointer && ref)
	{
	    if(ptr != nullptr)
		ptr->del_ref();

	    ptr = ref.ptr;
	    ref.ptr = nullptr;

	    return *this;
	};

	    /// assignment operator from a base type pointer (not from a smart_pointer)
	    ///
	    /// \note choice has been not to overload/use operator= to avoid risk of error
	    /// that would lead to create independent smart_pointer sets accidentally
	const smart_pointer & assign(T *arg)
	{
	    smart_pointer<T> tmp(arg);
	    *this = tmp;
	    return *this;
	}

	    /// content-of operator
	T & operator *() const { if(ptr == nullptr) throw SRC_BUG; return ptr->get_val(); };

	    /// content-of field operator (when the pointed to object is a struct or class
	T* operator ->() const { if(ptr == nullptr) throw SRC_BUG; return &(ptr->get_val()); };

	    /// return whether the smart_pointer is pointing to nullptr
	bool is_null() const { return ptr == nullptr; };

    private:
	smart_node<T> *ptr;
    };

} // end of namespace

#endif
