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
// $Id: pile.hpp,v 1.14 2011/04/17 13:12:29 edrusb Rel $
//
/*********************************************************************/

    /// \file pile.hpp
    /// \brief class pile definition. Used to manage a stack of generic_file objects
    /// \ingroup Private


#ifndef PILE_HPP
#define PILE_HPP

#include "../my_config.h"

#include <vector>
#include <list>
#include "generic_file.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

    class pile : public generic_file
    {
    public:
	    /// the constructor
	    ///
	    /// \note the mode (gf_mode) of the pile is the one of the first object pushed on the stack,
	    /// thus when empty we choose the arbitrary gf_read_only value, because the stack is empty.

	pile() : generic_file(gf_read_only) { stack.clear(); };
	pile(const pile & ref) : generic_file(ref) { copy_from(ref); };
	const pile & operator = (const pile & ref) { detruit(); copy_from(ref); return *this; };
	~pile() { detruit(); };

	    /// add a generic_file on the top
	    ///
	    /// \param[in] f is the address of the object to add to the stack
	    /// \param[in] label unique label associated to this object in the current stack, exception thrown if label already used in stack
	    /// \note once added, the object memory allocation is managed by the pile object
	    /// the pile is responsible of destroying this object if its destructor is called
	    /// however, the pile object releases its responsibility about any object that
	    /// will be poped (see pop() below) from the stack.
	    /// \note empty label (empty string) is the only value that can be used for several objects of the stack
	void push(generic_file *f, const std::string & label = "");

	    /// remove the top generic_file from the top
	    ///
	    /// \returns the address of the object that has been poped from the stack or NULL if the stack is empty
	    /// \note this is now the duty of the caller to release this object memory when the object is no more needed
	generic_file *pop();

	    /// remove the top generic_file and destroy it
	    /// \param[in] ptr is the type of the object that must be found on the top of the stack. It may be the type of an parent class.
	    /// \return true if and only if the object on the top of the stack could be matched to the given type, this object is then poped from the stack and destroyed.
	template <class T> bool pop_and_close_if_type_is(T *ptr);

	    /// returns the address of the top generic_file
	generic_file *top() { if(stack.empty()) return NULL; else return stack.back().ptr; };

	    /// returns the address of teh bottom generic_file
	generic_file *bottom() { if(stack.empty()) return NULL; else return stack[0].ptr; };

	    /// returns the number of objects in the stack
	U_I size() const { return stack.size(); };

	    /// returns true if the stack is empty, false otherwise.
	bool is_empty() const { return stack.empty(); };

	    /// clears the stack
	void clear() { detruit(); };

	    /// this template let the class user find out the higher object on the stack of the given type
	    /// \param[in,out] ref gives the type of the object to look for, and gets the address of the first object found starting from the top
	    /// \note if no object of that type could be found in the stack ref is set to NULL
	template<class T> void find_first_from_top(T * & ref);

	    /// this template is similar to the template "find_first_from_top" except that the search is started from the bottom of the stack
	template<class T> void find_first_from_bottom(T * & ref);


	    /// return the generic_file object just below the given object or NULL if the object is at the bottom of the stack or is not in the stack
	generic_file *get_below(const generic_file *ref);

	    /// return the generic_file object just above the given object or NULL if the object is at the bottom of the stack or is not in the stack
	generic_file *get_above(const generic_file *ref);


	    /// find the object associated to a given label
	    ///
	    /// \param[in] label is the label to look for, empty string is forbidden
	    /// \return the object associated to label, else an exception is thrown
	generic_file *get_by_label(const std::string & label);



	    /// if label is associated to a member of the stack, makes this member of the stack an anoymous member (the label is no more associated to this object, while this object stays in the stack untouched
	    ///
	    /// \param[in] label the label to clear, empty string is not a valid label an exception is thrown if used here
	    /// \note no exception is thrown else, even if the label is not present in the stack
	void clear_label(const std::string & label);


	    /// associate a additional label to the object currently at the top of the stack
	    ///
	    /// \param[in] label the label to add
	    /// \note this does not remove an eventually existing label that had been added either by push() or add_label() previously
	    /// \note an object of the stack can thus be refered by several different labels
	void add_label(const std::string & label);



	    // inherited methods from generic_file
	    // they all apply to the top generic_file object, they fail by Erange() exception if the stack is empty

	bool skip(const infinint & pos);
	bool skip_to_eof();
	bool skip_relative(S_I x);
	infinint get_position();
	void copy_to(generic_file & ref);
	void copy_to(generic_file & ref, crc & value);

    protected:
	U_I inherited_read(char *a, U_I size);
	void inherited_write(const char *a, U_I size);
	void inherited_sync_write();
	void inherited_terminate();

    private:
	struct face
	{
	    generic_file * ptr;
	    std::list<std::string> labels;
	};

	std::vector<face> stack;

	void copy_from(const pile & ref)
	{
	    throw SRC_BUG; // it is not possible to copy an object to its another of the exact same type when only a pure virtual pointer pointing on it is available, or when no virtual "clone'-like method is available from the root pure virtual class (generic_file here).
	};
	void detruit();
	std::vector<face>::iterator look_for_label(const std::string & label);
    };


    template <class T> bool pile::pop_and_close_if_type_is(T *ptr)
    {
	generic_file *top = NULL;

	if(!stack.empty())
	{
	    top = stack.back().ptr;
	    ptr = dynamic_cast<T *>(top);
	    if(ptr != NULL)
	    {
		stack.pop_back();
		delete top;
		return true;
	    }
	    else
		return false;
	}
	else
	    return false;
    }

    template <class T> void pile::find_first_from_top(T * & ref)
	{
	ref = NULL;
	for(std::vector<face>::reverse_iterator it = stack.rbegin(); it != stack.rend() && ref == NULL; ++it)
	    ref = dynamic_cast<T *>(it->ptr);
    }


    template <class T> void pile::find_first_from_bottom(T * & ref)
    {
	ref = NULL;
	for(std::vector<face>::iterator it = stack.begin(); it != stack.end() && ref == NULL; ++it)
	    ref = dynamic_cast<T *>(it->ptr);
    }

    	/// @}

} // end of namespace

#endif
