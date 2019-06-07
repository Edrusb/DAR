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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

    /// \file semaphore.hpp
    /// \brief definition of class semaphore, used to manage invocation of backup hook for files
    /// \ingroup Private

#ifndef SEMAPHORE_HPP
#define SEMAPHORE_HPP

#include "../my_config.h"

#include "mem_ui.hpp"
#include "mask.hpp"
#include "catalogue.hpp"
#include "on_pool.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

	/// class semaphore

	/// Its action is to invoke the execute hook for each file that match the given mask
	/// Each file to backup has to be "raised()", which, if it matches the mask, leads
	/// to the execution of the execute hook with the proper substitution for that file
	/// in the "start" context.
	/// Then the backup can take place.
	/// When the backup is finished, the lower() method has to be called to trigger the
	/// execution of the hook with the proper substitution but in the "end" context.
	/// but, things are a bit complicated due to the handle of directories:
	/// If a directory is "raised()" and matches the mask, next calls to raise() do not trigger any
	/// hook execution even if the file match the mask, while saving into the directory
	/// that matched first. Instead, each new call to raise() increments an internal counter when
	/// a new directory is met, which is decremented when an "eod" is given to raised().
	/// So it is important to feed raise() with any entry may it has to be saved or not.
	/// while lower() has only to be called when a file has been saved.
	/// This is only when this internal counters reaches zero, that the lower() call will
	/// trigger the execution for this first matched directory, of the hook in the "end" context.
	///
	/// So the expected use is to give each file to be saved (including eod) to the raise()
	/// method, before eventually saving the file, and call the lower() method only for files that had to
	/// be saved once the backup is completed, may it be normally or due to an exception being thrown.


    class semaphore : public mem_ui, public on_pool
    {
    public:

	    /// constructor

	    /// \param[in] dialog for user interaction
	    /// \param[in] backup_hook_file_execute is the string to execute, it can contains macros
	    /// to be substitued, %f by filename, %p by path, %u by uid, %g by gid, and %c by the context,
	    /// which is either "start" or "end".
	    /// \param[in] backup_hook_file_mask defines the path+filename of entry that need to have
	    /// the hook executed before and after their backup
	semaphore(user_interaction & dialog,
		  const std::string & backup_hook_file_execute,
		  const mask & backup_hook_file_mask);

	    /// copy constructor
	semaphore(const semaphore & ref) : mem_ui(ref) { copy_from(ref); };

	    /// assignment operator
	semaphore & operator = (const semaphore & ref) { detruit(); copy_from(ref); return *this; };

	    /// destructor
	~semaphore() { detruit(); };

	    /// to prepare a file for backup

	    /// all file has to be given to this call, even the eod objects
	    /// \param[in] path is the full path to the object
	    /// \param[in] object is the object about to be saved
	    /// \param[in] data_to_save tells whether this entry will have to be saved or just recursed into (directory for example)
	    /// \note, if data_to_save is true, the lower() method is expected to be used
	    /// before a next call to raise. For a directory this is only the call to lower()
	    /// of the matching EOD that will trigger the hook execution in the "end" context.
	    /// If instead data_to_save if false, no lower() call has to be done.
	void raise(const std::string & path,
		   const cat_entree *object,
		   bool data_to_save);

	    /// to tell that the backup is completed for the last "raised" entry.
	void lower();

    private:
	infinint count;       //< is the number of subdirectories currently saved in the last directory that matched the mask
	std::string chem;     //< path of the file that has to be call in the "end" context when count will drop to zero
	std::string filename; //< filename of that same file
	infinint uid;         //< UID of that same file
	infinint gid;         //< GID of that same file
	unsigned char sig;    //< object type
	std::string execute;  //< command to execute
	const mask *match;    //< for which file to run the execute command

	std::string build_string(const std::string & context);
	void copy_from(const semaphore & ref);
	void detruit();
    };

	/// @}

} // end of namespace

#endif
