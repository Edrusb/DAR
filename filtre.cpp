/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002 Denis Corbin
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
// to contact the author : dar.linux@free.fr
/*********************************************************************/
// $Id: filtre.cpp,v 1.25 2002/03/30 15:59:19 denis Rel $
//
/*********************************************************************/

#include "user_interaction.hpp"
#include "erreurs.hpp"
#include "filtre.hpp"
#include "filesystem.hpp"
#include "defile.hpp"
#include "test_memory.hpp"

static void save_inode(const string &info_quoi, inode * & ino, compressor *stock, bool info_details);

void filtre_restore(const mask &filtre, 
		    const mask & subtree,
		    compressor *stockage, 
		    catalogue & cat, 
		    bool detruire, 
		    const path & fs_racine,
		    bool fs_allow_overwrite,
		    bool fs_warn_overwrite,
		    bool info_details,
		    statistics & st)
{
    defile juillet = fs_racine;
    const eod tmp_eod;
    const entree *e;
    st.clear();

    filesystem_set_root(fs_racine, fs_allow_overwrite, fs_warn_overwrite, info_details);
    cat.reset_read();
    filesystem_reset_write();
    
    while(cat.read(e))
    {
	const nomme *e_nom = dynamic_cast<const nomme *>(e);
	const directory *e_dir = dynamic_cast<const directory *>(e);

	juillet.enfile(e);
	if(e_nom != NULL)
	{
	    try
	    {
		if(subtree.is_covered(juillet.get_string()) && (e_dir != NULL || filtre.is_covered(e_nom->get_name())))
		{
		    const detruit *e_det = dynamic_cast<const detruit *>(e);
		    const inode *e_ino = dynamic_cast<const inode *>(e);
		    
		    if(e_det != NULL)
		    {
			if(detruire)
			{
			    if(info_details)
				user_interaction_warning(string("removing file ") + juillet.get_string());
			    filesystem_write(e);
			    st.deleted++;
			}
		    }
		    else
			if(e_ino != NULL)
			{
			    if(e_ino->get_saved_status())
			    {
				if(info_details)
				    user_interaction_warning(string("restoring file ") + juillet.get_string());
				filesystem_write(e);
				st.treated++;
			    }
			    else
			    {
				const directory *dir = dynamic_cast<const directory *>(e_ino);
				if(dir != NULL)
				    filesystem_pseudo_write(dir);
				st.skipped++;
			    }
			}
			else
			    throw SRC_BUG; // a nomme is neither a detruit nor an inode !
		}
		else // inode not covered 
		{
		    st.ignored++;
		    if(e_dir != NULL)
		    {
			cat.skip_read_to_parent_dir();
			juillet.enfile(&tmp_eod);
		    }
		}
	    }
	    catch(Ebug & e)
	    {
		throw;
	    }
	    catch(Euser_abort & e)
	    {
		user_interaction_warning(juillet.get_string() + " not restored (user choice)");
		const directory *e_dir = dynamic_cast<const directory *>(e_nom);
		if(e_dir != NULL)
		{
		    cat.skip_read_to_parent_dir();
		    user_interaction_warning("any file in this directory will not be restored");
		}
	    }
	    catch(Egeneric & e)
	    {
		user_interaction_warning(string("error while restoring ") + juillet.get_string() + " : " + e.get_message());
		st.errored++;

		if(e_dir != NULL)
		{
		    cat.skip_read_to_parent_dir();
		    juillet.enfile(&tmp_eod);
		    user_interaction_warning("NO FILE IN THAT DIRECTORY WILL BE RESTORED");
		}
	    }
	}
	else
	    filesystem_write(e); // eod;
    }
    filesystem_freemem();
}

void filtre_sauvegarde(const mask &filtre,
		       const mask &subtree,
		       compressor *stockage, 
		       catalogue & cat,
		       catalogue &ref,
		       const path & fs_racine,
		       bool info_details,
		       statistics & st)
{
    entree *e, *f;
    defile juillet = fs_racine;
    const eod tmp_eod;
    st.clear();

    filesystem_set_root(fs_racine, false, false, info_details);
    cat.reset_add();
    ref.reset_compare();
    filesystem_reset_read();

    while(filesystem_read(e))
    {
	nomme *nom = dynamic_cast<nomme *>(e);
	directory *dir = dynamic_cast<directory *>(e);
	
	juillet.enfile(e);
	if(nom != NULL)
	{
	    try
	    {
		if(subtree.is_covered(juillet.get_string()) && (dir != NULL || filtre.is_covered(nom->get_name())))
		{
		    inode *e_ino = dynamic_cast<inode *>(e);
		    bool known = ref.compare(e, f);

		    try
		    {
			if(known)
			{
			    const inode *f_ino = dynamic_cast<const inode *>(f);
			    
			    if(e_ino == NULL || f_ino == NULL)
				throw SRC_BUG; // filesystem has provided a "nomme" which is not a "inode" thus which is a "detruit"
			    
			    if(e_ino->is_more_recent_than(*f_ino))
			    {
				if(!e_ino->get_saved_status())
				    throw SRC_BUG; // filsystem should always provide "saved" "entree"
				
				save_inode(juillet.get_string(), e_ino, stockage, info_details);
				st.treated++;
			    }
			    else // inode has not changed since last backup
			    {
				e_ino->set_saved_status(false);
				st.skipped++;
			    }
			}
			else // inode not present int catalogue of reference
			    if(e_ino != NULL)
			    {
				save_inode(juillet.get_string(), e_ino, stockage, info_details); 
				st.treated++;
			    }
			    else
				throw SRC_BUG;  // filesystem has provided a "nomme" which is not a "inode" thus which is a "detruit"

			cat.add(e);
		    }
		    catch(...)
		    {
			if(dir != NULL)
			    ref.compare(&tmp_eod, f);
			throw;
		    }
		}
		else // inode not covered
		{
		    nomme *ig = NULL;
		    inode *ignode = NULL;

		    if(dir != NULL)
			ig = ignode = new ignored_dir(*dir);
		    else
			ig = new ignored(nom->get_name());
		    st.ignored++;
		    
		    if(ig == NULL)
			throw Ememory("sauvegarde_all");
		    else
			cat.add(ig);

		    if(dir != NULL)
		    {
			inode *e_ino = dynamic_cast<inode *>(e);
			bool known = ref.compare(e, f);
			const inode *f_ino = known ? dynamic_cast<const inode *>(f) : NULL;
			bool tosave = known ? e_ino->is_more_recent_than(*f_ino) : true;

			ignode->set_saved_status(tosave);
			filesystem_skip_read_to_parent_dir();
			juillet.enfile(&tmp_eod);
		    }
		    delete e;
		}
	    }
	    catch(Ebug & e)
	    {
		throw;
	    }
	    catch(Euser_abort & e)
	    {
		throw;
	    }
	    catch(Egeneric & e)
	    {
		nomme *tmp = NULL;
		inode *itmp = NULL;
		user_interaction_warning(string("error while saving ") + juillet.get_string() + " : " + e.get_message());
		st.errored++;

		if(dir != NULL)
		    tmp = itmp = new ignored_dir(*dir);
		else
		    tmp = new ignored(nom->get_name());
		if(tmp == NULL)
		    throw Ememory("fitre_sauvegarde");
		cat.add(tmp);
		if(dir != NULL)
		{
		    filesystem_skip_read_to_parent_dir();
		    juillet.enfile(&tmp_eod);
		    user_interaction_warning("NO FILE IN THAT DIRECTORY CAN BE SAVED");
		    itmp->set_saved_status(false);		
		}
	    }    
	}
	else // eod
	{
	    ref.compare(e, f);
	    cat.add(e);
	}
    }
    filesystem_freemem();
}

void filtre_difference(const mask &filtre,
		       const mask &subtree,
		       compressor *stockage,
		       const catalogue & cat,
		       const path & fs_racine,
		       bool info_details,
		       statistics & st)
{
    throw Efeature("comparison is not yet implemented");
}

static void dummy_call(char *x)
{
    static char id[]="$Id: filtre.cpp,v 1.25 2002/03/30 15:59:19 denis Rel $";
    dummy_call(id);
}

static void save_inode(const string & info_quoi, inode * & ino, compressor *stock, bool info_details)
{
    if(ino == NULL || stock == NULL)
	throw SRC_BUG;
    if(!ino->get_saved_status())
	return;
    if(info_details)
	user_interaction_warning(string("adding file to archive : ") + info_quoi);

    file *fic = dynamic_cast<file *>(ino);

    if(fic != NULL)
    {
	generic_file *source = fic->get_data();
	
	fic->set_offset(stock->get_position());
	if(source != NULL)
	{
	    try
	    {
		source->copy_to(*stock);
		stock->flush_write();
	    }
	    catch(...)
	    {
		delete source;
		throw;
	    }
	    delete source;
	}
	else
	    throw SRC_BUG; // saved_status = true, but no data available, and no exception raised;
    }
}
