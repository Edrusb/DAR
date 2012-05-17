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
// to contact the author : dar.linux@free.fr
/*********************************************************************/
// $Id: test_catalogue.cpp,v 1.15 2005/11/17 15:24:11 edrusb Rel $
//
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
#include "user_interaction.hpp"
#include "test_memory.hpp"
#include "integers.hpp"
#include "macro_tools.hpp"
#include "shell_interaction.hpp"
#include "deci.hpp"

using namespace libdar;

#define FIC1 "test/dump.bin"
#define FIC2 "test/dump2.bin"

static user_interaction *ui = NULL;

void f1();
void f2();
void f3();

int main()
{
    MEM_BEGIN;
    MEM_IN;
    U_I maj, med, min;

    get_version(maj, med, min);
    ui = shell_interaction_init(&cout, &cerr, false);
    if(ui == NULL)
	cout << "ERREUR !" << endl;

    try
    {
        MEM_IN;
	f1();
        MEM_OUT;
	f2();
        MEM_OUT;
	f3();
        MEM_OUT;
    }
    catch(Egeneric & e)
    {
        throw SRC_BUG;
    }

    shell_interaction_close();
    if(ui != NULL)
	delete ui;
    MEM_OUT;
    MEM_END;
    return 0;
}

void f1()
{
    unlink(FIC1);
    unlink(FIC2);
    try
    {
        fichier *dump = new fichier(*ui, FIC1, gf_read_write);
        fichier *dump2 = new fichier(*ui, FIC2, gf_write_only);
	std::map <infinint, file_etiquette *> corres;

        eod *v_eod = new eod();
        file *v_file = new file(1024, 102, 0644, 1, 2, "fichier", "." , 1024, 0);
        lien *v_lien = new lien(1025, 103, 0645, 4, 5, "lien", "fichier", 0);
        directory *v_dir = new directory(1026, 104, 0646, 7, 8, "repertoire", 0);
        chardev *v_char = new chardev(1027, 105, 0647, 10, 11, "char device", 104, 202, 0);
        blockdev *v_block = new blockdev(1028, 106, 0651, 13, 14, "block device", 105, 203, 0);
        tube *v_tube = new tube(1029, 107, 0652, 16, 17, "tuyau", 0);
        prise *v_prise = new prise(1030, 108, 0650, 19, 20, "prise", 0);
        detruit *v_detruit = new detruit("ancien fichier", 'f');
        directory *v_sub_dir = new directory(200,20, 0777, 100, 101, "sous-repertoire", 0);

        entree *liste[] = { v_eod, v_file, v_lien, v_dir, v_char, v_block, v_tube, v_prise, v_detruit, v_sub_dir, NULL };

        for(S_I i = 0; liste[i] != NULL; i++)
        {
            inode *ino = dynamic_cast<inode *>(liste[i]);

            if(ino != NULL)
                ino->set_saved_status(s_saved);
            liste[i]->dump(*ui, *dump);
        }

        dump->skip(0);
        entree_stats stats;
        stats.clear();
        entree *ref = (entree *)1; // != NULL
        for(S_I i = 0; ref != NULL; i++)
        {
            ref = entree::read(*ui, *dump, "03", stats, corres, none, dump, dump);
            if(ref != NULL)
            {
                ref->dump(*ui, *dump2);
                delete ref;
            }
        }
        delete dump;
        delete dump2;
        delete v_eod;
        stats.listing(*ui);

        v_dir->add_children(v_file);
        v_dir->add_children(v_lien);
        v_sub_dir->add_children(v_char);
        v_dir->add_children(v_sub_dir);
        v_sub_dir->add_children(v_block);
        v_dir->add_children(v_tube);
        v_dir->add_children(v_detruit);
        v_dir->add_children(v_prise);

        v_dir->listing(*ui);

        unlink(FIC1);
        unlink(FIC2);

        stats.clear();
        dump = new fichier(*ui, FIC1, gf_read_write);
        v_dir->dump(*ui, *dump);
        dump->skip(0);
        ref = entree::read(*ui, *dump, "03", stats, corres, none, dump, dump);
        v_sub_dir = dynamic_cast<directory *>(ref);
        v_sub_dir->listing(*ui);

        delete ref;
        delete dump;
        delete v_dir;
    }
    catch(Egeneric & e)
    {
        e.dump();
    }
}

void f2()
{
        /// test catalogue a proprement parler

    try
    {
        catalogue cat = *ui;
        const entree *ref;

        cat.listing();

        cat.reset_add();
        try
        {
            cat.add(new eod());
        }
        catch(Egeneric & e)
        {
            e.dump();
        }
        cat.add(new file(1024, 102, 0644, 1, 2, "fichier", ".", 1024, 0));
        cat.add(new lien(1025, 103, 0645, 4, 5, "lien", "fichier", 0));
        cat.add(new directory(1026, 104, 0646, 7, 8, "repertoire", 0));
        cat.add(new chardev(1027, 105, 0647, 10, 11,  "char device", 104, 202, 0));
        cat.add(new blockdev(1028, 106, 0651, 13, 14, "block device", 105, 203, 0));
        cat.add(new eod());
        cat.add(new tube(1029, 107, 0652, 16, 17, "tuyau", 0));
        cat.add(new prise(1030, 108, 0650, 19, 20, "prise", 0));
        cat.add(new detruit("ancien fichier", 'f'));

        cat.listing();

        cat.reset_read();
        while(cat.read(ref))
        {
            const eod *e = dynamic_cast<const eod *>(ref);
            const nomme *n = dynamic_cast<const nomme *>(ref);
            const directory *d = dynamic_cast<const directory *>(ref);
            string type = "file";

            if(e != NULL)
                cout << " EOF "<< endl;
            if(d != NULL)
                type = "directory";
            if(n != NULL)
                cout << type << " name = " << n->get_name() << endl;
        }

        unlink(FIC1);
        fichier f = fichier(*ui, FIC1, gf_read_write);

        cat.dump(f);
        f.skip(0);
        catalogue lst = catalogue(*ui, f, "03", none, &f, &f);
        lst.listing();
        bool ok;

        lst.reset_read();
        cat.reset_compare();
        while(lst.read(ref))
        {
            const eod *e = dynamic_cast<const eod *>(ref);
            const detruit *d = dynamic_cast<const detruit *>(ref);
            const inode *i = dynamic_cast<const inode *>(ref);
            const entree *was;

            if(e != NULL)
                ok = cat.compare(e, was);
            else
                if(d != NULL)
                    cout << "fichier detruit ["<< d->get_signature() << "] name = " << d->get_name() << endl;
                else
                    if(i != NULL)
                    {
                        ok = cat.compare(i, was);
                        const inode *w;
                        if(ok)
                            w = dynamic_cast<const inode *>(was);
                        if(ok && w != NULL)
                            if(i->same_as(*w))
                                if(i->is_more_recent_than(*w, 0, inode::cf_ignore_owner))
				{
                                    cout << "plus recent" << endl;
				    cout << "new is " << deci(w->get_last_modif()).human() << " ref " << deci(i->get_last_modif()).human() << endl;
				}
                                else
                                    cout << "pas plus recent" << endl;
                            else
                                cout << "pas meme ou pas inode" << endl;
                    }
                    else
                        cout << "objet inconnu" << endl;
        }
    }
    catch(Egeneric &e)
    {
        e.dump();
    }
}

void f3()
{
    catalogue cat = *ui;
    catalogue dif = *ui;

    cat.reset_add();
    dif.reset_add();

    cat.add(new file(1024, 102, 0644, 1, 2, "fichier", ".", 1024, 0));
    cat.add(new lien(1025, 103, 0645, 4, 5, "lien", "fichier", 0));
    cat.add(new directory(1026, 104, 0646, 7, 8, "repertoire", 0));
    cat.add(new chardev(1027, 105, 0647, 10, 11, "char device", 104, 202, 0));
    cat.add(new blockdev(1028, 106, 0651, 13, 14, "block device", 105, 203, 0));
    cat.add(new eod());
    cat.add(new tube(1029, 107, 0652, 16, 17, "tuyau", 0));
    cat.add(new prise(1030, 108, 0650, 19, 20, "prise", 0));
    cat.add(new detruit("ancien fichier", 'f'));


    dif.add(new file(1024, 102, 0644, 1, 2, "fichier", ".",  1024, 0));
    dif.add(new lien(1025, 103, 0645, 4, 5, "lien", "fichier", 0));
    dif.add(new tube(1029, 107, 0652, 16, 17, "tuyau", 0));
    dif.add(new prise(1030, 108, 0650, 19, 20, "prise", 0));
    dif.add(new detruit("ancien fichier", 'f'));

    dif.update_destroyed_with(cat);

    cat.listing();
    dif.listing();
}
