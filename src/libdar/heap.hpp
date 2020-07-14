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

namespace libdar
{

	/// \addtogroup Private
	/// @{

    template <class T, class ARG> class heap
    {
    public:
	heap(U_I size, const ARG & val);
	heap(const heap & ref) = delete;
	heap(heap && ref) noexcept = default;
	heap & operator = (const heap & ref) = delete;
	heap & operator = (heap && ref) noexcept = default;

	std::unique_ptr<T> get();
	void put(std::unique_ptr<T> && obj);
	U_I get_size() const { return tas.size(); };

    private:
	std::deque<std::unique_ptr<T> > tas;
    };

    template <class T, class ARG> heap<T,ARG>::heap(U_I size, const ARG & val)
    {
	for(U_I i = 0; i < size; ++i)
	{
	    tas.push_back(std::unique_ptr<T>(new T(val)));
	    if(!tas.back()) // the unique_ptr we just added to 'tas' points to no object
	    {
		tas.clear();
		throw Ememory("hear::heap");
	    }
	}
    }

    template <class T, class ARG> std::unique_ptr<T> heap<T,ARG>::get()
    {
	std::unique_ptr<T> ret;

	if(tas.empty())
	    throw Erange("heap::get", "heap is empty, it should have be set larger");

	ret = std::move(tas.back()); // moving the object pointed to by tas.back() to ret
	tas.pop_back(); // removing the now empty pointer at the end of back

	return ret;
    }

    template <class T, class ARG> void heap<T, ARG>::put(std::unique_ptr<T> && obj)
    {
	tas.push_back(std::move(obj));
    }

	/// @}

} // end of namespace

#endif
