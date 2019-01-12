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

#include "on_pool.hpp"
#include "cat_nomme.hpp"
#include "cat_inode.hpp"
#include "cat_directory.hpp"

namespace libdar
{

	/// \addtogroup API
	/// @{

	/// the possible actions for overwriting data

    enum over_action_data
    {
	data_preserve,                     //< do not overwrite (keep the 'in place' entry)
	data_overwrite,                    //< overwirte the 'in place' entry by the 'to be added' one
	data_preserve_mark_already_saved,  //< keep the 'in place' but mark it as already saved in the archive of reference
	data_overwrite_mark_already_saved, //< overwrite the 'in place' but mark the 'to be added' as already saved in the archive of reference
	data_remove,                       //< remove the original data/EA (file is completely deleted)
	data_undefined,                    //< action still undefined at this step of the evaluation
	data_ask                           //< ask for user decision about file's data
    };


	/// the possible action for overwriting EA

    enum over_action_ea //< define the action to apply to each EA entry (not to the EA set of a particular inode)
    {
	EA_preserve,                     //< keep the EA of the 'in place' entry
	EA_overwrite,                    //< keep the EA of the 'to be added' entry
	EA_clear,                        //< drop the EA for the elected entry
        EA_preserve_mark_already_saved,  //< drop any EA but mark them as already saved in the archive of reference (ctime is the one of the 'in place' inode)
        EA_overwrite_mark_already_saved, //< drop any EA but mark them as already saved in the archive of reference (ctime is the one of the 'to be added' inode)
	EA_merge_preserve,               //< merge EA but do not overwrite existing EA of 'in place' by one of the same name of 'to be added' inode
	EA_merge_overwrite,              //< merge EA but if both inode share an EA with the same name, take keep the one of the 'to be added' inode
	EA_undefined,                    //< action still undefined at this step of the evaluation
	EA_ask                           //< ask for user decision about EA
    };


	/// the global action for overwriting

	/// this class is a generic interface to handle what action to perform on both EA and Data
	/// based on two files to evaluate.

    class crit_action: public on_pool
    {
    public:
	    /// the destructor
	virtual ~crit_action() {};

	    /// the action to take based on the files to compare

	    /// \param[in] first is the 'in place' inode
	    /// \param[in] second is the 'to be added' inode
	    /// \param[out] data is the action to perform with file's data
	    /// \param[out] ea is the action to perform with file's EA
	virtual void get_action(const cat_nomme & first, const cat_nomme & second, over_action_data & data, over_action_ea & ea) const = 0;

	    /// clone construction method

	    /// \return a new object of the same type,
	    /// \note this method must be implemented in all the leaf classes of the
	    /// action hierarchy class
	virtual crit_action *clone() const = 0;
    };


	/// the basic constant action

	/// the resulting action is not dependant on the files to compare
	/// it always returns the action provided through its constructor

    class crit_constant_action : public crit_action
    {
    public:
	    /// the constuctor

	    /// \param[in] data the action to perform on data
	    /// \param[in] ea the action to perform on EA
	crit_constant_action(over_action_data data, over_action_ea ea) { x_data = data; x_ea = ea; };


	    /// the inherited pure virtual methods from class action that must be implemented
	void get_action(const cat_nomme & first, const cat_nomme & second, over_action_data & data, over_action_ea & ea) const { data = x_data; ea = x_ea; };
	crit_action *clone() const { return new (get_pool()) crit_constant_action(*this); };

    private:
	over_action_data x_data;
	over_action_ea x_ea;
    };



	/// the generic criterium class, parent of all criterium

	/// this is a pure virtual class that is used in API call
	/// it is used to federate under a single type all the
	/// criterium classes defined below. It defines a common
	/// interface for all of them.

    class criterium : public on_pool
    {
    public:
	virtual ~criterium() {};

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



	/// the testing class binds criterium to actions

	/// a testing class is also an action that let the user build complex
	/// testing. It is thus possible to recursively use testing inside testing

    class testing : public crit_action
    {
    public:
	    /// the constructor

	    /// \param[in] input is the criterium to base the evaluation on
	    /// \param[in] go_true is the action to use for evaluation if the criterium states true
	    /// \param[in] go_false is the action to use for evaluation if the criterium states false
	testing(const criterium & input, const crit_action & go_true, const crit_action & go_false);
	testing(const testing & ref) : crit_action(ref) { copy_from(ref); if(!check()) throw Ememory("testing::testing(const testing &)"); };
	const testing & operator = (const testing & ref) { free(); copy_from(ref); if(!check()) throw Ememory("testing::testing(const testing &)"); return *this; };
	~testing() { free(); };


	    /// the inherited pure virtual method from class action that must be gimplemented
	void get_action(const cat_nomme & first, const cat_nomme & second, over_action_data & data, over_action_ea & ea) const
	{
	    if(x_input->evaluate(first, second))
		x_go_true->get_action(first, second, data, ea);
	    else
		x_go_false->get_action(first, second, data, ea);
	};

	crit_action *clone() const { return new (get_pool()) testing(*this); };

    private:
	criterium *x_input;
	crit_action *x_go_true;
	crit_action *x_go_false;

	void free();
	void copy_from(const testing & ref);
	bool check() const; //< returns false if an field is nullptr
    };


	/// the crit_chain class sequences crit_actions up to full definition of the action

	/// several expressions must be added. The first is evaluated, then the second, up to the last
	/// or up to the step the data_action and ea_action are both fully defined (no data_undefined nor ea_undefined)

    class crit_chain : public crit_action
    {
    public:
	crit_chain() { sequence.clear(); };
	crit_chain(const crit_chain & ref) : crit_action(ref) { copy_from(ref); };
	const crit_chain & operator = (const crit_chain & ref) { destroy(); copy_from(ref); return *this; };
	~crit_chain() { destroy(); };

	void add(const crit_action & act);
	void clear() { destroy(); };
	void gobe(crit_chain & to_be_voided);

	void get_action(const cat_nomme & first, const cat_nomme & second, over_action_data & data, over_action_ea & ea) const;

	crit_action *clone() const { return new (get_pool()) crit_chain(*this); };

    private:
	std::vector<crit_action *> sequence;

	void destroy();
	void copy_from(const crit_chain & ref);
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
	bool evaluate(const cat_nomme &first, const cat_nomme &second) const;
	criterium *clone() const { return new (get_pool()) crit_in_place_is_inode(*this); };
    };


	/// returns true if the first entry is a cat_directory (whatever is the second)

    class crit_in_place_is_dir : public criterium
    {
    public:
	bool evaluate(const cat_nomme &first, const cat_nomme &second) const { return dynamic_cast<const cat_directory *>(&first) != nullptr; };
	criterium *clone() const { return new (get_pool()) crit_in_place_is_dir(*this); };
    };


	/// returns true if the first entry is a plain file (whatever is the second)

    class crit_in_place_is_file : public criterium
    {
    public:
	bool evaluate(const cat_nomme &first, const cat_nomme &second) const;
	criterium *clone() const { return new (get_pool()) crit_in_place_is_file(*this); };
    };

	/// returns true if the first entry is a inode with several hard links (whatever is the second entry)

	/// it may be a plain file, a symlink a char device, a block device or a named pipe for example

    class crit_in_place_is_hardlinked_inode : public criterium
    {
    public:
	bool evaluate(const cat_nomme &first, const cat_nomme &second) const;
	criterium *clone() const { return new (get_pool()) crit_in_place_is_hardlinked_inode(*this); };
    };


	/// returns true if the first entry is a inode with several hard links (whatever is the second entry) and also if this first entry is the first we meet that points to this hard linked inode
    class crit_in_place_is_new_hardlinked_inode : public criterium
    {
    public:
	bool evaluate(const cat_nomme &first, const cat_nomme &second) const;
	criterium *clone() const { return new (get_pool()) crit_in_place_is_new_hardlinked_inode(*this); };
    };


	/// returns true if the data of the first entry is more recent or of the same date of the one of the second entry

	/// this class always returns false if both entry are not inode. Comparison is done on mtime

    class crit_in_place_data_more_recent : public criterium
    {
    public:
	crit_in_place_data_more_recent(const infinint & hourshift = 0) : x_hourshift(hourshift) {};

	bool evaluate(const cat_nomme &first, const cat_nomme &second) const;
	criterium *clone() const { return new (get_pool()) crit_in_place_data_more_recent(*this); };

    private:
	infinint x_hourshift;
    };


	/// returns true if the data of the first entry is more recent or of the same date as the fixed date given in argument to the constructor

	/// If the in_place entry is not an inode its date is considered equal to zero. Comparison is done on mtime


    class crit_in_place_data_more_recent_or_equal_to : public criterium
    {
    public:
	crit_in_place_data_more_recent_or_equal_to(const infinint & date, const infinint & hourshift = 0) : x_hourshift(hourshift), x_date(date) {};

	bool evaluate(const cat_nomme &first, const cat_nomme &second) const;
	criterium *clone() const { return new (get_pool()) crit_in_place_data_more_recent_or_equal_to(*this); };

    private:
	infinint x_hourshift;
	infinint x_date;
    };


	///  returns true if the data of the first entry is bigger or equal to the one of the second entry

	/// this class always returns false if both entries are not plain files

    class crit_in_place_data_bigger : public criterium
    {
    public:
	bool evaluate(const cat_nomme &first, const cat_nomme &second) const;
	criterium *clone() const { return new (get_pool()) crit_in_place_data_bigger(*this); };
    };



	/// returns true if the data of the first entry is saved int the archive (not marked as unchanged since the archive of reference)

	/// if the entry is not an inode the result is also true

    class crit_in_place_data_saved : public criterium
    {
    public:
	bool evaluate(const cat_nomme &first, const cat_nomme &second) const;
	criterium *clone() const { return new (get_pool()) crit_in_place_data_saved(*this); };
    };


	/// return true if the entry is a dirty file (or hard linked dirty file)

    class crit_in_place_data_dirty : public criterium
    {
    public:
	bool evaluate(const cat_nomme &first, const cat_nomme &second) const;
	criterium *clone() const { return new (get_pool()) crit_in_place_data_dirty(*this); };
    };

	/// return true if the entry is a sparse file (or hard linked sparse file)

    class crit_in_place_data_sparse : public criterium
    {
    public:
	bool evaluate(const cat_nomme &first, const cat_nomme &second) const;
	criterium *clone() const { return new (get_pool()) crit_in_place_data_sparse(*this); };
    };


	/// returns true if the first entry is first an inode, and has some EA (EA may be saved
	/// or just recorded as existing).

    class crit_in_place_EA_present : public criterium
    {
    public:
	bool evaluate(const cat_nomme &first, const cat_nomme &second) const
	{
	    const cat_inode *tmp = dynamic_cast<const cat_inode *>(&first);
	    return tmp != nullptr && tmp->ea_get_saved_status() != cat_inode::ea_none && tmp->ea_get_saved_status() != cat_inode::ea_removed;
	};
	criterium *clone() const { return new (get_pool()) crit_in_place_EA_present(*this); };
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

	bool evaluate(const cat_nomme &first, const cat_nomme &second) const;
	criterium *clone() const { return new (get_pool()) crit_in_place_EA_more_recent(*this); };

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

	bool evaluate(const cat_nomme &first, const cat_nomme &second) const;
	criterium *clone() const { return new (get_pool()) crit_in_place_EA_more_recent_or_equal_to(*this); };

    private:
	infinint x_hourshift;
	infinint x_date;
    };


	///  returns true if the first entry has more or even EA (in number not in size) than the second entry

	/// if an entry is not an inode or has no EA it is assumed it has zero EA, the comparison is done on that number.

    class crit_in_place_more_EA : public criterium
    {
    public:
	bool evaluate(const cat_nomme &first, const cat_nomme &second) const;
	criterium *clone() const { return new (get_pool()) crit_in_place_more_EA(*this); };
    };



	///  returns true if the space used by EA of the first entry is greater or equal to the space used by the EA of the second entry (no EA means 0 byte for EA storage)

	/// this criterium does not have any consideration for the second entry

    class crit_in_place_EA_bigger : public crit_in_place_more_EA
    {
    public:
	bool evaluate(const cat_nomme &first, const cat_nomme &second) const;
	criterium *clone() const { return new (get_pool()) crit_in_place_EA_bigger(*this); };
    };


	/// returns true if the in place entry has its EA saved (not just marked as saved) in the archve of reference

	/// this criterium does not have any consideration for the second entry

    class crit_in_place_EA_saved : public criterium
    {
    public:
	bool evaluate(const cat_nomme &first, const cat_nomme &second) const;
	criterium *clone() const { return new (get_pool()) crit_in_place_EA_saved(*this); };
    };


	/// returns true if the two entries are of the same type (plain-file/char dev/block dev/named pipe/symlink/directory/unix socket)

	/// two plain files are considered of same type even if one is hard linked while the other is not
	/// same thing whether one entry has EA while the other has not, they are still considered of the same type.

    class crit_same_type : public criterium
    {
    public:
	bool evaluate(const cat_nomme &first, const cat_nomme &second) const;
	criterium *clone() const { return new (get_pool()) crit_same_type(*this); };
    };


	/// realises the negation of the criterium given in argument to its constructor

    class crit_not : public criterium
    {
    public:
	crit_not(const criterium & crit) { x_crit = crit.clone(); if(x_crit == nullptr) throw Ememory("crit_not::crit_not"); };
	crit_not(const crit_not & ref) : criterium (ref) { copy_from(ref); };
	const crit_not & operator = (const crit_not & ref) { destroy(); copy_from(ref); return *this; };
	~crit_not() { destroy(); };

	bool evaluate(const cat_nomme & first, const cat_nomme & second) const { return ! x_crit->evaluate(first, second); };
	criterium *clone() const { return new (get_pool()) crit_not(*this); };

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
	const crit_and & operator = (const crit_and & ref) { detruit(); copy_from(ref); return *this; };
	~crit_and() { detruit(); };

	void add_crit(const criterium & ref);
	void clear() { detruit(); };

	    /// this call merges to the current call the arguments of another "crit_and", the given argument is cleared of its arguments.
	void gobe(crit_and & to_be_voided);

	virtual bool evaluate(const cat_nomme & first, const cat_nomme & second) const;
	criterium *clone() const { return new (get_pool()) crit_and(*this); };

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

	bool evaluate(const cat_nomme & first, const cat_nomme & second) const;
	criterium *clone() const { return new (get_pool()) crit_or(*this); };

    };

    class crit_invert : public crit_not
    {
    public:
	crit_invert(const criterium & crit) : crit_not(crit) {};

	bool evaluate(const cat_nomme & first, const cat_nomme & second) const { return x_crit->evaluate(second, first); };
	criterium *clone() const { return new (get_pool()) crit_invert(*this); };
    };


	/// ask user for EA action

	/// \param[in] dialog for user interaction
	/// \param[in] full_name full path to the entry do ask decision for
	/// \param[in] already_here pointer to the object 'in place'
	/// \param[in] dolly pointer to the object 'to be added'
	/// \return the action decided by the user. The user may also choose to abort, which will throw an Euser_abort exception
    extern over_action_ea crit_ask_user_for_EA_action(user_interaction & dialog, const std::string & full_name, const cat_entree *already_here, const cat_entree *dolly);

	/// ask user for FSA action

	/// \param[in] dialog for user interaction
	/// \param[in] full_name full path to the entry do ask decision for
	/// \param[in] already_here pointer to the object 'in place'
	/// \param[in] dolly pointer to the object 'to be added'
	/// \return the action decided by the user. The user may also choose to abort, which will throw an Euser_abort exception
    extern over_action_ea crit_ask_user_for_FSA_action(user_interaction & dialog, const std::string & full_name, const cat_entree *already_here, const cat_entree *dolly);

	/// ask user for Data action

	/// \param[in] dialog for user interaction
	/// \param[in] full_name full path to the entry do ask decision for
	/// \param[in] already_here pointer to the object 'in place'
	/// \param[in] dolly pointer to the object 'to be added'
	/// \return the action decided by the user. The user may also choose to abort, which will throw an Euser_abort exception
    extern over_action_data crit_ask_user_for_data_action(user_interaction & dialog, const std::string & full_name, const cat_entree *already_here, const cat_entree *dolly);


	/// show information suited for user comparison and decision for entry in conflict

	/// \param[in] dialog for user interaction
	/// \param[in] full_name path to the entry of the entry to display information
	/// \param[in] already_here pointer to the object 'in place'
	/// \param[in] dolly pointer to the object 'to be added'
    extern void crit_show_entry_info(user_interaction & dialog, const std::string & full_name, const cat_entree *already_here, const cat_entree *dolly);

	/// @}

} // end of namespace

#endif
