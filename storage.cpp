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
// $Id: storage.cpp,v 1.15 2002/03/18 11:00:54 denis Rel $
//
/*********************************************************************/

#include "storage.hpp"
#include "infinint.hpp"

unsigned long storage::alloc_size = 120000;
    // must be less than the third of the maxmimum value of a unsigned long

storage::storage(const infinint & size) throw(Ememory, Erange, Ebug)
{
    E_BEGIN;
    make_alloc(size, first, last);
    E_END("storage::storage","infinint");
}

unsigned char storage::operator [](const infinint &position) const throw(Ememory, Erange, Ebug)
{
    E_BEGIN;
    return (storage(*this))[position];
    E_END("storage::operator []","const");
}

unsigned char & storage::operator [](infinint position) throw(Ememory, Erange, Ebug)
{
    E_BEGIN;
    unsigned long offset = 0;
    struct cellule *ptr = first;

    do {
        if(ptr == NULL)
	    throw Erange("storage::operator[]", "asking for an element out of array");
        if(offset > ptr->size)
	{
	    offset -= ptr->size;
	    ptr = ptr->next;
        }
        else
	    position.unstack(offset);
    } while(offset > ptr->size);

    return ptr->data[offset];
    E_END("storage::operator []","");
}

infinint storage::size() const throw(Ememory, Erange, Ebug)
{
    E_BEGIN;
    infinint ret = 0;
    struct cellule *ptr = first;

    while(ptr != NULL)
    {
	ret += ptr->size;
	ptr = ptr->next;
    }

    return ret;
    E_END("storage::size","");
}

void storage::clear(unsigned char val) throw()
{
    E_BEGIN;
    struct cellule *cur = first;
    unsigned long int i;

    while(cur != NULL)
    {
	i = 0;
	while(i < cur->size)
	    cur->data[i++] = val;
	cur = cur->next;
    }
    E_END("storage::clear","");
}

unsigned int storage::write(iterator & it, unsigned char *a, unsigned int size) throw(Erange)
{
    E_BEGIN;
    register unsigned int i;

    if(it.ref != this)
	throw Erange("storage::write", "the iterator is not indexing the object it has been asked to write to");

    for(i = 0; i < size && it != end(); i++)
	*(it++) = a[i];

    return i;
    E_END("storage::write","");
}

unsigned int storage::read(iterator & it, unsigned char *a, unsigned int size) const throw(Erange)
{
    E_BEGIN;
    register unsigned int i;

    if(it.ref != this)
	throw Erange("storage::read", "the iterator is not indexing the object it has been asked to read from");

    for(i = 0; i < size && it != end(); i++)
	a[i] = *(it++);

    return i;
    E_END("storage::read","");
}

void storage::insert_null_bytes_at_iterator(iterator it, unsigned int size) throw(Erange, Ememory, Ebug)
{
    E_BEGIN;
    unsigned char a = 0;

    insert_bytes_at_iterator_cmn(it, true, &a, size);
    E_END("storage::insert_null_bytes_at_iterator","");
}

void storage::insert_const_bytes_at_iterator(iterator it, unsigned char a, unsigned int size) throw(Erange, Ememory, Ebug)
{
    E_BEGIN;
    insert_bytes_at_iterator_cmn(it, true, &a, size);
    E_END("storage::insert_const_bytes_at_iterator","");
}

void storage::insert_bytes_at_iterator(iterator it, unsigned char *a, unsigned int size) throw(Erange, Ememory, Ebug)
{
    E_BEGIN;
    insert_bytes_at_iterator_cmn(it, false, a, size);
    E_END("storage::insert_bytes_at_iterator","");
}

void storage::remove_bytes_at_iterator(iterator it, unsigned int number) throw(Ememory, Ebug)
{
    E_BEGIN;
    while(number > 0 && it.cell != NULL)
    {
	unsigned int can_rem = it.cell->size - it.offset;

        if(can_rem < number)
        {
	    if(it.offset > 0)
            {
  	        unsigned char *p = new unsigned char[it.offset];

  	        if(p != NULL)
    	        {
		    for(register unsigned int i = 0; i < it.offset; i++)
		        p[i] = it.cell->data[i];
  		    delete it.cell->data;
		    it.cell->data = p;
		    it.cell->size -= can_rem;
		    it.cell = it.cell->next;
		    it.offset = 0;
		    number -= can_rem;
                }
	        else
		    throw Ememory("storage::remove_bytes_at_iterator");
            }
            else
            {
                struct cellule *t = it.cell->next;

		if(it.cell->next != NULL)
 		    it.cell->next->prev = it.cell->prev;
		else
		    last = it.cell->prev;

		if(it.cell->prev != NULL)
		    it.cell->prev->next = it.cell->next;
		else
		    first = it.cell->next;

		number -= it.cell->size;
		it.cell->next = NULL;
		it.cell->prev = NULL;
		detruit(it.cell);
		it.cell = t;
            }
	}
	else
	{
 	    unsigned char *p = new unsigned char[it.cell->size - number];

	    if(p != NULL)
	    {
  	    	for(register unsigned int i = 0; i < it.offset; i++)
		    p[i] = it.cell->data[i];
		for(register unsigned int i = it.offset+number ; i < it.cell->size ; i++)
		    p[i-number] = it.cell->data[i];
		delete it.cell->data;
		it.cell->data = p;
		it.cell->size -= number;
		number = 0;
	    }
	    else
	        throw Ememory("storage::remove_bytes_at_iterator");
	}
    }
    reduce();
    E_END("storage::remove_bytes_at_iterator","unsigned int");
}

void storage::remove_bytes_at_iterator(iterator it, infinint number) throw(Ememory, Erange, Ebug)
{
    E_BEGIN;
    unsigned long int sz = 0;
    number.unstack(sz);

    while(sz > 0)
    {
	remove_bytes_at_iterator(it, sz);
	sz = 0;
	number.unstack(sz);
    }
    E_END("storage::remove_bytes_at_iterator","infinint");
}

void storage::fusionne(struct cellule *a_first, struct cellule *a_last, struct cellule *b_first, struct cellule *b_last,
		       struct cellule *&res_first, struct cellule * & res_last) throw(Ebug)
{
    E_BEGIN;
    if((a_first == NULL) ^ (a_last == NULL))
	throw SRC_BUG;

    if((b_first == NULL) ^ (b_last == NULL))
	throw SRC_BUG;

    if(a_last != NULL && b_first != NULL)
    {
	a_last->next = b_first;
	b_first->prev = a_last;
	res_first = a_first;
	res_last = b_last;
    }
    else
	if(a_first == NULL)
	{
	    res_first = b_first;
	    res_last = b_last;
	}
	else
	{
	    res_first = a_first;
	    res_last = a_last;
	}
    E_END("storage::fusionne","");
}

void storage::copy_from(const storage & ref) throw(Ememory, Erange, Ebug)
{
    E_BEGIN;
    unsigned long int pas = 0, delta;
    struct cellule *ptr = ref.first;
    first = last = NULL;

    try
    {
	while(ptr != NULL || pas > 0)
	{
	    if(ptr != NULL)
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
	first = last = NULL;
	throw;
    }

    iterator i_ref = ref.begin();
    iterator i_new = begin();

    while(i_ref != ref.end())
	*(i_new++) = *(i_ref++);
    E_END("storage::copy_from","");
}

static void dummy_call(char *x)
{
    static char id[]="$Id: storage.cpp,v 1.15 2002/03/18 11:00:54 denis Rel $";
    dummy_call(id);
}

signed long int storage::difference(const storage & ref) const throw()
{
    E_BEGIN;
    struct cellule *b = last, *a = ref.last;
    signed long int superior = 0;

    while((a != NULL || superior <= 0) && (b != NULL || superior >= 0) && (a != NULL || b != NULL))
    {
	if(superior >= 0 && a != NULL)
	{
	    superior -= a->size;
	    a = a->next;
	}
	if(superior <= 0 && b != NULL)
	{
	    superior += b->size;
	    b = b->next;
	}
    }
    return superior;
    E_END("storage::difference","");
}

void storage::reduce() throw(Ebug)
{
    E_BEGIN;
    struct cellule *glisseur = first;

    while(glisseur != NULL)
    {
	if(glisseur->next != NULL)
	{
	    unsigned int somme = glisseur->next->size + glisseur->size;

	    if(somme < alloc_size)
	    {
		unsigned char *p = new unsigned char[somme];

		if(p != NULL)
		{
		    struct cellule *tmp = glisseur->next;

		    for(register unsigned int i = 0; i < glisseur->size; i++)
			p[i] = glisseur->data[i];

		    for(register unsigned int i = glisseur->size; i < somme; i++)
			p[i] = tmp->data[i - glisseur->size];

		    delete glisseur->data;
		    glisseur->data = p;
		    glisseur->size = somme;

		    glisseur->next = tmp->next;
		    if(glisseur->next != NULL)
			glisseur->next->prev = glisseur;
		    else
			last = glisseur;

		    tmp->next = tmp->prev = NULL;
		    detruit(tmp);
		}
		else
		    glisseur = glisseur->next;
	    }
	    else
		glisseur = glisseur->next;
	}
	else
	    glisseur = glisseur->next;
    }
    E_END("storage::reduce","");
}

void storage::insert_bytes_at_iterator_cmn(iterator it, bool constant, unsigned char *a, unsigned int size) throw(Erange, Ememory, Ebug)
{
    E_BEGIN;
    if(it.ref != this)
	throw Erange("storage::insert_bytes_at_iterator_cmn", "the iterator is not indexing the object it has been asked to insert byte into");

    if(it.cell != NULL)
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

	before = it.cell->prev;
	after = it.cell->next;
	it.cell->prev = NULL;
	it.cell->next = NULL;

	if(temp.first == NULL || temp.last == NULL)
	    throw SRC_BUG;

	detruit(it.cell);

	if(before != NULL)
	    before->next = temp.first;
	else
	    ((storage *)(it.ref))->first = temp.first;
	temp.first->prev = before;

	if(after != NULL)
	    after->prev = temp.last;
	else
	    ((storage *)(it.ref))->last = temp.last;
	temp.last->next = after;

	temp.first = temp.last = NULL; // to make automatic destruction of "temp" while exiting of the block, keep the data untouched
    }
    else
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
	    if(last != NULL)
		last->next = temp.first;
	    else
		first = temp.first;
	    if(temp.first == NULL)
		throw SRC_BUG;
	    temp.first->prev = last;
	    last = temp.last;
	    break;
	case iterator::OFF_BEGIN :
	    if(first != NULL)
		first->prev = temp.last;
	    else
		last = temp.last;
	    if(temp.last == NULL)
		throw SRC_BUG;
	    temp.last->next = first;
	    last = temp.first;
	    break;
	default:
	    throw SRC_BUG;
	}

	temp.last = temp.first = NULL;
    }
    reduce();
    E_END("storage::insert_bytes_at_iterator_cmn","");
}

void storage::detruit(struct cellule *c) throw(Ebug)
{
    E_BEGIN;
    struct cellule *t;

    while(c != NULL)
    {
	if(c->size == 0 && c->data != NULL)
	    throw SRC_BUG;
	if(alloc_size < c->size)
	    alloc_size = c->size;
	if(c->data != NULL)
	    delete c->data;
	t = c->next;
	delete c;
	c = t;
    }
    E_END("storage::detruit","");
}

void storage::make_alloc(unsigned long int size, struct cellule * & begin, struct cellule * & end) throw (Ememory, Ebug)
{
    E_BEGIN;
    struct cellule *newone;
    struct cellule *previous = NULL;

    do{
	unsigned long dsize = alloc_size < size ? alloc_size : size;

	newone = new struct cellule;
	if(newone != NULL)
	{
	    newone->prev = previous;
	    newone->next = NULL;
	    if(previous != NULL)
		previous->next = newone;
	    else
		begin = newone;
	}
	else
	{
	    detruit(begin);
	    throw Ememory("storage::make_alloc");
	}

	newone->data = new unsigned char[dsize];
	if(newone->data != NULL)
	{
	    size -= dsize;
	    newone->size = dsize;
	    previous = newone;
	}
	else
	    if(alloc_size > 2)
		alloc_size /= 2;
	    else
	    {
		newone->size = 0;
		detruit(begin);
		throw Ememory("storage::make_alloc");
	    }
    } while (size > 0);

    end = newone;
    E_END("storage::make_alloc","unsigned long int");
}

void storage::make_alloc(infinint size, struct cellule * & begin, struct cellule * &end) throw(Ememory, Erange, Ebug)
{
    E_BEGIN;
    struct cellule *debut;
    struct cellule *fin;
    unsigned long int sz = 0;

    size.unstack(sz);
    begin = end = NULL;

    do{
	try {
	    make_alloc(sz, debut, fin);
	    if(end != NULL)
	    {
		end->next = debut;
		debut->prev = end;
		end = fin;
	    }
	    else
		if(begin != NULL)
		    throw SRC_BUG;
		else
		{
		    begin = debut;
		    end = fin;
		}
	}
	catch(Ememory & e)
	{
	    if(begin != NULL)
		detruit(begin);

	    throw;
	}
	sz = 0;
	size.unstack(sz);
    } while(sz > 0);
    E_END("storage::make_alloc","infinint");
}

///////////////////////////////////////////////////////////
//////////////////////// ITERATOR /////////////////////////
///////////////////////////////////////////////////////////

storage::iterator & storage::iterator::operator -= (unsigned long s) throw()
{
    E_BEGIN;
    static const unsigned long max = (unsigned long)(~0) >> 1;  // maximum unsigned long that can also be signed long
    if(s > max)
    {
	signed long t = s/2;
	signed long r = s%2;
	skip_to(-t);
	skip_to(-t);
	skip_to(-r);
    }
    else
	skip_to(-(signed long)(s));

    return *this;
    E_END("storage::iterator::operator -=","");
};

unsigned char & storage::iterator::operator *() const throw(Erange)
{
    E_BEGIN;
    if(points_on_data())
	return cell->data[offset];
    else
	throw Erange("storage::iterator::operator *()", "iterator does not point on data");
    E_END("storage::iterator::operator *","unary operator");
}

void storage::iterator::skip_to(const storage & st, infinint val) throw()
{
    E_BEGIN;
    unsigned long pas = 0;

    *this = st.begin();
    val.unstack(pas);
    do
    {
	skip_to(pas);
	pas = 0;
	val.unstack(pas);
    }
    while(pas > 0);
    E_END("storage::iterator::skip_to","infinint");
}

void storage::iterator::skip_to(signed long int val) throw()
{
    E_BEGIN;
    if(val >= 0)
    {
	while(val > 0 && cell != NULL)
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
	if(cell == NULL)
	    offset = OFF_END;
    }
    else
	while(val < 0 && cell != NULL)
	{
	    val += offset;
	    if(val < 0)
	    {
		cell = cell->prev;
		if(cell != NULL)
		    offset = cell->size;
		else
		    offset = OFF_BEGIN;
	    }
	    else
		offset = val;
	}
    E_END("storage::iterator::skip_to","signed long int");
}

infinint storage::iterator::get_position() const throw(Erange, Ememory, Ebug)
{
    E_BEGIN;
    if(ref == NULL || ref->first == NULL)
	throw Erange("storage::iterator::get_position", "reference storage of the iterator is empty or non existant");

    struct cellule *p = ref->first;
    infinint ret = 0;

    if(cell == NULL)
	throw Erange("storage::iterator::get_position", "iterator does not point on data");

    while(p != NULL && p != cell)
    {
	ret += p->size;
	p = p->next;
    }

    if(p != NULL)
	ret += offset;
    else
	throw Erange("storage::iterator::get_position", "the iterator position is not inside the storage of reference");

    return ret;
    E_END("storage::iterator::get_position","");
}




