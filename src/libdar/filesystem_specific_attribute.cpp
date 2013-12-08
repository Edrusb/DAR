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

#include "../my_config.h"

extern "C"
{

}

#include <new>
#include <algorithm>

#include "integers.hpp"
#include "erreurs.hpp"
#include "tools.hpp"
#include "filesystem_specific_attribute.hpp"

using namespace std;

namespace libdar
{

    static bool compare_for_sort(const filesystem_specific_attribute *a, const filesystem_specific_attribute *b);

    template <class T> bool binary_find_in_sorted_list(const vector<T*> & table, const T *val, U_I & index)
    {
	U_I min = 0;
	U_I max = table.size();

	if(val == NULL)
	    throw SRC_BUG;

	do
	{
	    index = (min + max)/2;
	    if(table[index] == NULL)
		throw SRC_BUG;
	    if(*(table[index]) < *val)
		min = index + 1;
	    else
		max = index;
	}
	while(*(table[index]) != *val && max - min > 0);

	return *(table[index]) == *val;
    }

///////////////////////////////////////////////////////////////////////////////////

    bool filesystem_specific_attribute::is_same_type_as(const filesystem_specific_attribute & ref) const
    {
	return get_familly() == ref.get_familly()
	    && get_nature() == ref.get_nature();
    }

    bool filesystem_specific_attribute::operator < (const filesystem_specific_attribute & ref) const
    {
	if(fam < ref.fam)
	    return true;
	else
	    if(fam > ref.fam)
		return false;
	    else
	        return nat < ref.nat;
    }

///////////////////////////////////////////////////////////////////////////////////

    const U_I FAM_SIG_WIDTH = 1;
    const U_I NAT_SIG_WIDTH = 2;

#define FSA_READ(FSA_TYPE, FAMILLY, TARGET)		   \
	try						   \
	{						   \
	    ptr = new (nothrow) FSA_TYPE(FAMILLY, TARGET); \
	    if(ptr == NULL)                                \
		throw SRC_BUG;                             \
	    fsa.push_back(ptr);                            \
	}                                                  \
	catch(Erange & e)                                  \
	{                                                  \
	    /* nothing to do */				   \
	}

    static bool compare_for_sort(const filesystem_specific_attribute *a, const filesystem_specific_attribute *b)
    {
	if(a == NULL || b == NULL)
	    throw SRC_BUG;
	return *a < *b;
    }


    void filesystem_specific_attribute_list::clear()
    {
	vector<filesystem_specific_attribute *>::iterator it = fsa.begin();

	while(it != fsa.end())
	{
	    if(*it != NULL)
	    {
		delete *it;
		*it = NULL;
	    }
	    ++it;
	}
	fsa.clear();
    }


    bool filesystem_specific_attribute_list::is_included_in(const filesystem_specific_attribute_list & ref, const fsa_scope & scope) const
    {
	bool ret = true;
	vector<filesystem_specific_attribute *>::const_iterator it = fsa.begin();
	vector<filesystem_specific_attribute *>::const_iterator rt = ref.fsa.begin();

	throw SRC_BUG; // implementation a revoir sachant que les listes sont triees

	while(ret && it != fsa.end())
	{
	    if(rt == ref.fsa.end())
	    {
		ret = false;
		continue; // skip the rest of the while loop
	    }

	    if(*it == NULL)
		throw SRC_BUG;
	    if(*rt == NULL)
		throw SRC_BUG;

	    if(scope.find((*it)->get_familly()) == scope.end())
	    {
		    // this FSA is out of the scope, skipping it
		++it;
		continue; // skip the rest of the while loop
	    }

	    while(rt != ref.fsa.end() && *(*rt) < *(*it))
	    {
		++rt;
		if(*rt == NULL)
		    throw SRC_BUG;
	    }

	    if(rt == ref.fsa.end())
		ret = false;
	    else
		if(*(*rt) == *(*it))
		    ++it;
		else
		    ret = false;
	}

	return ret;
    }

    void filesystem_specific_attribute_list::read(generic_file & f)
    {
	infinint size = infinint(f);
	U_I sub_size;

	do
	{
	    sub_size = 0;
	    size.unstack(sub_size);
	    if(size > 0 && sub_size == 0)
		throw SRC_BUG;

	    while(sub_size > 0)
	    {
		char buffer[FAM_SIG_WIDTH + NAT_SIG_WIDTH + 1];
		fsa_familly fam;
		fsa_nature nat;
		filesystem_specific_attribute *ptr = NULL;

		f.read(buffer, FAM_SIG_WIDTH);
		buffer[FAM_SIG_WIDTH] = '\0';
		fam = signature_to_familly(buffer);

		f.read(buffer, NAT_SIG_WIDTH);
		buffer[NAT_SIG_WIDTH] = '\0';
		nat = signature_to_nature(buffer);

		switch(nat)
		{
		case fsan_unset:
		    throw SRC_BUG;
		case fsan_creation_date:
		    ptr = new (nothrow) fsa_creation_date(f, fam);
		    break;
		case fsan_compressed:
		    ptr = new (nothrow) fsa_compressed(f, fam);
		    break;
		case fsan_no_dump:
		    ptr = new (nothrow) fsa_nodump(f, fam);
		    break;
		case fsan_immutable:
		    ptr = new (nothrow) fsa_immutable(f, fam);
		    break;
		case fsan_undeletable:
		    ptr = new (nothrow) fsa_undeleted(f, fam);
		    break;
		default:
		    throw SRC_BUG;
		}

		if(ptr == NULL)
		    throw Ememory("filesystem_specific_attribute_list::read");
		fsa.push_back(ptr);
		ptr = NULL;

		--sub_size;
	    }
	}
	while(size > 0);

	update_familles();
	sort_fsa();
    }

    void filesystem_specific_attribute_list::write(generic_file & f) const
    {
	infinint size = fsa.size();
	vector<filesystem_specific_attribute *>::const_iterator it = fsa.begin();

	size.dump(f);

	while(it != fsa.end())
	{
	    string tmp;

	    if(*it == NULL)
		throw SRC_BUG;

	    tmp = familly_to_signature((*it)->get_familly());
	    f.write(tmp.c_str(), tmp.size());
	    tmp = nature_to_signature((*it)->get_nature());
	    f.write(tmp.c_str(), tmp.size());
	    (*it)->write(f);

	    ++it;
	}
    }

    void filesystem_specific_attribute_list::get_fsa_from_filesystem_for(const string & target,
									 const fsa_scope & scope)
    {
	filesystem_specific_attribute *ptr = NULL;

	clear();

	if(scope.find(fsaf_hfs_plus) != scope.end())
	{
	    FSA_READ(fsa_creation_date, fsaf_hfs_plus, target);
	}

	if(scope.find(fsaf_linux_extX) != scope.end())
	{
	    FSA_READ(fsa_compressed, fsaf_linux_extX, target);
	    FSA_READ(fsa_nodump, fsaf_linux_extX, target);
	    FSA_READ(fsa_immutable, fsaf_linux_extX, target);
	    FSA_READ(fsa_undeleted, fsaf_linux_extX, target);
	}
	update_familles();
	sort_fsa();
    }

    bool filesystem_specific_attribute_list::set_fsa_to_filesystem_for(const string & target,
								       const fsa_scope & scope,
								       user_interaction & ui) const
    {
	vector<filesystem_specific_attribute *>::const_iterator it = fsa.begin();
	bool ret = false;

	while(it != fsa.end())
	{
	    if(*it == NULL)
		throw SRC_BUG;
	    if(scope.find((*it)->get_familly()) != scope.end())
	    {
		try
		{
		    (*it)->set_to_fs(target);
		    ret = true;
		}
		catch(Erange & e)
		{
		    string fsa_name = fsa_familly_to_string((*it)->get_familly()) + "/" + fsa_nature_to_string((*it)->get_nature());
		    string msg = e.get_message();

		    ui.printf("Error while setting filesystem specific attribute %s: %s",
			      &fsa_name,
			      &msg);
		}
	    }
	    ++it;
	}

	return ret;
    }

    const filesystem_specific_attribute & filesystem_specific_attribute_list::operator [] (U_I arg) const
    {
	if(arg >= fsa.size())
	    throw SRC_BUG;
	if(fsa[arg] == NULL)
	    throw SRC_BUG;

	return *(fsa[arg]);
    }

    infinint filesystem_specific_attribute_list::storage_size() const
    {
	infinint ret = 0;
	vector<filesystem_specific_attribute *>::const_iterator it = fsa.begin();

	while(it != fsa.end())
	{
	    if(*it == NULL)
		throw SRC_BUG;
	    ret += (*it)->storage_size();
	    ++it;
	}

	return ret;
    }

    filesystem_specific_attribute_list filesystem_specific_attribute_list::operator + (const filesystem_specific_attribute_list & arg) const
    {
	filesystem_specific_attribute_list ret = *this;
	vector<filesystem_specific_attribute *>::const_iterator it = arg.fsa.begin();

	while(it != arg.fsa.end())
	{
	    if(*it == NULL)
		throw SRC_BUG;
	    ret.add(*(*it));
	    ++it;
	}

	ret.update_familles();
	ret.sort_fsa();

	return ret;
    }

    void filesystem_specific_attribute_list::copy_from(const filesystem_specific_attribute_list & ref)
    {
	vector<filesystem_specific_attribute *>::const_iterator it = ref.fsa.begin();
	fsa.clear();

	while(it != ref.fsa.end())
	{
	    if(*it == NULL)
		throw SRC_BUG;
	    fsa.push_back((*it)->clone());
	    ++it;
	}

	familles = ref.familles;
    }

    void filesystem_specific_attribute_list::update_familles()
    {
	vector<filesystem_specific_attribute *>::iterator it = fsa.begin();

	familles.clear();
	while(it != fsa.end())
	{
	    if(*it == NULL)
		throw SRC_BUG;
	    familles.insert((*it)->get_familly());
	    ++it;
	}
    }

    void filesystem_specific_attribute_list::sort_fsa()
    {
	sort(fsa.begin(), fsa.end(), compare_for_sort);
    }

    void filesystem_specific_attribute_list::add(const filesystem_specific_attribute & ref)
    {
	U_I index = 0;

	if(binary_find_in_sorted_list(fsa, &ref, index))
	{
	    if(fsa[index] == NULL)
		throw SRC_BUG;
	    else
	    {
		filesystem_specific_attribute *rep = ref.clone();
		if(rep == NULL)
		    throw Ememory("filesystem_specific_attribute_list::add");
		try
		{
		    delete fsa[index];
		    fsa[index] = rep;
		}
		catch(...)
		{
		    delete rep;
		    throw;
		}
	    }
	}
	else
	{
	    filesystem_specific_attribute *rep = ref.clone();
	    if(rep == NULL)
		throw Ememory("filesystem_specific_attribute_list::add");

	    try
	    {
		for(U_I i = fsa.size() ; i > index ; --i)
		{
		    fsa[i] = fsa[i-1];
		    fsa[i-1] = NULL;
		}

		fsa[index] = rep;
	    }
	    catch(...)
	    {
		delete rep;
		throw;
	    }
	}
    }


    string filesystem_specific_attribute_list::familly_to_signature(fsa_familly f)
    {
	string ret;

	switch(f)
	{
	case fsaf_hfs_plus:
	    ret = "h";
	    break;
	case fsaf_linux_extX:
	    ret = "l";
	    break;
	default:
	    throw SRC_BUG;
	}

	if(ret.size() != FAM_SIG_WIDTH)
	    throw SRC_BUG;

	if(ret == "X")
	    throw SRC_BUG; // resevered for field extension if necessary in the future

	return ret;
    }

    string filesystem_specific_attribute_list::nature_to_signature(fsa_nature n)
    {
	string ret;

	switch(n)
	{
	case fsan_unset:
	    throw SRC_BUG;
	case fsan_creation_date:
	    ret = "aa";
	    break;
	case fsan_compressed:
	    ret = "ab";
	    break;
	case fsan_no_dump:
	    ret = "ac";
	    break;
	case fsan_immutable:
	    ret = "ad";
	    break;
	case fsan_undeletable:
	    ret = "ae";
	    break;
	default:
	    throw SRC_BUG;
	}

	if(ret.size() != NAT_SIG_WIDTH)
	    throw SRC_BUG;

	if(ret == "XX")
	    throw SRC_BUG; // resevered for field extension if necessary in the future

	return ret;
    }

    fsa_familly filesystem_specific_attribute_list::signature_to_familly(const string & sig)
    {
	if(sig.size() != FAM_SIG_WIDTH)
	    throw SRC_BUG;
	if(sig == "h")
	    return fsaf_hfs_plus;
	if(sig == "l")
	    return fsaf_linux_extX;
	if(sig == "X")
	    throw SRC_BUG;  // resevered for field extension if necessary in the future
	throw SRC_BUG;
    }

    fsa_nature filesystem_specific_attribute_list::signature_to_nature(const string & sig)
    {
	if(sig.size() != NAT_SIG_WIDTH)
	    throw SRC_BUG;
	if(sig == "aa")
	    return fsan_creation_date;
	if(sig == "ab")
	    return fsan_compressed;
	if(sig == "ac")
	    return fsan_no_dump;
	if(sig == "ad")
	    return fsan_immutable;
	if(sig == "ae")
	    return fsan_undeletable;
	if(sig == "XX")
	    throw SRC_BUG;  // resevered for field extension if necessary in the future
	throw SRC_BUG;
    }

///////////////////////////////////////////////////////////////////////////////////


    fsa_creation_date::fsa_creation_date(fsa_familly f, const string & target):
	filesystem_specific_attribute(f, target)
    {
	if(f != fsaf_hfs_plus)
	    throw Efeature("unknown familly for create_date Filesysttem Specific Attribute");
	set_nature(fsan_creation_date);
	throw Erange("fsa_creation_date::fsa_creation_date", "not yet implemented");
    }

    fsa_creation_date::fsa_creation_date(generic_file & f, fsa_familly fam): filesystem_specific_attribute(f, fam)
    {
	if(fam != fsaf_hfs_plus)
	    throw Efeature("unknown familly for create_date Filesysttem Specific Attribute");
	set_nature(fsan_creation_date);
	date.read(f);
    }

    string fsa_creation_date::show_val() const
    {
	return tools_display_date(date);
    }


    void fsa_creation_date::write(generic_file & f) const
    {
	date.dump(f);
    }

    void fsa_creation_date::set_to_fs(const string & target)
    {
	throw Efeature("hfs+ create date writing");
    }

    bool fsa_creation_date::equal_value_to(const filesystem_specific_attribute & ref) const
    {
	const fsa_creation_date *ref_ptr = dynamic_cast<const fsa_creation_date *>(&ref);

	if(ref_ptr != NULL)
	    return date == ref_ptr->date;
	else
	    return false;
    }

} // end of namespace

