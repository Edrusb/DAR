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

    /// \file path.hpp
    /// \brief here is the definition of the path class
    /// \ingroup API
    ///
    /// the path class handle path and provide several operation on them

#ifndef PATH_HPP
#define PATH_HPP

#include "../my_config.h"
#include <list>
#include <string>
#include "erreurs.hpp"
#include "on_pool.hpp"

#define FAKE_ROOT path(string("<ROOT>"), true)

namespace libdar
{

	/// the class path is here to manipulate paths in the Unix notation: using'/'

	/// several operations are provided as well as convertion functions, but
	/// for the API user, it can be used as if it was a std::string object.
	/// However if the argument is not a valid path, an exception may be thrown
	/// by libdar
	/// \ingroup API

    class path : public on_pool
    {
    public :
	    /// constructor from a string

	    //! This realizes the string to path convertion function
	    //! \param[in] s the string to convert to path
	    //! \param[in] x_undisclosed do not split the given string, consider it as a single directory name, even if some '/' are found in it
	    //! \note empty string is not a valid string (exception thrown)
	    //! \note having undisclosed set to true, does not allow one to pop() right away, first push must be made. While having undisclosed set to false
	    //! let the user pop() right away if the given string is composed of several path members ("member1/member2/member3" for example of path
	    //! allow one to pop() three time, while in the same example setting undisclosed to true, allow one to pop() just once).
        path(const std::string & s, bool x_undisclosed = false);

	    /// constructor from a char *

	    //! this realizes the char * to path convertion function
	    //! \note empty string is not a valid string (exception thrown)
        path(const char *s, bool x_undisclosed = false) { *this = path(std::string(s), x_undisclosed); };

	    /// copy constructor
        path(const path & ref);

	    /// assignment operator
        const path & operator = (const path & ref);

	    /// comparison operator
        bool operator == (const path & ref) const;
	bool operator != (const path & ref) const { return !(*this == ref); };

	    /// get the basename of a path

	    /// this function returns the basename that's it the right most member of a path
	std::string basename() const;

	    /// reset the read_subdir operation

	    /// reset for read_subdir. next call to read_subdir is the most global
        void reset_read() { reading = dirs.begin(); };

	    /// sequentially read the elements that compose the path

	    /// \param[out] r the next element of the path
	    /// \return true if a next element could be read
	    /// \note the reading starts at the root and ends with the basename of the path
        bool read_subdir(std::string & r);

	    /// whether the path is relative or absolute (= start with a /)
        bool is_relative() const { return relative; };

	    /// whether the path is absolute or relative
	bool is_absolute() const { return !relative; };

	    /// whether the path has an undisclosed part at the beginning
	bool is_undisclosed() const { return undisclosed; };

	    /// remove and gives in argument the basename of the path

	    /// \param[out] arg the basename of the path
	    /// \return false if the operation was not possible (no sub-directory to pop)
	    /// \note if the path is absolute the remaing value is '/' when no pop is anymore possible
	    /// while it is the first component of the original path if the path was relative.
	    /// a empty path is not a valide value
        bool pop(std::string & arg);

	    /// remove and gives in argument the outer most member of the path

	    /// \param[out] arg the value of the outer element of the path
	    /// \return true if the pop_front operation was possible and arg could be set.
	    /// \note removes and returns the first directory of the path,
            /// when just the basename is present returns false, if the path is absolute,
            /// the first call change it to relative (except if equal to "/" then return false)
        bool pop_front(std::string & arg);

	    /// add a path to the current path. The added path *must* be a relative path

	    /// \param[in] arg the relative path to add
	    /// \return the resulting path, (the current object is not modified, where from the "const" qualifier)
	    /// \note arg can be a string also, which is converted to a path on the fly
        path operator + (const path & arg) const { path tmp = *this; tmp += arg; return tmp; };


	    /// add a path to the current path. The added path *must* be a relative path

	    /// \param[in] arg the relative path to add
	    /// \return the value of the current (modified) object: "*this".
        path & operator += (const path & arg);

	    /// test whether the current object is a subdir of the method's argument

	    /// \param[in] p the path to test with
	    /// \param[in] case_sensit whether the test must be in case sensitive manner or not
        bool is_subdir_of(const path & p, bool case_sensit) const;

	    /// convert back a path to a string

	    /// the returned string is the representation of the current object in Unix notation
        std::string display() const;

	    /// \brief returns the number of member in the path
	    ///
	    /// \note a absolute path counts one more that its relative brother
        unsigned int degre() const { return dirs.size() + (relative ? 0 : 1); };

	    /// \brief if the current object is an undisclosed path, tries to convert it back to normal path
	void explode_undisclosed() const;

    private :
        std::list<std::string>::iterator reading;
        std::list<std::string> dirs;
        bool relative;
	bool undisclosed;

        void reduce();
    };


} // end of namespace

#endif
