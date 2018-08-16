/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2018 Denis Corbin
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
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
} // end extern "C"

#include <iostream>

#include "libdar.hpp"
#include "testtools.hpp"
#include "catalogue.hpp"
#include "cat_all_entrees.hpp"
#include "user_interaction.hpp"
#include "integers.hpp"
#include "macro_tools.hpp"
#include "shell_interaction.hpp"
#include "deci.hpp"
#include "label.hpp"
#include "fichier_local.hpp"
#include "pile.hpp"
#include "tools.hpp"

using namespace libdar;
using namespace std;

#define FIC1 "test/dump.bin"
#define FIC2 "test/dump2.bin"

static shared_ptr<user_interaction> ui;

void f1();
void f2();
void f3();
void f4();

int main()
{
    U_I maj, med, min;

    get_version(maj, med, min);
    ui.reset(new (nothrow) shell_interaction(cout, cerr, false));
    if(!ui)
	cout << "ERREUR !" << endl;

    try
    {
	f1();
	f2();
	f3();
	f4();
    }
    catch(Egeneric & e)
    {
        throw SRC_BUG;
    }

    ui.reset();
    return 0;
}

void f1()
{

	//
	// test save/backup of each inode type
	//

    unlink(FIC1);
    unlink(FIC2);
    try
    {
        fichier_local *dump = new fichier_local(ui, FIC1, gf_read_write, 0644, false, false, false);
	compressor *comp = new compressor(compression::none, *dump, 1);
	pile stack;
	smart_pointer<pile_descriptor> pdesc(new pile_descriptor());
	std::map <infinint, cat_etoile *> corres;

	if(pdesc.is_null())
	    throw Ememory("f1");
	stack.push(dump);
	dump = nullptr;
	stack.push(comp);
	comp = nullptr;
	*pdesc = &stack;

        cat_eod *v_eod = new cat_eod();
        cat_file *v_file = new cat_file(1024, 102, 0644, datetime(1), datetime(2), datetime(3), "fichier", "." , 1024, 0, false);
        cat_lien *v_lien = new cat_lien(1025, 103, 0645, datetime(4), datetime(5), datetime(6), "lien", "fichier", 0);
        cat_directory *v_dir = new cat_directory(1026, 104, 0646, datetime(7), datetime(8), datetime(9), "repertoire", 0);
        cat_chardev *v_char = new cat_chardev(1027, 105, 0647, datetime(10), datetime(11), datetime(12),  "char device", 104, 202, 0);
        cat_blockdev *v_block = new cat_blockdev(1028, 106, 0651, datetime(13), datetime(14), datetime(15),  "block device", 105, 203, 0);
        cat_tube *v_tube = new cat_tube(1029, 107, 0652, datetime(16), datetime(17), datetime(18), "tuyau", 0);
        cat_prise *v_prise = new cat_prise(1030, 108, 0650, datetime(19), datetime(20), datetime(21),  "prise", 0);
        cat_detruit *v_detruit = new cat_detruit("ancien fichier", 'f', datetime(192));
        cat_directory *v_sub_dir = new cat_directory(200,20, 0777, datetime(100), datetime(101), datetime(102), "sous-repertoire", 0);
	cat_mirage *v_mir = new cat_mirage("Zorro mirage", new cat_etoile(dynamic_cast<cat_inode *>(v_prise->clone()), 10));

        cat_entree *liste[] = { v_eod, v_file, v_lien, v_dir, v_char, v_block, v_tube, v_prise, v_detruit, v_sub_dir, v_mir, nullptr };

        for(S_I i = 0; liste[i] != nullptr; ++i)
        {
            cat_inode *ino = dynamic_cast<cat_inode *>(liste[i]);

            if(ino != nullptr)
                ino->set_saved_status(saved_status::saved);
            liste[i]->dump(*pdesc, false);
        }

        stack.skip(0);
        entree_stats stats;
        stats.clear();
        cat_entree *ref = (cat_entree *)1; // != nullptr
        for(S_I i = 0; ref != nullptr; ++i)
        {
            ref = cat_entree::read(ui, pdesc, macro_tools_supported_version, stats, corres, compression::none, false, false, false);
            if(ref != nullptr)
                delete ref;
        }
	stats.listing(*ui);
	stack.clear();

        v_dir->add_children(v_file);
        v_dir->add_children(v_lien);
        v_sub_dir->add_children(v_char);
        v_dir->add_children(v_sub_dir);
        v_sub_dir->add_children(v_block);
        v_dir->add_children(v_tube);
        v_dir->add_children(v_detruit);
        v_dir->add_children(v_prise);

        unlink(FIC1);
        unlink(FIC2);

        stats.clear();
        dump = new fichier_local(ui, FIC1, gf_read_write, 0644, false, true, false);
	comp = new compressor(compression::none, *dump, 1);
	stack.push(dump);
	dump = nullptr;
	stack.push(comp);
	comp = nullptr;
	*pdesc = &stack;

        v_dir->dump(*pdesc, false);
        stack.skip(0);
        ref = cat_entree::read(ui, pdesc, macro_tools_supported_version, stats, corres, compression::none, false, false, false);
        v_sub_dir = dynamic_cast<cat_directory *>(ref);
        delete ref;
	stack.clear();
    }
    catch(Egeneric & e)
    {
        cerr << e.dump_str();
    }
}

void f2()
{
	//
        // test catalogue a proprement parler
	//

    try
    {
	label data_name;
        catalogue cat(ui, datetime(12), data_name);
        const cat_entree *ref;
	bool_mask tmp = true;
	label lax_label;

	lax_label.clear();
//        cat.listing(*ui, false, tmp, tmp, false, false, false, "");
// listing feature has been moved out of catalogue to archive class for release 2.6.0

        cat.reset_add();
        try
        {
            cat.add(new cat_eod());
        }
        catch(Egeneric & e)
        {
            cerr << e.dump_str();
        }
        cat.add(new cat_file(1024, 102, 0644, datetime(1), datetime(2), datetime(3), "fichier", ".", 1024, 0, false));
        cat.add(new cat_lien(1025, 103, 0645, datetime(4), datetime(5), datetime(6),  "lien", "fichier", 0));
        cat.add(new cat_directory(1026, 104, 0646, datetime(7), datetime(8), datetime(9), "repertoire", 0));
        cat.add(new cat_chardev(1027, 105, 0647, datetime(10), datetime(11), datetime(12),  "char device", 104, 202, 0));
        cat.add(new cat_blockdev(1028, 106, 0651, datetime(13), datetime(14), datetime(15), "block device", 105, 203, 0));
        cat.add(new cat_eod());
        cat.add(new cat_tube(1029, 107, 0652, datetime(16), datetime(17), datetime(18), "tuyau", 0));
        cat.add(new cat_prise(1030, 108, 0650, datetime(19), datetime(20), datetime(21),  "prise", 0));
        cat.add(new cat_detruit("ancien fichier", 'f', datetime(102)));

//        cat.listing(*ui, false, tmp, tmp, false, false, false, "");

        cat.reset_read();
        while(cat.read(ref))
        {
            const cat_eod *e = dynamic_cast<const cat_eod *>(ref);
            const cat_nomme *n = dynamic_cast<const cat_nomme *>(ref);
            const cat_directory *d = dynamic_cast<const cat_directory *>(ref);
            string type = "file";

            if(e != nullptr)
                cout << " EOF "<< endl;
            if(d != nullptr)
                type = "directory";
            if(n != nullptr)
                cout << type << " name = " << n->get_name() << endl;
        }

        unlink(FIC1);
        fichier_local *f = new fichier_local(ui, FIC1, gf_read_write, 0644, false, true, false);
	compressor *comp = new compressor(compression::none, f, 1);
	pile stack;
	pile_descriptor pdesc;

	stack.push(f);
	f = nullptr;
	stack.push(comp);
	comp = nullptr;
	pdesc = & stack;

        cat.dump(pdesc);
        stack.skip(0);
        catalogue lst(ui, pdesc, macro_tools_supported_version, compression::none, false, lax_label);
//        lst.listing(*ui, false, tmp, tmp, false, false, false, "");
        bool ok;

        lst.reset_read();
        cat.reset_compare();
        while(lst.read(ref))
        {
            const cat_eod *e = dynamic_cast<const cat_eod *>(ref);
            const cat_detruit *d = dynamic_cast<const cat_detruit *>(ref);
            const cat_inode *i = dynamic_cast<const cat_inode *>(ref);
            const cat_entree *was;

            if(e != nullptr)
                ok = cat.compare(e, was);
            else
                if(d != nullptr)
                    cout << "fichier detruit ["<< d->get_signature() << "] name = " << d->get_name() << endl;
                else
                    if(i != nullptr)
                    {
                        ok = cat.compare(i, was);
                        const cat_inode *w;
                        if(ok)
                            w = dynamic_cast<const cat_inode *>(was);
                        if(ok && w != nullptr)
			{
                            if(i->same_as(*w))
                                if(i->is_more_recent_than(*w, 0))
				{
                                    cout << "plus recent" << endl;
				    cout << "new is " << tools_display_date(w->get_last_modif()) << " ref " << tools_display_date(i->get_last_modif()) << endl;
				}
                                else
                                    cout << "pas plus recent" << endl;
                            else
                                cout << "pas meme ou pas inode" << endl;
			}
                    }
                    else
                        cout << "objet inconnu" << endl;
        }
    }
    catch(Egeneric &e)
    {
	cerr << e.dump_str();
    }
}

void f3()
{
    label data_name;
    catalogue cat(ui, datetime(180), data_name);
    catalogue dif(ui, datetime(190), data_name);
    bool_mask tmp = true;

    cat.reset_add();
    dif.reset_add();

    cat.add(new cat_file(1024, 102, 0644, datetime(1), datetime(2), datetime(3), "fichier", ".", 1024, 0, false));
    cat.add(new cat_lien(1025, 103, 0645, datetime(4), datetime(5), datetime(6), "lien", "fichier", 0));
    cat.add(new cat_directory(1026, 104, 0646, datetime(7), datetime(8), datetime(9), "repertoire", 0));
    cat.add(new cat_chardev(1027, 105, 0647, datetime(10), datetime(11), datetime(12), "char device", 104, 202, 0));
    cat.add(new cat_blockdev(1028, 106, 0651, datetime(13), datetime(14), datetime(15), "block device", 105, 203, 0));
    cat.add(new cat_eod());
    cat.add(new cat_tube(1029, 107, 0652, datetime(16), datetime(17), datetime(18), "tuyau", 0));
    cat.add(new cat_prise(1030, 108, 0650, datetime(19), datetime(20), datetime(21), "prise", 0));
    cat.add(new cat_detruit("ancien fichier", 'f', datetime(190)));


    dif.add(new cat_file(1024, 102, 0644, datetime(1), datetime(2), datetime(3), "fichier", ".",  1024, 0, false));
    dif.add(new cat_lien(1025, 103, 0645, datetime(4), datetime(5), datetime(6), "lien", "fichier", 0));
    dif.add(new cat_tube(1029, 107, 0652, datetime(16), datetime(17), datetime(18),  "tuyau", 0));
    dif.add(new cat_prise(1030, 108, 0650, datetime(19), datetime(20), datetime(21), "prise", 0));
    dif.add(new cat_detruit("ancien fichier", 'f', datetime(12)));

    dif.update_destroyed_with(cat);

//    cat.listing(*ui, false, tmp, tmp, false, false, false, "");
//    dif.listing(*ui, false, tmp, tmp, false, false, false, "");
}


void f4()
{

	//
	// hard link testing
	//


    cat_tube *v_tube = new cat_tube(1029, 107, 0652, datetime(16), datetime(17), datetime(18), "tuyau", 0);
    cat_etoile *deneb = new cat_etoile(v_tube, 100);

    cout << deneb->get_ref_count() << endl;
    cout << deneb->get_etiquette() << endl;
    cout << deneb->is_wrote() << endl;

    cat_mirage *abell = new cat_mirage("Zorro et Bernardo", deneb);

    cout << deneb->get_ref_count() << endl;
    cout << deneb->get_etiquette() << endl;
    cout << deneb->is_wrote() << endl;

    cout << abell->is_inode_wrote() << endl;
    cout << abell->get_etiquette() << endl;

    cat_entree *tmp = abell->clone();
    cat_mirage *trefle =  dynamic_cast<cat_mirage *>(tmp);

    cout << deneb->get_ref_count() << endl;
    cout << deneb->get_etiquette() << endl;
    cout << deneb->is_wrote() << endl;

    cout << trefle->is_inode_wrote() << endl;
    cout << trefle->get_etiquette() << endl;

    abell->set_inode_wrote(true);
    cout << trefle->is_inode_wrote() << endl;

    delete trefle;
    cout << deneb->get_ref_count() << endl;
    cout << deneb->get_etiquette() << endl;
    cout << deneb->is_wrote() << endl;

    cout << abell->is_inode_wrote() << endl;
    cout << abell->get_etiquette() << endl;

    delete abell;
}
