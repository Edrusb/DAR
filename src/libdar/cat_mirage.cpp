/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2024 Denis Corbin
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
} // end extern "C"

#include <algorithm>
#include "cat_mirage.hpp"
#include "cat_file.hpp"
#include "cat_directory.hpp"

#define MIRAGE_ALONE 'X'
#define MIRAGE_WITH_INODE '>'


using namespace std;

namespace libdar
{

    cat_mirage::cat_mirage(const shared_ptr<user_interaction> & dialog,
			   const smart_pointer<pile_descriptor> & pdesc,
			   const archive_version & reading_ver,
			   saved_status saved,
			   entree_stats & stats,
			   std::map <infinint, cat_etoile *> & corres,
			   compression default_algo,
			   mirage_format fmt,
			   bool lax,
			   bool small) : cat_nomme(pdesc, small, saved_status::saved)
    {
        init(dialog,
             pdesc,
             reading_ver,
             saved,
             stats,
             corres,
             default_algo,
             fmt,
             lax,
             small);
    }

    cat_mirage::cat_mirage(const shared_ptr<user_interaction> & dialog,
			   const smart_pointer<pile_descriptor> & pdesc,
			   const archive_version & reading_ver,
			   saved_status saved,
			   entree_stats & stats,
			   std::map <infinint, cat_etoile *> & corres,
			   compression default_algo,
			   bool lax,
			   bool small): cat_nomme("TEMP", saved_status::saved)
    {
        init(dialog,
             pdesc,
             reading_ver,
             saved,
             stats,
             corres,
             default_algo,
             fmt_file_etiquette,
             lax,
             small);
    }

    void cat_mirage::init(const shared_ptr<user_interaction> & dialog,
			  const smart_pointer<pile_descriptor> & pdesc,
			  const archive_version & reading_ver,
			  saved_status saved,
			  entree_stats & stats,
			  std::map <infinint, cat_etoile *> & corres,
			  compression default_algo,
			  mirage_format fmt,
			  bool lax,
			  bool small)
    {
        infinint tmp_tiquette;
        char tmp_flag;
        map<infinint, cat_etoile *>::iterator etl;
        cat_inode *ino_ptr = nullptr;
        cat_entree *entree_ptr = nullptr;
        entree_stats fake_stats; // the call to cat_entree::read will increment counters with the inode we will read
            // but this inode will also be counted from the cat_entree::read we are called from.
            // thus we must not increment the real entree_stats structure here and we use a local variable
	generic_file *ptr = nullptr;

	pdesc->check(small);
	if(small)
	    ptr = pdesc->esc;
	else
	    ptr = pdesc->stack;


        if(fmt != fmt_file_etiquette)
            tmp_tiquette = infinint(*ptr);

        switch(fmt)
        {
        case fmt_mirage:
            ptr->read(&tmp_flag, 1);
            break;
        case fmt_hard_link:
            tmp_flag = MIRAGE_ALONE;
            break;
        case fmt_file_etiquette:
            tmp_flag = MIRAGE_WITH_INODE;
            break;
        default:
            throw SRC_BUG;
        }

        switch(tmp_flag)
        {
        case MIRAGE_ALONE:

                // we must link with the already existing cat_etoile

            etl = corres.find(tmp_tiquette);
            if(etl == corres.end())
                throw Erange("cat_mirage::cat_mirage", gettext("Incoherent catalogue structure: hard linked inode's data not found"));
            else
            {
                if(etl->second == nullptr)
                    throw SRC_BUG;
                star_ref = etl->second;
                star_ref->add_ref(this);
            }
            break;
        case MIRAGE_WITH_INODE:

                // we first read the attached inode

            if(fmt == fmt_file_etiquette)
            {
                cat_nomme *tmp_ptr = new (nothrow) cat_file(dialog, pdesc, reading_ver, saved, default_algo, small);
                entree_ptr = tmp_ptr;
                if(tmp_ptr != nullptr)
                {
                    change_name(tmp_ptr->get_name());
                    tmp_ptr->change_name("");
                    tmp_tiquette = infinint(*ptr);
                }
                else
                    throw Ememory("cat_mirage::init");
            }
            else
                entree_ptr = cat_entree::read(dialog, pdesc, reading_ver, fake_stats, corres, default_algo, lax, false, small);

            ino_ptr = dynamic_cast<cat_inode *>(entree_ptr);
            if(ino_ptr == nullptr || dynamic_cast<cat_directory *>(entree_ptr) != nullptr)
            {
                if(entree_ptr != nullptr)
                {
                    delete entree_ptr;
                    entree_ptr = nullptr;
                }
                throw Erange("cat_mirage::cat_mirage", gettext("Incoherent catalogue structure: hard linked data is not an inode"));
            }

                // then we can bind the inode to the next to be create cat_etoile object

            try
            {
                    // we must check that an already exiting cat_etoile is not present

                etl = corres.find(tmp_tiquette);
                if(etl == corres.end())
                {
                        // we can now create the cat_etoile and add it in the corres map;

                    star_ref = new (nothrow) cat_etoile(ino_ptr, tmp_tiquette);
                    try
                    {
                        if(star_ref == nullptr)
                            throw Ememory("cat_mirage::cat_mirage");
                        ino_ptr = nullptr; // the object pointed to by ino_ptr is now managed by star_ref
                        star_ref->add_ref(this);
                        corres[tmp_tiquette] = star_ref;
                    }
                    catch(...)
                    {
                        if(star_ref != nullptr)
                        {
                            delete star_ref;
                            star_ref = nullptr;
                        }
                        etl = corres.find(tmp_tiquette);
                        if(etl != corres.end())
                            corres.erase(etl);
                        throw;
                    }
                }
                else
                    throw Erange("cat_mirage::cat_mirage", gettext("Incoherent catalogue structure: duplicated hard linked inode's data"));
            }
            catch(...)
            {
                if(ino_ptr != nullptr)
                {
                    delete ino_ptr;
                    ino_ptr = nullptr;
                }
                throw;
            }

            break;
        default:
            throw Erange("cat_mirage::cat_mirage", gettext("Incoherent catalogue structure: unknown status flag for hard linked inode"));
        }
    }

    cat_mirage & cat_mirage::operator = (const cat_mirage & ref)
    {
	if(ref.star_ref == nullptr)
            throw SRC_BUG;

	    // copying the cat_nomme part of these objects
	cat_nomme::operator = (ref);

	if(ref.star_ref != star_ref)
	{
	    ref.star_ref->add_ref(this);
	    star_ref->drop_ref(this);
	    star_ref = ref.star_ref;
	}
        return *this;
    }

    cat_mirage & cat_mirage::operator = (cat_mirage && ref) noexcept
    {
	    // moving the cat_nomme part of these objects
	cat_nomme::operator = (move(ref));

	if(ref.star_ref != nullptr)
	{
	    if(ref.star_ref != star_ref)
	    {
		ref.star_ref->add_ref(this);
		star_ref->drop_ref(this);
		star_ref = ref.star_ref;
	    }
	}
	    // else this is a bug condition, but we are inside an noexcept method

	return *this;
    }

    void cat_mirage::post_constructor(const pile_descriptor & pdesc)
    {
	cat_nomme::post_constructor(pdesc);

        if(star_ref == nullptr)
            throw SRC_BUG;

        if(star_ref->get_ref_count() == 1) // first time this inode is seen
            star_ref->get_inode()->post_constructor(pdesc);
    }

    bool cat_mirage::operator == (const cat_entree & ref) const
    {
	const cat_mirage *ref_mirage = dynamic_cast<const cat_mirage *>(&ref);

	if(ref_mirage == nullptr)
	    return false;
	else
	{
	    cat_inode *me = get_inode();
	    cat_inode *you = ref_mirage->get_inode();

	    if(me == nullptr || you == nullptr)
		throw SRC_BUG;
	    me->change_name(get_name());
	    you->change_name(get_name());

	    return *me == *you
		&& cat_nomme::operator == (ref);
	}
    }


    void cat_mirage::inherited_dump(const pile_descriptor & pdesc, bool small) const
    {
	generic_file *ptr = nullptr;

	pdesc.check(small);
	if(small)
	    ptr = pdesc.esc;
	else
	    ptr = pdesc.stack;

        if(star_ref->get_ref_count() > 1 || star_ref->cannot_reduce_to_normal_inode())
        {
            char buffer[] = { MIRAGE_ALONE, MIRAGE_WITH_INODE };
            cat_nomme::inherited_dump(pdesc, small);
            star_ref->get_etiquette().dump(*ptr);
            if((small && !is_inode_wrote())        // inside the archive in sequential mode
               || (!small && !is_inode_dumped()))  // in the catalogue at the end of archive
            {
                ptr->write(buffer+1, 1); // writing one char MIRAGE_WITH_INODE
                star_ref->get_inode()->specific_dump(pdesc, small);
                if(!small)
                    set_inode_dumped(true);
            }
            else
                ptr->write(buffer, 1); // writing one char MIRAGE_ALONE
        }
        else // no need to record this inode with the hard link overhead
        {
            cat_inode *real = star_ref->get_inode();
            real->change_name(get_name()); // set the name of the cat_mirage object to the inode
            real->specific_dump(pdesc, small);
        }
    }

    void cat_mirage::dup_on(cat_etoile * ref)
    {
	star_ref = ref;
	star_ref->add_ref(this);
    }

} // end of namespace
