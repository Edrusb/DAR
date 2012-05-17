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
// $Id: mask.cpp,v 1.18 2005/11/09 18:31:53 edrusb Rel $
//
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
#if HAVE_FNMATCH_H
#include <fnmatch.h>
#endif
} // end extern "C"

#include "mask.hpp"
#include "tools.hpp"
#include "erreurs.hpp"


using namespace std;

namespace libdar
{

    simple_mask::simple_mask(const string & wilde_card_expression,
			     bool case_sensit)
    {
        the_mask = tools_str2charptr(wilde_card_expression);
        if(the_mask == NULL)
            throw Ememory("simple_mask::simple_mask");
	case_s = case_sensit;
	try
	{
	    if(!case_s)
		tools_to_upper(the_mask);
	}
	catch(...)
	{
	    delete [] the_mask;
	    throw;
	}
    }

    simple_mask & simple_mask::operator = (const simple_mask & m)
    {
	const mask *src = & m;
	mask *dst = this;
	*dst = *src; // explicitely invoke the inherited "mask" class's operator =
	detruit();
	copy_from(m);
	return *this;
    }

    bool simple_mask::is_covered(const string &expression) const
    {
        char *tmp = tools_str2charptr(expression);
        bool ret;

        if(tmp == NULL)
            throw Ememory("simple_mask::is_covered");

	try
	{
	    if(!case_s)
		tools_to_upper(tmp);
	    ret = fnmatch(the_mask, tmp, FNM_PERIOD) == 0;
	}
	catch(...)
	{
	    delete [] tmp;
	    throw;
	}
	delete [] tmp;

        return ret;
    }

    void simple_mask::copy_from(const simple_mask & m)
    {
        the_mask = new char[strlen(m.the_mask)+1];
        if(the_mask == NULL)
            throw Ememory("simple_mask::copy_from");
        strcpy(the_mask, m.the_mask);
	case_s = m.case_s;
    }

    regular_mask::regular_mask(const string & wilde_card_expression,
			       bool x_case_sensit)
    {
	mask_exp = wilde_card_expression;
	case_sensit = x_case_sensit;
	set_preg(mask_exp, case_sensit);
    }

    regular_mask::regular_mask(const regular_mask & ref) : mask(ref)
    {
	mask_exp = ref.mask_exp;
	case_sensit = ref.case_sensit;
	set_preg(mask_exp, case_sensit);
    }

    regular_mask & regular_mask::operator= (const regular_mask & ref)
    {
	const mask *ref_ptr = &ref;
	mask *me = this;
	*me = *ref_ptr; // initializing the inherited mask part of the object
	mask_exp = ref.mask_exp;
	case_sensit = ref.case_sensit;
	regfree(&preg);
	set_preg(mask_exp, case_sensit);

	return *this;
    }

    bool regular_mask::is_covered(const string & expression) const
    {
        char *tmp = tools_str2charptr(expression);
        bool matches;

        try
        {
            matches = regexec(&preg, tmp, 0, NULL, 0) != REG_NOMATCH;
        }
        catch(...)
        {
            delete [] tmp;
            throw;
        }
        delete [] tmp;

        return matches;
    }


    void regular_mask::set_preg(const string & wilde_card_expression, bool x_case_sensit)
    {
        char *tmp = tools_str2charptr(wilde_card_expression);

        try
        {
            S_I ret;

            if((ret = regcomp(&preg, tmp, REG_NOSUB|(x_case_sensit ? 0 : REG_ICASE)|REG_EXTENDED)) != 0)
            {
                const S_I msg_size = 1024;
                char msg[msg_size];
                regerror(ret, &preg, msg, msg_size);
                throw Erange("regular_mask::regular_mask", msg);
            }
        }
        catch(...)
        {
            delete [] tmp;
            throw;
        }
        delete [] tmp;
    }

    not_mask & not_mask::operator = (const not_mask & m)
    {
	const mask *src = &m;
	mask *dst = this;
	*dst = *src; // explicitely invoke the inherited "mask" class's operator =
	detruit();
	copy_from(m);
	return *this;
    }

    void not_mask::copy_from(const not_mask &m)
    {
        ref = m.ref->clone();
        if(ref == NULL)
            throw Ememory("not_mask::copy_from(not_mask)");
    }

    void not_mask::copy_from(const mask &m)
    {
        ref = m.clone();
        if(ref == NULL)
            throw Ememory("not_mask::copy_from(mask)");
    }

    void not_mask::detruit()
    {
        if(ref != NULL)
        {
            delete ref;
            ref = NULL;
        }
    }

    et_mask & et_mask::operator = (const et_mask &m)
    {
	const mask *src = &m;
	mask *dst = this;
	*dst = *src; // explicitely invoke the inherited "mask" class's operator =
	detruit();
	copy_from(m);
	return *this;
    }

    void et_mask::add_mask(const mask& toadd)
    {
        mask *t = toadd.clone();
        if(t != NULL)
            lst.push_back(t);
        else
            throw Ememory("et_mask::et_mask");
    }

    bool et_mask::is_covered(const string & expression) const
    {
        vector<mask *>::iterator it = const_cast<et_mask &>(*this).lst.begin();
        vector<mask *>::iterator fin = const_cast<et_mask &>(*this).lst.end();

        if(lst.size() == 0)
            throw Erange("et_mask::is_covered", gettext("No mask in the list of mask to operate on"));

        while(it != fin && (*it)->is_covered(expression))
            it++;

        return it == fin;
    }

    void et_mask::copy_from(const et_mask &m)
    {
        vector<mask *>::iterator it = const_cast<et_mask &>(m).lst.begin();
        vector<mask *>::iterator fin = const_cast<et_mask &>(m).lst.end();
        mask *tmp;

        while(it != fin && (tmp = (*it)->clone()) != NULL)
        {
            lst.push_back(tmp);
            it++;
        }

        if(it != fin)
        {
            detruit();
            throw Ememory("et_mask::copy_from");
        }
    }

    void et_mask::detruit()
    {
        vector<mask *>::iterator it = lst.begin();

        while(it != lst.end())
        {
            delete *it;
            it++;
        }
        lst.clear();
    }

    static void dummy_call(char *x)
    {
        static char id[]="$Id: mask.cpp,v 1.18 2005/11/09 18:31:53 edrusb Rel $";
        dummy_call(id);
    }

    bool ou_mask::is_covered(const string & expression) const
    {
        vector<mask *>::iterator it = const_cast<ou_mask &>(*this).lst.begin();
        vector<mask *>::iterator fin = const_cast<ou_mask &>(*this).lst.end();

        if(lst.size() == 0)
            throw Erange("et_mask::is_covered", gettext("No mask in the list of mask to operate on"));

        while(it != fin && ! (*it)->is_covered(expression))
            it++;

        return it != fin;
    }

    bool simple_path_mask::is_covered(const string &ch) const
    {
        path p = ch;
        return p.is_subdir_of(chemin, case_s) || chemin.is_subdir_of(p, case_s);
    }


    bool same_path_mask::is_covered(const std::string &ch) const
    {
	if(case_s)
	    return ch == chemin;
	else
	    return tools_is_case_insensitive_equal(ch, chemin);
    }

} // end of namespace
