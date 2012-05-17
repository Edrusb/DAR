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
// $Id: storage.hpp,v 1.11.4.3 2009/04/07 08:45:29 edrusb Rel $
//
/*********************************************************************/

    /// \file storage.hpp
    /// \brief contains a class that permits arbitrary large data storage

#ifndef STORAGE_HPP
#define STORAGE_HPP

#include "../my_config.h"
#include "erreurs.hpp"
#include "integers.hpp"

#ifndef LIBDAR_MODE
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

	/// arbitrary large storage structure

	/// used to store infinint
	/// \ingroup Private

    class storage
    {
    private:
        struct cellule
        {
	    cellule() : next(NULL), prev(NULL), data(NULL), size(0) {};
            struct cellule *next, *prev;
            unsigned char *data;
            U_32 size;
        };

    public:
        storage(U_32 size)
            { E_BEGIN; make_alloc(size, first, last); E_END("storage::storage","U_32"); };
        storage(const infinint & size);
        storage(const storage & ref)
            { E_BEGIN; copy_from(ref); E_END("storage::storage", "storage &"); };
        storage(generic_file & f, const infinint &size);
        ~storage()
            { E_BEGIN; detruit(first); E_END("storage::~storage", ""); };

        storage & operator = (const storage & val)
            { E_BEGIN; detruit(first); copy_from(val); return *this; E_END("storage::operator=",""); };

        bool operator < (const storage & ref) const
            { E_BEGIN; return difference(ref) < 0; E_END("storage::operator <",""); }; // true if arg uses more space than this
        bool operator == (const storage & ref) const
            { E_BEGIN; return difference(ref) == 0; E_END("storage::operator ==",""); }; //true if arg have same space than this
        bool operator > (const storage & ref) const
            { E_BEGIN; return difference(ref) > 0; E_END("storage::operator >", ""); };
        bool operator <= (const storage & ref) const
            { E_BEGIN; return difference(ref) <= 0; E_END("storage::operator <=", ""); };
        bool operator >= (const storage & ref) const
            { E_BEGIN; return difference(ref) >= 0; E_END("storage::operator >=", ""); };
        bool operator != (const storage & ref) const
            { E_BEGIN; return difference(ref) != 0; E_END("storage::operator !=", ""); };
        unsigned char & operator [](infinint position);
        unsigned char operator [](const infinint & position) const;
        infinint size() const;
        void clear(unsigned char val = 0);
        void dump(generic_file & f) const;

        class iterator
        {
        public :
            iterator() : ref(NULL), cell(NULL), offset(0) {};
                // default constructor by reference is OK
                // default destructor is OK
                // default operator = is OK

            iterator operator ++ (S_I x)
                { E_BEGIN; iterator ret = *this; skip_plus_one(); return ret;  E_END("storage::iterator::operator++", "(S_I)"); };
            iterator operator -- (S_I x)
                { E_BEGIN; iterator ret = *this; skip_less_one(); return ret; E_END("storage::iterator::operator--", "(S_I)");};
            iterator & operator ++ ()
                { E_BEGIN; skip_plus_one(); return *this; E_END("storage::iterator::operator++", "()"); };
            iterator & operator -- ()
                { E_BEGIN; skip_less_one(); return *this; E_END("storage::iterator::operator--", "()"); };
            iterator operator + (U_32 s) const
                { E_BEGIN; iterator ret = *this; ret += s; return ret; E_END("storage::iterator::operator +", ""); };
            iterator operator - (U_32 s) const
                { E_BEGIN; iterator ret = *this; ret -= s; return ret; E_END("storage::iterator::operator -", ""); };
            iterator & operator += (U_32 s);
	    iterator & operator -= (U_32 s);
            unsigned char &operator *() const;

            void skip_to(const storage & st, infinint val); // absolute position in st
            infinint get_position() const;

            bool operator == (const iterator & cmp) const
                { E_BEGIN; return ref == cmp.ref && cell == cmp.cell && offset == cmp.offset; E_END("storage::iterator::operator ==", ""); };
            bool operator != (const iterator & cmp) const
                { E_BEGIN; return ! (*this == cmp); E_END("storage::iterator::operator !=", ""); };

        private:
            static const U_32 OFF_BEGIN = 1;
            static const U_32 OFF_END = 2;

            const storage *ref;
            struct cellule *cell;
            U_32 offset;

            void relative_skip_to(S_32 val);
            bool points_on_data() const
                { E_BEGIN; return ref != NULL && cell != NULL && offset < cell->size; E_END("storage::iterator::point_on_data", "");};

            inline void skip_plus_one();
            inline void skip_less_one();

            friend class storage;
        };

            // public storage methode using iterator

        iterator begin() const
            { E_BEGIN; iterator ret; ret.cell = first; ret.offset = 0; ret.ref = this; return ret; E_END("storage::begin", ""); };
        iterator end() const
            { E_BEGIN; iterator ret; ret.cell = NULL; ret.offset = iterator::OFF_END; ret.ref = this; return ret; E_END("storage::end", ""); };

            // WARNING for the two following methods :
            // there is no "reverse_iterator" type, unlike the standart lib,
            // thus when going from rbegin() to rend(), you must use the -- operator
            // unlike the stdlib, that uses the ++ operator. this is the only difference in use with stdlib.
        iterator rbegin() const
            { E_BEGIN; iterator ret; ret.cell = last; ret.offset = last != NULL ? last->size-1 : 0; ret.ref = this; return ret; E_END("storage::rbegin", ""); };
        iterator rend() const
            { E_BEGIN; iterator ret; ret.cell = NULL, ret.offset = iterator::OFF_BEGIN; ret.ref = this; return ret; E_END("storage::rend", ""); };

        U_I write(iterator & it, unsigned char *a, U_I size);
        U_I read(iterator & it, unsigned char *a, U_I size) const;
        bool write(iterator & it, unsigned char a)
            { E_BEGIN; return write(it, &a, 1) == 1; E_END("storage::write", "unsigned char"); };
        bool read(iterator & it, unsigned char &a) const
            { E_BEGIN; return read(it, &a, 1) == 1; E_END("storage::read", "unsigned char"); };

            // after one of these 3 calls, the iterator given in argument are undefined (they may point nowhere)
        void insert_null_bytes_at_iterator(iterator it, U_I size);
        void insert_const_bytes_at_iterator(iterator it, unsigned char a, U_I size);
        void insert_bytes_at_iterator(iterator it, unsigned char *a, U_I size);
        void insert_as_much_as_necessary_const_byte_to_be_as_wider_as(const storage & ref, const iterator & it, unsigned char value);
        void remove_bytes_at_iterator(iterator it, U_I number);
        void remove_bytes_at_iterator(iterator it, infinint number);


    private:
        struct cellule *first, *last;

        void copy_from(const storage & ref);
        S_32 difference(const storage & ref) const;
        void reduce(); // heuristic that tries to free some memory;
        void insert_bytes_at_iterator_cmn(iterator it, bool constant, unsigned char *a, U_I size);
        void fusionne(struct cellule *a_first, struct cellule *a_last, struct cellule *b_first, struct cellule *b_last,
                      struct cellule *&res_first, struct cellule * & res_last);

            ///////////////////////////////
            // STATIC statments :
            //

        static void detruit(struct cellule *c);
        static void make_alloc(U_32 size, struct cellule * & begin, struct cellule * & end);
        static void make_alloc(infinint size, cellule * & begin, struct cellule * & end);

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
	{
            if(offset > 0)
                --offset;
            else
            {
                cell = cell->prev;
                if(cell != NULL)
                    offset = cell->size - 1;
                else
                    offset = OFF_BEGIN;
            }
	}
        E_END("storage::iterator::slik_plus_one", "");
    }

} // end of namespace

#endif
