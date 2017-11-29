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

#include "../my_config.h"

extern "C"
{
#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_STRINGS_H
#include <strings.h>
#endif

#if STDC_HEADERS
# include <string.h>
#else
# if !HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
    char *strchr (), *strrchr ();
# if !HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

} // end of extern "C"

#include "storage.hpp"
#include "infinint.hpp"
#include "generic_file.hpp"
#include "integers.hpp"
#include "tools.hpp"

using namespace std;

namespace libdar
{

    storage::storage(const infinint & size)
    {
        make_alloc(size, first, last);
    }

    storage::storage(generic_file & f, const infinint & size)
    {
        U_32 lu, tmp;
        make_alloc(size, first, last);
        struct cellule *ptr = first;

        try
        {
            while(ptr != nullptr)
            {
                lu = 0;

                do
                {
                    tmp = f.read(((char *)(ptr->data))+lu, ptr->size - lu);
                    lu += tmp;
                }
                while(lu < ptr->size && tmp != 0);

                if(lu < ptr->size)
                    throw Erange("storage::storage", gettext("Not enough data to initialize storage field"));
                ptr = ptr->next;
            }
        }
        catch(...)
        {
            detruit(first);
	    first = nullptr;
	    last = nullptr;
            throw;
        }
    }

    unsigned char storage::operator [](const infinint &position) const
    {
        return const_cast<storage &>(*this)[position];
    }

    unsigned char & storage::operator [](infinint position)
    {
        U_32 offset = 0;
        struct cellule *ptr = first;

        do {
            if(ptr == nullptr)
                throw Erange("storage::operator[]", gettext("Asking for an element out of array"));
            if(offset > ptr->size)
            {
                offset -= ptr->size;
                ptr = ptr->next;
            }
            else
                position.unstack(offset);
        } while(offset > ptr->size);

        return ptr->data[offset];
    }

    infinint storage::size() const noexcept
    {
        infinint ret = 0;
        struct cellule *ptr = first;

        while(ptr != nullptr)
        {
            ret += ptr->size;
            ptr = ptr->next;
        }

        return ret;
    }

    void storage::clear(unsigned char val) noexcept
    {
        struct cellule *cur = first;

        while(cur != nullptr)
        {
	    (void)memset(cur->data, val, cur->size);
            cur = cur->next;
        }
    }

    void storage::dump(generic_file & f) const
    {
        const struct cellule *ptr = first;

        while(ptr != nullptr)
        {
            f.write((const char *)(ptr->data), ptr->size);
            ptr = ptr->next;
        }
    }

    U_I storage::write(iterator & it, unsigned char *a, U_I size)
    {
        if(it.ref != this)
            throw Erange("storage::write", gettext("The iterator is not indexing the object it has been asked to write to"));

	U_I wrote = 0;
	while(wrote < size && it != end())
	{
	    U_32 to_write = size - wrote;
	    U_32 space = it.cell->size - it.offset;

	    if(to_write <= space)
	    {
		    // enough room in current data block
		(void)memcpy(it.cell->data + it.offset, a + wrote, to_write);
		wrote += to_write;
		it.offset += to_write;
	    }
	    else
	    {
		    // more to copy than available in current data block
		(void)memcpy(it.cell->data + it.offset, a + wrote, space);
		wrote += space;
		it.cell = it.cell->next;
		if(it.cell != nullptr)
		    it.offset = 0;
		else
		    it.offset = iterator::OFF_END;
	    }
	}

	return wrote;
    }

    U_I storage::read(iterator & it, unsigned char *a, U_I size) const
    {
        if(it.ref != this)
            throw Erange("storage::read", gettext("The iterator is not indexing the object it has been asked to read from"));

	U_I read = 0;
	while(read < size && it != end())
	{
	    U_32 to_read = size - read;
	    U_32 space = it.cell->size - it.offset;

	    if(to_read <= space)
	    {
		    // enough room in current data block
		(void)memcpy(a + read, it.cell->data + it.offset, to_read);
		read += to_read;
		it.offset += to_read;
	    }
	    else
	    {
		    // more to copy than available in current data block
		(void)memcpy(a + read, it.cell->data + it.offset, space);
		read += space;
		it.cell = it.cell->next;
		if(it.cell != nullptr)
		    it.offset = 0;
		else
		    it.offset = iterator::OFF_END;
	    }
	}

        return read;
    }

    void storage::insert_null_bytes_at_iterator(iterator it, U_I size)
    {
        unsigned char a = 0;

        insert_bytes_at_iterator_cmn(it, true, &a, size);
    }

    void storage::insert_const_bytes_at_iterator(iterator it, unsigned char a, U_I size)
    {
        insert_bytes_at_iterator_cmn(it, true, &a, size);
    }

    void storage::insert_bytes_at_iterator(iterator it, unsigned char *a, U_I size)
    {
        insert_bytes_at_iterator_cmn(it, false, a, size);
    }

    void storage::insert_as_much_as_necessary_const_byte_to_be_as_wider_as(const storage & ref, const iterator &it, unsigned char value)
    {
        S_32 to_add = 0;
        const cellule *c_ref = ref.first;
        cellule *c_me = first;

        while((c_ref != nullptr || to_add > 0) && (c_me != nullptr || to_add <= 0))
        {
            if(to_add > 0)
            {
                to_add -= c_me->size;
                c_me = c_me->next;
            }
            else
            {
                to_add += c_ref->size;
                c_ref = c_ref->next;
            }
        }

        while(to_add > 0)
        {
            insert_const_bytes_at_iterator(it, value, to_add);
            if(c_ref != nullptr)
            {
                to_add = c_ref->size;
                c_ref = c_ref->next;
            }
            else
                to_add = 0;
        }
    }

    void storage::remove_bytes_at_iterator(iterator it, U_I number)
    {
        while(number > 0 && it.cell != nullptr)
        {
            U_I can_rem = it.cell->size - it.offset;

            if(can_rem < number)
            {
                if(it.offset > 0) // we must keep data of the current cellule located before it.offset
                {
		    unsigned char *p = nullptr;
                    p = new (nothrow) unsigned char[it.offset];

                    if(p != nullptr)
                    {
			(void)memcpy(p, it.cell->data, it.offset);
                        delete [] it.cell->data;

                        it.cell->data = p;
                        it.cell->size -= can_rem;
                        it.cell = it.cell->next;
                        it.offset = 0;
                        number -= can_rem;
                    }
                    else
                        throw Ememory("storage::remove_bytes_at_iterator");
                }
                else // we can remove from the chain the whole cellule
                {
                    struct cellule *t = it.cell->next;

                    if(t != nullptr)
                        it.cell->next->prev = it.cell->prev;
                    else
                        last = it.cell->prev;

                    if(it.cell->prev != nullptr)
                        it.cell->prev->next = t;
                    else
                        first = t;

                    number -= it.cell->size;
                    it.cell->next = nullptr;
                    it.cell->prev = nullptr;
                    detruit(it.cell);
                    it.cell = t;
                }
            }
            else // can_rem >= number, some data of the current cell must be kept after part to be removed
            {
		unsigned char *p = nullptr;
                p = new (nothrow) unsigned char[it.cell->size - number];

                if(p != nullptr)
                {
		    (void)memcpy(p, it.cell->data, it.offset);
		    (void)memcpy(p + it.offset, it.cell->data + it.offset + number, it.cell->size - it.offset - number);
                    delete [] it.cell->data;

                    it.cell->data = p;
                    it.cell->size -= number;
                    number = 0;
                }
                else
                    throw Ememory("storage::remove_bytes_at_iterator");
            }
        }
        reduce();
    }

    void storage::remove_bytes_at_iterator(iterator it, infinint number)
    {
        U_32 sz = 0;
        number.unstack(sz);

        while(sz > 0)
        {
            remove_bytes_at_iterator(it, sz);
            sz = 0;
            number.unstack(sz);
        }
    }

    void storage::fusionne(struct cellule *a_first, struct cellule *a_last, struct cellule *b_first, struct cellule *b_last,
                           struct cellule *&res_first, struct cellule * & res_last)
    {
        if((a_first == nullptr) ^ (a_last == nullptr))
            throw SRC_BUG;

        if((b_first == nullptr) ^ (b_last == nullptr))
            throw SRC_BUG;

        if(a_last != nullptr && b_first != nullptr)
        {
            a_last->next = b_first;
            b_first->prev = a_last;
            res_first = a_first;
            res_last = b_last;
        }
        else
            if(a_first == nullptr)
            {
                res_first = b_first;
                res_last = b_last;
            }
            else
            {
                res_first = a_first;
                res_last = a_last;
            }
    }

    void storage::copy_from(const storage & ref)
    {
        U_32 pas = 0, delta;
        struct cellule *ptr = ref.first;
        first = last = nullptr;

        try
        {
            while(ptr != nullptr || pas > 0)
            {
                if(ptr != nullptr)
                {
                    delta = pas + ptr->size;
                    ptr = ptr->next;
                }
                else
                    delta = 0;
                if(delta < pas) // must make the allocation
                {
                    struct cellule *debut, *fin;
                    make_alloc(pas, debut, fin);
                    fusionne(first, last, debut, fin, first, last);
                    pas = delta;
                }
                else
                    pas = delta;
            }
        }
        catch(Ememory & e)
        {
            detruit(first);
            first = last = nullptr;
            throw;
        }

        iterator i_ref = ref.begin();
        iterator i_new = begin();

        while(i_ref != ref.end())
	{
	    *i_new = *i_ref;
	    ++i_new;
	    ++i_ref;
	}
    }

    void storage::move_from(storage && ref)
    {
	swap(first, ref.first);
	swap(last, ref.last);
    }

    S_32 storage::difference(const storage & ref) const
    {
        struct cellule *b = last, *a = ref.last;
        S_32 superior = 0;

        while((a != nullptr || superior <= 0) && (b != nullptr || superior >= 0) && (a != nullptr || b != nullptr))
        {
            if(superior >= 0 && a != nullptr)
            {
                superior -= a->size;
                a = a->next;
            }
            if(superior <= 0 && b != nullptr)
            {
                superior += b->size;
                b = b->next;
            }
        }
        return superior;
    }

    void storage::reduce()
    {
        struct cellule *glisseur = first;
	U_32 failed_alloc = ~0;

        while(glisseur != nullptr)
        {
            if(glisseur->next != nullptr)
            {
                U_I somme = glisseur->next->size + glisseur->size;

                if(somme < failed_alloc)
                {
		    unsigned char *p = nullptr;
                    p = new (nothrow) unsigned char[somme];

                    if(p != nullptr)
                    {
                        struct cellule *tmp = glisseur->next;

			(void)memcpy(p, glisseur->data, glisseur->size);
			(void)memcpy(p + glisseur->size, tmp->data, somme - glisseur->size);
                        delete [] glisseur->data;

                        glisseur->data = p;
                        glisseur->size = somme;

                        glisseur->next = tmp->next;
                        if(glisseur->next != nullptr)
                            glisseur->next->prev = glisseur;
                        else
                            last = glisseur;

                        tmp->next = tmp->prev = nullptr;
                        detruit(tmp);
                    }
                    else // alloc failed
		    {
			failed_alloc = somme;
                        glisseur = glisseur->next;
		    }
                }
                else // no fusion possible
                    glisseur = glisseur->next;
            }
            else // no next cellule
                glisseur = glisseur->next;
        }
    }

    void storage::insert_bytes_at_iterator_cmn(iterator it, bool constant, unsigned char *a, U_I size)
    {
        if(it.ref != this)
	    throw Erange("storage::insert_bytes_at_iterator_cmn", gettext("The iterator is not indexing the object it has been defined for"));


        if(it.cell != nullptr)
        {
            storage temp = size+it.cell->size;
            struct cellule *before, *after;
            iterator gliss = temp.begin();

            if(constant)
                temp.clear(*a);
            temp.write(gliss, it.cell->data, it.offset);
            if(!constant)
                temp.write(gliss, a, size);
            else
                gliss += size;
            temp.write(gliss, it.cell->data+it.offset, it.cell->size-it.offset);

            if(temp.first == nullptr || temp.last == nullptr)
                throw SRC_BUG;

		// now we move the chain of cellule from the temp object into the current object (this)
		// first we release the cellule that has been copied to "temp" object
            before = it.cell->prev;
            after = it.cell->next;
            it.cell->prev = nullptr;
            it.cell->next = nullptr;
            detruit(it.cell);
	    it.cell = nullptr;

            if(before != nullptr)
                before->next = temp.first;
            else
                first = temp.first;
            temp.first->prev = before;

            if(after != nullptr)
                after->prev = temp.last;
            else
                last = temp.last;
            temp.last->next = after;

	    temp.first = temp.last = nullptr;
		// this way when "temp" object will be destroyed
		// it will not affect the chain of cells which is now
		// part of "this" (current object).

        }
        else // it_cell == nullptr
        {
            storage temp = size;

            if(constant)
                temp.clear(*a);
            else
            {
                iterator ut = temp.begin();
                temp.write(ut, a,size);
            }

            switch(it.offset)
            {
            case iterator::OFF_END :
                if(last != nullptr)
                    last->next = temp.first;
                else
                    first = temp.first;
                if(temp.first == nullptr)
                    throw SRC_BUG;
                temp.first->prev = last;
                last = temp.last;
                break;
            case iterator::OFF_BEGIN :
                if(first != nullptr)
                    first->prev = temp.last;
                else
                    last = temp.last;
                if(temp.last == nullptr)
                    throw SRC_BUG;
                temp.last->next = first;
                first = temp.first;
                break;
            default:
                throw SRC_BUG;
            }

            temp.last = temp.first = nullptr;
        }

        reduce();
    }

    void storage::detruit(struct cellule *c)
    {
        struct cellule *t;

        while(c != nullptr)
        {
            if(c->data != nullptr)
	    {
                delete [] c->data;
		c->data = nullptr;
	    }
            t = c->next;
            delete c;
            c = t; // ready to destroy the next cellule
        }
    }

    void storage::make_alloc(U_32 size, struct cellule * & begin, struct cellule * & end)
    {
        struct cellule *newone;
        struct cellule *previous = nullptr;
	U_32 dsize = size;

	begin = end = nullptr;

	if(size > 0)
	{
	    do
	    {
		newone = new (nothrow) cellule;
		if(newone != nullptr)
		{
		    newone->prev = previous;
		    newone->next = nullptr;
		    if(previous != nullptr)
			previous->next = newone;
		    else
			begin = newone;
		}
		else
		{
		    detruit(begin);
		    begin = nullptr;
		    throw Ememory("storage::make_alloc");
		}

		do
		{
		    newone->data = new (nothrow) unsigned char[dsize];
		    if(newone->data != nullptr)
		    {
			size -= dsize;
			newone->size = dsize;
			previous = newone;
		    }
		    else
			if(dsize > 2)
			    dsize /= 2;
			else
			{
			    newone->size = 0;
			    detruit(begin);
			    begin = nullptr;
			    throw Ememory("storage::make_alloc");
			}
		}
		while(dsize > 1 && newone->data == nullptr);
	    }
	    while (size > 0);

	    end = newone;
	}
    }

    void storage::make_alloc(infinint size, struct cellule * & begin, struct cellule * &end)
    {
        struct cellule *debut;
        struct cellule *fin;
        U_32 sz = 0;
	begin = end = nullptr;

	if(!size.is_zero())
	{
	    size.unstack(sz);

	    do
	    {
		try
		{
		    make_alloc(sz, debut, fin);
		    if(end != nullptr)
		    {
			end->next = debut;
			debut->prev = end;
			end = fin;
		    }
		    else
			if(begin != nullptr)
			    throw SRC_BUG;
			else
			{
			    begin = debut;
			    end = fin;
			}
		}
		catch(Ememory & e)
		{
		    if(begin != nullptr)
		    {
			detruit(begin);
			begin = nullptr;
			end = nullptr;
		    }

		    throw;
		}
		sz = 0;
		size.unstack(sz);
	    }
	    while(sz > 0);
	}
    }

///////////////////////////////////////////////////////////
//////////////////////// ITERATOR /////////////////////////
///////////////////////////////////////////////////////////


    storage::iterator & storage::iterator::operator += (U_32 s)
    {
        S_32 t = s >> 1;
        S_32 r = s & 0x1;

        relative_skip_to(t);
        relative_skip_to(t+r);
        return *this;
    }

    storage::iterator & storage::iterator::operator -= (U_32 s)
    {
        static const U_32 max = (U_32)(~0) >> 1;  // maximum U_32 that can also be S_32
        if(s > max)
        {
            S_32 t = s >> 1; // equivalent to s/2;
            S_32 r = s & 0x01; // equivalent to s%2;
            relative_skip_to(-t);
            relative_skip_to(-t);
            relative_skip_to(-r);
        }
        else
            relative_skip_to(-(S_32)(s));

        return *this;
    }

    unsigned char & storage::iterator::operator *() const
    {
        if(points_on_data())
            return cell->data[offset];
        else
            throw Erange("storage::iterator::operator *()", gettext("Iterator does not point to data"));
    }

    void storage::iterator::skip_to(const storage & st, infinint val)
    {
	U_16 pas = 0; // relative_skip_to has S_32 as argument, cannot call it with U_32

        *this = st.begin();
        val.unstack(pas);
        do
        {
            relative_skip_to(pas);
            pas = 0;
            val.unstack(pas);
        }
        while(pas > 0);
    }

    void storage::iterator::relative_skip_to(S_32 val)
    {
        if(val >= 0)
        {
            while(val > 0 && cell != nullptr)
            {
                if(offset + val >= cell->size)
                {
                    val -= cell->size - offset;
                    cell = cell->next;
                    offset = 0;
                }
                else
                {
                    offset += val;
                    val = 0;
                }
            }
            if(cell == nullptr)
                offset = OFF_END;
        }
        else
            while(val < 0 && cell != nullptr)
            {
                val += offset;
                if(val < 0)
                {
                    cell = cell->prev;
                    if(cell != nullptr)
                        offset = cell->size;
                    else
                        offset = OFF_BEGIN;
                }
                else
                    offset = val;
            }
    }

    infinint storage::iterator::get_position() const
    {
        if(ref == nullptr || ref->first == nullptr)
            throw Erange("storage::iterator::get_position", gettext("Reference storage of the iterator is empty or non existent"));

        struct cellule *p = ref->first;
        infinint ret = 0;

        if(cell == nullptr)
            throw Erange("storage::iterator::get_position", gettext("Iterator does not point to data"));

        while(p != nullptr && p != cell)
        {
            ret += p->size;
            p = p->next;
        }

        if(p != nullptr)
            ret += offset;
        else
            throw Erange("storage::iterator::get_position", gettext("The iterator position is not inside the storage of reference"));

        return ret;
    }

} // end of namespace
