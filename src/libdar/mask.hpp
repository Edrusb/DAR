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

    /// \file mask.hpp
    /// \brief here lies a collection of mask classes
    ///
    /// The mask classes defined here are to be used to filter files
    /// in the libdar API calls.

#ifndef MASK_HPP
#define MASK_HPP

#include "../my_config.h"

extern "C"
{
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_REGEX_H
#include <regex.h>
#endif
} // end extern "C"

#include <string>
#include <vector>
#include "path.hpp"

namespace libdar
{

	/// \addtogroup API
	/// @{

        /// the generic class, parent of all masks

	/// this is a pure virtual class that is used in API call
	/// any of the following mask classes inherit from this class
    class mask
    {
    public :
        virtual ~mask() {};

	    /// check wether the given string is covered by the mask

	    /// \param[in] expression is the filename to check
	    /// \return true if the given filename is covered by the mask
	    /// \note only libdar internal need to call this method
        virtual bool is_covered(const std::string &expression) const = 0;

	    /// this is to be able to copy a mask without knowing its
	    /// exact class and without loosing its specialized data
        virtual mask *clone() const = 0;
    };


        /// boolean mask, either always true or false

	/// it matches all files or no files at all
    class bool_mask : public mask
    {
    public :
	    /// the constructor

	    /// \param[in] always is the value that this mask will always return
	    /// when the is_covered method will be used
	    /// \note once initialized an object cannot change its behavior
        bool_mask(bool always) { val = always; };

	    /// inherited from the mask class
        bool is_covered(const std::string &) const { return val; };
	    /// inherited from the mask class
        mask *clone() const { return new bool_mask(val); };

    private :
        bool val;
    };


        /// matches as done on shell command lines (see "man 7 glob")

    class simple_mask : public mask
    {
    public :

	    /// the constructor to use by libdar external programs

	    /// \param[in] wilde_card_expression is the glob expression that defines the mask
	    /// \param[in] case_sensit whether the mask is case sensitive or not
        simple_mask(const std::string & wilde_card_expression, bool case_sensit);
	    /// copy constructor
        simple_mask(const simple_mask & m) : mask(m) { copy_from(m); };
	    /// assignment operator
        simple_mask & operator = (const simple_mask & m);

	    /// inherited from the mask class
        bool is_covered(const std::string &expression) const;
	    /// inherited from the mask class
        mask *clone() const { return new simple_mask(*this); };

    private :
        std::string the_mask;
	bool case_s;

        void copy_from(const simple_mask & m);
    };


        /// matches regular expressions (see "man 7 regex")

    class regular_mask : public mask
    {
    public :

	    /// the constructor to be used by libdar external programs

	    /// \param[in] wilde_card_expression is the regular expression that defines the mask
	    /// \param[in] x_case_sensit whether the mask is case sensitive or not
        regular_mask(const std::string & wilde_card_expression,
		     bool x_case_sensit);
	    /// the copy constructor
	regular_mask(const regular_mask & ref);
	    /// the assignment operator
	regular_mask & operator= (const regular_mask & ref);

	    /// destructor
        virtual ~regular_mask() { regfree(&preg); };

	    /// inherited from the mask class
        bool is_covered(const std::string & expression) const;
	    /// inherited from the mask class
        mask *clone() const { return new regular_mask(*this); };

    private :
        regex_t preg;
	std::string mask_exp; //< used only by the copy constructor
	bool case_sensit;     //< used only by the copy constructor

	void set_preg(const std::string & wilde_card_expression,
		      bool x_case_sensit);
    };


        /// negation of another mask

	/// this is the first mask built over masks
	/// it realizes the negation of the given mask
    class not_mask : public mask
    {
    public :
	    /// the constructor to be used by libdar external programs

	    /// \param[in] m is the mask to negate
	    /// \note the mask used as argument need not to survive the just created not_mask object
	    /// as an internal copy of the mask given in argument has been done.
        not_mask(const mask &m) { copy_from(m); };
	    /// copy constructor
        not_mask(const not_mask & m) : mask(m) { copy_from(m); };
	    /// assignment operator
        not_mask & operator = (const not_mask & m);
	    /// destructor
        ~not_mask() { detruit(); };

	    /// inherited from the mask class
        bool is_covered(const std::string &expression) const { return !ref->is_covered(expression); };
	    /// inherited from the mask class
        mask *clone() const { return new not_mask(*this); };

    private :
        mask *ref;

        void copy_from(const not_mask &m);
        void copy_from(const mask &m);
        void detruit();
    };


        /// makes an *AND* operator between two or more masks

    class et_mask : public mask
    {
    public :

	    /// the constructor to be used by libdar external programs

	    /// \note at this stage the mask is not usable and will
	    /// throw an exception until some mask are added to the *AND*
	    /// thanks to the add_mask() method
        et_mask() {};
	    /// copy constructor
        et_mask(const et_mask &m) : mask(m) { copy_from(m); };
	    /// assignment operator
        et_mask & operator = (const et_mask &m);
	    /// destructor
        ~et_mask() { detruit(); };


	    /// add a mask to the operator

	    /// \param[in] toadd a mask to add to the *AND* operator
	    /// \note the mask given in argument has not to survive the et_mask to which it has been added
	    /// a internal copy of the mask has been done.
        void add_mask(const mask & toadd);

	    /// inherited from the mask class
        bool is_covered(const std::string & expression) const;
	    /// inherited from the mask class
        mask *clone() const { return new et_mask(*this); };

	    /// the number of mask on which is done the *AND* operator

	    /// \return the number of mask that has been added thanks to the add_mask() method
	    /// \note there is no mean to remove a given mask once it has been added (see the clear method)
        U_I size() const { return lst.size(); };

	    /// clear the mask

	    /// remove all previously added masks
	    /// \note that after this call the mask is no more usable as the *AND* operator cannot be done
	    /// on any mask
	void clear() { detruit(); };

    protected :
        std::vector<mask *> lst;

    private :
        void copy_from(const et_mask & m);
        void detruit();
    };


        /// makes the *OR* operator between two or more masks

	/// this mask has exactly the same use as the et_mask
	/// please see the et_mask documentation. The only difference
	/// is that it makes an *OR* operation rather than an *AND*
	/// with the masks added thanks to the add_mask method
    class ou_mask : public et_mask
    {
    public :
	    /// inherited from the mask class
        bool is_covered(const std::string & expression) const;
	    /// inherited from the mask class
        mask *clone() const { return new ou_mask(*this); };
    };


        /// string matches if it is subdir of mask or mask is a subdir of expression

    class simple_path_mask : public mask
    {
    public :
	    /// the constructor to be used by libdar external programs

	    /// \param[in] p the path the compare with
	    /// \param[in] case_sensit whether the mask is case sensitive or not
	    /// \note p must be a valid path
        simple_path_mask(const std::string &p, bool case_sensit) : chemin(p) { case_s = case_sensit; };

	    /// inherited from the mask class
        bool is_covered(const std::string &ch) const;
	    /// inherited from the mask class
        mask *clone() const { return new simple_path_mask(*this); };

    private :
        path chemin;
	bool case_s;
    };


        /// matches if string is exactly the given mask (no wilde card expression)

    class same_path_mask : public mask
    {
    public :
	    /// the constructor to be used by libdar external programs

	    /// \param[in] p is the path to compare with
	    /// \param[in] case_sensit whether the mask is case sensitive or not
        same_path_mask(const std::string &p, bool case_sensit) { chemin = p; case_s = case_sensit; };

	    /// inherited from the mask class
        bool is_covered(const std::string &ch) const;
	    /// inherited from the mask class
        mask *clone() const { return new same_path_mask(*this); };

    private :
        std::string chemin;
	bool case_s;
    };


	/// matches if string is the given string or a sub directory of it

    class exclude_dir_mask : public mask
    {
    public:
	    /// the constructor to be used by libdar external programs

	    /// \param[in] p is the path to compare with
	    /// \param[in] case_sensit whether the mask is case sensitive or not
	exclude_dir_mask(const std::string &p, bool case_sensit) { chemin = p; case_s = case_sensit;};

	    /// inherited from the mask class
	bool is_covered(const std::string &ch) const { return path(ch).is_subdir_of(chemin, case_s); }
	    /// inherited from the mask class
	mask *clone() const { return new exclude_dir_mask(*this); };

    private:
	std::string chemin;
	bool case_s;
    };

	/// @}

} // end of namespace

#endif
