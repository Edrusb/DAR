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

    mirage::mirage(user_interaction & dialog,
                   generic_file & f,
                   const archive_version & reading_ver,
                   saved_status saved,
                   entree_stats & stats,
                   std::map <infinint, etoile *> & corres,
                   compression default_algo,
                   generic_file *data_loc,
                   compressor *efsa_loc,
                   mirage_format fmt,
                   bool lax,
                   escape *ptr) : nomme(f)
    {
        init(dialog,
             f,
             reading_ver,
             saved,
             stats,
             corres,
             default_algo,
             data_loc,
             efsa_loc,
             fmt,
             lax,
             ptr);
    }

    mirage::mirage(user_interaction & dialog,
                   generic_file & f,
                   const archive_version & reading_ver,
                   saved_status saved,
                   entree_stats & stats,
                   std::map <infinint, etoile *> & corres,
                   compression default_algo,
                   generic_file *data_loc,
                   compressor *efsa_loc,
                   bool lax,
                   escape *ptr) : nomme("TEMP")
    {
        init(dialog,
             f,
             reading_ver,
             saved,
             stats,
             corres,
             default_algo,
             data_loc,
             efsa_loc,
             fmt_file_etiquette,
             lax,
             ptr);
    }

    void mirage::init(user_interaction & dialog,
                      generic_file & f,
                      const archive_version & reading_ver,
                      saved_status saved,
                      entree_stats & stats,
                      std::map <infinint, etoile *> & corres,
                      compression default_algo,
                      generic_file *data_loc,
                      compressor *efsa_loc,
                      mirage_format fmt,
                      bool lax,
                      escape *ptr)
    {
        infinint tmp_tiquette;
        char tmp_flag;
        map<infinint, etoile *>::iterator etl;
        inode *ino_ptr = NULL;
        entree *entree_ptr = NULL;
        entree_stats fake_stats; // the call to entree_read will increment counters with the inode we will read
            // but this inode will also be counted from the entree_read we are call from.
            // thus we must not increment the real entree_stats structure here


        if(fmt != fmt_file_etiquette)
            tmp_tiquette = infinint(f);

        switch(fmt)
        {
        case fmt_mirage:
            f.read(&tmp_flag, 1);
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

                // we must link with the already existing etoile

            etl = corres.find(tmp_tiquette);
            if(etl == corres.end())
                throw Erange("mirage::mirage", gettext("Incoherent catalogue structure: hard linked inode's data not found"));
            else
            {
                if(etl->second == NULL)
                    throw SRC_BUG;
                star_ref = etl->second;
                star_ref->add_ref(this);
            }
            break;
        case MIRAGE_WITH_INODE:

                // we first read the attached inode

            if(fmt == fmt_file_etiquette)
            {
                nomme *tmp_ptr = new (get_pool()) file(dialog, f, reading_ver, saved, default_algo, data_loc, efsa_loc, ptr);
                entree_ptr = tmp_ptr;
                if(tmp_ptr != NULL)
                {
                    change_name(tmp_ptr->get_name());
                    tmp_ptr->change_name("");
                    tmp_tiquette = infinint(f);
                }
                else
                    throw Ememory("mirage::init");
            }
            else
                entree_ptr = entree::read(dialog, get_pool(), f, reading_ver, fake_stats, corres, default_algo, data_loc, efsa_loc, lax, false, ptr);

            ino_ptr = dynamic_cast<inode *>(entree_ptr);
            if(ino_ptr == NULL || dynamic_cast<directory *>(entree_ptr) != NULL)
            {
                if(entree_ptr != NULL)
                {
                    delete entree_ptr;
                    entree_ptr = NULL;
                }
                throw Erange("mirage::mirage", gettext("Incoherent catalogue structure: hard linked data is not an inode"));
            }

                // then we can bind the inode to the next to be create etoile object

            try
            {
                    // we must check that an already exiting etoile is not present

                etl = corres.find(tmp_tiquette);
                if(etl == corres.end())
                {
                        // we can now create the etoile and add it in the corres map;

                    star_ref = new (get_pool()) etoile(ino_ptr, tmp_tiquette);
                    try
                    {
                        if(star_ref == NULL)
                            throw Ememory("mirage::mirage");
                        ino_ptr = NULL; // the object pointed to by ino_ptr is now managed by star_ref
                        star_ref->add_ref(this);
                        corres[tmp_tiquette] = star_ref;
                    }
                    catch(...)
                    {
                        if(star_ref != NULL)
                        {
                            delete star_ref;
                            star_ref = NULL;
                        }
                        etl = corres.find(tmp_tiquette);
                        if(etl != corres.end())
                            corres.erase(etl);
                        throw;
                    }
                }
                else
                    throw Erange("mirage::mirage", gettext("Incoherent catalogue structure: duplicated hard linked inode's data"));
            }
            catch(...)
            {
                if(ino_ptr != NULL)
                {
                    delete ino_ptr;
                    ino_ptr = NULL;
                }
                throw;
            }

            break;
        default:
            throw Erange("mirage::mirage", gettext("Incoherent catalogue structure: unknown status flag for hard linked inode"));
        }
    }

    const mirage & mirage::operator = (const mirage & ref)
    {
        etoile *tmp_ref;
        const nomme * ref_nom = & ref;
        nomme * this_nom = this;
        *this_nom = *ref_nom; // copying the nomme part of these objects

        if(ref.star_ref == NULL)
            throw SRC_BUG;
        tmp_ref = star_ref;
        star_ref = ref.star_ref;
        star_ref->add_ref(this);
        tmp_ref->drop_ref(this);

        return *this;
    }

    void mirage::post_constructor(generic_file & f)
    {
        if(star_ref == NULL)
            throw SRC_BUG;

        if(star_ref->get_ref_count() == 1) // first time this inode is seen
            star_ref->get_inode()->post_constructor(f);
    }

    void mirage::inherited_dump(generic_file & f, bool small) const
    {
        if(star_ref->get_ref_count() > 1)
        {
            char buffer[] = { MIRAGE_ALONE, MIRAGE_WITH_INODE };
            nomme::inherited_dump(f, small);
            star_ref->get_etiquette().dump(f);
            if((small && !is_inode_wrote())
               || (!small && !is_inode_dumped()))
            {
                f.write(buffer+1, 1); // writing one char MIRAGE_WITH_INODE
                star_ref->get_inode()->specific_dump(f, small);
                if(!small)
                    set_inode_dumped(true);
            }
            else
                f.write(buffer, 1); // writing one char MIRAGE_ALONE
        }
        else // no need to record this inode with the hard link overhead
        {
            inode *real = star_ref->get_inode();
            real->change_name(get_name()); // set the name of the mirage object to the inode
            real->specific_dump(f, small);
        }
    }


} // end of namespace

