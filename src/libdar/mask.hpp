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
// $Id: mask.hpp,v 1.10 2003/02/11 22:01:55 edrusb Rel $
//
/*********************************************************************/

#ifndef MASK_HPP
#define MASK_HPP

#pragma interface

#include <string.h>
#include <string>
#include <vector>
#include <regex.h>
#include "path.hpp"

using namespace std;

class mask
{
public :
    virtual ~mask() {};

    virtual bool is_covered(const string &expression) const = 0;
    virtual mask *clone() const = 0;
};

class bool_mask : public mask
{
public :
    bool_mask(bool always) { val = always; };

    bool is_covered(const string &exp) const { return val; };
    mask *clone() const { return new bool_mask(val); };

private :
    bool val;
};

class simple_mask : public mask
{
public :
    simple_mask(const string & wilde_card_expression);
    simple_mask(const simple_mask & m) { copy_from(m); };
    simple_mask & operator = (const simple_mask & m) { detruit(); copy_from(m); return *this; };
    virtual ~simple_mask() { detruit(); };

    bool is_covered(const string &expression) const;
    mask *clone() const { return new simple_mask(*this); };

private :
    char *the_mask;

    void copy_from(const simple_mask & m);
    void detruit() { if(the_mask != NULL) delete the_mask; the_mask = NULL; };
};


class regular_mask : public mask
{
public :
    regular_mask(const string & wild_card_expression);
    virtual ~regular_mask() { regfree(&preg); };

    bool is_covered(const string & expression) const;
    mask *clone() const { return new regular_mask(*this); };

private :
    regex_t preg;
};

class not_mask : public mask
{
public :
    not_mask(const mask &m) { copy_from(m); };
    not_mask(const not_mask & m) { copy_from(m); };
    not_mask & operator = (const not_mask & m) { detruit(); copy_from(m); return *this; };
    ~not_mask() { detruit(); };

    bool is_covered(const string &expression) const { return !ref->is_covered(expression); };
    mask *clone() const { return new not_mask(*this); };

private :
    mask *ref;

    void copy_from(const not_mask &m);
    void copy_from(const mask &m);
    void detruit();
};
    
class et_mask : public mask
{
public :
    et_mask() {};
    et_mask(const et_mask &m) { copy_from(m); };
    et_mask & operator = (const et_mask &m) { detruit(); copy_from(m); return *this; };
    ~et_mask() { detruit(); };

    void add_mask(const mask & toadd);

    bool is_covered(const string & expression) const;
    mask *clone() const { return new et_mask(*this); };
    U_I size() const { return lst.size(); };
protected :
    vector<mask *> lst;
    
private :
    void copy_from(const et_mask & m);
    void detruit();
};

class ou_mask : public et_mask
{
public :
    bool is_covered(const string & expression) const;
    mask *clone() const { return new ou_mask(*this); };
};

class simple_path_mask : public mask
{
public :
    simple_path_mask(const string &p) : chemin(p) {};

    bool is_covered(const string &ch) const;
    mask *clone() const { return new simple_path_mask(*this); };
    
private :
    path chemin;
};

class same_path_mask : public mask
{
public : 
    same_path_mask(const string &p) { chemin = p; };
    
    bool is_covered(const string &ch) const { return ch == chemin; };
    mask *clone() const { return new same_path_mask(*this); };

private :
    string chemin;
};

#endif
