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
// $Id: erreurs.cpp,v 1.14 2002/03/25 22:02:38 denis Rel $
//
/*********************************************************************/

#include "erreurs.hpp"
#include <iostream>
#include <sstream>

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

    cout << "---- exception nature = [" << (zombie ? "zombie" : "alive") << "]  exception type = ["  << exceptionID() << "] ----------" << endl;
    cout << "[source]" << endl;
    while(it != tmp.end())
    {
	cout << '\t' << it->lieu << " : " << it->objet << endl;
	it++;
    }
    cout << "[most outside call]" << endl;
    cout << "-----------------------------------" << endl << endl;
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
    static char id[]="$Id: erreurs.cpp,v 1.14 2002/03/25 22:02:38 denis Rel $";
    dummy_call(id);
}

static void status()
{
    cout << endl <<" Exceptions : " << endl;
    cout << "\t alive  = " << Egeneric::alive() << endl;
    cout << "\t zombie = " << Egeneric::zombies() << endl;
    cout << "\t --------------------" << endl;
    cout << "\t total  = " << Egeneric::total() << endl <<endl;
}

static void inattendue()
{
    cout << "###############################################" << endl;
    cout << "#   UNEXPECTED EXCEPTION,                     #" << endl;
    cout << "#                         E X I T I N G !     #" << endl;
    cout << "#                                             #" << endl;
    cout << "###############################################" << endl;
    status();
    cout << "###############################################" << endl;
    cout << "#                                             #" << endl;
    cout << "#     LIST OF STILL ALIVE EXCEPTIONS :        #" << endl;
    cout << "#                                             #" << endl;
    cout << "###############################################" << endl;
    Egeneric::display_alive();
    cout << "###############################################" << endl;
    cout << "#                                             #" << endl;
    cout << "#     LIST OF LAST DESTROYED EXCEPTIONS :     #" << endl;
    cout << "#                                             #" << endl;
    cout << "###############################################" << endl;
    Egeneric::display_last_destroyed();
}

static void notcatched()
{
    cout << "###############################################" << endl;
    cout << "#   NOT CAUGHT EXCEPTION,                     #" << endl;
    cout << "#                         E X I T I N G !     #" << endl;
    cout << "#                                             #" << endl;
    cout << "###############################################" << endl;
    status();
    cout << "###############################################" << endl;
    cout << "#                                             #" << endl;
    cout << "#     LIST OF STILL ALIVE EXCEPTIONS :        #" << endl;
    cout << "#                                             #" << endl;
    cout << "###############################################" << endl;
    Egeneric::display_alive();
    cout << "###############################################" << endl;
    cout << "#                                             #" << endl;
    cout << "#     LIST OF LAST DESTROYED EXCEPTIONS :     #" << endl;
    cout << "#                                             #" << endl;
    cout << "###############################################" << endl;
    Egeneric::display_last_destroyed();
}

static string int_to_string(int i)
{
    ostringstream r;
    string s;
    r << i << '\0';
    s = r.str();

    return s;
}
