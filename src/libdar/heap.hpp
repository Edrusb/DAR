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

    /// \file heap.hpp
    /// \brief heap data structure (relying on FIFO)
    /// \ingroup Private
    ///

#ifndef HEAP_HPP
#define HEAP_HPP

#include "../my_config.h"
#include <string>

#include "integers.hpp"
#include <memory>
#include <deque>
#if HAVE_LIBTHREADAR_LIBTHREADAR_HPP
#include <libthreadar/libthreadar.hpp>
#endif

namespace libdar
{

	/// \addtogroup Private
	/// @{

	/// the class heap is nothing related to the common heap datastructure this is just a "heap" in the sense of a pool of preallocated objects

    template <class T> class heap
    {
    public:
	heap() {}; // start with an empty heap
	heap(const heap & ref) = delete;
	heap(heap && ref) noexcept = default;
	heap & operator = (const heap & ref) = delete;
	heap & operator = (heap && ref) noexcept = default;

	std::unique_ptr<T> get();
	void put(std::unique_ptr<T> && obj);
	void put(std::deque<std::unique_ptr<T> > & list);
	U_I get_size() const { return tas.size(); };

    private:
	std::deque<std::unique_ptr<T> > tas;
	libthreadar::mutex access;
    };

    template <class T> std::unique_ptr<T> heap<T>::get()
    {
	std::unique_ptr<T> ret;

	access.lock();
	try
	{
	    if(tas.empty())
		throw Erange("heap::get", "heap is empty, it should have be set larger");

	    ret = std::move(tas.back()); // moving the object pointed to by tas.back() to ret
	    tas.pop_back(); // removing the now empty pointer at the end of 'tas'
	}
	catch(...)
	{
	    access.unlock();
	    throw;
	}
	access.unlock();

	return ret;
    }

    template <class T> void heap<T>::put(std::unique_ptr<T> && obj)
    {
	access.lock();
	try
	{
	    tas.push_back(std::move(obj));
	}
	catch(...)
	{
	    access.unlock();
	    throw;
	}
	access.unlock();
    }

    template <class T> void heap<T>::put(std::deque<std::unique_ptr<T> > & list)
    {
	typename std::deque<std::unique_ptr<T> >::iterator it = list.begin();

	access.lock();
	try
	{
	    while(it != list.end())
	    {
		tas.push_back(std::move(*it));
		++it;
	    }
	}
	catch(...)
	{
	    access.unlock();
	    throw;
	}
	access.unlock();
    }

	/// @}

} // end of namespace

#endif
