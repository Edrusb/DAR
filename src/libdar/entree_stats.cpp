/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2021 Denis Corbin
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

#include "entree_stats.hpp"
#include "cat_all_entrees.hpp"

using namespace std;


namespace libdar
{

    void entree_stats::add(const cat_entree *ref)
    {
        if(dynamic_cast<const cat_eod *>(ref) == nullptr // we ignore cat_eod
           && dynamic_cast<const cat_ignored *>(ref) == nullptr // as well we ignore "cat_ignored"
           && dynamic_cast<const cat_ignored_dir *>(ref) == nullptr) // and "cat_ignored_dir"
        {
            const cat_inode *ino = dynamic_cast<const cat_inode *>(ref);
            const cat_mirage *h = dynamic_cast<const cat_mirage *>(ref);
            const cat_detruit *x = dynamic_cast<const cat_detruit *>(ref);


            if(h != nullptr) // won't count twice the same inode if it is referenced with hard_link
            {
                ++num_hard_link_entries;
                if(!h->is_inode_counted())
                {
                    ++num_hard_linked_inodes;
                    h->set_inode_counted(true);
                    ino = h->get_inode();
                }
            }

            if(ino != nullptr)
            {
                ++total;
		switch(ino->get_saved_status())
		{
		case saved_status::saved:
		    ++saved;
		    break;
		case saved_status::inode_only:
		    ++inode_only;
		    break;
		case saved_status::fake:
		case saved_status::not_saved:
		    break;
		case saved_status::delta:
		    ++patched;
		    break;
		default:
		    throw SRC_BUG;
		}
            }

            if(x != nullptr)
                ++num_x;
            else
            {
                const cat_directory *d = dynamic_cast<const cat_directory*>(ino);
                if(d != nullptr)
                    ++num_d;
                else
                {
                    const cat_chardev *c = dynamic_cast<const cat_chardev *>(ino);
                    if(c != nullptr)
                        ++num_c;
                    else
                    {
                        const cat_blockdev *b = dynamic_cast<const cat_blockdev *>(ino);
                        if(b != nullptr)
                            ++num_b;
                        else
                        {
                            const cat_tube *p = dynamic_cast<const cat_tube *>(ino);
                            if(p != nullptr)
                                ++num_p;
                            else
                            {
                                const cat_prise *s = dynamic_cast<const cat_prise *>(ino);
                                if(s != nullptr)
                                    ++num_s;
                                else
                                {
                                    const cat_lien *l = dynamic_cast<const cat_lien *>(ino);
                                    if(l != nullptr)
                                        ++num_l;
                                    else
                                    {
					const cat_door *D = dynamic_cast<const cat_door *>(ino);
					if(D != nullptr)
					    ++num_D;
					else
					{
					    const cat_file *f = dynamic_cast<const cat_file *>(ino);
					    if(f != nullptr)
						++num_f;
					    else
						if(h == nullptr)
						    throw SRC_BUG; // unknown entry
					}
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    void entree_stats::listing(user_interaction & dialog) const
    {
	dialog.printf("");
        dialog.printf(gettext("CATALOGUE CONTENTS :"));
	dialog.printf("");
        dialog.printf(gettext("total number of inode : %i"), &total);
        dialog.printf(gettext("fully saved           : %i"), &saved);
	dialog.printf(gettext("binay delta patch     : %i"), &patched);
	dialog.printf(gettext("inode metadata only   : %i"), &inode_only);
        dialog.printf(gettext("distribution of inode(s)"));
        dialog.printf(gettext(" - directories        : %i"), &num_d);
        dialog.printf(gettext(" - plain files        : %i"), &num_f);
        dialog.printf(gettext(" - symbolic links     : %i"), &num_l);
        dialog.printf(gettext(" - named pipes        : %i"), &num_p);
        dialog.printf(gettext(" - unix sockets       : %i"), &num_s);
        dialog.printf(gettext(" - character devices  : %i"), &num_c);
        dialog.printf(gettext(" - block devices      : %i"), &num_b);
	dialog.printf(gettext(" - Door entries       : %i"), &num_D);
        dialog.printf(gettext("hard links information"));
        dialog.printf(gettext(" - number of inode with hard link           : %i"), &num_hard_linked_inodes);
        dialog.printf(gettext(" - number of reference to hard linked inodes: %i"), &num_hard_link_entries);
        dialog.printf(gettext("destroyed entries information"));
        dialog.printf(gettext("   %i file(s) have been record as destroyed since backup of reference"), &num_x);
	dialog.printf("");
    }


} // end of namespace
