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
// $Id: test_catalogue.cpp,v 1.12 2003/03/02 10:58:47 edrusb Rel $
//
/*********************************************************************/

#include <iostream>
#include <unistd.h>
#include "testtools.hpp"
#include "catalogue.hpp"
#include "user_interaction.hpp"
#include "test_memory.hpp"
#include "integers.hpp"
#include "macro_tools.hpp"

#define FIC1 "test/dump.bin"
#define FIC2 "test/dump2.bin"

void f1();
void f2();
void f3();

S_I main()
{
    MEM_BEGIN;
    MEM_IN;
    user_interaction_init(&cout, &cerr);
    catalogue_set_reading_version("03");
    try
    {
	MEM_IN;
//	f1();
	MEM_OUT;
//	f2();
	MEM_OUT;
//	f3();
	MEM_OUT;
    }
    catch(Egeneric & e)
    {
	throw SRC_BUG;
    }

    user_interaction_close();
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
	fichier *dump = new fichier(FIC1, gf_read_write);
	fichier *dump2 = new fichier(FIC2, gf_write_only);
	
	eod *v_eod = new eod();
	file *v_file = new file(1024, 102, 0644, 1, 2, "fichier", "." , 1024);
	lien *v_lien = new lien(1025, 103, 0645, 4, 5, "lien", "fichier");
	directory *v_dir = new directory(1026, 104, 0646, 7, 8, "repertoire");
	chardev *v_char = new chardev(1027, 105, 0647, 10, 11, "char device", 104, 202);
	blockdev *v_block = new blockdev(1028, 106, 0651, 13, 14, "block device", 105, 203);
	tube *v_tube = new tube(1029, 107, 0652, 16, 17, "tuyau");
	prise *v_prise = new prise(1030, 108, 0650, 19, 20, "prise");
	detruit *v_detruit = new detruit("ancien fichier", 'f');
	directory *v_sub_dir = new directory(200,20, 0777, 100, 101, "sous-repertoire");
	
	entree *liste[] = { v_eod, v_file, v_lien, v_dir, v_char, v_block, v_tube, v_prise, v_detruit, v_sub_dir, NULL };
	
	for(S_I i = 0; liste[i] != NULL; i++)
	{
	    inode *ino = dynamic_cast<inode *>(liste[i]);

	    if(ino != NULL)
		ino->set_saved_status(s_saved);
	    liste[i]->dump(*dump);
	}

	dump->skip(0);
	entree *ref = (entree *)1; // != NULL
	for(S_I i = 0; ref != NULL; i++)
	{
	    ref = entree::read(*dump);
	    if(ref != NULL)
	    {
		ref->dump(*dump2);
		delete ref;
	    }
	}
	delete dump;
	delete dump2;
	delete v_eod;

	v_dir->add_children(v_file);
	v_dir->add_children(v_lien);
	v_sub_dir->add_children(v_char);
	v_dir->add_children(v_sub_dir);
	v_sub_dir->add_children(v_block);
	v_dir->add_children(v_tube);
	v_dir->add_children(v_detruit);
	v_dir->add_children(v_prise);
	
	v_dir->listing(cout);
	
	unlink(FIC1);
	unlink(FIC2);

	dump = new fichier(FIC1, gf_read_write);
	v_dir->dump(*dump);
	dump->skip(0);
	ref = entree::read(*dump);
	v_sub_dir = dynamic_cast<directory *>(ref);
	v_sub_dir->listing(cout);

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
	catalogue cat;
	const entree *ref;

	cat.listing(cout);

	cat.reset_add();
	try
	{
	    cat.add(new eod());
	}
	catch(Egeneric & e)
	{
	    e.dump();
	}
	cat.add(new file(1024, 102, 0644, 1, 2, "fichier", ".", 1024));
	cat.add(new lien(1025, 103, 0645, 4, 5, "lien", "fichier"));
	cat.add(new directory(1026, 104, 0646, 7, 8, "repertoire"));
	cat.add(new chardev(1027, 105, 0647, 10, 11,  "char device", 104, 202));
	cat.add(new blockdev(1028, 106, 0651, 13, 14, "block device", 105, 203));
	cat.add(new eod());
	cat.add(new tube(1029, 107, 0652, 16, 17, "tuyau"));
	cat.add(new prise(1030, 108, 0650, 19, 20, "prise"));
	cat.add(new detruit("ancien fichier", 'f'));
	
	cat.listing(cout);

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
	fichier f = fichier(FIC1, gf_read_write);

	cat.dump(f);
	f.skip(0);
	catalogue lst = f;
	lst.listing(cout);
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
				if(i->is_more_recent_than(*w))
				    cout << "plus recent" << endl;
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
    catalogue cat;
    catalogue dif;

    cat.reset_add();
    dif.reset_add();

    cat.add(new file(1024, 102, 0644, 1, 2, "fichier", ".", 1024));
    cat.add(new lien(1025, 103, 0645, 4, 5, "lien", "fichier"));
    cat.add(new directory(1026, 104, 0646, 7, 8, "repertoire"));
    cat.add(new chardev(1027, 105, 0647, 10, 11, "char device", 104, 202));
    cat.add(new blockdev(1028, 106, 0651, 13, 14, "block device", 105, 203));
    cat.add(new eod());
    cat.add(new tube(1029, 107, 0652, 16, 17, "tuyau"));
    cat.add(new prise(1030, 108, 0650, 19, 20, "prise"));
    cat.add(new detruit("ancien fichier", 'f'));


    dif.add(new file(1024, 102, 0644, 1, 2, "fichier", ".",  1024));
    dif.add(new lien(1025, 103, 0645, 4, 5, "lien", "fichier"));
    dif.add(new tube(1029, 107, 0652, 16, 17, "tuyau"));
    dif.add(new prise(1030, 108, 0650, 19, 20, "prise"));
    dif.add(new detruit("ancien fichier", 'f'));

    dif.update_destroyed_with(cat);
    
    cat.listing(cout);
    dif.listing(cout);
}
 
