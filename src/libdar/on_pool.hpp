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

    /// \file on_pool.hpp
    /// \brief this is the base class of object that can be allocated on a memory pool
    /// \ingroup Private



#ifndef ON_POOL_HPP
#define ON_POOL_HPP

#include "../my_config.h"
#include <vector>
#include <new>
#include "integers.hpp"
#include "memory_pool.hpp"
#include "mem_allocator.hpp"


namespace libdar
{


	/// \addtogroup Private
	/// @{

	/// class on_pool is the ancestors of all class that are able to be allocated on a memory pool
	///
	/// \note: new and new[] operators using memory pool never throw an exception upon allocation failure
	/// but rather return NULL pointer. This is the way it was working before g++03 and g++11

    class on_pool
    {
    public:
	    /// constructor
	    ///
	    /// \note If an object has been created by new (not new[], temporary, or local variable)
	    /// the get_pool() method can return the pool used for allocation, else it returns NULL.
	    /// The consequence is that objects dynamically created within an array (new []) cannot know
	    /// the whether they have been allocated using a memory pool or not
	on_pool() { dynamic_init(); };

	    /// copy constructor
	    ///
	    /// \note the default copy constructor is not adequate as it would copy the value of dynamic
	    /// of the source object whatever is the way the new object is created (dynamically allocated or not)
	on_pool(const on_pool & ref) { dynamic_init(); };

	    /// the assignment operator
	    ///
	    /// \note the assignement operator must not modify the field "dynamic" so we must not use
	    /// the default operator
	const on_pool & operator = (const on_pool & ref) { return *this; };

	    /// virtual destructor as this class will have inherited classes
	virtual ~on_pool() {};


	    /// the usual new operator is wrapped to allow proper delete operation later on (throws std::bad_alloc upon allocation failure)
	    ///
	    /// \note memory allocation done this way does not use the default C++ new[] operator, which may be slower than using memory pool
	void *operator new(size_t n_byte) { void *ret = alloc(n_byte, NULL); if(ret == NULL) throw std::bad_alloc(); return ret; };


	    /// the usual new operator is wrapped to allow proper delete operation later on (does not throw exception upon allocation failure)
	    ///
	    /// \note memory allocation done this way does not use the default C++ new[] operator, which may be slower than using memory pool
	void *operator new(size_t n_byte, const std::nothrow_t & nothrow_value) { return alloc(n_byte, NULL); };


	    /// the usual new[] operator is wrapped to allow proper delete[] operation later on (throws std::bad_alloc upon allocation failure)
	    ///
	    /// \note memory allocation done this way does not use the default C++ new[] operator, which may be slower than using memory pool
	void *operator new[](size_t n_byte) { void *ret = alloc(n_byte, NULL); if(ret == NULL) throw std::bad_alloc(); return ret; };


	    /// the usual new[] operator is wrapped to allow proper delete[] operation later on (does not throw exception upon allocation failure)
	    ///
	    /// \note memory allocation done this way does not use the default C++ new[] operator, which may be slower than using memory pool
	void *operator new[](size_t n_byte, const std::nothrow_t & nothrow_value) { return alloc(n_byte, NULL); };


	    /// this operator allocates a single object on a memory pool
	    ///
	    /// \note usage is: type *ptr = new (pool_obj) type(initial,values)
	    /// \note such object has to be delete normally no need to call the destructor manually
	void *operator new(size_t n_byte, memory_pool *p) { return alloc(n_byte, p); };


	    /// this operator allocates an array of objects on a memory pool
	    ///
	    /// \note usage is: type *ptr = new (pool_obj) type(initial,values)
	    /// \note such object has to be delete normally no need to call the destructor manually
	void *operator new[] (size_t n_byte, memory_pool *p) { return alloc(n_byte, p); };


	    /// this operator is called by the compiler if an exception is throw from the constructor of the allocated object
	void operator delete(void *ptr, memory_pool *p) { dealloc(ptr); };


	    /// this operator is called by the compiler if an exception is throw from the constructor of the allocated objects
	void operator delete[](void *ptr, memory_pool *p) { dealloc(ptr); };


	    /// this is the usual delete operator, modified to handle allocated objects allocated on a memory pool or not
	void operator delete(void *ptr) { dealloc(ptr); };


	    /// this is the usual delete[] operator, modified to handle allocated objects allocated on a memory pool or not
	void operator delete[](void *ptr) { dealloc(ptr); };

    protected:
	    /// get the pool used to allocate "this"
	    ///
	    /// \return the address of the memory pool that has been used to allocate the object
	    /// \note if the object has not been allocated using a memory pool NULL is returned
	    /// \note if the object has not been dynamically allocated, that's to say is a local variable
	    /// or a temporary object, get_pool() must not be called as it will return unpredictable
	    /// result and could most probably crash the application if the returned data is used
	memory_pool *get_pool() const { return dynamic ? (((pool_ptr *)this) - 1)->reserve : NULL; };


	template <class T> void meta_new(T * & ptr, size_t num)
	{
	    size_t byte = num * sizeof(T);

	    if(get_pool() != NULL)
		ptr = (T *)get_pool()->alloc(byte);
	    else
		ptr = (T *)new (std::nothrow) char [byte];
	}

	template <class T> void meta_delete(T * ptr)
	{
	    if(get_pool() != NULL)
		get_pool()->release(ptr);
	    else
		delete [] (char *)(ptr);
	}

    private:

	    /// this data structure is placed at the beginning of any allocated block
	    ///
	    /// \note thanks to this structure, it is possible to know which memory pool has to be
	    /// informed of the memory release in order for the memory block to be recyclable
	union pool_ptr
	{
	    memory_pool *reserve; //< this is to be able to pass the pool object to the constructor if it requires dynamic memory allocation
	    U_I  alignment__i; //< to properly align the allocated memory block that follows
	    U_32 alignment_32; //< to properly align the allocated memory block that follows
	    U_64 alignment_64; //< to properly align the allocated memory block that follows
	};

	    // this field is necessary to make distinction between object on the head that have a pool_ptr header from those
	    // created as local or temporary variable (on the stack).
	bool dynamic;

	    /// used from constructors to setup field "dynamic"
	void dynamic_init() const { const_cast<on_pool *>(this)->dynamic = (alloc_not_constructed == this); alloc_not_constructed = NULL; };

	    /// does the whole magic of memory allocation with and without memory_pool
	    ///
	    /// \param[in] size is the size of the requested block of memory to allocate
	    /// \param[in] is the address of the pool to request the memory block to, NULL if default memory allocation shall be used
	    /// \return the address of the allocated memory block is returned, NULL is returned upon memory allocation failure
	static void *alloc(size_t size, memory_pool *p);

	    /// does the whole magic of memory release with and without memory_pool
	    ///
	    /// \param[in] ptr is the address of the memory block to release
	    /// \note may throw exceptions if the given address has never been allocated or is NULL
	static void dealloc(void *ptr);

	thread_local static on_pool * alloc_not_constructed;
    };

	/// @}

} // end of namespace

#endif
