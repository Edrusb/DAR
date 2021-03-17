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

    /// \file mycurl_param_list.hpp
    /// \brief wrapper for element a CURL* object can receive as parameter in order to be put in etherogeneous list
    /// \ingroup Private

#ifndef MYCURL_PARAM_LIST_HPP
#define MYCURL_PARAM_LIST_HPP

#include "../my_config.h"

extern "C"
{
#if LIBCURL_AVAILABLE
#if HAVE_CURL_CURL_H
#include <curl/curl.h>
#endif
#endif
} // end extern "C"


#include <string>
#include <memory>
#include <map>
#include <list>
#include "integers.hpp"
#include "erreurs.hpp"

namespace libdar
{

        /// \addtogroup Private
	/// @{


	// libcurl uses a list of options to a CURL handle, which argument is of variable type
	// we want to record this list to be able to reset, copy, and do other fancy operations
	// on CURL handle and have all these managed within a single class (mycurl_easyhandle_node)
	// for that we need here a list of association of CURLoption type with variable typed value
	//
	// This is implemented by a ancestor type (mycurl_param_element_generic) which is an pure abstracted
	// class, from which derives many template based classes: mycurl_param_element<T>.



	/// the ancestor class of etherogeneous list/map

    class mycurl_param_element_generic
    {
    public:
	virtual ~mycurl_param_element_generic() = default;

	virtual bool operator == (const mycurl_param_element_generic & val) const = 0;
	virtual bool operator != (const mycurl_param_element_generic & val) const { return ! (*this == val); };

	virtual std::unique_ptr<mycurl_param_element_generic> clone() const = 0;
    };




	/// the implemented inherited classes of the abstracted class for etherogeneous list/map

    template <class T> class mycurl_param_element: public mycurl_param_element_generic
    {
    public:
	mycurl_param_element(const T & arg): val(arg) {};
	mycurl_param_element(const mycurl_param_element<T> & e): val(e.val) {};
	mycurl_param_element(mycurl_param_element<T> && e) noexcept: val(std::move(e.val)) {};
	mycurl_param_element<T> & operator = (const mycurl_param_element<T> & e) { val = e.val; return this; };
	mycurl_param_element<T> & operator = (mycurl_param_element<T> && e) { val = std::move(e.val); return this; };
	~mycurl_param_element() = default;

	virtual bool operator == (const mycurl_param_element_generic & arg) const override
	{
	    const mycurl_param_element<T>* arg_ptr = dynamic_cast<const mycurl_param_element<T>*>(&arg);
	    if(arg_ptr == nullptr)
		return false;
	    return arg_ptr->val == val;
	}

	T get_value() const { return val; };
	const T* get_value_address() const { return &val; };
	void set_value(const T & arg) { val = arg; };

	virtual std::unique_ptr<mycurl_param_element_generic> clone() const override
	{
	    std::unique_ptr<mycurl_param_element_generic> ret;

	    try
	    {
		ret = std::make_unique<mycurl_param_element<T> >(val);
		if(ret == nullptr)
		    throw Ememory("mycurl_param_list::clone");
	    }
	    catch(...)
	    {
		throw Ememory("mycurl_param_list::clone");
	    }

	    return ret;
	};

    private:
	T val;
    };


	/// This class holds an etherogenous list, more precisely an map that associate a CURLoption value
	/// to a value of a random type.

    class mycurl_param_list
    {
    public:
	mycurl_param_list() {};

#if LIBCURL_AVAILABLE

	mycurl_param_list(const mycurl_param_list & ref) { copy_from(ref); };
	mycurl_param_list(mycurl_param_list && ref) noexcept = default;
	mycurl_param_list & operator = (const mycurl_param_list & ref) { element_list.clear(); copy_from(ref); reset_read(); return *this; };
	mycurl_param_list & operator = (mycurl_param_list && ref) = default;
	~mycurl_param_list() = default;

	    // operations with the list

	template<class T> void add(CURLoption opt, const T & val) { element_list[opt] = std::make_unique<mycurl_param_element<T> >(val); reset_read(); }
	void clear(CURLoption opt);
	void clear() { element_list.clear(); reset_read(); };
	U_I size() const { return element_list.size(); };
	void reset_read() const { cursor = element_list.begin(); };
	bool read_next(CURLoption & opt);

	template<class T> void read_opt(const T* & val) const
	{
	    if(cursor == element_list.end())
		throw Erange("mycurl_param_list::read_opt", "Cannot read option when no more option is available");

	    if(cursor->second)
	    {
		const mycurl_param_element<T>* ptr = dynamic_cast<const mycurl_param_element<T>*>(cursor->second.get());

		if(ptr != nullptr)
		    val = ptr->get_value_address();
		else
		    val = nullptr;
	    }
	    else
		val = nullptr;

	    ++cursor;
	}

	template<class T>bool get_val(CURLoption opt, const T* & val) const
	{
	    std::map<CURLoption, std::unique_ptr<mycurl_param_element_generic> >::const_iterator it = element_list.find(opt);

	    if(it == element_list.end())
		return false;

	    if(it->second)
	    {
		const mycurl_param_element<T>* ptr = dynamic_cast<const mycurl_param_element<T>*>(it->second.get());

		if(ptr != nullptr)
		    val = ptr->get_value_address();
		else
		    val = nullptr;
	    }
	    else
		val = nullptr;

	    return true;
	}

	    // operations between lists


	    /// this method update the current object with parameters from wanted and returns
	    /// the list of modified options

	    /// \note if a CURLoption in wanted is not present in 'this' it is added with the
	    /// associated value and the CURLoption is
	    /// added to the returned list. If a CURLoption is present in both wanted and
	    /// "this" and its value differ between the two lists
	    /// it is updated in "this" and the option is added to the returned list, else
	    /// nothing is changed and the option is not added to the
	    /// returned list.

	std::list<CURLoption> update_with(const mycurl_param_list & wanted);

    private:
	std::map<CURLoption, std::unique_ptr<mycurl_param_element_generic> > element_list;
	mutable std::map<CURLoption, std::unique_ptr<mycurl_param_element_generic> >::const_iterator cursor;

	void add_clone(CURLoption opt, const mycurl_param_element_generic & val) { element_list[opt] = val.clone(); }
	void copy_from(const mycurl_param_list & ref);

#endif
    };


       	/// @}


} // end of namespace

#endif

