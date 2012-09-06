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

    /// \file special_alloc.hpp
    /// \brief re-definition of new and delete class operator
    /// \ingroup Private
    ///
    /// this is a set of macro that makes the new and delete operator for
    /// a class be re-defined in a way that allocation is done in big chunks
    /// at the system level, and is split in small requested pieces for a
    /// given object allocation. This bring some performance improvment,
    /// because a lot a small objects that live and die toghether have to
    /// be allocated.

#ifndef SPECIAL_ALLOC_HPP
#define SPECIAL_ALLOC_HPP

#include "../my_config.h"
#include <iostream>
#include <new>

#ifdef LIBDAR_SPECIAL_ALLOC

extern "C"
{
#if HAVE_STDDEF_H
#include <stddef.h>
#else
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#endif
} // end extern "C"

    /// \addtogroup Private
    /// @{

#define USE_SPECIAL_ALLOC(BASE_TYPE) \
        void *operator new(size_t taille) { return special_alloc_new(taille); };                \
	void *operator new(size_t taille, const std::nothrow_t& nothrow_constant) { return special_alloc_new(taille); }; \
        void *operator new(size_t taille, BASE_TYPE * & place) { return (void *) place; };      \
        void *operator new(size_t taille, void * & place) { return place; };                    \
        void operator delete(void *ptr) throw() { special_alloc_delete(ptr); }                  \
        void operator delete(void* ptr, const std::nothrow_t& nothrow_constant) throw() { special_alloc_delete(ptr); }

namespace libdar
{
	// this following call is to be used in a
	// multi-thread environment and is called from
	// libdar global initialization function
	// this makes libdar thread-safe if POSIX mutex
	// are available

    extern void *special_alloc_new(size_t taille);
    extern void special_alloc_delete(void *ptr);

	// this should be called for sanity and control purposes just before ending the program,
	// it will report any block still not yet released
    extern void special_alloc_garbage_collect(std::ostream & output);


} // end of namespace

#endif

    /// @}

#endif

