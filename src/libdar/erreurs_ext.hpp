/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2023 Denis Corbin
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

    /// \file erreurs_ext.hpp
    /// \brief contains some additional exception class thrown by libdar
    /// \ingroup Private

#ifndef ERREURS_EXT_HPP
#define ERREURS_EXT_HPP

#include "erreurs.hpp"
#include "infinint.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{


	/// Ethread_cancel with infinint attribute

    class Ethread_cancel_with_attr : public Ethread_cancel
    {
    public :
	Ethread_cancel_with_attr(bool now, U_64 x_flag, const infinint & attr);
	Ethread_cancel_with_attr(const Ethread_cancel_with_attr & ref): Ethread_cancel(ref) { copy_from(ref); };
	Ethread_cancel_with_attr(Ethread_cancel_with_attr && ref) noexcept : Ethread_cancel(std::move(ref)) { x_attr = nullptr; std::swap(x_attr, ref.x_attr); };
	Ethread_cancel_with_attr & operator = (const Ethread_cancel_with_attr & ref) { detruit(); copy_from(ref); return *this; };
	Ethread_cancel_with_attr & operator = (Ethread_cancel_with_attr && ref) noexcept;
	~Ethread_cancel_with_attr() { detruit(); };

	const infinint get_attr() const { return *x_attr; };

    private :
	    // infinint may throw Ebug exception from destructor.
	    // Having a field of type infinint lead this class
	    // to have a default destructor throwing Ebug too
	    // which implies Egeneric to have the same which
	    // makes circular dependency as Ebug cannot be defined
	    // before Egeneric. Here is the reason why we use a infinint* here
	infinint *x_attr;

	void detruit();
	void copy_from(const Ethread_cancel_with_attr & ref);
    };

	/// @}

} // end of namespace

#endif
