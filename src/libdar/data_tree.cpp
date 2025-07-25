/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2025 Denis Corbin
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
// to contact the author, see the AUTHOR file
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
// to allow compilation under Cygwin we need <sys/types.h>
// else Cygwin's <netinet/in.h> lack __int16_t symbol !?!
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
} // end extern "C"

#include <iomanip>

#include "data_tree.hpp"
#include "tools.hpp"
#include "user_interaction.hpp"
#include "deci.hpp"
#include "path.hpp"
#include "datetime.hpp"
#include "cat_all_entrees.hpp"
#include "candidates.hpp"

using namespace std;

namespace libdar
{

    void data_tree::status::dump(generic_file & f) const
    {
	date.dump(f);
	switch(present)
	{
	case db_etat::et_saved:
	    f.write(ETAT_SAVED, 1);
	    break;
	case db_etat::et_patch:
	    f.write(ETAT_PATCH, 1);
	    break;
	case db_etat::et_patch_unusable:
	    f.write(ETAT_PATCH_UNUSABLE, 1);
	    break;
	case db_etat::et_inode:
	    f.write(ETAT_INODE, 1);
	    break;
	case db_etat::et_present:
	    f.write(ETAT_PRESENT, 1);
	    break;
	case db_etat::et_removed:
	    f.write(ETAT_REMOVED, 1);
	    break;
	case db_etat::et_absent:
	    f.write(ETAT_ABSENT, 1);
	    break;
	default:
	    throw SRC_BUG;
	}
    }

    void data_tree::status::read(generic_file & f, unsigned char db_version)
    {
	char tmp;

	date.read(f, db2archive_version(db_version));
	if(f.read(&tmp, 1) != 1)
	    throw Erange("data_tree::status::read", gettext("reached End of File before all expected data could be read"));
	switch(tmp)
	{
	case ETAT_SAVED[0]:
	    present = db_etat::et_saved;
	    break;
	case ETAT_PRESENT[0]:
	    present = db_etat::et_present;
	    break;
	case ETAT_REMOVED[0]:
	    present = db_etat::et_removed;
	    break;
	case ETAT_INODE[0]:
	    present = db_etat::et_inode;
	    break;
	case ETAT_ABSENT[0]:
	    present = db_etat::et_absent;
	    break;
	case ETAT_PATCH[0]:
	    present = db_etat::et_patch;
	    break;
	case ETAT_PATCH_UNUSABLE[0]:
	    present = db_etat::et_patch_unusable;
	    break;
	default:
	    throw Erange("data_tree::status::read", gettext("Unexpected value found in database"));
	}
    }

    data_tree::status_plus::status_plus(const datetime & d,
					db_etat p,
					const crc *xbase,
					const crc *xresult): status(d, p)
    {
	base = result = nullptr;

	try
	{
	    if(xbase != nullptr)
	    {
		base = xbase->clone();
		if(base == nullptr)
		    throw Ememory("data_tree::status_plus::status_plus");
	    }

	    if(xresult != nullptr)
	    {
		result = xresult->clone();
		if(result == nullptr)
		    throw Ememory("data_tree::status_plus::status_plus");
	    }
	}
	catch(...)
	{
	    if(base != nullptr)
		delete base;
	    if(result != nullptr)
		delete result;
	    throw;
	}
    }

    void data_tree::status_plus::dump(generic_file & f) const
    {
	unsigned char flag = 0;
	if(base != nullptr)
	    flag |= STATUS_PLUS_FLAG_ME;
	if(result != nullptr)
	    flag |= STATUS_PLUS_FLAG_REF;

	status::dump(f);
	f.write((char *)&flag, 1);
	if(base != nullptr)
	    base->dump(f);
	if(result != nullptr)
	    result->dump(f);
    }

    void data_tree::status_plus::read(generic_file &f,
				      unsigned char db_version)
    {
	unsigned char flag;

	detruit();

	status::read(f, db_version);
	switch(db_version)
	{
	case 1:
	case 2:
	case 3:
	case 4:
		// nothing to be done, only a status class/struct
		// was used for these versions
	    break;
	case 5:
	case 6:
	    f.read((char *)&flag, 1);
	    if((flag & STATUS_PLUS_FLAG_ME) != 0)
		base = create_crc_from_file(f, false);
	    if((flag & STATUS_PLUS_FLAG_REF) != 0)
		result = create_crc_from_file(f, false);
	    break;
	default:
	    throw SRC_BUG;
	}
    }

    void data_tree::status_plus::copy_from(const status_plus & xref)
    {
	status *moi = this;
	const status *toi = &xref;

	*moi = *toi;

	if(xref.base != nullptr)
	{
	    base = xref.base->clone();
	    if(base == nullptr)
		throw Ememory("data_tree::status_plus::copy_from");
	}
	else
	    base = nullptr;

	if(xref.result != nullptr)
	{
	    result = xref.result->clone();
	    if(result == nullptr)
		throw Ememory("data_tree::status_plus::copy_from");
	}
	else
	    result = nullptr;
    }

    void data_tree::status_plus::move_from(status_plus && ref) noexcept
    {
	swap(base, ref.base);
	swap(result, ref.result);
    }

    void data_tree::status_plus::detruit()
    {
	if(base != nullptr)
	{
	    delete base;
	    base = nullptr;
	}
	if(result != nullptr)
	{
	    delete result;
	    result = nullptr;
	}
    }

    data_tree::data_tree(const string & name)
    {
	filename = name;
    }

    data_tree::data_tree(generic_file & f, unsigned char db_version)
    {
	archive_num k;
	status sta;
	status_plus sta_plus;

	    // signature has already been read
	tools_read_string(f, filename);
	infinint tmp = infinint(f); // number of entry in last_mod map
	while(!tmp.is_zero())
	{
	    k.read_from_file(f);
	    switch(db_version)
	    {
	    case 1:
		sta_plus.date = infinint(f);
		sta_plus.present = db_etat::et_saved;
		    // me and ref fields are already set to nullptr
		last_mod[k] = sta_plus;
		break;
	    case 2:
	    case 3:
	    case 4:
	    case 5:
	    case 6:
		sta_plus.read(f, db_version);
		last_mod[k] = sta_plus;
		break;
	    default: // unsupported database version
		throw SRC_BUG; // this statement should have been prevented earlier in the code to avoid destroying or loosing data from the database
	    }
	    --tmp;
	}

	tmp = infinint(f); // number of entry in last_change map
	while(!tmp.is_zero())
	{
	    k.read_from_file(f);
	    switch(db_version)
	    {
	    case 1:
		sta.date = infinint(f);
		sta.present = db_etat::et_saved;
		last_change[k] = sta;
		break;
	    case 2:
	    case 3:
	    case 4:
	    case 5:
	    case 6:
		sta.read(f, db_version);
		last_change[k] = sta;
		break;
	    default:
		throw SRC_BUG;
	    }
	    --tmp;
	}
    }

    void data_tree::dump(generic_file & f) const
    {
	char tmp = obj_signature();
	infinint sz;
	map<archive_num, status_plus>::const_iterator itp = last_mod.begin();

	f.write(&tmp, 1);
	tools_write_string(f, filename);

	    // last mod table dump

	sz = last_mod.size();
	sz.dump(f);
	while(itp != last_mod.end())
	{
	    itp->first.write_to_file(f); // key
	    itp->second.dump(f);         // value
	    ++itp;
	}

	    // last change table dump

	sz = last_change.size();
	sz.dump(f);

	map<archive_num, status>::const_iterator it = last_change.begin();
	while(it != last_change.end())
	{
	    it->first.write_to_file(f); // key
	    it->second.dump(f);         // value
	    ++it;
	}
    }

    db_lookup data_tree::get_data(set<archive_num> & archive, const datetime & date, bool even_when_removed) const
    {

		// we will pass each element of the last_mod map, following the order used by std::map
		// which is based on the order of the first element, here archive_num, thus "it" will
		// be pointing at each entry, from the smallest to the highest archive number

	datetime max_seen_date = datetime(0);
	candidates candy(even_when_removed); //< Au pays de Candy, comme dans tous les pays ...
	map<archive_num, status_plus>::const_iterator it = last_mod.begin();

	while(it != last_mod.end())
	{
		// this test is necessary to take care of problem of order in the database
	    if(it->second.date >= max_seen_date
		   // ">=" (not ">") because there should never be twice the same value
		   // and we must be able to see a value of 0 (initially max = 0) which is valid.
	       && (date.is_null() || it->second.date <= date))
	    {
		max_seen_date = it->second.date;
		candy.add(it->first, it->second.present);
	    }
	    ++it;
	}

	candy.set_the_set(archive);
	return candy.get_status();
    }

    db_lookup data_tree::get_EA(archive_num & archive, const datetime & date, bool even_when_removed) const
    {
	map<archive_num, status>::const_iterator it = last_change.begin();
	datetime max_seen_date = datetime(0), max_real_date = datetime(0);
	bool presence_seen = false, presence_real = false;
	archive_num last_archive_seen = 0;
	archive_num last_archive_even_when_removed = 0;
	db_lookup ret;

	archive = 0; // 0 is never assigned to an archive number
	while(it != last_change.end())
	{
	    if(it->second.date >= max_seen_date  // > and = because there should never be twice the same value
		   // and we must be able to see a value of 0 (initially max = 0) which is valid.
	       && (date.is_null() || it->second.date <= date))
	    {
		max_seen_date = it->second.date;
		last_archive_seen = it->first;
		switch(it->second.present)
		{
		case db_etat::et_saved:
		case db_etat::et_present:
		    presence_seen = true;
		    break;
		case db_etat::et_removed:
		case db_etat::et_absent:
		    presence_seen = false;
		    break;
		default:
		    throw SRC_BUG;
		}
	    }

	    if(it->second.date >= max_real_date  // > and = because there should never be twice the same value
		   // and we must be able to see a value of 0 (initially max = 0) which is valid.
	       && (date.is_null() || it->second.date <= date))
	    {
		if(it->second.present != db_etat::et_present)
		{
		    max_real_date = it->second.date;
		    archive = it->first;
		    switch(it->second.present)
		    {
		    case db_etat::et_saved:
			presence_real = true;
			last_archive_even_when_removed = archive;
			break;
		    case db_etat::et_removed:
		    case db_etat::et_absent:
			presence_real = false;
			break;
		    case db_etat::et_present:
			throw SRC_BUG;
		    default:
			throw SRC_BUG;
		    }
		}
	    }

	    ++it;
	}

	if(even_when_removed && last_archive_even_when_removed > 0)
	{
	    archive = last_archive_even_when_removed;
	    presence_seen = presence_real = true;
	}


	if(archive == 0)
	    if(last_archive_seen != 0) // last acceptable entry is a file present but not saved, but no full backup is present in a previous archive
		ret = db_lookup::not_restorable;
	    else
		ret = db_lookup::not_found;
	else
	    if(last_archive_seen != 0)
		if(presence_seen && !presence_real) // last acceptable entry is a file present but not saved, but no full backup is present in a previous archive
		    ret = db_lookup::not_restorable;
		else
		    if(presence_seen != presence_real)
			throw SRC_BUG;
		    else  // archive > 0 && presence_seen == present_real
			if(presence_real)
			    ret = db_lookup::found_present;
			else
			    ret = db_lookup::found_removed;
	    else
		throw SRC_BUG;

	return ret;
    }

    bool data_tree::read_data(archive_num num,
			      datetime & val,
			      db_etat & present) const
    {
	map<archive_num, status_plus>::const_iterator it = last_mod.find(num);

	if(it != last_mod.end())
	{
	    val = it->second.date;
	    present = it->second.present;
	    return true;
	}
	else
	    return false;
    }

    bool data_tree::read_EA(archive_num num,
			    datetime & val,
			    db_etat & present) const
    {
	map<archive_num, status>::const_iterator it = last_change.find(num);

	if(it != last_change.end())
	{
	    val = it->second.date;
	    present = it->second.present;
	    return true;
	}
	else
	    return false;
    }

    void data_tree::finalize(const archive_num & archive,
			     const datetime & deleted_date,
			     const archive_num & ignore_archives_greater_or_equal)
    {
	map<archive_num, status_plus>::iterator itp = last_mod.begin();
	bool presence_max = false;
	archive_num num_max = 0;
	datetime last_mtime = datetime(0); // used as deleted_date for EA if last mtime is found
	bool found_in_archive = false;

	    // checking the last_mod map

	while(itp != last_mod.end() && !found_in_archive)
	{
	    if(itp->first == archive && itp->second.present != db_etat::et_absent)
		found_in_archive = true;
	    else
		if(itp->first > num_max && (itp->first < ignore_archives_greater_or_equal || ignore_archives_greater_or_equal == 0))
		{
		    num_max = itp->first;
		    switch(itp->second.present)
		    {
		    case db_etat::et_saved:
		    case db_etat::et_present:
		    case db_etat::et_patch:
		    case db_etat::et_patch_unusable:
		    case db_etat::et_inode:
			presence_max = true;
			last_mtime = itp->second.date; // used as deleted_data for EA
			break;
		    case db_etat::et_removed:
		    case db_etat::et_absent:
			presence_max = false;
			last_mtime = itp->second.date; // keeping this date as it is
			    // not possible to know when the EA have been removed
			    // since it does not change mtime of file nor of its parent
			    // directory (this is an heuristic).
			break;
		    default:
			throw SRC_BUG;
		    }
		}
	    ++itp;
	}

	if(!found_in_archive) // not entry found for asked archive (or recorded as db_etat::et_absent)
	{
	    if(presence_max)
	    {
		    // the most recent entry found stated that this file
		    // existed at the time "last_mtime"
		    // and this entry is absent from the archive under addition
		    // thus we must record that it has been removed in the
		    // archive we currently add.

		if(deleted_date > last_mtime)
		    set_data(archive, deleted_date, db_etat::et_absent);
		    // add an entry telling that this file does no more exist in the current archive
		else
		    set_data(archive, last_mtime, db_etat::et_absent);
		    // add an entry telling thatthis file does no more exists, using the last known date
		    // as deleted data. This situation may appear when one makes a first backup
		    // then a second one but excluding from the backup that particular file. This file
		    // may reappear later with if is backup included in the backup possibily with the same
		    // date. In 2.4.x we added 1 second to the last known date to create the deleted date
		    // which lead out of order warning to show for the database. Since 2.5.x date resolution
		    // may be one microsecond (no more 1 second) thus we now keep the last known date as
		    // delete date
	    }
	    else // the entry has been seen previously but as removed in the latest state,
		    // if we already have an et_absent entry (which is possible during a reordering archive operation within a database), we can (and must) suppress it.
	    {
		itp = last_mod.find(archive);
		if(itp != last_mod.end()) // we have an entry associated to this archive
		{
		    switch(itp->second.present)
		    {
		    case db_etat::et_saved:
		    case db_etat::et_present:
		    case db_etat::et_patch:
		    case db_etat::et_patch_unusable:
		    case db_etat::et_inode:
			throw SRC_BUG; // entry has not been found in the current archive
		    case db_etat::et_removed:
			break;         // we must keep it, it was in the original archive
		    case db_etat::et_absent:
			last_mod.erase(itp); // this entry had been added from previous neighbor archive, we must remove it now, it was not part of the original archive
			break;
		    default:
			throw SRC_BUG;
		    }
		}
		    // else already absent from the base for this archive, cool.
	    }
	}
	    // else, entry found for the current archive



	    //////////////////////
	    // now checking the last_change map
	    //


	map<archive_num, status>::iterator it = last_change.begin();
	presence_max = false;
	num_max = 0;
	found_in_archive = false;

	while(it != last_change.end() && !found_in_archive)
	{
	    if(it->first == archive && it->second.present != db_etat::et_absent)
		found_in_archive = true;
	    else
		if(it->first > num_max && (it->first < ignore_archives_greater_or_equal || ignore_archives_greater_or_equal == 0))
		{
		    num_max = it->first;
		    switch(it->second.present)
		    {
		    case db_etat::et_saved:
		    case db_etat::et_present:
			presence_max = true;
			break;
		    case db_etat::et_removed:
		    case db_etat::et_absent:
			presence_max = false;
			break;
		    case db_etat::et_patch:
			throw SRC_BUG;
		    case db_etat::et_patch_unusable:
			throw SRC_BUG;
		    default:
			throw SRC_BUG;
		    }
		}
	    ++it;
	}

	if(!found_in_archive) // not entry found for asked archive
	{
	    if(num_max != 0) // num_max may be equal to zero if this entry never had any EA in any recorded archive
		if(presence_max)
		    set_EA(archive, (last_mtime < deleted_date ? deleted_date : last_mtime), db_etat::et_absent); // add an entry telling that EA for this file do no more exist in the current archive
		// else last entry found stated the EA was removed, nothing more to do
	}
	    // else, entry found for the current archive
    }

    bool data_tree::remove_all_from(const archive_num & archive_to_remove, const archive_num & last_archive)
    {
	map<archive_num, status_plus>::iterator itp = last_mod.begin();


	    // if this file was stored as "removed" in the archive we tend to remove from the database
	    // this "removed" information is propagated to the next archive if both:
	    //   - the next archive exists and has no information recorded about this file
	    //   - this entry does not only exist in the archive about to be removed
	if(archive_to_remove < last_archive)
	{
	    datetime del_date;
	    db_etat status;
	    if(last_mod.size() > 1 && read_data(archive_to_remove, del_date, status))
		if(status == db_etat::et_removed)
		{
		    datetime tmp;
		    if(!read_data(archive_to_remove + 1, tmp, status))
			set_data(archive_to_remove + 1, del_date, db_etat::et_removed);
		}
	    if(last_change.size() > 1 && read_EA(archive_to_remove, del_date, status))
		if(status == db_etat::et_removed)
		{
		    datetime tmp;
		    if(!read_EA(archive_to_remove + 1, tmp, status))
			set_EA(archive_to_remove + 1, del_date, db_etat::et_removed);
		}
	}

	while(itp != last_mod.end())
	{
	    if(itp->first == archive_to_remove)
	    {
		last_mod.erase(itp);
		break; // stops the while loop, as there is at most one element with that key
	    }
	    else
		++itp;
	}

	map<archive_num, status>::iterator it = last_change.begin();
	while(it != last_change.end())
	{
	    if(it->first == archive_to_remove)
	    {
		last_change.erase(it);
		break; // stops the while loop, as there is at most one element with that key
	    }
	    else
		++it;
	}

	(void)check_delta_validity();

	return last_mod.empty() && last_change.empty();
    }

    void data_tree::listing(database_listing_get_version_callback callback,
			    void *tag) const
    {
	map<archive_num, status_plus>::const_iterator it = last_mod.begin();
	map<archive_num, status>::const_iterator ut = last_change.begin();

	while(it != last_mod.end() || ut != last_change.end())
	{
	    if(it != last_mod.end())
		if(ut != last_change.end())
		    if(it->first == ut->first)
		    {
			display_line(callback, tag, it->first, &(it->second.date), it->second.present, &(ut->second.date), ut->second.present);
			++it;
			++ut;
		    }
		    else // not in the same archive
			if(it->first < ut->first) // it only
			{
			    display_line(callback, tag, it->first, &(it->second.date), it->second.present, nullptr, db_etat::et_removed);
			    ++it;
			}
			else // ut only
			{
			    display_line(callback, tag, ut->first, nullptr, db_etat::et_removed, &(ut->second.date), ut->second.present);
			    ++ut;
			}
		else // ut at end of list thus it != last_mod.end() (see while condition)
		{
		    display_line(callback, tag, it->first, &(it->second.date), it->second.present, nullptr, db_etat::et_removed);
		    ++it;
		}
	    else // it at end of list, this ut != last_change.end() (see while condition)
	    {
		display_line(callback, tag, ut->first, nullptr, db_etat::et_removed, &(ut->second.date), ut->second.present);
		++ut;
	    }
	}
    }

    void data_tree::apply_permutation(archive_num src, archive_num dst)
    {
	map<archive_num, status_plus> transfertp;
	map<archive_num, status_plus>::iterator itp = last_mod.begin();

	while(itp != last_mod.end())
	{
	    transfertp[data_tree_permutation(src, dst, itp->first)] = itp->second;
	    ++itp;
	}
	last_mod = transfertp;
	transfertp.clear();

	map<archive_num, status> transfert;
	map<archive_num, status>::iterator it = last_change.begin();

	while(it != last_change.end())
	{
	    transfert[data_tree_permutation(src, dst, it->first)] = it->second;
	    ++it;
	}
	last_change = transfert;
	(void)check_delta_validity();
    }

    void data_tree::skip_out(archive_num num)
    {
	map<archive_num, status_plus> resultantp;
	map<archive_num, status_plus>::iterator itp = last_mod.begin();
	infinint tmp;

	while(itp != last_mod.end())
	{
	    if(itp->first > num)
		resultantp[itp->first-1] = itp->second;
	    else
		resultantp[itp->first] = itp->second;
	    ++itp;
	}
	last_mod = resultantp;
	resultantp.clear();

	map<archive_num, status> resultant;
	map<archive_num, status>::iterator it = last_change.begin();
	while(it != last_change.end())
	{
	    if(it->first > num)
		resultant[it->first-1] = it->second;
	    else
		resultant[it->first] = it->second;
	    ++it;
	}
	last_change = resultant;
    }

    void data_tree::compute_most_recent_stats(deque<infinint> & data,
					      deque<infinint> & ea,
					      deque<infinint> & total_data,
					      deque<infinint> & total_ea,
					      const datetime & ignore_older_than_that) const
    {
	set<db_etat> flag_set;

	flag_set.insert(db_etat::et_saved);
	compute_for_flag_set(data,
			     ea,
			     total_data,
			     total_ea,
			     ignore_older_than_that,
			     flag_set,
			     true);
   }


    void data_tree::compute_restoration_needed_archives(deque<infinint> & data,
							deque<infinint> & ea,
							deque<infinint> & total_data,
							deque<infinint> & total_ea,
							const datetime & ignore_older_than_that) const
    {
	set<db_etat> flag_set;

	flag_set.insert(db_etat::et_saved);
	flag_set.insert(db_etat::et_patch);
	flag_set.insert(db_etat::et_inode);
	flag_set.insert(db_etat::et_removed);
	flag_set.insert(db_etat::et_absent);

	compute_for_flag_set(data,
			     ea,
			     total_data,
			     total_ea,
			     ignore_older_than_that,
			     flag_set,
			     false);
    }

    bool data_tree::fix_corruption()
    {
	bool ret = true;
	map<archive_num, status_plus>::iterator itp = last_mod.begin();

	while(itp != last_mod.end() && ret)
	{
	    if(itp->second.present != db_etat::et_removed && itp->second.present != db_etat::et_absent)
		ret = false;
	    ++itp;
	}

	map<archive_num, status>::iterator it = last_change.begin();
	while(it != last_change.end() && ret)
	{
	    if(it->second.present != db_etat::et_removed && it->second.present != db_etat::et_absent)
		ret = false;
	    ++it;
	}

	return ret;
    }

	// helper data structure for check_map_order method

    struct trecord
    {
	datetime date;
	bool set;

	trecord() { set = false; date = datetime(0); };
	trecord(const trecord & ref) { date = ref.date; set = ref.set; };
	trecord & operator = (const trecord & ref) { date = ref.date; set = ref.set; return *this; };
    };


    template<class T> bool data_tree::check_map_order(user_interaction & dialog,
						      const map<archive_num, T> the_map,
						      const path & current_path,
						      const string & field_nature,
						      bool & initial_warn) const
    {
	    // some variable to work with around the_map

	U_I dates_size = the_map.size()+1;
	vector<trecord> dates = vector<trecord>(dates_size);
	typename map<archive_num, T>::const_iterator it = the_map.begin();
	vector<trecord>::iterator rec_it;
	datetime last_date = datetime(0);

	    // now the algorithm

	    // extracting dates from map into an ordered vector

	while(it != the_map.end())
	{
	    trecord rec;

	    rec.date = it->second.date;
	    rec.set = true;
	    while(dates_size <= it->first)
	    {
		dates.push_back(trecord());
		++dates_size;
	    }
	    dates[it->first] = rec;

	    ++it;
	}

	    // checking whether dates are sorted crescendo following the archive number

	rec_it = dates.begin();

	while(rec_it != dates.end())
	{
	    if(rec_it->set)
	    {
		if(rec_it->date >= last_date)
		    last_date = rec_it->date;
		else // order is not respected
		{
		    string tmp = current_path.display() == "." ? get_name() : (current_path.append(get_name())).display();
		    dialog.printf(gettext("Dates of file's %S are not increasing when database's archive number grows. Concerned file is: %S"), &field_nature, &tmp);
		    if(initial_warn)
		    {
			dialog.message(gettext("Dates are not increasing for all files when database's archive number grows, working with this database may lead to improper file's restored version. Please reorder the archive within the database in the way that the older is the first archive and so on up to the most recent archive being the last of the database"));
			try
			{
			    dialog.pause(gettext("Do you want to ignore the same type of error for other files?"));
			    return false;
			}
			catch(Euser_abort & e)
			{
			    initial_warn = false;
			}
		    }
		    break; // aborting the while loop
		}
	    }
	    ++rec_it;
	}

	return true;
    }


    bool data_tree::check_delta_validity()
    {
	bool ret = true;
	const crc *prev = nullptr;

	for(map<archive_num, status_plus>::iterator it = last_mod.begin(); it != last_mod.end(); ++it)
	{
	    switch(it->second.present)
	    {
	    case db_etat::et_saved:
		prev = it->second.result;
		break;
	    case db_etat::et_patch:
	    case db_etat::et_patch_unusable:
		if(it->second.base == nullptr)
		    throw SRC_BUG;
		if(prev != nullptr && *prev == *(it->second.base))
		    it->second.present = db_etat::et_patch;
		else
		{
		    it->second.present = db_etat::et_patch_unusable;
		    ret = false;
		}
		prev = it->second.result;
		break;
	    case db_etat::et_present:
	    case db_etat::et_inode:
		    // we do not modify prev
		break;
	    case db_etat::et_removed:
	    case db_etat::et_absent:
		prev = nullptr;
		break;
	    default:
		throw SRC_BUG;
	    }
	}

	return ret;
    }

    void data_tree::compute_for_flag_set(deque<infinint> & data,
					 deque<infinint> & ea,
					 deque<infinint> & total_data,
					 deque<infinint> & total_ea,
					 const datetime & ignore_older_than_that,
					 const set<db_etat> & flag_set,
					 bool even_when_removed) const
    {
	archive_num most_recent = 0;
	datetime max = datetime(0);
	db_etat last_state = db_etat::et_absent;
	map<archive_num, status_plus>::const_iterator itp = last_mod.begin();

	while(itp != last_mod.end())
	{
	    if(flag_set.find(itp->second.present) != flag_set.end())
	    {
		if(itp->second.date >= max
		   &&
		   (ignore_older_than_that.is_null() || itp->second.date <= ignore_older_than_that)
		    )
		{
		    most_recent = itp->first;
		    max = itp->second.date;
		    last_state = itp->second.present;
		}
		++total_data[itp->first];
	    }
	    ++itp;
	}
	if(most_recent > 0)
	    if(even_when_removed || (last_state != db_etat::et_absent && last_state != db_etat::et_removed))
		++data[most_recent];

	most_recent = 0;
	max = datetime(0);
	last_state = db_etat::et_absent;
	map<archive_num, status>::const_iterator it = last_change.begin();

	while(it != last_change.end())
	{
	    if(flag_set.find(it->second.present) != flag_set.end())
	    {
		if(it->second.date >= max
		    &&
		   (ignore_older_than_that.is_null() || it->second.date <= ignore_older_than_that)
		    )
		{
		    most_recent = it->first;
		    max = it->second.date;
		    last_state = it->second.present;
		}
		++total_ea[it->first];
	    }
	    ++it;
	}
	if(most_recent > 0)
	    if(even_when_removed || (last_state != db_etat::et_absent && last_state != db_etat::et_removed))
		++ea[most_recent];
    }

    archive_num data_tree::data_tree_permutation(archive_num src, archive_num dst, archive_num x)
    {
	if(src < dst)
	    if(x < src || x > dst)
		return x;
	    else
		if(x == src)
		    return dst;
		else
		    return x-1;
	else
	    if(src == dst)
		return x;
	    else // src > dst
		if(x > src || x < dst)
		    return x;
		else
		    if(x == src)
			return dst;
		    else
			return x+1;
    }

    void data_tree::display_line(database_listing_get_version_callback callback,
				 void *tag,
				 archive_num num,
				 const datetime *data,
				 db_etat data_presence,
				 const datetime *ea,
				 db_etat ea_presence)
    {
	bool has_data_date = true;
	bool has_ea_date = true;

	if(data == nullptr)
	has_data_date = false;

	if(ea == nullptr)
	    has_ea_date = false;

	if(callback == nullptr)
	    throw Erange("data_tree::display_line", "nullptr given as callback function");

	try
	{
	    callback(tag,
		     num,
		     data_presence,
		     has_data_date,
		     has_data_date ? *data : datetime(0),
		     ea_presence,
		     has_ea_date,
		     has_ea_date ? *ea : datetime(0));
	}
	catch(...)
	{
	    throw Elibcall("data_tree::display_line", "provided callback function should not throw any exception");
	}
    }


} // end of namespace


