/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2019 Denis Corbin
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

    /// \file crit_action.hpp
    /// \brief contains classes that let the user define the policy for overwriting files
    /// \ingroup API

#ifndef CRIT_ACTION_HPP
#define CRIT_ACTION_HPP

#include "../my_config.h"

#include <deque>

#include "user_interaction.hpp"
#include "criterium.hpp"

namespace libdar
{

	/// \addtogroup API
	/// @{

	/// no need to dig into class cat_nomme here
    class cat_nomme;

	/// the possible actions for overwriting data

    enum over_action_data
    {
	data_preserve,                     ///< do not overwrite (keep the 'in place' entry)
	data_overwrite,                    ///< overwirte the 'in place' entry by the 'to be added' one
	data_preserve_mark_already_saved,  ///< keep the 'in place' but mark it as already saved in the archive of reference
	data_overwrite_mark_already_saved, ///< overwrite the 'in place' but mark the 'to be added' as already saved in the archive of reference
	data_remove,                       ///< remove the original data/EA (file is completely deleted)
	data_undefined,                    ///< action still undefined at this step of the evaluation
	data_ask                           ///< ask for user decision about file's data
    };


	/// the possible action for overwriting EA

	/// define the action to apply to each EA entry (not to the EA set of a particular inode)
    enum over_action_ea
    {
	EA_preserve,                     ///< keep the EA of the 'in place' entry
	EA_overwrite,                    ///< keep the EA of the 'to be added' entry
	EA_clear,                        ///< drop the EA for the elected entry
        EA_preserve_mark_already_saved,  ///< drop any EA but mark them as already saved in the archive of reference (ctime is the one of the 'in place' inode)
        EA_overwrite_mark_already_saved, ///< drop any EA but mark them as already saved in the archive of reference (ctime is the one of the 'to be added' inode)
	EA_merge_preserve,               ///< merge EA but do not overwrite existing EA of 'in place' by one of the same name of 'to be added' inode
	EA_merge_overwrite,              ///< merge EA but if both inode share an EA with the same name, take keep the one of the 'to be added' inode
	EA_undefined,                    ///< action still undefined at this step of the evaluation
	EA_ask                           ///< ask for user decision about EA
    };


	/// the global action for overwriting

	/// this class is a generic interface to handle what action to perform on both EA and Data
	/// based on two files to evaluate.

    class crit_action
    {
    public:
	crit_action() = default;
	crit_action(const crit_action & ref) = default;
	crit_action(crit_action && ref) noexcept = default;
	crit_action & operator = (const crit_action & ref) = default;
	crit_action & operator = (crit_action && ref) noexcept = default;

	    /// the destructor
	virtual ~crit_action() = default;

	    /// the action to take based on the files to compare

	    /// \param[in] first is the 'in place' inode
	    /// \param[in] second is the 'to be added' inode
	    /// \param[out] data is the action to perform with file's data
	    /// \param[out] ea is the action to perform with file's EA
	virtual void get_action(const cat_nomme & first, const cat_nomme & second, over_action_data & data, over_action_ea & ea) const = 0;

	    /// clone construction method

	    /// \return a new object of the same type,
	    /// \note this method must be implemented in all the leaf classes of the
	    /// class crit_action hierarchy
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
	crit_constant_action(const crit_constant_action & ref) = default;
	crit_constant_action & operator = (const crit_constant_action & ref) = default;
	~crit_constant_action() = default;


	    /// the inherited pure virtual methods from class action that must be implemented
	virtual void get_action(const cat_nomme & first, const cat_nomme & second, over_action_data & data, over_action_ea & ea) const override { data = x_data; ea = x_ea; };
	virtual crit_action *clone() const override { return new (std::nothrow) crit_constant_action(*this); };

    private:
	over_action_data x_data;
	over_action_ea x_ea;
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
	testing(testing && ref) noexcept : crit_action(std::move(ref)) { nullifyptr(); move_from(std::move(ref)); };
	testing & operator = (const testing & ref) { free(); copy_from(ref); if(!check()) throw Ememory("testing::testing(const testing &)"); return *this; };
	testing & operator = (testing && ref) noexcept { crit_action::operator = (std::move(ref)); move_from(std::move(ref)); return *this; };
	~testing() { free(); };


	    /// the inherited pure virtual method from class crit_action that must be implemented
	virtual void get_action(const cat_nomme & first, const cat_nomme & second, over_action_data & data, over_action_ea & ea) const override
	{
	    if(x_input->evaluate(first, second))
		x_go_true->get_action(first, second, data, ea);
	    else
		x_go_false->get_action(first, second, data, ea);
	};

	virtual crit_action *clone() const override { return new (std::nothrow) testing(*this); };

    private:
	criterium *x_input;
	crit_action *x_go_true;
	crit_action *x_go_false;

	void nullifyptr() noexcept { x_input = nullptr; x_go_true = x_go_false = nullptr; };
	void free() noexcept;
	void copy_from(const testing & ref);
	void move_from(testing && ref) noexcept;
	bool check() const; ///< returns false if an field is nullptr
    };


	/// the crit_chain class sequences crit_actions up to full definition of the action

	/// several expressions must be added. The first is evaluated, then the second, up to the last
	/// or up to the step the data_action and ea_action are both fully defined (no data_undefined nor ea_undefined)

    class crit_chain : public crit_action
    {
    public:
	crit_chain() { sequence.clear(); };
	crit_chain(const crit_chain & ref) : crit_action(ref) { copy_from(ref); };
	crit_chain(crit_chain && ref) noexcept : crit_action(std::move(ref)) { sequence = std::move(ref.sequence); };
	crit_chain & operator = (const crit_chain & ref) { destroy(); copy_from(ref); return *this; };
	crit_chain & operator = (crit_chain && ref) noexcept { crit_action::operator = (std::move(ref)); sequence = std::move(ref.sequence); return *this; };
	~crit_chain() { destroy(); };

	void add(const crit_action & act);
	void clear() { destroy(); };
	void gobe(crit_chain & to_be_voided);

	virtual void get_action(const cat_nomme & first, const cat_nomme & second, over_action_data & data, over_action_ea & ea) const override;

	virtual crit_action *clone() const override { return new (std::nothrow) crit_chain(*this); };

    private:
	std::deque<crit_action *> sequence;

	void destroy();
	void copy_from(const crit_chain & ref);
    };

	/// @}

} // end of namespace

#endif
