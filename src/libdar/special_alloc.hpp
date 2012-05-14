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
// to contact the author : dar.linux@free.fr
/*********************************************************************/
// $Id: special_alloc.hpp,v 1.4 2003/10/18 14:43:07 edrusb Rel $
//
/*********************************************************************/

#ifndef SPECIAL_ALLOC_HPP
#define SPECIAL_ALLOC_HPP

#ifdef SPECIAL_ALLOC

#if HAVE_STDDEF_H
#include <stddef.h>
#else
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#endif

#define USE_SPECIAL_ALLOC(BASE_TYPE) \
        void *operator new(size_t taille) { return special_alloc_new(taille); };                \
        void *operator new(size_t taille, BASE_TYPE * & place) { return (void *) place; };      \
        void *operator new(size_t taille, void * & place) { return place; };                    \
        void operator delete(void *ptr) { special_alloc_delete(ptr); }

namespace libdar
{

    extern void *special_alloc_new(size_t taille);
    extern void special_alloc_delete(void *ptr);

} // end of namespace

#endif

#endif
