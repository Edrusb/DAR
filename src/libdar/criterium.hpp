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

    /// \file criterium.hpp
    /// \brief contains classes that let the user define the policy for overwriting files
    /// \ingroup API

#ifndef CRITERIUM_HPP
#define CRITERIUM_HPP

#include "../my_config.h"

#include "cat_nomme.hpp"
#include "cat_inode.hpp"
#include "cat_directory.hpp"

namespace libdar
{

	/// \addtogroup API
	/// @{

	/// the generic criterium class, parent of all criterium

	/// this is a pure virtual class that is used in API call
	/// it is used to federate under a single type all the
	/// criterium classes defined below. It defines a common
	/// interface for all of them.

    class criterium
    {
    public:
	criterium() = default;
	criterium(const criterium & ref) = default;
	criterium & operator = (const criterium & ref) = default;
	virtual ~criterium() throw(Ebug) {};

	    /// criterum interface method

	    /// \param[in] first entry to compare with the following (this is the original or 'in place' entry)
	    /// \param[in] second the other entry to compare with the previous one (this is the new entry to add)
	    /// \return the result of the criterium evaluation (true or false)
	virtual bool evaluate(const cat_nomme &first, const cat_nomme &second) const = 0;

	    /// clone construction method

	    /// \return a new object of the same type,
	    /// \note this method must be implemented in all the leaf classes of the
	    /// class hierarchy rooted at the criterium class
	virtual criterium *clone() const = 0;

    protected:
	static const cat_inode *get_inode(const cat_nomme * arg);
    };

	////////////////////////////////////////////////////////////
	//////////// a set of criterium classes follows ////////////
        ////////////////////////////////////////////////////////////


	/// returns true if the first entry is an inode (whatever is the second)

	/// \note the current only entry that can be found in an archive which is not an inode, is an entry
	/// signaling that a file has been destroyed since the archive of reference.

    class crit_in_place_is_inode : public criterium
    {
    public:
	crit_in_place_is_inode() = default;
	crit_in_place_is_inode(const crit_in_place_is_inode & ref) = default;
	crit_in_place_is_inode & operator = (const crit_in_place_is_inode & ref) = default;
	~crit_in_place_is_inode() = default;

	virtual bool evaluate(const cat_nomme &first, const cat_nomme &second) const override;
	virtual criterium *clone() const override { return new (std::nothrow) crit_in_place_is_inode(*this); };
    };


	/// returns true if the first entry is a cat_directory (whatever is the second)

    class crit_in_place_is_dir : public criterium
    {
    public:
	crit_in_place_is_dir() = default;
	crit_in_place_is_dir(const crit_in_place_is_dir & ref) = default;
	crit_in_place_is_dir & operator = (const crit_in_place_is_dir & ref) = default;
	~crit_in_place_is_dir() = default;

	virtual bool evaluate(const cat_nomme &first, const cat_nomme &second) const { return dynamic_cast<const cat_directory *>(&first) != nullptr; };
	virtual criterium *clone() const override { return new (std::nothrow) crit_in_place_is_dir(*this); };
    };


	/// returns true if the first entry is a plain file (whatever is the second)

    class crit_in_place_is_file : public criterium
    {
    public:
	crit_in_place_is_file() = default;
	crit_in_place_is_file(const crit_in_place_is_file & ref) = default;
	crit_in_place_is_file & operator = (const crit_in_place_is_file & ref) = default;
	~crit_in_place_is_file() = default;

	virtual bool evaluate(const cat_nomme &first, const cat_nomme &second) const override;
	virtual criterium *clone() const override { return new (std::nothrow) crit_in_place_is_file(*this); };
    };

	/// returns true if the first entry is a inode with several hard links (whatever is the second entry)

	/// it may be a plain file, a symlink a char device, a block device or a named pipe for example

    class crit_in_place_is_hardlinked_inode : public criterium
    {
    public:
	crit_in_place_is_hardlinked_inode() = default;
	crit_in_place_is_hardlinked_inode(const crit_in_place_is_hardlinked_inode & ref) = default;
	crit_in_place_is_hardlinked_inode & operator = (const crit_in_place_is_hardlinked_inode & ref) = default;
	~crit_in_place_is_hardlinked_inode() = default;

	virtual bool evaluate(const cat_nomme &first, const cat_nomme &second) const override;
	virtual criterium *clone() const override { return new (std::nothrow) crit_in_place_is_hardlinked_inode(*this); };
    };


	/// returns true if the first entry is a inode with several hard links (whatever is the second entry) and also if this first entry is the first we meet that points to this hard linked inode
    class crit_in_place_is_new_hardlinked_inode : public criterium
    {
    public:
	crit_in_place_is_new_hardlinked_inode() = default;
	crit_in_place_is_new_hardlinked_inode(const crit_in_place_is_new_hardlinked_inode & ref) = default;
	crit_in_place_is_new_hardlinked_inode & operator = (const crit_in_place_is_new_hardlinked_inode & ref) = default;
	~crit_in_place_is_new_hardlinked_inode() = default;

	virtual bool evaluate(const cat_nomme &first, const cat_nomme &second) const override;
	virtual criterium *clone() const override { return new (std::nothrow) crit_in_place_is_new_hardlinked_inode(*this); };
    };


	/// returns true if the data of the first entry is more recent or of the same date of the one of the second entry

	/// this class always returns false if both entry are not inode. Comparison is done on mtime

    class crit_in_place_data_more_recent : public criterium
    {
    public:
	crit_in_place_data_more_recent(const infinint & hourshift = 0) : x_hourshift(hourshift) {};
	crit_in_place_data_more_recent(const crit_in_place_data_more_recent & ref) = default;
	crit_in_place_data_more_recent & operator = (const crit_in_place_data_more_recent & ref) = default;
	~crit_in_place_data_more_recent() = default;

	virtual bool evaluate(const cat_nomme &first, const cat_nomme &second) const override;
	virtual criterium *clone() const override { return new (std::nothrow) crit_in_place_data_more_recent(*this); };

    private:
	infinint x_hourshift;
    };


	/// returns true if the data of the first entry is more recent or of the same date as the fixed date given in argument to the constructor

	/// If the in_place entry is not an inode its date is considered equal to zero. Comparison is done on mtime


    class crit_in_place_data_more_recent_or_equal_to : public criterium
    {
    public:
	crit_in_place_data_more_recent_or_equal_to(const infinint & date, const infinint & hourshift = 0) : x_hourshift(hourshift), x_date(date) {};
	crit_in_place_data_more_recent_or_equal_to(const crit_in_place_data_more_recent_or_equal_to & ref) = default;
	crit_in_place_data_more_recent_or_equal_to & operator = (const crit_in_place_data_more_recent_or_equal_to & ref) = default;
	~crit_in_place_data_more_recent_or_equal_to() = default;

	virtual bool evaluate(const cat_nomme &first, const cat_nomme &second) const override;
	virtual criterium *clone() const override { return new (std::nothrow) crit_in_place_data_more_recent_or_equal_to(*this); };

    private:
	infinint x_hourshift;
	infinint x_date;
    };


	///  returns true if the data of the first entry is bigger or equal to the one of the second entry

	/// this class always returns false if both entries are not plain files

    class crit_in_place_data_bigger : public criterium
    {
    public:
	crit_in_place_data_bigger() = default;
	crit_in_place_data_bigger(const crit_in_place_data_bigger & ref) = default;
	crit_in_place_data_bigger & operator = (const crit_in_place_data_bigger & ref) = default;
	~crit_in_place_data_bigger() = default;

	virtual bool evaluate(const cat_nomme &first, const cat_nomme &second) const override;
	virtual criterium *clone() const override { return new (std::nothrow) crit_in_place_data_bigger(*this); };
    };



	/// returns true if the data of the first entry is saved int the archive (not marked as unchanged since the archive of reference)

	/// if the entry is not an inode the result is also true

    class crit_in_place_data_saved : public criterium
    {
    public:
	crit_in_place_data_saved() = default;
	crit_in_place_data_saved(const crit_in_place_data_saved & ref) = default;
	crit_in_place_data_saved & operator = (const crit_in_place_data_saved & ref) = default;
	~crit_in_place_data_saved() = default;

	virtual bool evaluate(const cat_nomme &first, const cat_nomme &second) const override;
	virtual criterium *clone() const override { return new (std::nothrow) crit_in_place_data_saved(*this); };
    };


	/// return true if the entry is a dirty file (or hard linked dirty file)

    class crit_in_place_data_dirty : public criterium
    {
    public:
	crit_in_place_data_dirty() = default;
	crit_in_place_data_dirty(const crit_in_place_data_dirty & ref) = default;
	crit_in_place_data_dirty & operator = (const crit_in_place_data_dirty & ref) = default;
	~crit_in_place_data_dirty() = default;

	virtual bool evaluate(const cat_nomme &first, const cat_nomme &second) const override;
	virtual criterium *clone() const override { return new (std::nothrow) crit_in_place_data_dirty(*this); };
    };

	/// return true if the entry is a sparse file (or hard linked sparse file)

    class crit_in_place_data_sparse : public criterium
    {
    public:
	crit_in_place_data_sparse() = default;
	crit_in_place_data_sparse(const crit_in_place_data_sparse & ref) = default;
	crit_in_place_data_sparse & operator = (const crit_in_place_data_sparse & ref) = default;
	~crit_in_place_data_sparse() = default;

	virtual bool evaluate(const cat_nomme &first, const cat_nomme &second) const override;
	virtual criterium *clone() const override { return new (std::nothrow) crit_in_place_data_sparse(*this); };
    };


    	/// return true if the entry has delta signatur

    class crit_in_place_has_delta_sig : public criterium
    {
    public:
	crit_in_place_has_delta_sig() = default;
	crit_in_place_has_delta_sig(const crit_in_place_has_delta_sig & ref) = default;
	crit_in_place_has_delta_sig & operator = (const crit_in_place_has_delta_sig & ref) = default;
	~crit_in_place_has_delta_sig() = default;

	virtual bool evaluate(const cat_nomme &first, const cat_nomme &second) const override;
	virtual criterium *clone() const override { return new (std::nothrow) crit_in_place_has_delta_sig(*this); };
    };


	/// returns true if the first entry is first an inode, and has some EA (EA may be saved
	/// or just recorded as existing).

    class crit_in_place_EA_present : public criterium
    {
    public:
	crit_in_place_EA_present() = default;
	crit_in_place_EA_present(const crit_in_place_EA_present & ref) = default;
	crit_in_place_EA_present & operator = (const crit_in_place_EA_present & ref) = default;
	~crit_in_place_EA_present() = default;

	virtual bool evaluate(const cat_nomme &first, const cat_nomme &second) const override
	{
	    const cat_inode *tmp = dynamic_cast<const cat_inode *>(&first);
	    return tmp != nullptr && tmp->ea_get_saved_status() != cat_inode::ea_none && tmp->ea_get_saved_status() != cat_inode::ea_removed;
	};
	virtual criterium *clone() const override { return new (std::nothrow) crit_in_place_EA_present(*this); };
    };


	///  returns true if the EA of the first entry is more recent or equal to the one of the second entry

	/// if no EA are present in 'to be added' or if it even not an inode true is returned. If 'in place'
	/// does not have EA or is even not an inode true is returned unless 'to be added' has EA present.
	/// \note that the comparison is done on the ctime, EA may be just marked as saved in the archive of
	/// reference or be saved in the current archive, this does not have any impact on the comparison.

    class crit_in_place_EA_more_recent : public criterium
    {
    public:
	crit_in_place_EA_more_recent(const infinint & hourshift = 0) : x_hourshift(hourshift) {};
	crit_in_place_EA_more_recent(const crit_in_place_EA_more_recent & ref) = default;
	crit_in_place_EA_more_recent & operator = (const crit_in_place_EA_more_recent & ref) = default;
	~crit_in_place_EA_more_recent() = default;


	virtual bool evaluate(const cat_nomme &first, const cat_nomme &second) const override;
	virtual criterium *clone() const override { return new (std::nothrow) crit_in_place_EA_more_recent(*this); };

    private:
	infinint x_hourshift;
    };


	///  returns true if the EA of the first entry is more recent or equal to the fixed date given in argument to the constructor

	/// comparison using ctime of the "in place" object. If no ctime is available (not an inode for example)
	/// the date is considered equal to zero.

    class crit_in_place_EA_more_recent_or_equal_to : public criterium
    {
    public:
	crit_in_place_EA_more_recent_or_equal_to(const infinint & date, const infinint & hourshift = 0) : x_hourshift(hourshift), x_date(date) {};
	crit_in_place_EA_more_recent_or_equal_to(const crit_in_place_EA_more_recent_or_equal_to & ref) = default;
	crit_in_place_EA_more_recent_or_equal_to & operator = (const crit_in_place_EA_more_recent_or_equal_to & ref) = default;
	~crit_in_place_EA_more_recent_or_equal_to() = default;

	virtual bool evaluate(const cat_nomme &first, const cat_nomme &second) const override;
	virtual criterium *clone() const override { return new (std::nothrow) crit_in_place_EA_more_recent_or_equal_to(*this); };

    private:
	infinint x_hourshift;
	infinint x_date;
    };


	///  returns true if the first entry has more or even EA (in number not in size) than the second entry

	/// if an entry is not an inode or has no EA it is assumed it has zero EA, the comparison is done on that number.

    class crit_in_place_more_EA : public criterium
    {
    public:
	crit_in_place_more_EA() = default;
	crit_in_place_more_EA(const crit_in_place_more_EA & ref) = default;
	crit_in_place_more_EA & operator = (const crit_in_place_more_EA & ref) = default;
	~crit_in_place_more_EA() = default;

	virtual bool evaluate(const cat_nomme &first, const cat_nomme &second) const override;
	virtual criterium *clone() const override { return new (std::nothrow) crit_in_place_more_EA(*this); };
    };



	///  returns true if the space used by EA of the first entry is greater or equal to the space used by the EA of the second entry (no EA means 0 byte for EA storage)

	/// this criterium does not have any consideration for the second entry

    class crit_in_place_EA_bigger : public crit_in_place_more_EA
    {
    public:
	crit_in_place_EA_bigger() = default;
	crit_in_place_EA_bigger(const crit_in_place_EA_bigger & ref) = default;
	crit_in_place_EA_bigger & operator = (const crit_in_place_EA_bigger & ref) = default;
	~crit_in_place_EA_bigger() = default;

	virtual bool evaluate(const cat_nomme &first, const cat_nomme &second) const override;
	virtual criterium *clone() const override { return new (std::nothrow) crit_in_place_EA_bigger(*this); };
    };


	/// returns true if the in place entry has its EA saved (not just marked as saved) in the archve of reference

	/// this criterium does not have any consideration for the second entry

    class crit_in_place_EA_saved : public criterium
    {
    public:
	crit_in_place_EA_saved() = default;
	crit_in_place_EA_saved(const crit_in_place_EA_saved & ref) = default;
	crit_in_place_EA_saved & operator = (const crit_in_place_EA_saved & ref) = default;
	~crit_in_place_EA_saved() = default;

	virtual bool evaluate(const cat_nomme &first, const cat_nomme &second) const override;
	virtual criterium *clone() const override { return new (std::nothrow) crit_in_place_EA_saved(*this); };
    };


	/// returns true if the two entries are of the same type (plain-file/char dev/block dev/named pipe/symlink/directory/unix socket)

	/// two plain files are considered of same type even if one is hard linked while the other is not
	/// same thing whether one entry has EA while the other has not, they are still considered of the same type.

    class crit_same_type : public criterium
    {
    public:
	crit_same_type() = default;
	crit_same_type(const crit_same_type & ref) = default;
	crit_same_type & operator = (const crit_same_type & ref) = default;
	~crit_same_type() = default;

	virtual bool evaluate(const cat_nomme &first, const cat_nomme &second) const override;
	virtual criterium *clone() const override { return new (std::nothrow) crit_same_type(*this); };
    };


	/// realises the negation of the criterium given in argument to its constructor

    class crit_not : public criterium
    {
    public:
	crit_not(const criterium & crit) { x_crit = crit.clone(); if(x_crit == nullptr) throw Ememory("crit_not::crit_not"); };
	crit_not(const crit_not & ref) : criterium (ref) { copy_from(ref); };
	crit_not & operator = (const crit_not & ref) { destroy(); copy_from(ref); return *this; };
	~crit_not() { destroy(); };

	virtual bool evaluate(const cat_nomme & first, const cat_nomme & second) const override { return ! x_crit->evaluate(first, second); };
	virtual criterium *clone() const override { return new (std::nothrow) crit_not(*this); };

    protected:
	const criterium *x_crit;

    private:
	void copy_from(const crit_not & ref);
	void destroy() { if(x_crit != nullptr) { delete x_crit; x_crit = nullptr; } };
    };

	/// realises the *AND* operator

    class crit_and : public criterium
    {
    public:
	crit_and() { clear(); };
	crit_and(const crit_and & ref) : criterium(ref) { copy_from(ref); };
	crit_and & operator = (const crit_and & ref) { detruit(); copy_from(ref); return *this; };
	~crit_and() { detruit(); };

	void add_crit(const criterium & ref);
	void clear() { detruit(); };

	    /// this call merges to the current call the arguments of another "crit_and", the given argument is cleared of its arguments.
	void gobe(crit_and & to_be_voided);

	virtual bool evaluate(const cat_nomme & first, const cat_nomme & second) const override;
	virtual criterium *clone() const override { return new (std::nothrow) crit_and(*this); };

    protected:
	std::vector<criterium *> operand;

    private:
	void copy_from(const crit_and & ref);
	void detruit();
    };

    class crit_or : public crit_and
    {
    public:
	crit_or() { clear(); };
	crit_or(const crit_or & ref) = default;
	crit_or & operator = (const crit_or & ref) = default;
	~crit_or() = default;

	virtual bool evaluate(const cat_nomme & first, const cat_nomme & second) const override;
	virtual criterium *clone() const override { return new (std::nothrow) crit_or(*this); };

    };

    class crit_invert : public crit_not
    {
    public:
	crit_invert(const criterium & crit) : crit_not(crit) {};
	crit_invert(const crit_invert & ref) = default;
	crit_invert & operator = (const crit_invert & ref) = default;
	~crit_invert() = default;

	virtual bool evaluate(const cat_nomme & first, const cat_nomme & second) const override { return x_crit->evaluate(second, first); };
	virtual criterium *clone() const override { return new (std::nothrow) crit_invert(*this); };
    };

	/// @}

} // end of namespace

#endif
