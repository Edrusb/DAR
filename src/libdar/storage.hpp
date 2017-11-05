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
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

    /// \file storage.hpp
    /// \brief contains a class that permits arbitrary large data storage
    /// \ingroup Private

#include "../my_config.h"
#include "erreurs.hpp"
#include "integers.hpp"

#ifdef LIBDAR_MODE
#include "infinint.hpp"
#endif

    // it is necessary to not protect the previous inclusion inside
    // the STORAGE_HPP protection to avoid cyclic dependancies.

#ifndef STORAGE_HPP
#define STORAGE_HPP

#ifndef LIBDAR_MODE
namespace libdar
{
    class infinint;
}
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
	    cellule() : next(nullptr), prev(nullptr), data(nullptr), size(0) {};
            struct cellule *next, *prev;
            unsigned char *data;
            U_32 size;
        };

    public:
        storage(U_32 size) { make_alloc(size, first, last); };
        storage(const infinint & size);
        storage(generic_file & f, const infinint & size);
        storage(const storage & ref) { copy_from(ref); };
	storage(storage && ref): first(nullptr), last(nullptr) { move_from(std::move(ref)); };
        storage & operator = (const storage & val) { detruit(first); copy_from(val); return *this; };
	storage & operator = (storage && val) { move_from(std::move(val)); return *this; };
        ~storage() { detruit(first); };

        bool operator < (const storage & ref) const
	{ return difference(ref) < 0; }; // true if arg uses more space than this
        bool operator == (const storage & ref) const
	{ return difference(ref) == 0; }; //true if arg have same space than this
        bool operator > (const storage & ref) const
	{ return difference(ref) > 0; };
        bool operator <= (const storage & ref) const
	{ return difference(ref) <= 0; };
        bool operator >= (const storage & ref) const
	{ return difference(ref) >= 0; };
        bool operator != (const storage & ref) const
	{ return difference(ref) != 0; };
        unsigned char & operator [](infinint position);
        unsigned char operator [](const infinint & position) const;
        infinint size() const;
        void clear(unsigned char val = 0);
        void dump(generic_file & f) const;

        class iterator
        {
        public :
            iterator() : ref(nullptr), cell(nullptr), offset(0) {};
	    iterator(const iterator & ref) = default;
	    iterator(iterator && ref) = default;
	    iterator & operator = (const iterator & ref) = default;
	    iterator & operator = (iterator && ref) = default;
	    ~iterator() = default;

            iterator operator ++ (S_I x)
	    { iterator ret = *this; skip_plus_one(); return ret; };
            iterator operator -- (S_I x)
	    { iterator ret = *this; skip_less_one(); return ret; };
            iterator & operator ++ ()
	    { skip_plus_one(); return *this; };
            iterator & operator -- ()
	    { skip_less_one(); return *this; };
            iterator operator + (U_32 s) const
	    { iterator ret = *this; ret += s; return ret; };
            iterator operator - (U_32 s) const
	    { iterator ret = *this; ret -= s; return ret; };
            iterator & operator += (U_32 s);
	    iterator & operator -= (U_32 s);
            unsigned char &operator *() const;

            void skip_to(const storage & st, infinint val); // absolute position in st
            infinint get_position() const;

            bool operator == (const iterator & cmp) const
	    { return ref == cmp.ref && cell == cmp.cell && offset == cmp.offset; };
            bool operator != (const iterator & cmp) const
	    { return ! (*this == cmp); };

        private:
            static const U_32 OFF_BEGIN = 1;
            static const U_32 OFF_END = 2;

            const storage *ref;
            struct cellule *cell;
            U_32 offset;

            void relative_skip_to(S_32 val);
            bool points_on_data() const
	    {  return ref != nullptr && cell != nullptr && offset < cell->size; };

            inline void skip_plus_one();
            inline void skip_less_one();

            friend class storage;
        };

            // public storage methode using iterator

        iterator begin() const
	{ iterator ret; ret.cell = first; if(ret.cell != nullptr) ret.offset = 0; else ret.offset = iterator::OFF_END; ret.ref = this; return ret; };
        iterator end() const
	{ iterator ret; ret.cell = nullptr; ret.offset = iterator::OFF_END; ret.ref = this; return ret;  };

            // WARNING for the two following methods :
            // there is no "reverse_iterator" type, unlike the standart lib,
            // thus when going from rbegin() to rend(), you must use the -- operator
            // unlike the stdlib, that uses the ++ operator. this is the only difference in use with stdlib.
        iterator rbegin() const
	{ iterator ret; ret.cell = last; ret.offset = last != nullptr ? last->size-1 : 0; ret.ref = this; return ret; };
        iterator rend() const
	{ iterator ret; ret.cell = nullptr, ret.offset = iterator::OFF_BEGIN; ret.ref = this; return ret; };

	    /// write data to the storage at the location pointed to by it

	    /// \param[in,out] it where to write data to, at the end this iterator points just after the data that has been wrote
	    /// \param[in] a gives to the address where is located the data to write to the storage object
	    /// \param[in] size how much bytes to write to the storage
        U_I write(iterator & it, unsigned char *a, U_I size);
        U_I read(iterator & it, unsigned char *a, U_I size) const;
        bool write(iterator & it, unsigned char a)
	{ return write(it, &a, 1) == 1; };
        bool read(iterator & it, unsigned char &a) const
	{ return read(it, &a, 1) == 1; };

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
	void move_from(storage && ref);
        S_32 difference(const storage & ref) const;
        void reduce(); // heuristic that tries to free some memory;
        void insert_bytes_at_iterator_cmn(iterator it, bool constant, unsigned char *a, U_I size);
        void fusionne(struct cellule *a_first, struct cellule *a_last, struct cellule *b_first, struct cellule *b_last,
                      struct cellule *&res_first, struct cellule * & res_last);

        static void detruit(struct cellule *c); // destroy all cells following 'c' including 'c' itself
        static void make_alloc(U_32 size, struct cellule * & begin, struct cellule * & end);
        static void make_alloc(infinint size, cellule * & begin, struct cellule * & end);

        friend class storage::iterator;
    };

    inline void storage::iterator::skip_plus_one()
    {
        if(cell != nullptr)
            if(++offset >= cell->size)
            {
                cell = cell->next;
                if(cell != nullptr)
                    offset = 0;
                else
                    offset = OFF_END;
            }
    }

    inline void storage::iterator::skip_less_one()
    {
        if(cell != nullptr)
	{
            if(offset > 0)
                --offset;
            else
            {
                cell = cell->prev;
                if(cell != nullptr)
                    offset = cell->size - 1;
                else
                    offset = OFF_BEGIN;
            }
	}
    }

} // end of namespace

#endif
