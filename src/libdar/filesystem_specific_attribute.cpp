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

#include "erreurs.hpp"
#include "tools.hpp"
#include "filesystem_specific_attribute.hpp"

using namespace std;

namespace libdar
{

///////////////////////////////////////////////////////////////////////////////////

    bool filesystem_specific_attribute::is_same_type_as(const filesystem_specific_attribute & ref) const
    {
	return get_familly() == ref.get_familly()
	    && get_nature() == ref.get_nature();
    }

    string filesystem_specific_attribute::familly_to_string(filesystem_specific_attribute::familly f)
    {
	switch(f)
	{
	case hfs_plus:
	    return "HFS+";
	case linux_extX:
	    return "ext2/3/4";
	default:
	    throw SRC_BUG;
	}
    }

    string filesystem_specific_attribute::nature_to_string(nature n)
    {
	switch(n)
	{
	case nat_unset:
	    throw SRC_BUG;
	case creation_date:
	    return gettext("creation date");
	case compressed:
	    return gettext("compressed");
	case no_dump:
	    return gettext("no dump flag");
	case immutable:
	    return gettext("immutable");
	case undeletable:
	    return gettext("undeletable");
	default:
	    throw SRC_BUG;
	}
    }


///////////////////////////////////////////////////////////////////////////////////

    const U_I FAM_SIG_WIDTH = 1;
    const U_I NAT_SIG_WIDTH = 2;
    const U_I FSA_CRC_SIZE = 2;

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


    bool filesystem_specific_attribute_list::is_equal_to(const filesystem_specific_attribute_list & ref, const fsa_scope & scope) const
    {
	bool ret = true;
	vector<filesystem_specific_attribute *>::const_iterator it = fsa.begin();

	while(ret && it != fsa.end())
	{
	    if(*it != NULL)
	    {
		set<filesystem_specific_attribute::familly>::const_iterator f = scope.find((*it)->get_familly());
		if(scope.size() == 0 || f != scope.end())
		{
		    vector<filesystem_specific_attribute *>::const_iterator ut = ref.fsa.begin();
		    while(ut != ref.fsa.end() && *ut != NULL && !(*it)->is_same_type_as(**ut))
			++ut;
		    if(ut != ref.fsa.end()) // did not reached end of list
		    {
			if(*ut == NULL) // because found a NULL pointer
			    throw SRC_BUG;
			else // or because found the same type FSA
			{
			    if((**it) != (**ut))
				ret = false;      // FSA value differ so we abort and return false
				// else nothing to do, found FSA of same type and it has the same value,  checking next FSA
			}
		    }
			// else nothing to do
		}
		    // else ignoring this FSA as it is out of familly scope
	    }
	    else
		throw SRC_BUG;
	    ++it;
	}

	return ret;
    }

    void filesystem_specific_attribute_list::read(generic_file & f)
    {
	infinint size = infinint(f);
	U_I sub_size;
	crc *c_calc = NULL;

	if(f.crc_status())
	    throw SRC_BUG;
	f.reset_crc(FSA_CRC_SIZE);
	try
	{
	    do
	    {
		sub_size = 0;
		size.unstack(sub_size);
		if(size > 0 && sub_size == 0)
		    throw SRC_BUG;

		while(sub_size > 0)
		{
		    char buffer[FAM_SIG_WIDTH + NAT_SIG_WIDTH + 1];
		    filesystem_specific_attribute::familly fam;
		    filesystem_specific_attribute::nature nat;
		    filesystem_specific_attribute *ptr = NULL;

		    f.read(buffer, FAM_SIG_WIDTH);
		    buffer[FAM_SIG_WIDTH] = '\0';
		    fam = signature_to_familly(buffer);

		    f.read(buffer, NAT_SIG_WIDTH);
		    buffer[NAT_SIG_WIDTH] = '\0';
		    nat = signature_to_nature(buffer);

		    switch(nat)
		    {
		    case filesystem_specific_attribute::nat_unset:
			throw SRC_BUG;
		    case filesystem_specific_attribute::creation_date:
			ptr = new (nothrow) fsa_creation_date(f, fam);
			break;
		    case filesystem_specific_attribute::compressed:
			ptr = new (nothrow) fsa_compressed(f, fam);
			break;
		    case filesystem_specific_attribute::no_dump:
			ptr = new (nothrow) fsa_nodump(f, fam);
			break;
		    case filesystem_specific_attribute::immutable:
			ptr = new (nothrow) fsa_immutable(f, fam);
			break;
		    case filesystem_specific_attribute::undeletable:
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

	}
	catch(...)
	{
		// keep f in coherent status in regard to crc calculation
	    c_calc = f.get_crc();
	    if(c_calc != NULL)
		delete c_calc;
	    throw;
	}

	try
	{
	    c_calc = f.get_crc();
	    if(c_calc == NULL)
		throw SRC_BUG;
	    crc_n *c_calc_n = dynamic_cast<crc_n *>(c_calc);
	    crc_n c_read = crc_n(FSA_CRC_SIZE, f);
	    if(c_calc_n == NULL)
		throw SRC_BUG;
	    if(*c_calc_n != c_read)
		throw Edata(gettext("data corruption met while reading Filesystem Specific Attribute"));
	}
	catch(...)
	{
	    if(c_calc != NULL)
	    {
		delete c_calc;
		c_calc = NULL;
	    }
	    throw;
	}

	if(c_calc != NULL)
	{
	    delete c_calc;
	    c_calc = NULL;
	}
    }

    void filesystem_specific_attribute_list::write(generic_file & f) const
    {
	infinint size = fsa.size();
	vector<filesystem_specific_attribute *>::const_iterator it = fsa.begin();
	crc *ptr = NULL;

	if(f.crc_status())
	    throw SRC_BUG;
	f.reset_crc(FSA_CRC_SIZE);

	try
	{

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
	catch(...)
	{
	    ptr = f.get_crc();
	    if(ptr != NULL)
		delete ptr;
	    throw;
	}

	try
	{
	    ptr = f.get_crc();
	    if(ptr == NULL)
		throw SRC_BUG;
	    ptr->dump(f);
	}
	catch(...)
	{
	    if(ptr != NULL)
	    {
		delete ptr;
		ptr = NULL;
	    }
	    throw;
	}

	if(ptr != NULL)
	{
	    delete ptr;
	    ptr = NULL;
	}
    }

    void filesystem_specific_attribute_list::get_fsa_from_filesystem_for(const string & target)
    {
	filesystem_specific_attribute *ptr = NULL;

	clear();

	FSA_READ(fsa_creation_date, filesystem_specific_attribute::hfs_plus, target);
	FSA_READ(fsa_compressed, filesystem_specific_attribute::linux_extX, target);
	FSA_READ(fsa_nodump, filesystem_specific_attribute::linux_extX, target);
	FSA_READ(fsa_immutable, filesystem_specific_attribute::linux_extX, target);
	FSA_READ(fsa_undeleted, filesystem_specific_attribute::linux_extX, target);
    }

    void filesystem_specific_attribute_list::set_fsa_to_filesystem_for(const string & target,
								       const fsa_scope & scope,
								       user_interaction & ui) const
    {
	vector<filesystem_specific_attribute *>::const_iterator it = fsa.begin();

	while(it != fsa.end())
	{
	    if(*it == NULL)
		throw SRC_BUG;
	    if(scope.find((*it)->get_familly()) != scope.end())
	    {
		try
		{
		    (*it)->set_to_fs(target);
		}
		catch(Erange & e)
		{
		    string fsa_name = filesystem_specific_attribute::familly_to_string((*it)->get_familly()) + "/" + filesystem_specific_attribute::nature_to_string((*it)->get_nature());
		    string msg = e.get_message();

		    ui.printf("Error while setting filesystem specific attribute %s: %s",
			      &fsa_name,
			      &msg);
		}
	    }
	    ++it;
	}
    }

    const filesystem_specific_attribute & filesystem_specific_attribute_list::operator [] (U_I arg) const
    {
	if(arg >= fsa.size())
	    throw SRC_BUG;
	if(fsa[arg] == NULL)
	    throw SRC_BUG;

	return *(fsa[arg]);
    }

    string filesystem_specific_attribute_list::familly_to_signature(filesystem_specific_attribute::familly f)
    {
	string ret;

	switch(f)
	{
	case filesystem_specific_attribute::hfs_plus:
	    ret = "h";
	    break;
	case filesystem_specific_attribute::linux_extX:
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

    string filesystem_specific_attribute_list::nature_to_signature(filesystem_specific_attribute::nature n)
    {
	string ret;

	switch(n)
	{
	case filesystem_specific_attribute::nat_unset:
	    throw SRC_BUG;
	case filesystem_specific_attribute::creation_date:
	    ret = "aa";
	    break;
	case filesystem_specific_attribute::compressed:
	    ret = "ab";
	    break;
	case filesystem_specific_attribute::no_dump:
	    ret = "ac";
	    break;
	case filesystem_specific_attribute::immutable:
	    ret = "ad";
	    break;
	case filesystem_specific_attribute::undeletable:
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

    filesystem_specific_attribute::familly filesystem_specific_attribute_list::signature_to_familly(const string & sig)
    {
	if(sig.size() != FAM_SIG_WIDTH)
	    throw SRC_BUG;
	if(sig == "h")
	    return filesystem_specific_attribute::hfs_plus;
	if(sig == "l")
	    return filesystem_specific_attribute::linux_extX;
	if(sig == "X")
	    throw SRC_BUG;  // resevered for field extension if necessary in the future
	throw SRC_BUG;
    }

    filesystem_specific_attribute::nature filesystem_specific_attribute_list::signature_to_nature(const string & sig)
    {
	if(sig.size() != NAT_SIG_WIDTH)
	    throw SRC_BUG;
	if(sig == "aa")
	    return filesystem_specific_attribute::creation_date;
	if(sig == "ab")
	    return filesystem_specific_attribute::compressed;
	if(sig == "ac")
	    return filesystem_specific_attribute::no_dump;
	if(sig == "ad")
	    return filesystem_specific_attribute::immutable;
	if(sig == "ae")
	    return filesystem_specific_attribute::undeletable;
	if(sig == "XX")
	    throw SRC_BUG;  // resevered for field extension if necessary in the future
	throw SRC_BUG;
    }

///////////////////////////////////////////////////////////////////////////////////


    fsa_creation_date::fsa_creation_date(filesystem_specific_attribute::familly f, const string & target):
	filesystem_specific_attribute(f, target)
    {
	if(f != filesystem_specific_attribute::hfs_plus)
	    throw Efeature("unknown familly for create_date Filesysttem Specific Attribute");
	set_nature(filesystem_specific_attribute::creation_date);
	throw Erange("fsa_creation_date::fsa_creation_date", "not yet implemented");
    }

    fsa_creation_date::fsa_creation_date(generic_file & f, familly fam): filesystem_specific_attribute(f, fam)
    {
	if(fam != filesystem_specific_attribute::hfs_plus)
	    throw Efeature("unknown familly for create_date Filesysttem Specific Attribute");
	set_nature(filesystem_specific_attribute::creation_date);
	date.read(f);
    }

    bool fsa_creation_date::operator == (const filesystem_specific_attribute & ref) const
    {
	const fsa_creation_date * ref_s = dynamic_cast<const fsa_creation_date *>(&ref);

	if(ref_s != NULL)
	    return ref_s->date == date;
	else
	    return false;
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

} // end of namespace

