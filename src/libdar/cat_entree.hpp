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

    /// \file cat_entree.hpp
    /// \brief base class for all object contained in a catalogue
    /// \ingroup Private

#ifndef CAT_ENTREE_HPP
#define CAT_ENTREE_HPP

#include "../my_config.h"

extern "C"
{
} // end extern "C"

#include "infinint.hpp"
#include "generic_file.hpp"
#include "user_interaction.hpp"
#include "escape.hpp"
#include "on_pool.hpp"
#include "archive_version.hpp"
#include "compressor.hpp"

namespace libdar
{
    class etoile;
    class entree;

	/// \addtogroup Private
	/// @{

    enum saved_status
    {
	s_saved,      //< inode is saved in the archive
	s_fake,       //< inode is not saved in the archive but is in the archive of reference (isolation context) s_fake is no more used in archive format "08" and above: isolated catalogue do keep the data pointers and s_saved stays a valid status in isolated catalogues.
	s_not_saved   //< inode is not saved in the archive
    };

	/// holds the statistics contents of a catalogue
    struct entree_stats
    {
        infinint num_x;                  //< number of file referenced as destroyed since last backup
        infinint num_d;                  //< number of directories
        infinint num_f;                  //< number of plain files (hard link or not, thus file directory entries)
	infinint num_c;                  //< number of char devices
	infinint num_b;                  //< number of block devices
	infinint num_p;                  //< number of named pipes
	infinint num_s;                  //< number of unix sockets
	infinint num_l;                  //< number of symbolic links
	infinint num_D;                  //< number of Door
	infinint num_hard_linked_inodes; //< number of inode that have more than one link (inode with "hard links")
        infinint num_hard_link_entries;  //< total number of hard links (file directory entry pointing to \an
            //< inode already linked in the same or another directory (i.e. hard linked))
        infinint saved; //< total number of saved inode (unix inode, not inode class) hard links do not count here
        infinint total; //< total number of inode in archive (unix inode, not inode class) hard links do not count here
        void clear() { num_x = num_d = num_f = num_c = num_b = num_p
                = num_s = num_l = num_D = num_hard_linked_inodes
                = num_hard_link_entries = saved = total = 0; };
        void add(const entree *ref);
        void listing(user_interaction & dialog) const;
    };

	/// the root class from all other inherite for any entry in the catalogue
    class entree : public on_pool
    {
    public :
        static entree *read(user_interaction & dialog,
			    memory_pool *pool,
			    generic_file & f, const archive_version & reading_ver,
			    entree_stats & stats,
			    std::map <infinint, etoile *> & corres,
			    compression default_algo,
			    generic_file *data_loc,
			    compressor *efsa_loc,
			    bool lax,
			    bool only_detruit,
			    escape *ptr);

        virtual ~entree() {};

	    /// write down the object information to a generic_file

	    /// \param[in,out] f is the file where to write the data to
	    /// \param[in] small defines whether to do a small or normal dump
	    /// \note small dump are used beside escape sequence marks they can be done
	    /// before the a file's data or EA has took its place within the archive
	    /// while normal dump are used with catalogue dump at the end of the archive
	    /// creation
        void dump(generic_file & f, bool small) const;

	    /// this call gives an access to inherited_dump

	    /// \param[in,out] f is the file where to write the data to
	    /// \param[in] small defines whether to do a small or normal dump
	    /// \note this method is to avoid having class mirage and class directory being
	    /// a friend of class entree. Any other class may use it, sure, but neither
	    /// class mirage nor class directory has not access to class entree's private
	    /// data, only to what it needs.
	void specific_dump(generic_file & f, bool small) const { inherited_dump(f, small); };

	    /// called by entree::read and mirage::post_constructor, let inherited classes builds object's data after CRC has been read from file

	    /// \param[in,out] f is the file where to write the data to
	    /// \note only used when an non NULL escape pointer is given to entree::read (reading a small dump).
	virtual void post_constructor(generic_file & f) {};

        virtual unsigned char signature() const = 0;
        virtual entree *clone() const = 0;


    protected:
	virtual void inherited_dump(generic_file & f, bool small) const;


    private:
	static const U_I ENTREE_CRC_SIZE;

    };

    extern bool compatible_signature(unsigned char a, unsigned char b);
    extern unsigned char mk_signature(unsigned char base, saved_status state);
    extern unsigned char get_base_signature(unsigned char a);

	/// @}

} // end of namespace

#endif
