//*********************************************************************/
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

    /// \file statistics.hpp
    /// \brief handle the statistic structure that gives a summary of treated files after each operatio
    /// \ingroup API

#ifndef STATISTICS_HPP
#define STATISTICS_HPP

#include "../my_config.h"

#include "infinint.hpp"
#include "user_interaction.hpp"

extern "C"
{
#if MUTEX_WORKS
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif
#endif
}

    /// \addtogroup Private
    /// @{

#if MUTEX_WORKS
#define LOCK_IN pthread_mutex_lock(&lock_mutex)
#define LOCK_OUT pthread_mutex_unlock(&lock_mutex)
#define LOCK_IN_CONST pthread_mutex_lock(const_cast<pthread_mutex_t *>(&lock_mutex))
#define LOCK_OUT_CONST pthread_mutex_unlock(const_cast<pthread_mutex_t *>(&lock_mutex))
#else
#define LOCK_IN //
#define LOCK_OUT //
#define LOCK_IN_CONST //
#define LOCK_OUT_CONST //
#endif

namespace libdar
{

	/// structure returned by libdar call to give a summary of the operation done in term of file treated

	/// the different fields are used for backup, restoration and other operation
	/// their meaning changes a bit depending on the operation. Some operation may
	/// not use all fields. To have a detailed view of what fields get used and what
	// are their meaning see the archive class constructor and methods documentation
	/// \ingroup API
    class statistics
    {
    public:
	    /// constructor

	    /// \param[in] lock whether to use mutex to manipulate (read or write) variables of that object
	    /// \note using a statistics object built without lock (false given as argument to the constructor) may
	    /// lead to application crash if several threads are accessing at the same object at the same time when
	    /// at least one thread is modifying this object, unless you really know what you are doing, it is better
	    /// to always use the default value for this constructor or to explicitely give "true" as argument.
	statistics(bool lock = true) { init(lock); clear(); };
	statistics(const statistics & ref) { copy_from(ref); };
	const statistics & operator = (const statistics & ref) { detruit(); copy_from(ref); return *this; };

	    /// destructor
	~statistics() { detruit(); };

	    /// reset counters to zero
        void clear();

	    /// total number of file treated
        infinint total() const;

	void incr_treated() { (this->*increment)(&treated); };       ///< increment by one the treated counter
	void incr_hard_links() { (this->*increment)(&hard_links); }; ///< increment by one the hard_links counter
	void incr_skipped() { (this->*increment)(&skipped); };       ///< increment by one the skipped counter
	void incr_ignored() { (this->*increment)(&ignored); };       ///< increment by one the ignored counter
	void incr_tooold() { (this->*increment)(&tooold); };         ///< increment by one the tooold counter
	void incr_errored() { (this->*increment)(&errored); };       ///< increment by one the errored counter
	void incr_deleted() { (this->*increment)(&deleted); };       ///< increment by one the deleted counter
	void incr_ea_treated() { (this->*increment)(&ea_treated); }; ///< increment by one the ea_treated counter
	void incr_fsa_treated() { (this->*increment)(&fsa_treated); }; ///< increment by one the fsa treated counter

	void add_to_ignored(const infinint & val) { (this->*add_to)(&ignored, val); };  ///< increment the ignored counter by a given value
	void add_to_errored(const infinint & val) { (this->*add_to)(&errored, val); };  ///< increment the errored counter by a given value
	void add_to_deleted(const infinint & val) { (this->*add_to)(&deleted, val); };  ///< increment the deleted counter by a given value
	void add_to_byte_amount(const infinint & val) { (this->*add_to)(&byte_amount, val); }; ///< increment the byte amount counter by a given value

	void sub_from_treated(const infinint & val) { (this->*sub_from)(&treated, val); };
	void sub_from_ea_treated(const infinint & val) { (this->*sub_from)(&ea_treated, val); };
	void sub_from_hard_links(const infinint & val) { (this->*sub_from)(&hard_links, val); };
	void sub_from_fsa_treated(const infinint & val) { (this->*sub_from)(&fsa_treated, val); };

	infinint get_treated() const { return (this->*returned)(&treated); };     ///< returns the current value of the treated counter
	infinint get_hard_links() const { return (this->*returned)(&hard_links); }; ///< returns the current value of the hard_links counter
	infinint get_skipped() const { return (this->*returned)(&skipped); };     ///< returns the current value of the skipped counter
	infinint get_ignored() const { return (this->*returned)(&ignored); };     ///< returns the current value of the ignored counter
	infinint get_tooold() const { return (this->*returned)(&tooold); };       ///< returns the current value of the tooold counter
	infinint get_errored() const { return (this->*returned)(&errored); };     ///< returns the current value of the errored counter
	infinint get_deleted() const { return (this->*returned)(&deleted); };     ///< returns the current value of the deleted counter
	infinint get_ea_treated() const { return (this->*returned)(&ea_treated); };  ///< returns the current value of the ea_treated counter
	infinint get_byte_amount() const { return (this->*returned)(&byte_amount); };  ///< returns the current value of the byte_amount counter
	infinint get_fsa_treated() const { return (this->*returned)(&fsa_treated); }; ///< returns the current value of the fsa_treated counter

	void decr_treated() { (this->*decrement)(&treated); };        ///< decrement by one the treated counter
	void decr_hard_links() { (this->*decrement)(&hard_links); };  ///< decrement by one the hard_links counter
	void decr_skipped() { (this->*decrement)(&skipped); };        ///< decrement by one the skipped counter
	void decr_ignored() { (this->*decrement)(&ignored); };        ///< decrement by one the ignored counter
	void decr_tooold() { (this->*decrement)(&tooold); };          ///< decrement by one the toold counter
	void decr_errored() { (this->*decrement)(&errored); };        ///< decrement by one the errored counter
	void decr_deleted() { (this->*decrement)(&deleted); };        ///< decrement by one the deleted counter
	void decr_ea_treated() { (this->*decrement)(&ea_treated); };  ///< decrement by one the ea_treated counter
	void decr_fsa_treated() { (this->*decrement)(&fsa_treated); };///< decrement by one the fsa_treated counter

	void set_byte_amount(const infinint & val) { (this->*set_to)(&byte_amount, val); }; ///< set to the given value the byte_amount counter

	    // debuging method
	void dump(user_interaction & dialog) const;

    private:
#if MUTEX_WORKS
	pthread_mutex_t lock_mutex;   ///< lock the access to the private variable of the curent object
#endif
	bool locking;           ///< whether we use locking or not

        infinint treated;       ///< number of inode treated (saved, restored, etc.) [all operations]
        infinint hard_links;    ///< number of hard linked inodes treated (including those ignored by filters)
        infinint skipped;       ///< files not changed since last backup / file not restored because not saved in backup
        infinint ignored;       ///< ignored files due to filters
        infinint tooold;        ///< ignored files because less recent than the filesystem entry [restoration] / modfied during backup
        infinint errored;       ///< files that could not be saved / files that could not be restored (filesystem access right)
        infinint deleted;       ///< deleted file seen / number of files deleted during the operation [restoration]
        infinint ea_treated;    ///< number of EA saved / number of EA restored
	infinint byte_amount;   ///< auxilliary counter, holds the wasted bytes due to repeat on change feature for example.
	infinint fsa_treated;   ///< number of FSA saved / number of FSA restored


	void (statistics::*increment)(infinint * var);                    ///< generic method for incrementing a variable
	void (statistics::*add_to)(infinint * var, const infinint & val); ///< generic method for add a value to a variable
 	infinint (statistics::*returned)(const infinint * var) const;     ///< generic method for obtaining the value of a variable
	void (statistics::*decrement)(infinint * var);                    ///< generic method for decrementing a variable
	void (statistics::*set_to)(infinint * var, const infinint & val); ///< generic method for setting a variable to a given value
	void (statistics::*sub_from)(infinint *var, const infinint & val);///< generic method for substracting to a variable

	void increment_locked(infinint * var)
	{
	    LOCK_IN;
	    (*var)++;
	    LOCK_OUT;
	};

	void increment_unlocked(infinint * var)
	{
	    (*var)++;
	}

	void add_to_locked(infinint * var, const infinint & val)
	{
	    LOCK_IN;
	    (*var) += val;
	    LOCK_OUT;
	}

	void add_to_unlocked(infinint *var, const infinint & val)
	{
	    (*var) += val;
	}

	infinint returned_locked(const infinint * var) const
	{
	    infinint ret;

	    LOCK_IN_CONST;
	    ret = *var;
	    LOCK_OUT_CONST;

	    return ret;
	};

	infinint returned_unlocked(const infinint * var) const
	{
	    return *var;
	};

	void decrement_locked(infinint * var)
	{
	    LOCK_IN;
	    (*var)--;
	    LOCK_OUT;
	}

	void decrement_unlocked(infinint * var)
	{
	    (*var)--;
	}

	void set_to_locked(infinint *var, const infinint & val)
	{
	    LOCK_IN;
	    (*var) = val;
	    LOCK_OUT;
	}

	void set_to_unlocked(infinint *var, const infinint & val)
	{
	    *var = val;
	}

	void sub_from_unlocked(infinint *var, const infinint & val)
	{
	    *var -= val;
	}

	void sub_from_locked(infinint *var, const infinint & val)
	{
	    LOCK_IN;
	    *var -= val;
	    LOCK_OUT;
	}


	void init(bool lock); // set locking & mutex
	void detruit();       // release and free the mutex
	void copy_from(const statistics & ref); // reset mutex and copy data from the object of reference

    };

} // end of namespace

    /// @}

#endif
