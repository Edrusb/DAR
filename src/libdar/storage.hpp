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
// $Id: storage.hpp,v 1.7 2003/10/18 14:43:07 edrusb Rel $
//
/*********************************************************************/

#ifndef STORAGE_HPP
#define STORAGE_HPP

#pragma interface

#include "../my_config.h"
#include "erreurs.hpp"
#include "integers.hpp"

#ifndef MODE
namespace libdar
{
    class infinint;
}
#else
#include "infinint.hpp"
#endif


namespace libdar
{
    class generic_file;

    class storage
    {
    private:
        struct cellule
        {
            struct cellule *next, *prev;
            unsigned char *data;
            U_32 size;
        };

    public:
        storage(U_32 size) throw(Ememory, Ebug)
            { E_BEGIN; make_alloc(size, first, last); E_END("storage::storage","U_32"); };
        storage(const infinint & size) throw(Ememory, Erange, Ebug);
        storage(const storage & ref) throw(Ememory, Ebug)
            { E_BEGIN; copy_from(ref); E_END("storage::storage", "storage &"); };
        storage(generic_file & f, const infinint &size);
        ~storage() throw(Ebug)
            { E_BEGIN; detruit(first); E_END("storage::~storage", ""); };

        storage & operator = (const storage & val) throw(Ememory, Ebug)
            { E_BEGIN; detruit(first); copy_from(val); return *this; E_END("storage::operator=",""); };

        bool operator < (const storage & ref) const throw()
            { E_BEGIN; return difference(ref) < 0; E_END("storage::operator <",""); }; // true if arg uses more space than this
        bool operator == (const storage & ref) const throw()
            { E_BEGIN; return difference(ref) == 0; E_END("storage::operator ==",""); }; //true if arg have same space than this
        bool operator > (const storage & ref) const throw()
            { E_BEGIN; return difference(ref) > 0; E_END("storage::operator >", ""); }; 
        bool operator <= (const storage & ref) const throw()
            { E_BEGIN; return difference(ref) <= 0; E_END("storage::operator <=", ""); };
        bool operator >= (const storage & ref) const throw()
            { E_BEGIN; return difference(ref) >= 0; E_END("storage::operator >=", ""); };
        bool operator != (const storage & ref) const throw()
            { E_BEGIN; return difference(ref) != 0; E_END("storage::operator !=", ""); };
        unsigned char & operator [](infinint position) throw(Ememory, Erange, Ebug); 
        unsigned char operator [](const infinint & position) const throw(Ememory, Erange, Ebug); 
        infinint size() const throw(Ememory, Erange, Ebug);
        void clear(unsigned char val = 0) throw();
        void dump(generic_file & f) const;

        class iterator
        {
        public :
            iterator() { ref = NULL; cell = NULL; offset = 0; };
                // default constructor by reference is OK
                // default destructor is OK
                // default operator = is OK

            iterator operator ++ (S_I x) throw()
                { E_BEGIN; iterator ret = *this; skip_plus_one(); return ret;  E_END("storage::iterator::operator++", "(S_I)"); };
            iterator operator -- (S_I x) throw()
                { E_BEGIN; iterator ret = *this; skip_less_one(); return ret; E_END("storage::iterator::operator--", "(S_I)");}; 
            iterator & operator ++ () throw()
                { E_BEGIN; skip_plus_one(); return *this; E_END("storage::iterator::operator++", "()"); }; 
            iterator & operator -- () throw()
                { E_BEGIN; skip_less_one(); return *this; E_END("storage::iterator::operator--", "()"); }; 
            iterator operator + (U_32 s) const throw()
                { E_BEGIN; iterator ret = *this; ret += s; return ret; E_END("storage::iterator::operator +", ""); }; 
            iterator operator - (U_32 s) const throw()
                { E_BEGIN; iterator ret = *this; ret -= s; return ret; E_END("storage::iterator::operator -", ""); };
            iterator & operator += (U_32 s) throw();
            iterator & operator -= (U_32 s) throw();
            unsigned char &operator *() const throw(Erange);

            void skip_to(const storage & st, infinint val) throw(); // absolute position in st
            infinint get_position() const throw(Erange, Ememory, Ebug);

            bool operator == (const iterator & cmp) const throw()
                { E_BEGIN; return ref == cmp.ref && cell == cmp.cell && offset == cmp.offset; E_END("storage::iterator::operator ==", ""); };
            bool operator != (const iterator & cmp) const throw()
                { E_BEGIN; return ! (*this == cmp); E_END("storage::iterator::operator !=", ""); };

        private:
            static const U_32 OFF_BEGIN = 1;
            static const U_32 OFF_END = 2;

            const storage *ref;
            struct cellule *cell;
            U_32 offset; 

            void relative_skip_to(S_32 val) throw();
            bool points_on_data() const throw()
                { E_BEGIN; return ref != NULL && cell != NULL && offset < cell->size; E_END("storage::iterator::point_on_data", "");};

            inline void skip_plus_one();
            inline void skip_less_one();
        
            friend class storage;
        };

            // public storage methode using iterator

        iterator begin() const throw()
            { E_BEGIN; iterator ret; ret.cell = first; ret.offset = 0; ret.ref = this; return ret; E_END("storage::begin", ""); };
        iterator end() const throw()
            { E_BEGIN; iterator ret; ret.cell = NULL; ret.offset = iterator::OFF_END; ret.ref = this; return ret; E_END("storage::end", ""); };

            // WARNING for the two following methods :
            // there is no "reverse_iterator" type, unlike the standart lib,
            // thus when going from rbegin() to rend(), you must use the -- operator
            // unlike the stdlib, that uses the ++ operator. this is the only difference in use with stdlib.
        iterator rbegin() const throw()
            { E_BEGIN; iterator ret; ret.cell = last; ret.offset = last != NULL ? last->size-1 : 0; ret.ref = this; return ret; E_END("storage::rbegin", ""); };
        iterator rend() const throw()
            { E_BEGIN; iterator ret; ret.cell = NULL, ret.offset = iterator::OFF_BEGIN; ret.ref = this; return ret; E_END("storage::rend", ""); };
    
        U_I write(iterator & it, unsigned char *a, U_I size) throw(Erange);
        U_I read(iterator & it, unsigned char *a, U_I size) const throw(Erange);
        bool write(iterator & it, unsigned char a) throw(Erange)
            { E_BEGIN; return write(it, &a, 1) == 1; E_END("storage::write", "unsigned char"); };
        bool read(iterator & it, unsigned char &a) const throw(Erange)
            { E_BEGIN; return read(it, &a, 1) == 1; E_END("storage::read", "unsigned char"); }; 

            // after one of theses 3 calls, the iterator given in argument are undefined (they may point nowhere)
        void insert_null_bytes_at_iterator(iterator it, U_I size) throw(Erange, Ememory, Ebug);
        void insert_const_bytes_at_iterator(iterator it, unsigned char a, U_I size) throw(Erange, Ememory, Ebug);
        void insert_bytes_at_iterator(iterator it, unsigned char *a, U_I size) throw(Erange, Ememory, Ebug); 
        void insert_as_much_as_necessary_const_byte_to_be_as_wider_as(const storage & ref, const iterator & it, unsigned char value);
        void remove_bytes_at_iterator(iterator it, U_I number) throw(Ememory, Ebug);
        void remove_bytes_at_iterator(iterator it, infinint number) throw(Ememory, Erange, Ebug);


    private:
        struct cellule *first, *last;   

        void copy_from(const storage & ref) throw(Ememory, Erange, Ebug);
        S_32 difference(const storage & ref) const throw();
        void reduce() throw(Ebug); // heuristic that tries to free some memory;
        void insert_bytes_at_iterator_cmn(iterator it, bool constant, unsigned char *a, U_I size) throw(Erange, Ememory, Ebug); 
        void fusionne(struct cellule *a_first, struct cellule *a_last, struct cellule *b_first, struct cellule *b_last,
                      struct cellule *&res_first, struct cellule * & res_last) throw(Ebug);

            ///////////////////////////////
            // STATIC statments :
            //

        static U_32 alloc_size; // stores the last biggest memory allocation successfully realized

        static void detruit(struct cellule *c) throw(Ebug);
        static void make_alloc(U_32 size, struct cellule * & begin, struct cellule * & end) throw(Ememory, Ebug);
        static void make_alloc(infinint size, cellule * & begin, struct cellule * & end) throw(Ememory, Erange, Ebug);

        friend class storage::iterator;
    };

    inline void storage::iterator::skip_plus_one()
    {
        E_BEGIN;
        if(cell != NULL)
            if(++offset >= cell->size)
            {
                cell = cell->next;
                if(cell != NULL)
                    offset = 0;
                else
                    offset = OFF_END;
            }
        E_END("storage::iterator::slik_plus_one", "");
    }

    inline void storage::iterator::skip_less_one()
    {
        E_BEGIN;
        if(cell != NULL)
            if(offset > 0)
                offset--;
            else
            {
                cell = cell->prev;
                if(cell != NULL)
                    offset = cell->size - 1;
                else
                    offset = OFF_BEGIN;
            }
        E_END("storage::iterator::slik_plus_one", "");
    }

} // end of namespace

#endif
