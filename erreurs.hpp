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
// $Id: erreurs.hpp,v 1.22 2002/03/18 11:00:54 denis Rel $
//
/*********************************************************************/

#ifndef ERREURS_HPP
#define ERREURS_HPP

#include <string>
#include <list>

using namespace std;

#define E_BEGIN try {
#define E_END(passage, message)  } catch(Egeneric & e) { e.stack(passage, message); throw; } 

class Egeneric 
{
public : 
    Egeneric(const string &source, const string &message);
    Egeneric(const Egeneric & ref);
    virtual ~Egeneric() { all_instances.remove(this); };

    virtual void stack(string passage, string message = "") { pile.push_back(niveau(passage, message)); };
    string get_message() const { return pile.front().objet; };
    void dump() const;

    static unsigned int total() { return all_instances.size(); };
    static unsigned int zombies() { return destroyed.size(); };
    static unsigned int alive();
    static void clear_last_destroyed();
    static void display_last_destroyed(); // displays and clear the last destroyed exceptions fifo
    static void display_alive();

protected :
    bool zombie;

    virtual string exceptionID() const = 0;
    void add_to_last_destroyed(Egeneric *obj);

private :
    struct niveau
    {
	niveau(const string &ou, const string &quoi) { lieu = ou; objet = quoi; };
	string lieu, objet;
    };
    list<niveau> pile;

    static const unsigned int fifo_size = 10; // number of last destroyed exceptions recorded
    static list<Egeneric *> destroyed; // copies of destroyed last execeptions 
    static list<Egeneric *> all_instances;
};

class Ememory : public Egeneric
{
public:
    Ememory(string source) : Egeneric(source, "Lack of Memory") {};
    ~Ememory() { if(!zombie) add_to_last_destroyed(new Ememory(*this)); };

protected :
    string exceptionID() const { return "MEMORY"; };
    Ememory *dup() const { return new Ememory(*this); };
};

#define SRC_BUG Ebug(__FILE__, __LINE__)
#define XMT_BUG(exception, call) exception.stack(call, __FILE__, __LINE__)

class Ebug : public Egeneric 
{
public :
    Ebug(string file, int line);
    ~Ebug() { if(!zombie) add_to_last_destroyed(new Ebug(*this)); };
    
    void stack(string passage, string file, string line);
protected :
    string exceptionID() const { return "BUG"; };
    Ebug *dup() const { return new Ebug(*this); };
};

class Einfinint : public Egeneric 
{
public :
    Einfinint(string source, string message) : Egeneric(source, message) {};
    ~Einfinint() { if(!zombie) add_to_last_destroyed(new Einfinint(*this)); };

protected :
    string exceptionID() const { return "INFININT"; };
    Einfinint *dup() const { return new Einfinint(*this); };
};

class Erange : public Egeneric 
{
public :
    Erange(string source, string message = "") : Egeneric(source, message) {};
    ~Erange() { if(!zombie) add_to_last_destroyed(new Erange(*this)); };

protected : 
    string exceptionID() const { return "RANGE"; };
    Erange *dup() const { return new Erange(*this); };
};

class Edeci : public Egeneric 
{
public :
    Edeci(string source, string message = "") : Egeneric(source, message) {};
    ~Edeci() { if(!zombie) add_to_last_destroyed(new Edeci(*this)); };

protected :
    string exceptionID() const { return "DECI"; };
    Edeci *dup() const { return new Edeci(*this); };
};

class Efeature : public Egeneric
{
public :
    Efeature(string message) : Egeneric("", message) {};
    ~Efeature() { if(!zombie) add_to_last_destroyed(new Efeature(*this)); };

protected :
    string exceptionID() const { return "UNIMPLEMENTED FEATURE"; };
    Efeature *dup() const { return new Efeature(*this); };
};

class Ehardware : public Egeneric
{
public :
    Ehardware(string source, string message = "") : Egeneric(source, message) {};
    ~Ehardware() { if(!zombie) add_to_last_destroyed(new Ehardware(*this)); };

protected :
    string exceptionID() const { return "HARDWARE ERROR"; };
    Ehardware *dup() const { return new Ehardware(*this); };
};

class Euser_abort : public Egeneric
{
public :
    Euser_abort(string msg) : Egeneric("",msg) {};
    ~Euser_abort() { if(!zombie) add_to_last_destroyed(new Euser_abort(*this)); };

protected :
    string exceptionID() const { return "USER ABORTED OPERATION"; };
    Euser_abort *dup() const { return new Euser_abort(*this); };
};

#endif
