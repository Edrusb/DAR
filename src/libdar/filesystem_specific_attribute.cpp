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

#include "../my_config.h"

extern "C"
{
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#ifdef LIBDAR_NODUMP_FEATURE
#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#if LIBDAR_NODUMP_FEATURE == NODUMP_LINUX
#include <linux/ext2_fs.h>
#else
#if LIBDAR_NODUMP_FEATURE == NODUMP_EXT2FS
#include <ext2fs/ext2_fs.h>
#else
#error "unknown location of ext2_fs.h include file"
#endif
#endif
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
}

#include <algorithm>

#include "integers.hpp"
#include "erreurs.hpp"
#include "tools.hpp"
#include "filesystem_specific_attribute.hpp"
#include "cygwin_adapt.hpp"
#include "deci.hpp"
#include "fichier_local.hpp"
#include "compile_time_features.hpp"
#include "capabilities.hpp"
#include "erreurs.hpp"
#include "archive_version.hpp"

using namespace std;

namespace libdar
{

    static bool compare_for_sort(const filesystem_specific_attribute *a, const filesystem_specific_attribute *b);

    template <class T> bool binary_search_in_sorted_list(const deque<T*> & table, const T *val, U_I & index)
    {
	U_I min = 0;
	U_I max = table.size();

	if(val == nullptr)
	    throw SRC_BUG;

	if(max == 0) // empty table
	    return false;

	do
	{
	    index = (min + max)/2;
	    if(table[index] == nullptr)
		throw SRC_BUG;
	    if(*(table[index]) < *val)
		min = index + 1;
	    else
		max = index;
	}
	while(!table[index]->is_same_type_as(*val) && max - min > 0);

	if(max - min <= 0)
	    index = min;

	return min < table.size() && (table[index])->is_same_type_as(*val);
    }

///////////////////////////////////////////////////////////////////////////////////

    bool filesystem_specific_attribute::is_same_type_as(const filesystem_specific_attribute & ref) const
    {
	return get_family() == ref.get_family()
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


    static bool compare_for_sort(const filesystem_specific_attribute *a, const filesystem_specific_attribute *b)
    {
	if(a == nullptr || b == nullptr)
	    throw SRC_BUG;
	return *a < *b;
    }


    void filesystem_specific_attribute_list::clear()
    {
	deque<filesystem_specific_attribute *>::iterator it = fsa.begin();

	while(it != fsa.end())
	{
	    if(*it != nullptr)
	    {
		delete *it;
		*it = nullptr;
	    }
	    ++it;
	}
	fsa.clear();
    }

    void filesystem_specific_attribute_list::add(const filesystem_specific_attribute & fsa)
    {
	priv_add(fsa);
	update_familes();
	sort_fsa();
    }

    bool filesystem_specific_attribute_list::is_included_in(const filesystem_specific_attribute_list & ref, const fsa_scope & scope) const
    {
	bool ret = true;
	deque<filesystem_specific_attribute *>::const_iterator it = fsa.begin();
	deque<filesystem_specific_attribute *>::const_iterator rt = ref.fsa.begin();

	while(ret && it != fsa.end())
	{
	    if(rt == ref.fsa.end())
	    {
		ret = false;
		continue; // skip the rest of the while loop
	    }

	    if(*it == nullptr)
		throw SRC_BUG;
	    if(*rt == nullptr)
		throw SRC_BUG;

	    if(scope.find((*it)->get_family()) == scope.end())
	    {
		    // this FSA is out of the scope, skipping it
		++it;
		continue; // skip the rest of the while loop
	    }

	    while(rt != ref.fsa.end() && *(*rt) < *(*it))
	    {
		++rt;
		if(*rt == nullptr)
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

    void filesystem_specific_attribute_list::read(generic_file & f, archive_version ver)
    {
	infinint size = infinint(f);
	U_I sub_size;
	U_I tmp;

	do
	{
	    sub_size = 0;
	    size.unstack(sub_size);
	    if(size > 0 && sub_size == 0)
		throw SRC_BUG;

	    while(sub_size > 0)
	    {
		char buffer[FAM_SIG_WIDTH + NAT_SIG_WIDTH + 1];
		fsa_family fam;
		fsa_nature nat;
		filesystem_specific_attribute *ptr = nullptr;

		tmp = f.read(buffer, FAM_SIG_WIDTH);
		if(tmp < FAM_SIG_WIDTH)
		    throw Erange("filesystem_specific_attribute_list::read", gettext("invalid length for FSA family flag"));
		buffer[FAM_SIG_WIDTH] = '\0';
		fam = signature_to_family(buffer);

		tmp = f.read(buffer, NAT_SIG_WIDTH);
		if(tmp < NAT_SIG_WIDTH)
		    throw Erange("filesystem_specific_attribute_list::read", gettext("invalid length for FSA nature flag"));
		buffer[NAT_SIG_WIDTH] = '\0';
		nat = signature_to_nature(buffer);

		switch(nat)
		{
		case fsan_unset:
		    throw SRC_BUG;
		case fsan_creation_date:
		    ptr = new (nothrow) fsa_time(f, ver, fam, nat);
		    break;
		case fsan_append_only:
		case fsan_compressed:
		case fsan_no_dump:
		case fsan_immutable:
		case fsan_data_journaling:
		case fsan_secure_deletion:
		case fsan_no_tail_merging:
		case fsan_undeletable:
		case fsan_noatime_update:
		case fsan_synchronous_directory:
		case fsan_synchronous_update:
		case fsan_top_of_dir_hierarchy:
		    ptr = new (nothrow) fsa_bool(f, fam, nat);
		    break;
		default:
		    throw SRC_BUG;
		}

		if(ptr == nullptr)
		    throw Ememory("filesystem_specific_attribute_list::read");
		fsa.push_back(ptr);
		ptr = nullptr;

		--sub_size;
	    }
	}
	while(!size.is_zero());

	update_familes();
	sort_fsa();
    }

    void filesystem_specific_attribute_list::write(generic_file & f) const
    {
	infinint size = fsa.size();
	deque<filesystem_specific_attribute *>::const_iterator it = fsa.begin();

	size.dump(f);

	while(it != fsa.end())
	{
	    string tmp;

	    if(*it == nullptr)
		throw SRC_BUG;

	    tmp = family_to_signature((*it)->get_family());
	    f.write(tmp.c_str(), tmp.size());
	    tmp = nature_to_signature((*it)->get_nature());
	    f.write(tmp.c_str(), tmp.size());
	    (*it)->write(f);

	    ++it;
	}
    }

    void filesystem_specific_attribute_list::get_fsa_from_filesystem_for(user_interaction & ui,
									 const string & target,
									 const fsa_scope & scope,
									 mode_t itype,
									 bool auto_zeroing_neg_dates)
    {
	clear();

	if(scope.find(fsaf_hfs_plus) != scope.end())
	{
	    fill_HFS_FSA_with(ui,
			      target,
			      itype,
			      auto_zeroing_neg_dates);
	}

	if(scope.find(fsaf_linux_extX) != scope.end())
	{
	    fill_extX_FSA_with(target, itype);
	}
	update_familes();
	sort_fsa();
    }

    bool filesystem_specific_attribute_list::has_linux_immutable_set() const
    {

	const filesystem_specific_attribute *ptr = nullptr;
	const fsa_bool *bptr = nullptr;

	if(find(fsaf_linux_extX, fsan_immutable, ptr))
	{
	    bptr = dynamic_cast<const fsa_bool *>(ptr);
	    if(bptr == nullptr)
		throw SRC_BUG; // immutable flag should be a boolean
	    return bptr->get_value();
	}
	else // immutable attribute not present
	    return false;
    }

    bool filesystem_specific_attribute_list::set_fsa_to_filesystem_for(const string & target,
								       const fsa_scope & scope,
								       user_interaction & ui,
								       bool set_linux_immutable) const
    {
	bool ret = false;

	if(scope.find(fsaf_linux_extX) != scope.end())
	    ret |= set_extX_FSA_to(ui, target, set_linux_immutable);

	if(!set_linux_immutable && scope.find(fsaf_hfs_plus) != scope.end())
	    ret |= set_hfs_FSA_to(ui, target);

	return ret;
    }

    const filesystem_specific_attribute & filesystem_specific_attribute_list::operator [] (U_I arg) const
    {
	if(arg >= fsa.size())
	    throw SRC_BUG;
	if(fsa[arg] == nullptr)
	    throw SRC_BUG;

	return *(fsa[arg]);
    }

    infinint filesystem_specific_attribute_list::storage_size() const
    {
	infinint ret = infinint(size()).get_storage_size();
	deque<filesystem_specific_attribute *>::const_iterator it = fsa.begin();
	infinint overhead = family_to_signature(fsaf_hfs_plus).size()
	    + nature_to_signature(fsan_creation_date).size();

	while(it != fsa.end())
	{
	    if(*it == nullptr)
		throw SRC_BUG;
	    ret += (*it)->storage_size() + overhead;
	    ++it;
	}

	return ret;
    }

    filesystem_specific_attribute_list filesystem_specific_attribute_list::operator + (const filesystem_specific_attribute_list & arg) const
    {
	filesystem_specific_attribute_list ret = *this;
	deque<filesystem_specific_attribute *>::const_iterator it = arg.fsa.begin();

	while(it != arg.fsa.end())
	{
	    if(*it == nullptr)
		throw SRC_BUG;
	    ret.priv_add(*(*it));
	    ++it;
	}

	ret.update_familes();
	ret.sort_fsa();

	return ret;
    }

    bool filesystem_specific_attribute_list::find(fsa_family fam, fsa_nature nat, const filesystem_specific_attribute *&ptr) const
    {
	fsa_bool tmp = fsa_bool(fam, nat, true);
	U_I index;

	if(binary_search_in_sorted_list(fsa, (filesystem_specific_attribute *)(&tmp), index))
	{
	    ptr = fsa[index];
	    return true;
	}
	else
	    return false;
    }

    void filesystem_specific_attribute_list::copy_from(const filesystem_specific_attribute_list & ref)
    {
	deque<filesystem_specific_attribute *>::const_iterator it = ref.fsa.begin();
	fsa.clear();

	while(it != ref.fsa.end())
	{
	    if(*it == nullptr)
		throw SRC_BUG;
	    fsa.push_back((*it)->clone());
	    ++it;
	}

	familes = ref.familes;
    }

    void filesystem_specific_attribute_list::update_familes()
    {
	deque<filesystem_specific_attribute *>::iterator it = fsa.begin();

	familes.clear();
	while(it != fsa.end())
	{
	    if(*it == nullptr)
		throw SRC_BUG;
	    familes.insert((*it)->get_family());
	    ++it;
	}
    }

    void filesystem_specific_attribute_list::priv_add(const filesystem_specific_attribute & ref)
    {
	U_I index = 0;

	if(binary_search_in_sorted_list(fsa, &ref, index))
	{
	    if(fsa[index] == nullptr)
		throw SRC_BUG;
	    else
	    {
		filesystem_specific_attribute *rep = ref.clone();
		if(rep == nullptr)
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
	    if(rep == nullptr)
		throw Ememory("filesystem_specific_attribute_list::add");

	    try
	    {
		fsa.resize(fsa.size()+1, nullptr);
		for(U_I i = fsa.size()-1 ; i > index ; --i)
		{
		    fsa[i] = fsa[i-1];
		    fsa[i-1] = nullptr;
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


    void filesystem_specific_attribute_list::sort_fsa()
    {
	sort(fsa.begin(), fsa.end(), compare_for_sort);
    }

    template <class T, class U> void create_or_throw(T *& ref, fsa_family f, fsa_nature n, const U & val)
    {
	if(ref != nullptr)
	    throw SRC_BUG;

	ref = new (nothrow) T(f, n, val);
	if(ref == nullptr)
	    throw Ememory("template create_or_throw");
    }

    void filesystem_specific_attribute_list::fill_extX_FSA_with(const std::string & target,
								mode_t itype)
    {
#ifdef LIBDAR_NODUMP_FEATURE
	S_I fd = -1;

	    // symlink do not have their own extX FSA: restoring FSA of a symlink change the FSA of the target of the symlink
	if(S_ISREG(itype) || S_ISDIR(itype))
	{
	    try
	    {
		fichier_local ftmp = fichier_local(target, compile_time::furtive_read());
		fd = ftmp.give_fd_and_terminate();
	    }
	    catch(Egeneric & e)
	    {
		if(!compile_time::furtive_read())
		    throw; // not a problem about furtive read mode

		try // trying openning not using furtive read mode
		{
		    fichier_local ftmp = fichier_local(target, false);
		    fd = ftmp.give_fd_and_terminate();
		}
		catch(Egeneric & e)
		{
		    fd = -1;
			// we assume this FSA family is not supported for that file
		}
	    }

	    if(fd < 0)
		return; // silently aborting assuming FSA family not supported for that file

	    try
	    {
		S_I f = 0;
		fsa_bool * ptr = nullptr;

		if(ioctl(fd, EXT2_IOC_GETFLAGS, &f) < 0)
		{
		    close(fd);
		    return; // assuming there is no support for that FSA family
		}

#ifdef EXT2_APPEND_FL
		create_or_throw(ptr, fsaf_linux_extX, fsan_append_only, (f & EXT2_APPEND_FL) != 0);
		fsa.push_back(ptr);
		ptr = nullptr;
#endif
#ifdef EXT2_COMPR_FL
		create_or_throw(ptr, fsaf_linux_extX, fsan_compressed, (f & EXT2_COMPR_FL) != 0);
		fsa.push_back(ptr);
		ptr = nullptr;
#endif
#ifdef EXT2_NODUMP_FL
		create_or_throw(ptr, fsaf_linux_extX, fsan_no_dump, (f & EXT2_NODUMP_FL) != 0);
		fsa.push_back(ptr);
		ptr = nullptr;
#endif
#ifdef EXT2_IMMUTABLE_FL
		create_or_throw(ptr, fsaf_linux_extX, fsan_immutable, (f & EXT2_IMMUTABLE_FL) != 0);
		fsa.push_back(ptr);
		ptr = nullptr;
#endif
#ifdef EXT3_JOURNAL_DATA_FL
		create_or_throw(ptr, fsaf_linux_extX, fsan_data_journaling, (f & EXT3_JOURNAL_DATA_FL) != 0);
		fsa.push_back(ptr);
		ptr = nullptr;
#else
#ifdef EXT2_JOURNAL_DATA_FL
		create_or_throw(ptr, fsaf_linux_extX, fsan_data_journaling, (f & EXT2_JOURNAL_DATA_FL) != 0);
		fsa.push_back(ptr);
		ptr = nullptr;
#endif
#endif
#ifdef	EXT2_SECRM_FL
		create_or_throw(ptr, fsaf_linux_extX, fsan_secure_deletion, (f & EXT2_SECRM_FL) != 0);
		fsa.push_back(ptr);
		ptr = nullptr;
#endif
#ifdef EXT2_NOTAIL_FL
		create_or_throw(ptr, fsaf_linux_extX, fsan_no_tail_merging, (f & EXT2_NOTAIL_FL) != 0);
		fsa.push_back(ptr);
		ptr = nullptr;
#endif
#ifdef	EXT2_UNRM_FL
		create_or_throw(ptr, fsaf_linux_extX, fsan_undeletable, (f & EXT2_UNRM_FL) != 0);
		fsa.push_back(ptr);
		ptr = nullptr;
#endif
#ifdef EXT2_NOATIME_FL
		create_or_throw(ptr, fsaf_linux_extX, fsan_noatime_update, (f & EXT2_NOATIME_FL) != 0);
		fsa.push_back(ptr);
		ptr = nullptr;
#endif
#ifdef EXT2_DIRSYNC_FL
		create_or_throw(ptr, fsaf_linux_extX, fsan_synchronous_directory, (f & EXT2_DIRSYNC_FL) != 0);
		fsa.push_back(ptr);
		ptr = nullptr;
#endif
#ifdef EXT2_SYNC_FL
		create_or_throw(ptr, fsaf_linux_extX, fsan_synchronous_update, (f & EXT2_SYNC_FL) != 0);
		fsa.push_back(ptr);
		ptr = nullptr;
#endif
#ifdef EXT2_TOPDIR_FL
		create_or_throw(ptr, fsaf_linux_extX, fsan_top_of_dir_hierarchy, (f & EXT2_TOPDIR_FL) != 0);
		fsa.push_back(ptr);
		ptr = nullptr;
#endif
	    }
	    catch(...)
	    {
		close(fd);
		throw;
	    }
	    close(fd);
	}
#else
	    // nothing to do, as this FSA has not been activated at compilation time
#endif
    }

    void filesystem_specific_attribute_list::fill_HFS_FSA_with(user_interaction & ui,
							       const std::string & target,
							       mode_t itype,
							       bool auto_zeroing_neg_dates)
    {
#ifdef LIBDAR_BIRTHTIME
	struct stat tmp;
	int lu = stat(target.c_str(), &tmp);

	if(lu < 0)
	    return; // silently aborting assuming FSA family not supported for that file
	else
	{
	    fsa_time * ptr = nullptr;
#ifdef LIBDAR_MICROSECOND_READ_ACCURACY
	    tools_check_negative_date(tmp.st_birthtim.tv_sec,
				      ui,
				      target.c_str(),
				      "birthtime",
				      !auto_zeroing_neg_dates,
				      auto_zeroing_neg_dates);
	    datetime birthtime = datetime(tmp.st_birthtim.tv_sec, tmp.st_birthtim.tv_nsec/1000, datetime::tu_microsecond);
	    if(birthtime.is_null()) // assuming an error avoids getting time that way
		birthtime = datetime(tmp.st_birthtime, 0, datetime::tu_second);
#else
	    tools_check_negative_date(tmp.st_birthtime,
				      ui,
				      target.c_str(),
				      "birthtime",
				      !auto_zeroing_neg_dates,
				      auto_zeroing_neg_dates);
	    datetime birthtime = datetime(tmp.st_birthtime, 0, datetime::tu_second);
#endif
	    create_or_throw(ptr, fsaf_hfs_plus, fsan_creation_date, birthtime);

	    fsa.push_back(ptr);
	    ptr = nullptr;
	}
#endif
    }

    bool filesystem_specific_attribute_list::set_extX_FSA_to(user_interaction & ui,
							     const std::string & target,
							     bool set_immutable) const
    {
	bool ret = false;
	bool has_extX_FSA = false;
	deque<filesystem_specific_attribute *>::const_iterator it = fsa.begin();

	while(!has_extX_FSA && it != fsa.end())
	{
	    if(*it == nullptr)
		throw SRC_BUG;
	    if((*it)->get_family() == fsaf_linux_extX)
		has_extX_FSA = true;
	    ++it;
	}

#ifdef LIBDAR_NODUMP_FEATURE

	if(has_extX_FSA)
	{
	    S_I fd = -1;

	    try
	    {
		bool furtive = capability_FOWNER(ui, false) == capa_set ? compile_time::furtive_read() : false;
		fichier_local ftmp = fichier_local(target, furtive);
		fd = ftmp.give_fd_and_terminate();
	    }
	    catch(Egeneric & e)
	    {
		throw Erange("filesystem_specific_attribute_list::fill_extX_FSA_with", string(gettext("Failed setting (opening) extX family FSA: ")) + e.get_message());
	    }

	    if(fd < 0)
		throw SRC_BUG;

	    try
	    {
		S_I f = 0;      // will contain the desirable flag bits field
		S_I f_orig = 0; // will contain the original flag bits field
		const fsa_bool *it_bool = nullptr;

		if(ioctl(fd, EXT2_IOC_GETFLAGS, &f_orig) < 0)
		    throw Erange("filesystem_specific_attribute_list::fill_extX_FSA_with", string(gettext("Failed reading existing extX family FSA: ")) + tools_strerror_r(errno));
		f = f_orig;

		for(it = fsa.begin() ; it != fsa.end() ; ++it)
		{
		    if(*it == nullptr)
			throw SRC_BUG;

		    it_bool = dynamic_cast<const fsa_bool *>(*it);

		    if((*it)->get_family() == fsaf_linux_extX)
		    {
			switch((*it)->get_nature())
			{
			case fsan_unset:
			    throw SRC_BUG;
			case fsan_creation_date:
			    throw SRC_BUG; // unknown nature for this family type
			case fsan_append_only:
			    if(set_immutable)
				continue;
			    if(it_bool == nullptr)
				throw SRC_BUG; // should be a boolean
#ifdef EXT2_APPEND_FL
			    if(it_bool->get_value())
				f |= EXT2_APPEND_FL;
			    else
				f &= ~EXT2_APPEND_FL;
#else
			    ui.printf(gettext("Warning: FSA %s/%s support has not been found at compilation time, cannot restore it for inode %s"),
				      fsa_family_to_string(fsaf_linux_extX).c_str(),
				      fsa_nature_to_string((*it)->get_nature()).c_str(),
				      target.c_str());
#endif
			    break;
			case fsan_compressed:
			    if(set_immutable)
				continue;
#ifdef EXT2_COMPR_FL
			    if(it_bool->get_value())
				f |= EXT2_COMPR_FL;
			    else
				f &= ~EXT2_COMPR_FL;
#else
			    ui.printf(gettext("Warning: FSA %s/%s support has not been found at compilation time, cannot restore it for inode %s"),
				      fsa_family_to_string(fsaf_linux_extX).c_str(),
				      fsa_nature_to_string((*it)->get_nature()).c_str(),
				      target.c_str());
#endif
			    break;
			case fsan_no_dump:
			    if(set_immutable)
				continue;
#ifdef EXT2_NODUMP_FL
			    if(it_bool->get_value())
				f |= EXT2_NODUMP_FL;
			    else
				f &= ~EXT2_NODUMP_FL;
#else
			    ui.printf(gettext("Warning: FSA %s/%s support has not been found at compilation time, cannot restore it for inode %s"),
				      fsa_family_to_string(fsaf_linux_extX).c_str(),
				      fsa_nature_to_string((*it)->get_nature()).c_str(),
				      target.c_str());
#endif
			    break;
			case fsan_immutable:
			    if(!set_immutable) // if *NOT* set_immutable, here
				continue;
#ifdef EXT2_IMMUTABLE_FL
			    if(it_bool->get_value())
				f |= EXT2_IMMUTABLE_FL;
			    else
				f &= ~EXT2_IMMUTABLE_FL;
#else
			    ui.printf(gettext("Warning: FSA %s/%s support has not been found at compilation time, cannot restore it for inode %s"),
				      fsa_family_to_string(fsaf_linux_extX).c_str(),
				      fsa_nature_to_string((*it)->get_nature()).c_str(),
				      target.c_str());
#endif
			    break;
			case fsan_data_journaling:
			    if(set_immutable)
				continue;
#ifdef EXT3_JOURNAL_DATA_FL
			    if(it_bool->get_value())
				f |= EXT3_JOURNAL_DATA_FL;
			    else
				f &= ~EXT3_JOURNAL_DATA_FL;
#else
#ifdef EXT2_JOURNAL_DATA_FL
			    if(it_bool->get_value())
				f |= EXT2_JOURNAL_DATA_FL;
			    else
				f &= ~EXT2_JOURNAL_DATA_FL;
#else
			    ui.printf(gettext("Warning: FSA %s/%s support has not been found at compilation time, cannot restore it for inode %s"),
				      fsa_family_to_string(fsaf_linux_extX).c_str(),
				      fsa_nature_to_string((*it)->get_nature()).c_str(),
				      target.c_str());
#endif
#endif
			    break;
			case fsan_secure_deletion:
			    if(set_immutable)
				continue;
#ifdef EXT2_SECRM_FL
			    if(it_bool->get_value())
				f |= EXT2_SECRM_FL;
			    else
				f &= ~EXT2_SECRM_FL;
#else
			    ui.printf(gettext("Warning: FSA %s/%s support has not been found at compilation time, cannot restore it for inode %s"),
				      fsa_family_to_string(fsaf_linux_extX).c_str(),
				      fsa_nature_to_string((*it)->get_nature()).c_str(),
				      target.c_str());
#endif
			    break;
			case fsan_no_tail_merging:
			    if(set_immutable)
				continue;
#ifdef EXT2_NOTAIL_FL
			    if(it_bool->get_value())
				f |= EXT2_NOTAIL_FL;
			    else
				f &= ~EXT2_NOTAIL_FL;
#else
			    ui.printf(gettext("Warning: FSA %s/%s support has not been found at compilation time, cannot restore it for inode %s"),
				      fsa_family_to_string(fsaf_linux_extX).c_str(),
				      fsa_nature_to_string((*it)->get_nature()).c_str(),
				      target.c_str());
#endif
			    break;
			case fsan_undeletable:
			    if(set_immutable)
				continue;
#ifdef EXT2_UNRM_FL
			    if(it_bool->get_value())
				f |= EXT2_UNRM_FL;
			    else
				f &= ~EXT2_UNRM_FL;
#else
			    ui.printf(gettext("Warning: FSA %s/%s support has not been found at compilation time, cannot restore it for inode %s"),
				      fsa_family_to_string(fsaf_linux_extX).c_str(),
				      fsa_nature_to_string((*it)->get_nature()).c_str(),
				      target.c_str());
#endif
			    break;
			case fsan_noatime_update:
			    if(set_immutable)
				continue;
#ifdef EXT2_NOATIME_FL
			    if(it_bool->get_value())
				f |= EXT2_NOATIME_FL;
			    else
				f &= ~EXT2_NOATIME_FL;
#else
			    ui.printf(gettext("Warning: FSA %s/%s support has not been found at compilation time, cannot restore it for inode %s"),
				      fsa_family_to_string(fsaf_linux_extX).c_str(),
				      fsa_nature_to_string((*it)->get_nature()).c_str(),
				      target.c_str());
#endif
			    break;
			case fsan_synchronous_directory:
			    if(set_immutable)
				continue;
#ifdef EXT2_DIRSYNC_FL
			    if(it_bool->get_value())
				f |= EXT2_DIRSYNC_FL;
			    else
				f &= ~EXT2_DIRSYNC_FL;
#else
			    ui.printf(gettext("Warning: FSA %s/%s support has not been found at compilation time, cannot restore it for inode %s"),
				      fsa_family_to_string(fsaf_linux_extX).c_str(),
				      fsa_nature_to_string((*it)->get_nature()).c_str(),
				      target.c_str());
#endif
			    break;
			case fsan_synchronous_update:
			    if(set_immutable)
				continue;
#ifdef EXT2_SYNC_FL
			    if(it_bool->get_value())
				f |= EXT2_SYNC_FL;
			    else
				f &= ~EXT2_SYNC_FL;
#else
			    ui.printf(gettext("Warning: FSA %s/%s support has not been found at compilation time, cannot restore it for inode %s"),
				      fsa_family_to_string(fsaf_linux_extX).c_str(),
				      fsa_nature_to_string(fsan_append_only).c_str(),
				      target.c_str());
#endif
			    break;
			case fsan_top_of_dir_hierarchy:
			    if(set_immutable)
				continue;
#ifdef EXT2_TOPDIR_FL
			    if(it_bool->get_value())
				f |= EXT2_TOPDIR_FL;
			    else
				f &= ~EXT2_TOPDIR_FL;
#else
			    ui.printf(gettext("Warning: FSA %s/%s support has not been found at compilation time, cannot restore it for inode %s"),
				      fsa_family_to_string(fsaf_linux_extX).c_str(),
				      fsa_nature_to_string((*it)->get_nature()).c_str(),
				      target.c_str());
#endif
			    break;
			default:
			    throw SRC_BUG;
			}
		    }
		}

		    // now that f has been totally computed
		    // we must handle the point that some FSA flag
		    // need specific privileged to be set or cleared

		S_I mask_IMMUT = 0;   // will carry the mask for flags that need the IMMUTABLE capability
		S_I mask_SYS_RES = 0; // will carry the mask for flags that need the SYS_RESOURCE capability
#ifdef EXT2_APPEND_FL
		mask_IMMUT |= EXT2_APPEND_FL;
#endif
#ifdef EXT2_IMMUTABLE_FL
		mask_IMMUT |= EXT2_IMMUTABLE_FL;
#endif
#ifdef EXT3_JOURNAL_DATA_FL
		mask_SYS_RES |= EXT3_JOURNAL_DATA_FL;
#else
#ifdef EXT3_JOURNAL_DATA_FL
		mask_SYS_RES |= EXT2_JOURNAL_DATA_FL;
#endif
#endif
		    // now that masks have been computed, we will proceed
		    // in several steps:
		    // - first setting the flag that do not need any privileges (abort upon error)
		    // - second set the flags that need IMMUTABLE capability (warn and continue upon error)
		    // - third set the flags that need SYS_RESOURCE capability (warn and continue upon error)

		    // STEP 1: non privileged flags

		S_I tmp_f = (f & ~mask_IMMUT & ~mask_SYS_RES) | (f_orig & (mask_IMMUT | mask_SYS_RES));


		if(tmp_f != f_orig)
		{
		    if(ioctl(fd, EXT2_IOC_SETFLAGS, &tmp_f) < 0)
			throw Erange("filesystem_specific_attribute_list::fill_extX_FSA_with", string(gettext("Failed set extX family FSA: ")) + tools_strerror_r(errno ));
		    ret = true; // some flags have been set or cleared
		}

		f_orig = tmp_f; // f_orig has been modified with the new values of the non priviledged flag bits

		    // STEP 2 : setting the IMMUTABLE flags only

		if((f & mask_IMMUT) != (f_orig & mask_IMMUT)) // some immutable flags need to be changed
		{
		    tmp_f = (f & mask_IMMUT) | (f_orig & ~mask_IMMUT); // only diff is IMMUTABLE flags
		    switch(capability_LINUX_IMMUTABLE(ui, true))
		    {
		    case capa_set:
		    case capa_unknown:
			if(ioctl(fd, EXT2_IOC_SETFLAGS, &tmp_f) < 0)
			{
			    string tmp = tools_strerror_r(errno);
			    ui.printf("Failed setting FSA extX IMMUTABLE flags for %s: %", target.c_str(), tmp.c_str());
			}
			else
			{
			    f_orig = tmp_f; // f_orig now integrates the IMMUTABLE flags that we could set
			    ret = true; // some flags have been set or cleared
			}
			break;
		    case capa_clear:
			ui.printf(gettext("Not setting FSA extX IMMUTABLE flags for %s due to of lack of capability"), target.c_str());
			break;
		    default:
			throw SRC_BUG;
		    }
		}

		    /////// setting the SYS_RESOURCE flags only

		if((f & mask_SYS_RES) != (f_orig & mask_SYS_RES)) // some SYS_RESOURCE flags need to be changed
		{
		    tmp_f = (f & mask_SYS_RES) | (f_orig & ~mask_SYS_RES); // only diff is the SYS_RES flags
		    switch(capability_SYS_RESOURCE(ui, true))
		    {
		    case capa_set:
		    case capa_unknown:
			if(ioctl(fd, EXT2_IOC_SETFLAGS, &tmp_f) < 0)
			{
			    string tmp = tools_strerror_r(errno);
			    ui.printf("Failed setting FSA extX SYSTEME RESOURCE flags for %s: %", target.c_str(), tmp.c_str());
			}
			else
			{
			    f_orig = tmp_f; // f_orig now integrates the SYS_RESOURCE flags that we could set
			    ret = true; // some flags have been set or cleared
			}
			break;
		    case capa_clear:
			ui.printf(gettext("Not setting FSA extX SYSTEM RESOURCE flags for %s due to of lack of capability"), target.c_str());
			break;
		    default:
			throw SRC_BUG;
		    }
		}
	    }
	    catch(...)
	    {
		close(fd);
		throw;
	    }
	    close(fd);
	}

#else
	if(has_extX_FSA)
	{
	    ui.printf(gettext("Warning! %s Filesystem Specific Attribute support have not been activated at compilation time and could not be restored for %s"),
		      fsa_family_to_string(fsaf_linux_extX).c_str(),
		      target.c_str());
	}
#endif

	return ret;
    }

    bool filesystem_specific_attribute_list::set_hfs_FSA_to(user_interaction & ui, const std::string & target) const
    {
	bool ret = false;

	    // the birthtime is set with the different other dates of that inode, so
	    // here we just check that this FSA list provides a birthtime info:
	const filesystem_specific_attribute *tmp = nullptr;

	ret = find(fsaf_hfs_plus, fsan_creation_date, tmp);
#ifndef LIBDAR_BIRTHTIME
	if(ret)
	    ui.printf(gettext("Birth Time attribute cannot be restored for %s because no FSA familly able to carry that attribute could be activated at compilation time."),
		      target.c_str());
	    // here we just warn, the birthtime restoration will be tried (calling twice utime()), even
	    // if dar has not been compiled with birthtime support. Birthtime support is necessary only to
	    // read birthtime value of an inode
#endif

	return ret;
    }

    string filesystem_specific_attribute_list::family_to_signature(fsa_family f)
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
	case fsan_append_only:
	    ret = "ba";
	    break;
	case fsan_compressed:
	    ret = "bb";
	    break;
	case fsan_no_dump:
	    ret = "bc";
	    break;
	case fsan_immutable:
	    ret = "bd";
	    break;
	case fsan_data_journaling:
	    ret = "be";
	    break;
	case fsan_secure_deletion:
	    ret = "bf";
	    break;
	case fsan_no_tail_merging:
	    ret = "bg";
	    break;
	case fsan_undeletable:
	    ret = "bh";
	    break;
	case fsan_noatime_update:
	    ret = "bi";
	    break;
	case fsan_synchronous_directory:
	    ret = "bj";
	    break;
	case fsan_synchronous_update:
	    ret = "bk";
	    break;
	case fsan_top_of_dir_hierarchy:
	    ret = "bl";
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

    fsa_family filesystem_specific_attribute_list::signature_to_family(const string & sig)
    {
	if(sig.size() != FAM_SIG_WIDTH)
	    throw Erange("filesystem_specific_attribute_list::signature_to_family", gettext("invalid length for FSA family flag"));
	if(sig == "h")
	    return fsaf_hfs_plus;
	if(sig == "l")
	    return fsaf_linux_extX;
	if(sig == "X") // resevered for field extension if necessary in the future
	    throw Erange("filesysttem_specific_attribute_list::signature_to_family", gettext("invalid FSA family flag"));
	throw Erange("filesysttem_specific_attribute_list::signature_to_family", gettext("invalid FSA family flag"));
    }

    fsa_nature filesystem_specific_attribute_list::signature_to_nature(const string & sig)
    {
	if(sig.size() != NAT_SIG_WIDTH)
	    throw SRC_BUG;
	if(sig == "aa")
	    return fsan_creation_date;
	if(sig == "ba")
	    return fsan_append_only;
	if(sig == "bb")
	    return fsan_compressed;
	if(sig == "bc")
	    return fsan_no_dump;
	if(sig == "bd")
	    return fsan_immutable;
	if(sig == "be")
	    return fsan_data_journaling;
	if(sig == "bf")
	    return fsan_secure_deletion;
	if(sig == "bg")
	    return fsan_no_tail_merging;
	if(sig == "bh")
	    return fsan_undeletable;
	if(sig == "bi")
	    return fsan_noatime_update;
	if(sig == "bj")
	    return fsan_synchronous_directory;
	if(sig == "bk")
	    return fsan_synchronous_update;
	if(sig == "bl")
	    return fsan_top_of_dir_hierarchy;
	if(sig == "XX") // resevered for field extension if necessary in the future
	    throw Erange("filesystem_specific_attribute_list::signature_to_nature", gettext("invalid FSA nature flag"));

	throw Erange("filesystem_specific_attribute_list::signature_to_nature", gettext("invalid FSA nature flag"));
    }



///////////////////////////////////////////////////////////////////////////////////

    fsa_bool::fsa_bool(generic_file & f, fsa_family fam, fsa_nature nat):
	filesystem_specific_attribute(f, fam, nat)
    {
	char ch;
	S_I lu = f.read(&ch, 1);

	if(lu == 1)
	{
	    switch(ch)
	    {
	    case 'T':
		val = true;
		break;
	    case 'F':
		val = false;
		break;
	    default:
		throw Edata(gettext("Unexepected value for boolean FSA, data corruption may have occurred"));
	    }
	}
	else
	    throw Erange("fsa_bool::fsa_bool", string(gettext("Error while reading FSA: ")) + tools_strerror_r(errno));
    }


    bool fsa_bool::equal_value_to(const filesystem_specific_attribute & ref) const
    {
	const fsa_bool *ptr = dynamic_cast<const fsa_bool *>(&ref);

	if(ptr != nullptr)
	    return val == ptr->val;
	else
	    return false;
    }

///////////////////////////////////////////////////////////////////////////////////

    fsa_infinint::fsa_infinint(generic_file & f, fsa_family fam, fsa_nature nat):
	filesystem_specific_attribute(f, fam, nat)
    {
	val.read(f);
    }

    string fsa_infinint::show_val() const
    {
	return deci(val).human();
    }

    bool fsa_infinint::equal_value_to(const filesystem_specific_attribute & ref) const
    {
	const fsa_infinint *ptr = dynamic_cast<const fsa_infinint *>(&ref);

	if(ptr != nullptr)
	    return val == ptr->val;
	else
	    return false;
    }

///////////////////////////////////////////////////////////////////////////////////

    fsa_time::fsa_time(generic_file & f, archive_version ver, fsa_family fam, fsa_nature nat):
	filesystem_specific_attribute(f, fam, nat)
    {
	val.read(f, ver);
    }

    string fsa_time::show_val() const
    {
	return tools_display_date(val);
    }

    bool fsa_time::equal_value_to(const filesystem_specific_attribute & ref) const
    {
	const fsa_time *ptr = dynamic_cast<const fsa_time *>(&ref);

	if(ptr != nullptr)
	    return val == ptr->val;
	else
	    return false;
    }


} // end of namespace
