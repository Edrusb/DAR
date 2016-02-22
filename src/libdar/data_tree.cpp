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

using namespace std;
using namespace libdar;

static data_tree *read_from_file(generic_file & f, unsigned char db_version, memory_pool *pool);
static void read_from_file(generic_file &f, archive_num &a);
static void write_to_file(generic_file &f, archive_num a);
static void display_line(user_interaction & dialog,
			 archive_num num,
			 const datetime *data,
			 data_tree::etat data_presence,
			 const datetime *ea,
			 data_tree::etat ea_presence);

const char * const ETAT_SAVED = "S";
const char * const ETAT_PATCH = "O";
const char * const ETAT_PATCH_UNUSABLE = "U";
const char * const ETAT_PRESENT = "P";
const char * const ETAT_REMOVED = "R";
const char * const ETAT_ABSENT = "A";

const unsigned char STATUS_PLUS_FLAG_ME = 0x01;
const unsigned char STATUS_PLUS_FLAG_REF = 0x02;

namespace libdar
{

    void data_tree::status::dump(generic_file & f) const
    {
	date.dump(f);
	switch(present)
	{
	case et_saved:
	    f.write(ETAT_SAVED, 1);
	    break;
	case et_patch:
	    f.write(ETAT_PATCH, 1);
	    break;
	case et_patch_unusable:
	    f.write(ETAT_PATCH_UNUSABLE, 1);
	    break;
	case et_present:
	    f.write(ETAT_PRESENT, 1);
	    break;
	case et_removed:
	    f.write(ETAT_REMOVED, 1);
	    break;
	case et_absent:
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
	    present = et_saved;
	    break;
	case ETAT_PRESENT[0]:
	    present = et_present;
	    break;
	case ETAT_REMOVED[0]:
	    present = et_removed;
	    break;
	case ETAT_ABSENT[0]:
	    present = et_absent;
	    break;
	case ETAT_PATCH[0]:
	    present = et_patch;
	    break;
	case ETAT_PATCH_UNUSABLE[0]:
	    present = et_patch_unusable;
	    break;
	default:
	    throw Erange("data_tree::status::read", gettext("Unexpected value found in database"));
	}
    }

    data_tree::status_plus::status_plus(const datetime & d,
					etat p,
					const crc *xme,
					const crc *xref): status(d, p)
    {
	me = ref = nullptr;

	try
	{
	    if(xme != nullptr)
	    {
		me = xme->clone();
		if(me == nullptr)
		    throw Ememory("data_tree::status_plus::status_plus");
	    }

	    if(xref != nullptr)
	    {
		ref = xref->clone();
		if(ref == nullptr)
		    throw Ememory("data_tree::status_plus::status_plus");
	    }
	}
	catch(...)
	{
	    if(me != nullptr)
		delete me;
	    if(ref != nullptr)
		delete ref;
	    throw;
	}
    }

    void data_tree::status_plus::dump(generic_file & f) const
    {
	unsigned char flag = 0;
	if(me != nullptr)
	    flag |= STATUS_PLUS_FLAG_ME;
	if(ref != nullptr)
	    flag |= STATUS_PLUS_FLAG_REF;

	status::dump(f);
	f.write((char *)&flag, 1);
	if(me != nullptr)
	    me->dump(f);
	if(ref != nullptr)
	    ref->dump(f);
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
	    f.read((char *)&flag, 1);
	    if((flag & STATUS_PLUS_FLAG_ME) != 0)
		me = create_crc_from_file(f, get_pool(), false);
	    if((flag & STATUS_PLUS_FLAG_REF) != 0)
		ref = create_crc_from_file(f, get_pool(), false);
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

	if(xref.me != nullptr)
	{
	    me = xref.me->clone();
	    if(me == nullptr)
		throw Ememory("data_tree::status_plus::copy_from");
	}
	else
	    me = nullptr;

	if(xref.ref != nullptr)
	{
	    ref = xref.ref->clone();
	    if(ref == nullptr)
		throw Ememory("data_tree::status_plus::copy_from");
	}
	else
	    ref = nullptr;
    }

    void data_tree::status_plus::detruit()
    {
	if(me != nullptr)
	{
	    delete me;
	    me = nullptr;
	}
	if(ref != nullptr)
	{
	    delete ref;
	    ref = nullptr;
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
	    read_from_file(f, k);
	    switch(db_version)
	    {
	    case 1:
		sta_plus.date = infinint(f);
		sta_plus.present = et_saved;
		    // me and ref fields are already set to nullptr
		last_mod[k] = sta_plus;
		break;
	    case 2:
	    case 3:
	    case 4:
	    case 5:
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
	    read_from_file(f, k);
	    switch(db_version)
	    {
	    case 1:
		sta.date = infinint(f);
		sta.present = et_saved;
		last_change[k] = sta;
		break;
	    case 2:
	    case 3:
	    case 4:
	    case 5:
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
	    write_to_file(f, itp->first); // key
	    itp->second.dump(f); // value
	    ++itp;
	}

	    // last change table dump

	sz = last_change.size();
	sz.dump(f);

	map<archive_num, status>::const_iterator it = last_change.begin();
	while(it != last_change.end())
	{
	    write_to_file(f, it->first); // key
	    it->second.dump(f); // value
	    ++it;
	}
    }

    data_tree::lookup data_tree::get_data(set<archive_num> & archive, const datetime & date, bool even_when_removed) const
    {
	    // archive (first argument of the method) will hold the set of archive numbers to use to obtain the latest requested state of the file
	    // for plain data the set will contain 1 number, for delta difference, it may contain many number of archive to restore in turn for that file

	map<archive_num, status_plus>::const_iterator it = last_mod.begin();
	datetime max_seen_date = datetime(0);
	datetime max_real_date = datetime(0);
	archive_num last_archive_seen = 0; //< last archive number (in the order of the database) in which a valid entry has been found (any state)
	set<archive_num> last_archive_even_when_removed; //< last archive number in which a valid entry with data available has been found
	bool presence_seen = false; //< whether the last found valid entry indicates file was present or removed
	bool presence_real = false; //< whether the last found valid entry not being an "et_present" state was present or removed
	lookup ret;

	archive.clear(); // empty set will be used if no archive found

	while(it != last_mod.end())
	{
	    if(it->second.date >= max_seen_date
		   // ">=" (not ">") because there should never be twice the same value
		   // and we must be able to see a value of 0 (initially max = 0) which is valid.
	       && (date.is_null() || it->second.date <= date))
	    {
		max_seen_date = it->second.date;
		last_archive_seen = it->first;
		switch(it->second.present)
		{
		case et_saved:
		case et_present:
		case et_patch:
		    presence_seen = true;
		    break;
		case et_removed:
		case et_absent:
		case et_patch_unusable:
		    presence_seen = false;
		    break;
		default:
		    throw SRC_BUG;
		}
	    }

	    if(it->second.date >= max_real_date
		   // ">=" (not ">") because there should never be twice the same value
		   // and we must be able to see a value of 0 (initially max = 0) which is valid.
	       && (date.is_null() || it->second.date <= date))
	    {
		if(it->second.present != et_present)
		{
		    max_real_date = it->second.date;
		    switch(it->second.present)
		    {
		    case et_saved:
			archive.clear();
			last_archive_even_when_removed.clear();
			    // no break !!! //
		    case et_patch:
			archive.insert(it->first);
			last_archive_even_when_removed.insert(it->first);
			presence_real = true;
			break;
		    case et_removed:
		    case et_absent:
		    case et_patch_unusable:
			archive.clear();
			archive.insert(it->first);
			presence_real = false;
			break;
		    case et_present:
			throw SRC_BUG;
		    default:
			throw SRC_BUG;
		    }
		}
	    }

	    ++it;
	}

	if(even_when_removed && !last_archive_even_when_removed.empty())
	{
	    archive = last_archive_even_when_removed;
	    presence_seen = presence_real = true;
	}

	if(archive.empty())
	    if(last_archive_seen != 0) // last acceptable entry is a file present but not saved, but no full backup is present in a previous archive
	    {
		archive.clear();
		archive.insert(last_archive_seen);
		ret = not_restorable;
	    }
	    else
		ret = not_found;
	else
	    if(last_archive_seen != 0)
		if(presence_seen && !presence_real)
			// last acceptable entry is a file present but not saved,
			// but no full backup is present in a previous archive
		{
		    archive.clear();
		    archive.insert(last_archive_seen);
		    ret = not_restorable;
		}
		else
		    if(presence_seen != presence_real)
			throw SRC_BUG;
		    else  // archive > 0 && presence_seen == present_real
			if(presence_real)
			    ret = found_present;
			else
			    ret = found_removed;
	    else // archive != 0 but last_archive_seen == 0
		throw SRC_BUG;

	return ret;
    }

    data_tree::lookup data_tree::get_EA(archive_num & archive, const datetime & date, bool even_when_removed) const
    {
	map<archive_num, status>::const_iterator it = last_change.begin();
	datetime max_seen_date = datetime(0), max_real_date = datetime(0);
	bool presence_seen = false, presence_real = false;
	archive_num last_archive_seen = 0;
	archive_num last_archive_even_when_removed = 0;
	lookup ret;

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
		case et_saved:
		case et_present:
		    presence_seen = true;
		    break;
		case et_removed:
		case et_absent:
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
		if(it->second.present != et_present)
		{
		    max_real_date = it->second.date;
		    archive = it->first;
		    switch(it->second.present)
		    {
		    case et_saved:
			presence_real = true;
			last_archive_even_when_removed = archive;
			break;
		    case et_removed:
		    case et_absent:
			presence_real = false;
			break;
		    case et_present:
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
		ret = not_restorable;
	    else
		ret = not_found;
	else
	    if(last_archive_seen != 0)
		if(presence_seen && !presence_real) // last acceptable entry is a file present but not saved, but no full backup is present in a previous archive
		    ret = not_restorable;
		else
		    if(presence_seen != presence_real)
			throw SRC_BUG;
		    else  // archive > 0 && presence_seen == present_real
			if(presence_real)
			    ret = found_present;
			else
			    ret = found_removed;
	    else
		throw SRC_BUG;

	return ret;
    }

    bool data_tree::read_data(archive_num num, datetime & val,
			      etat & present) const
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

    bool data_tree::read_EA(archive_num num, datetime & val, etat & present) const
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
	    if(itp->first == archive && itp->second.present != et_absent)
		found_in_archive = true;
	    else
		if(itp->first > num_max && (itp->first < ignore_archives_greater_or_equal || ignore_archives_greater_or_equal == 0))
		{
		    num_max = itp->first;
		    switch(itp->second.present)
		    {
		    case et_saved:
		    case et_present:
		    case et_patch:
		    case et_patch_unusable:
			presence_max = true;
			last_mtime = itp->second.date; // used as deleted_data for EA
			break;
		    case et_removed:
		    case et_absent:
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

	if(!found_in_archive) // not entry found for asked archive (or recorded as et_absent)
	{
	    if(presence_max)
	    {
		    // the most recent entry found stated that this file
		    // existed at the time "last_mtime"
		    // and this entry is absent from the archive under addition
		    // thus we must record that it has been removed in the
		    // archive we currently add.

		if(deleted_date > last_mtime)
		    set_data(archive, deleted_date, et_absent);
		    // add an entry telling that this file does no more exist in the current archive
		else
		    set_data(archive, last_mtime, et_absent);
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
		    case et_saved:
		    case et_present:
		    case et_patch:
		    case et_patch_unusable:
			throw SRC_BUG; // entry has not been found in the current archive
		    case et_removed:
			break;         // we must keep it, it was in the original archive
		    case et_absent:
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
	    if(it->first == archive && it->second.present != et_absent)
		found_in_archive = true;
	    else
		if(it->first > num_max && (it->first < ignore_archives_greater_or_equal || ignore_archives_greater_or_equal == 0))
		{
		    num_max = it->first;
		    switch(it->second.present)
		    {
		    case et_saved:
		    case et_present:
			presence_max = true;
			break;
		    case et_removed:
		    case et_absent:
			presence_max = false;
			break;
		    case et_patch:
			throw SRC_BUG;
		    case et_patch_unusable:
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
		    set_EA(archive, (last_mtime < deleted_date ? deleted_date : last_mtime), et_absent); // add an entry telling that EA for this file do no more exist in the current archive
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
	    etat status;
	    if(last_mod.size() > 1 && read_data(archive_to_remove, del_date, status))
		if(status == et_removed)
		{
		    datetime tmp;
		    if(!read_data(archive_to_remove + 1, tmp, status))
			set_data(archive_to_remove + 1, del_date, et_removed);
		}
	    if(last_change.size() > 1 && read_EA(archive_to_remove, del_date, status))
		if(status == et_removed)
		{
		    datetime tmp;
		    if(!read_EA(archive_to_remove + 1, tmp, status))
			set_EA(archive_to_remove + 1, del_date, et_removed);
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

    void data_tree::listing(user_interaction & dialog) const
    {
	map<archive_num, status_plus>::const_iterator it = last_mod.begin();
	map<archive_num, status>::const_iterator ut = last_change.begin();

	dialog.printf(gettext("Archive number |  Data                   | status ||  EA                     | status \n"));
	dialog.printf(gettext("---------------+-------------------------+--------++-------------------------+----------\n"));

	while(it != last_mod.end() || ut != last_change.end())
	{
	    if(it != last_mod.end())
		if(ut != last_change.end())
		    if(it->first == ut->first)
		    {
			display_line(dialog, it->first, &(it->second.date), it->second.present, &(ut->second.date), ut->second.present);
			++it;
			++ut;
		    }
		    else // not in the same archive
			if(it->first < ut->first) // it only
			{
			    display_line(dialog, it->first, &(it->second.date), it->second.present, nullptr, data_tree::et_removed);
			    ++it;
			}
			else // ut only
			{
			    display_line(dialog, ut->first, nullptr, data_tree::et_removed, &(ut->second.date), ut->second.present);
			    ++ut;
			}
		else // ut at end of list thus it != last_mod.end() (see while condition)
		{
		    display_line(dialog, it->first, &(it->second.date), it->second.present, nullptr, data_tree::et_removed);
		    ++it;
		}
	    else // it at end of list, this ut != last_change.end() (see while condition)
	    {
		display_line(dialog, ut->first, nullptr, data_tree::et_removed, &(ut->second.date), ut->second.present);
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

    void data_tree::compute_most_recent_stats(vector<infinint> & data, vector<infinint> & ea,
					      vector<infinint> & total_data, vector<infinint> & total_ea) const
    {
	archive_num most_recent = 0;
	datetime max = datetime(0);
	map<archive_num, status_plus>::const_iterator itp = last_mod.begin();

	while(itp != last_mod.end())
	{
	    if(itp->second.present == et_saved)
	    {
		if(itp->second.date >= max)
		{
		    most_recent = itp->first;
		    max = itp->second.date;
		}
		++total_data[itp->first];
	    }
	    ++itp;
	}
	if(most_recent > 0)
	    ++data[most_recent];

	most_recent = 0;
	max = datetime(0);
	map<archive_num, status>::const_iterator it = last_change.begin();

	while(it != last_change.end())
	{
	    if(it->second.present == et_saved)
	    {
		if(it->second.date >= max)
		{
		    most_recent = it->first;
		    max = it->second.date;
		}
		++total_ea[it->first];
	    }
	    ++it;
	}
	if(most_recent > 0)
	    ++ea[most_recent];
    }

    bool data_tree::fix_corruption()
    {
	bool ret = true;
	map<archive_num, status_plus>::iterator itp = last_mod.begin();

	while(itp != last_mod.end() && ret)
	{
	    if(itp->second.present != et_removed && itp->second.present != et_absent)
		ret = false;
	    ++itp;
	}

	map<archive_num, status>::iterator it = last_change.begin();
	while(it != last_change.end() && ret)
	{
	    if(it->second.present != et_removed && it->second.present != et_absent)
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
	const trecord & operator = (const trecord & ref) { date = ref.date; set = ref.set; return *this; };
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
		    string tmp = current_path.display() == "." ? get_name() : (current_path + get_name()).display();
		    dialog.printf(gettext("Dates of file's %S are not increasing when database's archive number grows. Concerned file is: %S"), &field_nature, &tmp);
		    if(initial_warn)
		    {
			dialog.warning(gettext("Dates are not increasing for all files when database's archive number grows, working with this database may lead to improper file's restored version. Please reorder the archive within the database in the way that the older is the first archive and so on up to the most recent archive being the last of the database"));
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
	    case et_saved:
		prev = it->second.me;
		break;
	    case et_patch:
	    case et_patch_unusable:
		if(it->second.ref == nullptr)
		    throw SRC_BUG;
		if(prev != nullptr && *prev == *(it->second.ref))
		    it->second.present = et_patch;
		else
		{
		    it->second.present = et_patch_unusable;
		    ret = false;
		}
		prev = it->second.me;
		break;
	    case et_present:
		break;
	    case et_removed:
	    case et_absent:
		prev = nullptr;
		break;
	    default:
		throw SRC_BUG;
	    }
	}

	return ret;
    }


////////////////////////////////////////////////////////////////

    data_dir::data_dir(const string &name) : data_tree(name)
    {
	rejetons.clear();
    }

    data_dir::data_dir(generic_file &f, unsigned char db_version) : data_tree(f, db_version)
    {
	infinint tmp = infinint(f); // number of children
	data_tree *entry = nullptr;
	rejetons.clear();

	try
	{
	    while(!tmp.is_zero())
	    {
		entry = read_from_file(f, db_version, get_pool());
		if(entry == nullptr)
		    throw Erange("data_dir::data_dir", gettext("Unexpected end of file"));
		rejetons.push_back(entry);
		entry = nullptr;
		--tmp;
	    }
	}
	catch(...)
	{
	    list<data_tree *>::iterator next = rejetons.begin();
	    while(next != rejetons.end())
	    {
		delete *next;
		*next = nullptr;
		++next;
	    }
	    if(entry != nullptr)
		delete entry;
	    throw;
	}
    }

    data_dir::data_dir(const data_dir & ref) : data_tree(ref)
    {
	rejetons.clear();
    }

    data_dir::data_dir(const data_tree & ref) : data_tree(ref)
    {
	rejetons.clear();
    }

    data_dir::~data_dir()
    {
	list<data_tree *>::iterator next = rejetons.begin();
	while(next != rejetons.end())
	{
	    delete *next;
	    *next = nullptr;
	    ++next;
	}
    }

    void data_dir::dump(generic_file & f) const
    {
	list<data_tree *>::const_iterator it = rejetons.begin();
	infinint tmp = rejetons.size();

	data_tree::dump(f);
	tmp.dump(f);
	while(it != rejetons.end())
	{
	    if(*it == nullptr)
		throw SRC_BUG;
	    (*it)->dump(f);
	    ++it;
	}
    }


    data_tree *data_dir::find_or_addition(const string & name, bool is_dir, const archive_num & archive)
    {
	const data_tree *fils = read_child(name);
	data_tree *ret = nullptr;

	if(fils == nullptr) // brand-new data_tree to build
	{
	    if(is_dir)
		ret = new (get_pool()) data_dir(name);
	    else
		ret = new (get_pool()) data_tree(name);
	    if(ret == nullptr)
		throw Ememory("data_dir::find_or_addition");
	    add_child(ret);
	}
	else  // already saved in another archive
	{
		// check if dir/file nature has changed
	    const data_dir *fils_dir = dynamic_cast<const data_dir *>(fils);
	    if(fils_dir == nullptr && is_dir) // need to upgrade data_tree to data_dir
	    {
		ret = new (get_pool()) data_dir(*fils); // upgrade data_tree in an empty data_dir
		if(ret == nullptr)
		    throw Ememory("data_dir::find_or_addition");
		try
		{
		    remove_child(name);
		    add_child(ret);
		}
		catch(...)
		{
		    delete ret;
		    throw;
		}
	    }
	    else // no change in dir/file nature
		ret = const_cast<data_tree *>(fils);
	}

	return ret;
    }

    void data_dir::add(const cat_inode *entry, const archive_num & archive)
    {
	const cat_directory *entry_dir = dynamic_cast<const cat_directory *>(entry);
	const cat_file *entry_file = dynamic_cast<const cat_file *>(entry);
	data_tree * tree = find_or_addition(entry->get_name(), entry_dir != nullptr, archive);
	archive_num last_archive;
	lookup result;
	datetime last_mod = entry->get_last_modif() > entry->get_last_change() ? entry->get_last_modif() : entry->get_last_change();
	const crc *me;
	const crc *ref;

	switch(entry->get_saved_status())
	{
	case s_saved:
	case s_fake:
	    if(entry_file != nullptr)
	    {
		if(!entry_file->get_crc(me))
		    me = nullptr;
	    }
	    else
		me = nullptr;
	    ref = nullptr;
	    tree->set_data(archive, last_mod, et_saved, me, ref);
	    break;
	case s_delta:
	    if(entry_file == nullptr)
		throw SRC_BUG;
	    if(!entry_file->get_crc(me))
		me = nullptr;
	    if(!entry_file->get_patch_base_crc(ref))
		ref = nullptr;
	    tree->set_data(archive, last_mod, et_patch, me, ref);
	    break;
	case s_not_saved:
	    if(entry_file != nullptr)
	    {
		if(!entry_file->get_crc(me))
		    me = nullptr;
	    }
	    else
		me = nullptr;
	    ref = nullptr;
	    tree->set_data(archive, last_mod, et_present, me, ref);
	    break;
	default:
	    throw SRC_BUG;
	}

	switch(entry->ea_get_saved_status())
	{
	case cat_inode::ea_none:
	    break;
	case cat_inode::ea_removed:
	    result = tree->get_EA(last_archive, datetime(0), false);
	    if(result == found_present || result == not_restorable)
		tree->set_EA(archive, entry->get_last_change(), et_removed);
		// else no need to add an et_remove entry in the map
	    break;
	case cat_inode::ea_partial:
	    tree->set_EA(archive, entry->get_last_change(), et_present);
	    break;
	case cat_inode::ea_fake:
	case cat_inode::ea_full:
	    tree->set_EA(archive, entry->get_last_change(), et_saved);
	    break;
	default:
	    throw SRC_BUG;
	}
    }

    void data_dir::add(const cat_detruit *entry, const archive_num & archive)
    {
	data_tree * tree = find_or_addition(entry->get_name(), false, archive);
	set<archive_num> last_archive_set;
	archive_num last_archive;
	lookup result;

	result = tree->get_data(last_archive_set, datetime(0), false);
	if(result == found_present || result == not_restorable)
	    tree->set_data(archive, entry->get_date(), et_removed);
	result = tree->get_EA(last_archive, datetime(0), false);
	if(result == found_present || result == not_restorable)
	    tree->set_EA(archive, entry->get_date(), et_removed);
    }

    const data_tree *data_dir::read_child(const string & name) const
    {
	list<data_tree *>::const_iterator it = rejetons.begin();

	while(it != rejetons.end() && *it != nullptr && (*it)->get_name() != name)
	    ++it;

	if(it == rejetons.end())
	    return nullptr;
	else
	    if(*it == nullptr)
		throw SRC_BUG;
	    else
		return *it;
    }

    void data_dir::read_all_children(vector<string> & fils) const
    {
	list<data_tree *>::const_iterator it = rejetons.begin();

	fils.clear();
	while(it != rejetons.end())
	    fils.push_back((*it++)->get_name());
    }

    bool data_dir::check_order(user_interaction & dialog, const path & current_path, bool & initial_warn) const
    {
	list<data_tree *>::const_iterator it = rejetons.begin();
	bool ret = data_tree::check_order(dialog, current_path, initial_warn);
	path subpath = current_path.display() == "." ? get_name() : current_path + get_name();

	while(it != rejetons.end() && ret)
	{
	    if(*it == nullptr)
		throw SRC_BUG;
	    ret = (*it)->check_order(dialog, subpath, initial_warn);
	    ++it;
	}

	return ret;
    }

    void data_dir::finalize(const archive_num & archive, const datetime & deleted_date, const archive_num & ignore_archives_greater_or_equal)
    {
	datetime new_deleted_date;
	set<archive_num> tmp_archive_set;
	etat tmp_presence;

	data_tree::finalize(archive, deleted_date, ignore_archives_greater_or_equal);

	switch(get_data(tmp_archive_set, datetime(0), false))
	{
	case found_present:
	case found_removed:
	    break; // acceptable result
	case not_found:
	    if(fix_corruption())
		throw Edata("This is to signal the caller of this method that this object has to be removed from database"); // exception caugth in data_dir::finalize_except_self
	    throw Erange("data_dir::finalize", gettext("This database has been corrupted probably due to a bug in release 2.4.0 to 2.4.9, and it has not been possible to cleanup this corruption, please rebuild the database from archives or extracted \"catalogues\", if the database has never been used by one of the previously mentioned released, you are welcome to open a bug report and provide as much as possible details about the circumstances"));
	case not_restorable:
	    break;  // also an acceptable result;
	default:
	    throw SRC_BUG;
	}

	if(tmp_archive_set.empty())
	    throw SRC_BUG;
	if(!read_data(*(tmp_archive_set.rbegin()), new_deleted_date, tmp_presence))
	    throw SRC_BUG;

	finalize_except_self(archive, new_deleted_date, ignore_archives_greater_or_equal);
    }

    void data_dir::finalize_except_self(const archive_num & archive, const datetime & deleted_date, const archive_num & ignore_archives_greater_or_equal)
    {
	list<data_tree *>::iterator it = rejetons.begin();

	while(it != rejetons.end())
	{
	    if(*it == nullptr)
		throw SRC_BUG;
	    try
	    {
		(*it)->finalize(archive, deleted_date, ignore_archives_greater_or_equal);
		++it;
	    }
	    catch(Edata & e)
	    {
		delete (*it);
		rejetons.erase(it);
		it = rejetons.begin();
	    }
	}
    }


    bool data_dir::remove_all_from(const archive_num & archive_to_remove, const archive_num & last_archive)
    {
	list<data_tree *>::iterator it = rejetons.begin();

	while(it != rejetons.end())
	{
	    if((*it) == nullptr)
		throw SRC_BUG;
	    if((*it)->remove_all_from(archive_to_remove, last_archive))
	    {
		delete *it; // release the memory used by the object
		*it = nullptr;
		rejetons.erase(it); // remove the entry from the list
		it = rejetons.begin(); // does not seems "it" points to the next item after erase, so we restart from the beginning
	    }
	    else
		++it;
	}

	return data_tree::remove_all_from(archive_to_remove, last_archive) && rejetons.size() == 0;
    }

    void data_dir::show(user_interaction & dialog, archive_num num, string marge) const
    {
	list<data_tree *>::const_iterator it = rejetons.begin();
	set<archive_num> ou_data;
	archive_num ou_ea;
	bool data, ea;
	string etat, name;
	lookup lo_data, lo_ea;
	bool even_when_removed = (num != 0);

	while(it != rejetons.end())
	{
	    if(*it == nullptr)
		throw SRC_BUG;
	    data_dir *dir = dynamic_cast<data_dir *>(*it);

	    lo_data = (*it)->get_data(ou_data, datetime(0), even_when_removed);
	    lo_ea = (*it)->get_EA(ou_ea, datetime(0), even_when_removed);
	    data = lo_data == found_present && (ou_data.find(num) != ou_data.end() || num == 0);
	    ea = lo_ea == found_present && (ou_ea == num || num == 0);
	    name = (*it)->get_name();
	    if(data || ea || num == 0)
	    {
		etat = "";
		if(data)
		    etat += gettext("[ Saved ]");
		else
		    etat += gettext("[       ]");

		if(ea)
		    etat += gettext("[  EA   ]");
		else
		    etat += gettext("[       ]");

		if(dialog.get_use_dar_manager_show_files())
		    dialog.dar_manager_show_files(name, data, ea);
		else
		    dialog.printf("%S  %S%S\n", &etat, &marge, &name);
	    }
	    if(dir != nullptr)
		dir->show(dialog, num, marge+name+"/");
	    ++it;
	}
    }

    void data_dir::apply_permutation(archive_num src, archive_num dst)
    {
	list<data_tree *>::iterator it = rejetons.begin();

	data_tree::apply_permutation(src, dst);
	while(it != rejetons.end())
	{
	    (*it)->apply_permutation(src, dst);
	    ++it;
	}
    }


    void data_dir::skip_out(archive_num num)
    {
	list<data_tree *>::iterator it = rejetons.begin();

	data_tree::skip_out(num);
	while(it != rejetons.end())
	{
	    (*it)->skip_out(num);
	    ++it;
	}
    }

    void data_dir::compute_most_recent_stats(vector<infinint> & data, vector<infinint> & ea,
					     vector<infinint> & total_data, vector<infinint> & total_ea) const
    {
	list<data_tree *>::const_iterator it = rejetons.begin();

	data_tree::compute_most_recent_stats(data, ea, total_data, total_ea);
	while(it != rejetons.end())
	{
	    (*it)->compute_most_recent_stats(data, ea, total_data, total_ea);
	    ++it;
	}
    }

    bool data_dir::fix_corruption()
    {
	while(rejetons.begin() != rejetons.end() && *(rejetons.begin()) != nullptr && (*(rejetons.begin()))->fix_corruption())
	{
	    delete *(rejetons.begin());
	    rejetons.erase(rejetons.begin());
	}

	if(rejetons.begin() != rejetons.end())
	    return false;
	else
	    return data_tree::fix_corruption();

    }

    void data_dir::add_child(data_tree *fils)
    {
	if(fils == nullptr)
	    throw SRC_BUG;
	rejetons.push_back(fils);
    }

    void data_dir::remove_child(const string & name)
    {
	list<data_tree *>::iterator it = rejetons.begin();

	while(it != rejetons.end() && *it != nullptr && (*it)->get_name() != name)
	    ++it;

	if(it != rejetons.end())
	{
	    if(*it == nullptr)
		throw SRC_BUG;
	    else
		rejetons.erase(it);
	}
    }


////////////////////////////////////////////////////////////////

    data_dir *data_tree_read(generic_file & f, unsigned char db_version, memory_pool *pool)
    {
	data_tree *lu = read_from_file(f, db_version, pool);
	data_dir *ret = dynamic_cast<data_dir *>(lu);

	if(ret == nullptr && lu != nullptr)
	    delete lu;

	return ret;
    }

    bool data_tree_find(path chemin, const data_dir & racine, const data_tree *& ptr)
    {
	string filename;
	const data_dir *current = &racine;
	bool loop = true;

	if(!chemin.is_relative())
	    throw SRC_BUG;

	while(loop)
	{
	    if(!chemin.pop_front(filename))
	    {
		filename = chemin.display();
		loop = false;
	    }
	    ptr = current->read_child(filename);
	    if(ptr == nullptr)
		loop = false;
	    if(loop)
	    {
		current = dynamic_cast<const data_dir *>(ptr);
		if(current == nullptr)
		{
		    loop = false;
		    ptr = nullptr;
		}
	    }
	}

	return ptr != nullptr;
    }

    void data_tree_update_with(const cat_directory *dir, archive_num archive, data_dir *racine)
    {
	const cat_nomme *entry;

	dir->reset_read_children();
	while(dir->read_children(entry))
	{
	    const cat_directory *entry_dir = dynamic_cast<const cat_directory *>(entry);
	    const cat_inode *entry_ino = dynamic_cast<const cat_inode *>(entry);
	    const cat_mirage *entry_mir = dynamic_cast<const cat_mirage *>(entry);
	    const cat_detruit *entry_det = dynamic_cast<const cat_detruit *>(entry);

	    if(entry_mir != nullptr)
	    {
		entry_ino = entry_mir->get_inode();
		entry_mir->get_inode()->change_name(entry_mir->get_name());
	    }

	    if(entry_ino == nullptr)
		if(entry_det != nullptr)
		{
		    if(!entry_det->get_date().is_null())
			racine->add(entry_det, archive);
			// else this is an old archive that does not store date with cat_detruit objects
		}
		else
		    continue; // continue with next loop, we ignore entree objects that are neither inode nor cat_detruit
	    else
		racine->add(entry_ino, archive);

	    if(entry_dir != nullptr) // going into recursion
	    {
		data_tree *new_root = const_cast<data_tree *>(racine->read_child(entry->get_name()));
		data_dir *new_root_dir = dynamic_cast<data_dir *>(new_root);

		if(new_root == nullptr)
		    throw SRC_BUG; // the racine->add method did not add an item for "entry"
		if(new_root_dir == nullptr)
		    throw SRC_BUG; // the racine->add method did not add a data_dir item
		data_tree_update_with(entry_dir, archive, new_root_dir);
	    }
	}
    }

    archive_num data_tree_permutation(archive_num src, archive_num dst, archive_num x)
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

} // end of namespace


////////////////////////////////////////////////////////////////

static data_tree *read_from_file(generic_file & f, unsigned char db_version, memory_pool *pool)
{
    char sign;
    data_tree *ret;

    if(f.read(&sign, 1) != 1)
        return nullptr; // nothing more to read

    if(sign == data_tree::signature())
        ret = new (pool) data_tree(f, db_version);
    else if(sign == data_dir::signature())
        ret = new (pool) data_dir(f, db_version);
    else
        throw Erange("read_from_file", gettext("Unknown record type"));

    if(ret == nullptr)
        throw Ememory("read_from_file");

    return ret;
}

static void read_from_file(generic_file &f, archive_num &a)
{
    char buffer[sizeof(archive_num)];
    archive_num *ptr = (archive_num *)&(buffer[0]);

    f.read(buffer, sizeof(archive_num));
    a = ntohs(*ptr);
}

static void write_to_file(generic_file &f, archive_num a)
{
    char buffer[sizeof(archive_num)];
    archive_num *ptr = (archive_num *)&(buffer[0]);

    *ptr = htons(a);
    f.write(buffer, sizeof(archive_num));
}

static void display_line(user_interaction & dialog,
			 archive_num num,
			 const datetime *data,
			 data_tree::etat data_presence,
			 const datetime *ea,
			 data_tree::etat ea_presence)
{

    const string REMOVED = gettext("removed ");
    const string PRESENT = gettext("present ");
    const string SAVED   = gettext("saved   ");
    const string ABSENT  = gettext("absent  ");
    const string PATCH   = gettext("patch   ");
    const string BROKEN  = gettext("BROKEN  ");
    const string NO_DATE = "                          ";

    string data_state;
    string ea_state;
    string data_date;
    string ea_date;

    switch(data_presence)
    {
    case data_tree::et_saved:
	data_state = SAVED;
	break;
    case data_tree::et_patch:
	data_state = PATCH;
	break;
    case data_tree::et_patch_unusable:
	data_state = BROKEN;
	break;
    case data_tree::et_present:
	data_state = PRESENT;
	break;
    case data_tree::et_removed:
	data_state = REMOVED;
	break;
    case data_tree::et_absent:
	data_state = ABSENT;
	break;
    default:
	throw SRC_BUG;
    }

    switch(ea_presence)
    {
    case data_tree::et_saved:
	ea_state = SAVED;
	break;
    case data_tree::et_present:
	ea_state = PRESENT;
	break;
    case data_tree::et_removed:
	ea_state = REMOVED;
	break;
    case data_tree::et_absent:
	throw SRC_BUG; // state not used for EA
    case data_tree::et_patch:
	throw SRC_BUG;
    case data_tree::et_patch_unusable:
	throw SRC_BUG;
    default:
	throw SRC_BUG;
    }

    if(data == nullptr)
    {
	data_state = ABSENT;
	data_date = NO_DATE;
    }
    else
	data_date = tools_display_date(*data);

    if(ea == nullptr)
    {
	ea_state = ABSENT;
	ea_date = NO_DATE;
    }
    else
	ea_date = tools_display_date(*ea);

    if(dialog.get_use_dar_manager_show_version())
	dialog.dar_manager_show_version(num, data_date, data_state, ea_date, ea_state);
    else
	dialog.printf(" \t%u\t%S  %S  %S  %S\n", num, &data_date, &data_state, &ea_date, &ea_state);
}
