/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002 Denis Corbin
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
// $Id: path.hpp,v 1.8 2002/10/28 20:39:34 edrusb Rel $
//
/*********************************************************************/

#ifndef PATH_HPP
#define PATH_HPP

#pragma interface

#include <list>
#include <string>
#include "erreurs.hpp"

using namespace std;

class path
{
public :
    path(string s); // empty string is not a valid string (exception thrown)
    path(const char *s) { *this = path(string(s)); };
    path(const path & ref);
    path & operator = (const path & ref);
    bool operator == (const path & ref) const;
    
    string basename() const; // name of the innermost directory/file of the path 
    void reset_read() { reading = dirs.begin(); }; // reset for read_subdir. next call to read_subdir is the most global 
	// directory 
    bool read_subdir(string & r); // return the name of the next directory part of the path to basename(), starting at root
    bool is_relative() const { return relative; };
    bool pop(string & arg); // remove and return in argument the basename of the path, return false if not possible (no sub-directory)
    bool pop_front(string & arg); // removes and returns the first directory of the path,
       // when just the basename is present returns false, if the path is absolute, 
       // the first call change it to relative (except if equal to "/" then return false)

    path operator + (const path & arg) const { path tmp = *this; tmp += arg; return tmp; }; 
	// add arg as a subdir of the object, arg can be a string also, which is converted to a path on the fly
    path & operator += (const path & arg);
    bool is_subdir_of(const path & p) const;
    string display() const;
    unsigned int degre() const { return dirs.size() + (relative ? 0 : 1); };
    
private :
    list<string>::iterator reading;
    list<string> dirs;
    bool relative;

    void reduce();
};

#endif
