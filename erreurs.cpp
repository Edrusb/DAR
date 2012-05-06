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
// $Id: erreurs.cpp,v 1.16 2002/05/17 16:17:48 denis Rel $
//
/*********************************************************************/

#pragma implementation

#include "erreurs.hpp"
#include <iostream.h>
#include <strstream>

static bool initialized = false;

static void init();
static void inattendue();
static void notcatched();
static string int_to_string(int i);

list<Egeneric *> Egeneric::destroyed;
list<Egeneric *> Egeneric::all_instances;

Egeneric::Egeneric(const string &source, const string &message)
{
    if(!initialized)
	init();
    pile.push_front(niveau(source, message));
    zombie = false;
    all_instances.push_back(this);
}

Egeneric::Egeneric(const Egeneric & ref)
{
    pile = ref.pile;
    zombie = ref.zombie;
    all_instances.push_back(this);
}

 void Egeneric::add_to_last_destroyed(Egeneric *obj)
{
    if(obj->zombie)
	throw SRC_BUG;
    else
    {
	destroyed.push_back(obj);
	obj->zombie = true;
	if(destroyed.size() > fifo_size)
	{
	    delete destroyed.front();
	    destroyed.pop_front();
	}
    }
}

void Egeneric::dump() const
{
    string s;
    list<niveau> & tmp = const_cast< list<niveau> & >(pile);
    list<niveau>::iterator it;

    it = tmp.begin();

    cerr << "---- exception nature = [" << (zombie ? "zombie" : "alive") << "]  exception type = ["  << exceptionID() << "] ----------" << endl;
    cerr << "[source]" << endl;
    while(it != tmp.end())
    {
	cerr << '\t' << it->lieu << " : " << it->objet << endl;
	it++;
    }
    cerr << "[most outside call]" << endl;
    cerr << "-----------------------------------" << endl << endl;
}

unsigned int Egeneric::alive()
{
    unsigned int ret = 0;
    list<Egeneric *>::iterator it = all_instances.begin();
    
    while(it != all_instances.end())
	if(! (*it++)->zombie)
	    ret++;

    return ret;
}

void Egeneric::clear_last_destroyed()
{
    list<Egeneric *>::iterator it = destroyed.begin(); 

    while(it != destroyed.end())
	delete (*it++);

    destroyed.clear();
}

void Egeneric::display_last_destroyed()
{
    list<Egeneric *>::iterator it = destroyed.begin(); 

    while(it != destroyed.end())
	(*it++)->dump();
}

void Egeneric::display_alive()
{
    list<Egeneric *>::iterator it = all_instances.begin();
    
    while(it != all_instances.end())
    {
	if(! (*it)->zombie)
	    (*it)->dump();
	it++;
    }
}

Ebug::Ebug(string file, int line) : Egeneric(string("file ") + file + " line " + int_to_string(line), "it seems to be a bug here") {};
void Ebug::stack(string passage, string file, string line) 
{
    Egeneric::stack(passage, string("in file ") + file + " line " + string(line));
}

static void init()
{
    set_unexpected(inattendue);
    set_terminate(notcatched);
    initialized = true;
}

static void dummy_call(char *x)
{
    static char id[]="$Id: erreurs.cpp,v 1.16 2002/05/17 16:17:48 denis Rel $";
    dummy_call(id);
}

static void status()
{
    cerr << endl <<" Exceptions : " << endl;
    cerr << "\t alive  = " << Egeneric::alive() << endl;
    cerr << "\t zombie = " << Egeneric::zombies() << endl;
    cerr << "\t --------------------" << endl;
    cerr << "\t total  = " << Egeneric::total() << endl <<endl;
}

static void inattendue()
{
    cerr << "###############################################" << endl;
    cerr << "#   UNEXPECTED EXCEPTION,                     #" << endl;
    cerr << "#                         E X I T I N G !     #" << endl;
    cerr << "#                                             #" << endl;
    cerr << "###############################################" << endl;
    status();
    cerr << "###############################################" << endl;
    cerr << "#                                             #" << endl;
    cerr << "#     LIST OF STILL ALIVE EXCEPTIONS :        #" << endl;
    cerr << "#                                             #" << endl;
    cerr << "###############################################" << endl;
    Egeneric::display_alive();
    cerr << "###############################################" << endl;
    cerr << "#                                             #" << endl;
    cerr << "#     LIST OF LAST DESTROYED EXCEPTIONS :     #" << endl;
    cerr << "#                                             #" << endl;
    cerr << "###############################################" << endl;
    Egeneric::display_last_destroyed();
}

static void notcatched()
{
    cerr << "###############################################" << endl;
    cerr << "#   NOT CAUGHT EXCEPTION,                     #" << endl;
    cerr << "#                         E X I T I N G !     #" << endl;
    cerr << "#                                             #" << endl;
    cerr << "###############################################" << endl;
    status();
    cerr << "###############################################" << endl;
    cerr << "#                                             #" << endl;
    cerr << "#     LIST OF STILL ALIVE EXCEPTIONS :        #" << endl;
    cerr << "#                                             #" << endl;
    cerr << "###############################################" << endl;
    Egeneric::display_alive();
    cerr << "###############################################" << endl;
    cerr << "#                                             #" << endl;
    cerr << "#     LIST OF LAST DESTROYED EXCEPTIONS :     #" << endl;
    cerr << "#                                             #" << endl;
    cerr << "###############################################" << endl;
    Egeneric::display_last_destroyed();
}

static string int_to_string(int i)
{
    ostrstream r;
    string s;
    char *t;
    r << i << '\0';
    t = r.str();
    if(t != NULL)
    {
	s = t;
	delete t;
    }
    else
	s = "number is impossible to display";

    return s;
}
