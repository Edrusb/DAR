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
// $Id: user_interaction.hpp,v 1.14 2004/11/21 22:29:48 edrusb Rel $
//
/*********************************************************************/

    /// \file user_interaction.hpp
    /// \brief defines the interaction between libdar and the user.
    ///
    /// Two classes are defined
    /// - user_interaction is the root class that you can use to make your own classes
    /// - user_interaction_callback is a specialized inherited class which is implements
    ///   user interaction thanks to callback functions
    /// .

#ifndef USER_INTERACTION_HPP
#define USER_INTERACTION_HPP

#include "../my_config.h"
#include <string>

namespace libdar
{

	/// This is a pure virtual class that is used by libdar when interaction with the user is required.

	//! You can base your own class on it using C++ inheritance
	//! or use the class user_interaction_callback which implements
	//! the interaction based on callback functions.
	//! The user_interaction class is used by libdar in the following circumpstances:
	//! - when is required a boolean answer to a question the pause() method is used
	//! - when a warning needs to be displayed to the user the warning() method is used
	//! - when a directory listing needs to be returned to the user the listing() method is used
	//! .
	//! the printf() method is built over the warning() methods to display a formated message
	//! it has not to be redefined in any inherited class.
	//! If you want to define you own class as inherited class of user_interaction
	//! you need to overwrite:
	//! - the clone() method. It is used to make local temporary copies of objets
	//!  in libdar. It acts like the constructor copy but is virtual.
	//! - the pause() method
	//! - the warning() method
	//! - the listing() method (this is not mandatory).  inherited classes *can*
	//! overwrite the listing() method, which will be used if the use_listing
	//! is set to true thanks to the set_use_listing() protected method.
	//! In that case the listing of archive contents is done thanks to this listing()
	//! method instead of the warning() method.
	//! - get_string() method
	//! .
	//! WARNING !
	//! if your own class has specific fields, you will probably
	//! need to redefine the copy constructor as well as operator =
	//! if you don't understand this and why, don't play trying making your
	//! own class, and/or read good C++ book about canonical form
	//! of a C++ class, as well as how to properly make an inherited class.
	//! And don't, complain if libdar segfault or core dumps. Libdar
	//! *needs* to make local copies of theses objects, if the copy constructor
	//!  is not properly defined in your inherited class this will crash the application.
	//! \ingroup API
    class user_interaction
    {
    public:

	    /// class constructor.
	user_interaction() { use_listing = false; };

	    /// method used to ask a boolean question to the user.

	    //! \param[in] message is the message to be displayed, that is the question.
	    //! \exception Euser_abort If the user answer "false" or "no" to the question the method
	    //! must throw an exception of type "Euser_abort".
	virtual void pause(const std::string & message) = 0;

	    /// \brief method used to display a warning or a message to the user.
	    ///
	    /// \param[in] message is the message to display.
	virtual void warning(const std::string & message) = 0;

	    /// method used to ask a question that needs an arbitrary answer.

	    //! \param[in] message is the question to display to the user.
	    //! \param[in] echo is set to false is the answer must not be shown while the user answers.
	    //! \return the user's answer.
	virtual std::string get_string(const std::string & message, bool echo) = 0;

	    /// optional method to use if you want file listing splitted in several fields.

	    //! \param[in] flag is the given information about the EA, compression, presence of saved data.
	    //! \param[in] perm is the access permission of the file.
	    //! \param[in] uid User ID of the file.
	    //! \param[in] gid Group ID of the file.
	    //! \param[in] size file size.
	    //! \param[in] date file modification date.
	    //! \param[in] filename file name.
	    //! \param[in] is_dir true if file is a directory.
	    //! \param[in] has_children true if file is a directory which is not empty.
	    //! \note This is not a pure virtual method, this is normal,
	    //! so your inherited class is not obliged to overwrite it.
	virtual void listing(const std::string & flag,
			     const std::string & perm,
			     const std::string & uid,
			     const std::string & gid,
			     const std::string & size,
			     const std::string & date,
			     const std::string & filename,
			     bool is_dir,
			     bool has_children);


	    /// libdar uses this call to format output before send to warning() method.

	    //! This is not a virtual method, it has not to be overwritten, it is
	    //! just a sublayer over warning()
	    //! Supported masks for the format string are:
	    //! - \%s \%c \%d \%\%  (normal behavior)
	    //! - \%i (matches infinint *)
	    //! - \%S (matches std::string *)
	    //! .
	void printf(char *format, ...);

	    /// for libdar to know if it is interesting to use listing() or to keep reporting
	    /// listing thanks to the warning() method.

	    //! this is not a virtual method, it has not to be overwritten in inherited classes.
	bool get_use_listing() const { return use_listing; };

	    /// make a newly allocated object which has the same properties as "this".

	    //! This *is* a virtual method, it *must* be overwritten in any inherited class
	    //! copy constructor and = operator may have to be overwrittent too if necessary
	    //! Warning !
	    //! clone() must throw exception if necessary (Ememory), but never
	    //! return a NULL pointer !
	virtual user_interaction *clone() const = 0;

    protected:

	    /// method to be called with true as argument if you have defined a listing() method.

	    //! in the constructor of any inherited class that define a listing() method
	    //! it is advisable to call set_use_listing() with true as argument for libdar
	    //! knows that the listing() call has to be used in place of the warning() call
	    //! for file listing.
	void set_use_listing(bool val) { use_listing = val; };

    private:
	bool use_listing;

    };

	/// full implemented class for user_interaction based on callback functions.

	//! this class is an inherited class of user_interaction it is used by
	//! dar command line programs, but you can use it if you wish.
	//! \ingroup API
    class user_interaction_callback : public user_interaction
    {
    public:

	    /// constructor which receive the callback functions.

	    //! \param[in] x_warning_callback is used by warning() method
	    //! \param[in] x_answer_callback is used by the pause() method
	    //! \param[in] x_string_callback is used by get_string() method
	    //! \param[in] context_value will be passed as last argument of callbacks when
	    //! called from this object.
	    //! \note The context argument of each callback is set with the context_value given
	    //! in the user_interaction_callback object constructor. The value can
	    //! can be any arbitrary value (NULL is valid), and can be used as you wish.
	    //! Note that the listing callback is not defined here, but thanks to a specific method
	user_interaction_callback(void (*x_warning_callback)(const std::string &x, void *context),
				  bool (*x_answer_callback)(const std::string &x, void *context),
				  std::string (*x_string_callback)(const std::string &x, bool echo, void *context),
				  void *context_value);


	    /// overwritting method from parent class.
       	void pause(const std::string & message);
	    /// overwritting method from parent class.
	void warning(const std::string & message);
	    /// overwritting method from parent class.
	std::string get_string(const std::string & message, bool echo);

	    /// overwritting method from parent class.
        void listing(const std::string & flag,
		     const std::string & perm,
		     const std::string & uid,
		     const std::string & gid,
		     const std::string & size,
		     const std::string & date,
		     const std::string & filename,
		     bool is_dir,
		     bool has_children);

	    /// You can set a listing callback thanks to this method.

	    //! If set, when file listing will this callback function will
	    //! be used instead of the x_warning_callback given as argument
	    //! of the constructor.
        void set_listing_callback(void (*callback)(const std::string & flag,
						   const std::string & perm,
						   const std::string & uid,
						   const std::string & gid,
						   const std::string & size,
						   const std::string & date,
						   const std::string & filename,
						   bool is_dir,
						   bool has_children,
						   void *context))
	    {
		tar_listing_callback = callback;
		set_use_listing(true); // this is to inform libdar to use listing()
	    };

	    /// overwritting method from parent class.
	virtual user_interaction *clone() const;

    private:
	void (*warning_callback)(const std::string & x, void *context);  // pointer to function
	bool (*answer_callback)(const std::string & x, void *context);   // pointer to function
	std::string (*string_callback)(const std::string & x, bool echo, void *context); // pointer to function
	void (*tar_listing_callback)(const std::string & flags,
				     const std::string & perm,
				     const std::string & uid,
				     const std::string & gid,
				     const std::string & size,
				     const std::string & date,
				     const std::string & filename,
				     bool is_dir,
				     bool has_children,
				     void *context);
	void *context_val;
    };

} // end of namespace

#endif

