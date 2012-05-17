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
// $Id: special_alloc.cpp,v 1.18.2.6 2012/03/10 20:57:21 edrusb Rel $
//
/*********************************************************************/

#include "../my_config.h"
#include "special_alloc.hpp"
#include "erreurs.hpp"
#include "tools.hpp"
#include "memory_check.hpp"

#include <list>
#include <errno.h>

#ifdef LIBDAR_SPECIAL_ALLOC

#ifdef MUTEX_WORKS
extern "C"
{
#if HAVE_PTHREAD_H
#include <pthread.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif
}
static bool alloc_mutex_initialized = false;
static pthread_mutex_t alloc_mutex;

#define CRITICAL_START                                                  \
    if(!alloc_mutex_initialized)					\
	throw Elibcall("alloc_mutex_initialized", gettext("Thread-safe not initialized for libdar, read manual or contact maintainer of the application that uses libdar"));                                                       \
    sigset_t Critical_section_mem_mask_memory;				\
    tools_block_all_signals(Critical_section_mem_mask_memory);		\
    pthread_mutex_lock(&alloc_mutex)

#define CRITICAL_END							\
    pthread_mutex_unlock(&alloc_mutex);					\
    tools_set_back_blocked_signals(Critical_section_mem_mask_memory)


#else // MUTEX does not work or is not available
#define CRITICAL_START //
#define CRITICAL_END   //
#endif // MUTEX_WORKS

using namespace std;

namespace libdar
{

	//////////////////////////////////////////////
	//  manages allocated blocks and eases lookup at memory release
	//

    class zone
    {
    public:
	zone() { recorded = false; };
	virtual ~zone() { if(recorded) throw SRC_BUG; };

	virtual void release(void *ptr);

	static zone *lookup(void *ptr); //< returns a pointer to the zone in which ptr has been allocated

    protected:
	virtual void inherited_release(void *ptr) = 0;
	void record_me(void *ptr, U_I allocated);
	void unrecord_me(void *ptr);

    private:
	bool recorded;  //< whether this zone has been recorded

	class record
	{
	public:
	    record(void *ptr, U_I alloc) : min((char *)ptr), size(alloc) { if(alloc == 0) throw SRC_BUG; };
	    bool operator < (const record & ref) const { return min + (size - 1) < ref.min; };

	    char *min;
	    U_I size;
	};

	static map<record, zone *> ordered; //< class global map used to for zone lookup
    };

    map<zone::record, zone *> zone::ordered;

	//////////////////////////////////////////////
	// forward definition of class cluster
	//

    class cluster;

	//////////////////////////////////////////////
	// managed all clusters of a given size
	//

    class sized
    {
    public:
	sized(U_I x_block_size);
	sized(const sized & ref) { throw SRC_BUG; };
	const sized & operator = (const sized & ref) { throw SRC_BUG; };
	~sized();

	void *alloc();

	void push_to_release_list(cluster *ref);
	bool is_empty() const;
	void dump(ostream & output) const;

#ifdef LIBDAR_DEBUG_MEMORY
	U_I max_percent_full() const;
#endif

    private:
	static const U_I average_table_size = 10240;

	U_I table_size_64;
	list<cluster *> clusters;
	list<cluster *>::iterator next_free_in_table;
	cluster *pending_release;
#ifdef LIBDAR_DEBUG_MEMORY
	U_I sum_percent;
	U_I num_cluster;
#endif
    };


	//////////////////////////////////////////////
	// holds some memory allocation blocks of a given fixed size
	//

    class cluster : public zone
    {
    public:
	cluster(U_I x_block_size,   //< block size that will be allocated from this cluster
		U_I table_size_64,  //< the total number of block in this cluster is table_size_64 * 64
		sized *x_holder);   //< this is the sized object that holds this cluster object
	cluster(const cluster & ref) : zone(ref) { throw SRC_BUG; };
	const cluster & operator = (const cluster & ref) { throw SRC_BUG; };
	~cluster();

	bool is_full() const { return available_blocks == 0; };
	bool is_empty() const { return available_blocks == max_available_blocks; };
	void *alloc();
	U_I get_block_size() const { return block_size; };
	void dump(ostream & output) const;
#ifdef LIBDAR_DEBUG_MEMORY
	U_I max_percent_full() const { return (max_available_blocks - min_avail_reached)*100/max_available_blocks; };
#endif

    protected:
	void inherited_release(void *ptr);

    private:
	static const U_64 FULL = ~(U_64)(0);            //< this is 1111...111 integer in binary notation
	static const U_64 HALF = (~(U_64)(0)) >> 1;     //< this is 0111...111 integer in binary notation
	static const U_64 LEAD = ~((~(U_64)(0)) >> 1);  //< this is 1000...000 integer in binary notation

	char *alloc_area;          //< points to the allocated memory pool
	U_I alloc_area_size;       //< size of allocated memory pool in bytes
	U_I block_size;            //< size of requested blocks
	U_64 *alloc_table;         //< maps the blocks of the allocated memory pool that have been sub-allocated
	U_I alloc_table_size;      //< size of the map (in number of U_64 integers)
	U_I next_free_in_table;    //< next U_64 to look at for a request of block allocation
	U_I available_blocks;      //< current number of available block in alloc
	U_I max_available_blocks;  //< max available block in alloc
	sized *holder;             //< holder object of type sized that this object is expected to be managed by
#ifdef LIBDAR_DEBUG_MEMORY
	U_I min_avail_reached;     //< records the max fullfilment reached
#endif


	U_I find_free_slot_in(U_I table_integer) const;
	void set_slot_in(U_I table_integer, U_I bit_offset, bool value);
	void examination_status(ostream & output) const; // debugging, displays the list of unallocated blocks
    };

	//////////////////////////////////////////////
	// global memory management class, manages all sized objects
	//

    class global_alloc
    {
    public:
	global_alloc() { carte.clear(); };
	global_alloc(const global_alloc & ref) { throw SRC_BUG ; };
	const global_alloc & operator = (const global_alloc & ref) { throw SRC_BUG; };
	~global_alloc();

	void *alloc(U_I size);
	void release(void *ptr);
	void garbage_collect();
	bool is_empty() const { return carte.size() == 0; };
	void dump(ostream & output) const;
#ifdef LIBDAR_DEBUG_MEMORY
	void max_percent_full(ostream & output) const;
#endif
    private:
	map<U_I, sized *> carte;
#ifdef LIBDAR_DEBUG_MEMORY
	map<U_I, U_I> count;
#endif
    };



	//////////////////////////////////////////////
	//  global_alloc class implementation
	//

    void zone::record_me(void *ptr, U_I allocated)
    {
	record rec = record(ptr, allocated);

	ordered[rec] = this;
	recorded = true;
    }

    void zone::unrecord_me(void *ptr)
    {
	record rec = record(ptr, 1);
	map<record, zone *>::iterator it = ordered.find(rec);

	if(it == ordered.end())
	    throw SRC_BUG;
	ordered.erase(it);
	recorded = false;
    }


    void zone::release(void *ptr)
    {
	if(recorded)
	    inherited_release(ptr);
	else
	    throw SRC_BUG;
    }

    zone *zone::lookup(void *ptr)
    {
	record rec = record(ptr, 1);
	map<record, zone *>::iterator it = ordered.find(rec);

	if(it == ordered.end())
	    throw SRC_BUG;

	return it->second;
    }



	//////////////////////////////////////////////
	// cluster class implementation
	//


    cluster::cluster(U_I x_block_size, U_I table_size_64, sized *x_holder)
    {
	block_size = x_block_size;
	alloc_table_size = table_size_64;
	next_free_in_table = 0;
	max_available_blocks = table_size_64 * 64;
	available_blocks = max_available_blocks;
	alloc_area_size = max_available_blocks * block_size;
	holder = x_holder;
	alloc_table = NULL;
	alloc_area = NULL;
#ifdef LIBDAR_DEBUG_MEMORY
	min_avail_reached = max_available_blocks;
#endif

	try
	{
	    alloc_table = (U_64 *)new char[alloc_table_size*sizeof(U_64) + alloc_area_size];
	    if(alloc_table == NULL)
		throw Ememory("cluster::cluster");
	    alloc_area = (char *)(alloc_table + alloc_table_size);

	    for(U_I i = 0; i < alloc_table_size; ++i)
		alloc_table[i] = 0;
	    record_me(alloc_area, alloc_area_size);
	}
	catch(...)
	{
	    if(alloc_table != NULL)
		delete [] alloc_table;
	    throw;
	}
    }

    cluster::~cluster()
    {
	unrecord_me(alloc_area);
	if(alloc_table != NULL)
	    delete [] alloc_table;
    }

    void *cluster::alloc()
    {
	void * ret = NULL;

	if(is_full())
	    throw SRC_BUG;

	while(next_free_in_table < alloc_table_size && alloc_table[next_free_in_table] == FULL)
	    ++next_free_in_table;

	if(next_free_in_table == alloc_table_size)
	{
	    next_free_in_table = 0;

	    while(next_free_in_table < alloc_table_size && alloc_table[next_free_in_table] == FULL)
		++next_free_in_table;

	    if(next_free_in_table == alloc_table_size)
		throw SRC_BUG; // should be reported as full by full() method
	}

	U_I offset = find_free_slot_in(next_free_in_table);
	ret = alloc_area + block_size * (next_free_in_table * 64 + offset);
	set_slot_in(next_free_in_table, offset, true);
	--available_blocks;
#ifdef LIBDAR_DEBUG_MEMORY
	if(available_blocks < min_avail_reached)
	    min_avail_reached = available_blocks;
#endif

	return ret;
    }

    U_I cluster::find_free_slot_in(U_I table_integer) const
    {
	U_I ret = 0;
	U_64 focus = alloc_table[table_integer];


	while(focus > HALF)
	{
	    focus <<= 1;
	    ++ret;
	}

	return ret;
    }

    void cluster::set_slot_in(U_I table_integer, U_I bit_offset, bool value)
    {
	U_64 add_mask = LEAD >> bit_offset;

	if(value)
	{
	    if((alloc_table[table_integer] & add_mask) != 0)
		throw SRC_BUG; // double allocation
	    alloc_table[table_integer] |= add_mask;
	}
	else
	{
	    if((alloc_table[table_integer] & add_mask) == 0)
		throw SRC_BUG; // double release
	    alloc_table[table_integer] &= ~add_mask;
	}
    }

    void cluster::inherited_release(void *ptr)
    {
	if(ptr < alloc_area || ptr >= alloc_area + alloc_area_size)
	    throw SRC_BUG; // not allocated here
	else
	{
	    U_I char_offset = (char *)(ptr) - alloc_area;
	    U_I block_number = char_offset / block_size;
	    U_I table_integer = block_number / 64;
	    U_I offset = block_number % 64;

	    if(char_offset % block_size != 0)
		throw SRC_BUG; // not at a block boundary

	    set_slot_in(table_integer, offset, false);
	    ++available_blocks;
	    if(available_blocks > max_available_blocks)
		throw SRC_BUG;
	    if(is_empty())
	    {
		if(holder != NULL)
		    holder->push_to_release_list(this);
		else
		    throw SRC_BUG;
	    }
	}
    }

    void cluster::dump(ostream & output) const
    {
	U_I counted = max_available_blocks - available_blocks;

	output << "      Cluster dump:" << endl;
	output << "         block_size           = " << block_size << endl;
	output << "         available_blocks      = " <<  available_blocks << endl;
	output << "         max_available_blocks = " << max_available_blocks << endl;
	output << "         which makes " << counted << " unallocated block(s)" << endl;
	output << "         Follows list of unallocated blocks : " << endl;
	examination_status(output);
	output << endl << endl;
    }

    void cluster::examination_status(ostream & output) const
    {
	for(U_I table_ptr = 0; table_ptr < alloc_table_size; ++table_ptr)
	{
	    U_64 mask = LEAD;

	    for(U_I offset = 0; offset < 64; ++offset)
	    {
		if((alloc_table[table_ptr] & mask) != 0)
		    output << "                 unallocated memory (" << block_size << ") at: 0x" << hex << (U_I)(alloc_area + block_size * ( 64 * table_ptr + offset)) << dec << endl;
		mask >>= 1;
	    }
	}
    }

	//////////////////////////////////////////////
	// sized class implementation
	//

    sized::sized(U_I block_size)
    {
	cluster * tmp = NULL;

	table_size_64 = average_table_size / (64 * block_size) + 1;
	pending_release = NULL;
#ifdef LIBDAR_DEBUG_MEMORY
	sum_percent = 0;
	num_cluster = 0;
#endif
	tmp = new cluster(block_size, table_size_64, this);
	if(tmp == NULL)
	    throw Ememory("sized::sized");
	try
	{
	    clusters.push_front(tmp);
	    next_free_in_table = clusters.begin();
	}
	catch(...)
	{
	    delete tmp;
	    throw;
	}
    }

    sized::~sized()
    {
	list<cluster *>::iterator it = clusters.begin();

	while(it != clusters.end())
	{
	    if(*it != NULL)
		delete *it;
	    ++it;

	}
	clusters.clear();
	pending_release = NULL;
    }

    void *sized::alloc()
    {
	while(next_free_in_table != clusters.end()
	      && (*next_free_in_table) != NULL
	      && ( (*next_free_in_table) == pending_release || (*next_free_in_table)->is_full() )
	    )
	    ++next_free_in_table;

	if(next_free_in_table == clusters.end())
	{
	    next_free_in_table = clusters.begin();

	    while(next_free_in_table != clusters.end()
		  && (*next_free_in_table) != NULL
		  && ( (*next_free_in_table) == pending_release || (*next_free_in_table)->is_full() )
		)
		++next_free_in_table;

	    if(next_free_in_table == clusters.end())
	    {
		if(pending_release == NULL)
		{

		    // all clusters are full, we must allocate a new one

		    if(clusters.empty())
			throw SRC_BUG;
		    if((*clusters.begin()) == NULL)
			throw SRC_BUG;
		    cluster *tmp = new cluster((*clusters.begin())->get_block_size(), table_size_64, this);
		    if(tmp == NULL)
			throw Ememory("sized::alloc");
		    try
		    {
			clusters.push_front(tmp);
		    }
		    catch(...)
		    {
			delete tmp;
			throw;
		    }
		    next_free_in_table = clusters.begin();
		}
		else
		{

			// recycling the cluster that was pending for release

		    next_free_in_table = clusters.begin();
		    while(next_free_in_table != clusters.end() && *next_free_in_table != pending_release)
			++next_free_in_table;

		    if(next_free_in_table == clusters.end())
			throw SRC_BUG;
		    pending_release = NULL;
		}
	    }
	}

	if(*next_free_in_table == NULL)
	    throw SRC_BUG;

	return (*next_free_in_table)->alloc();
    }

    void sized::push_to_release_list(cluster *ref)
    {
	if(pending_release != NULL)
	{
	    list<cluster *>::iterator it = clusters.begin();

#ifdef LIBDAR_DEBUG_MEMORY
	    sum_percent += pending_release->max_percent_full();
	    ++num_cluster;
#endif
	    while(it != clusters.end() && (*it) != pending_release)
		++it;

	    if(it == clusters.end())
		throw SRC_BUG; // cannot release previously recorded cluster
	    delete pending_release;
	    pending_release = NULL;
	    clusters.erase(it);
	    if(clusters.size() == 0)
		throw SRC_BUG;
	}

	pending_release = ref;
    }

    bool sized::is_empty() const
    {
	return clusters.size() == 1 && (*clusters.begin()) != NULL && (*clusters.begin())->is_empty();
    }

    void sized::dump(ostream & output) const
    {
	list<cluster *>::const_iterator it = clusters.begin();
	output << "   this sized object bytes blocks contains " << clusters.size() << " clusters among which the following are not empty" << endl;

	while(it != clusters.end())
	{
	    if(*it == NULL)
		output << "  !?! NULL pointer in cluster list !?!" << endl;
	    else
	    {
		if(!(*it)->is_empty())
		    (*it)->dump(output);
	    }

	    ++it;
	}
    }

#ifdef LIBDAR_DEBUG_MEMORY
    U_I sized::max_percent_full() const
    {
	U_I tmp_sum = sum_percent;
	U_I tmp_num = num_cluster;

	if(pending_release != NULL)
	{
	    tmp_sum += pending_release->max_percent_full();
	    ++tmp_num;
	}

	return tmp_sum / tmp_num;
    }
#endif

	//////////////////////////////////////////////
	//  global_alloc class implementation
	//

    global_alloc::~global_alloc()
    {
	map<U_I, sized *>::iterator it = carte.begin();

	while(it != carte.end())
	{
	    if(it->second != NULL)
		delete it->second;
	    ++it;
	}
    }

    void *global_alloc::alloc(U_I size)
    {
	map<U_I, sized *>::iterator it = carte.find(size);

#ifdef LIBDAR_DEBUG_MEMORY
	map<U_I, U_I>::iterator cit = count.find(size);
	if(cit == count.end())
	    count[size] = 1;
	else
	    ++(cit->second);
#endif

	if(it != carte.end())
	    if(it->second == NULL)
		throw SRC_BUG;
	    else
		return it->second->alloc();
	else
	{
	    memory_check_special_new_sized(size);
	    sized *tmp = new sized(size);
	    if(tmp == NULL)
		throw SRC_BUG;
	    try
	    {
		carte[size] = tmp;
	    }
	    catch(...)
	    {
		delete tmp;
		throw;
	    }
	    return tmp->alloc();
	}
    }

    void global_alloc::release(void *ptr)
    {
	zone *cl = zone::lookup(ptr);

	if(cl == NULL)
	    throw SRC_BUG;

	cl->release(ptr);
    }

    void global_alloc::garbage_collect()
    {
	map<U_I, sized *>::iterator it = carte.begin();

	while(it != carte.end())
	{
	    if(it->second == NULL)
		throw SRC_BUG;
	    if(it->second->is_empty())
	    {
		delete it->second;
		carte.erase(it);
	    }
	    else
		++it;
	}
#ifdef LIBDAR_DEBUG_MEMORY
	count.clear();
#endif
    }

    void global_alloc::dump(ostream & output) const
    {
	map<U_I, sized *>::const_iterator it = carte.begin();

	output << "###################################################################" << endl;
	output << "  SPECIAL ALLOCATION MODULE REPORTS UNRELEASED MEMORY ALLOCATIONS\n" << endl;
	while(it != carte.end())
	{
	    if(it->second == NULL)
		output << "!?! NO corresponding sized object for block size " << it->first << endl;
	    else
	    {
		if(!it->second->is_empty())
		{
		    output << "Dumping list for blocks of " << it->first << " bytes size" << endl;
		    it->second->dump(output);
		}
	    }
	    ++it;
	}
	output << "###################################################################" << endl;
    }


#ifdef LIBDAR_DEBUG_MEMORY
    void global_alloc:: max_percent_full(ostream & output) const
    {
	map<U_I, sized *>::const_iterator it = carte.begin();
	map<U_I, U_I>::const_iterator cit;
	U_I freq;

	output << " ---------------------------------------------------- " << endl;
	output << " Statistical use of memory allocation per block size: " << endl;
	output << " ---------------------------------------------------- " << endl;
	while(it != carte.end())
	{
	    cit = count.find(it->first);
	    if(cit == count.end())
		freq = 0;
	    else
		freq = cit->second;
	    if(it->second == NULL)
		output << " NULL reference associated to " << it->first << " bytes blocks !?!?! (number of requests " << freq << ")" << endl;
	    else
		output << " Usage for " << it->first << " bytes blocks : " << it->second->max_percent_full() << " % (number of requests " << freq << ")" <<endl;
	    ++it;
	}
	output << " ---------------------------------------------------- " << endl;
    }
#endif


	//////////////////////////////////////////////
	// main object for memory management
	//

    static global_alloc main_alloc;



	//////////////////////////////////////////////
	// exported routine
	//

    void *special_alloc_new(size_t taille)
    {
        void *ret = NULL;

	CRITICAL_START;

	try
	{
	    ret = main_alloc.alloc(taille);
	    memory_check_special_report_new(ret, taille);
	}
	catch(...)
	{
	    CRITICAL_END;
	    throw;
	}
	CRITICAL_END;

        return ret;
    }

    static void dummy_call(char *x)
    {
        static char id[]="$Id: special_alloc.cpp,v 1.18.2.6 2012/03/10 20:57:21 edrusb Rel $";
        dummy_call(id);
    }

    void special_alloc_delete(void *ptr)
    {
	CRITICAL_START;

	try
	{
	    memory_check_special_report_delete(ptr);
	    main_alloc.release(ptr);
	}
	catch(...)
	{
	    CRITICAL_END;
	    throw;
	}

	CRITICAL_END;
    }

    void special_alloc_init_for_thread_safe()
    {
#ifdef MUTEX_WORKS
	if(alloc_mutex_initialized)
	    throw SRC_BUG; // should not be initialized twice
	alloc_mutex_initialized = true;
	    // we must not be in multi-thread
	    // environment at that time.

	if(pthread_mutex_init(&alloc_mutex, NULL) < 0)
	{
	    alloc_mutex_initialized = false;
	    throw Erange("special_alloca_init_for_thread_safe", string(gettext("Cannot initialize mutex: ")) + strerror(errno));
	}
#endif
    }

    void special_alloc_garbage_collect(ostream & output)
    {
#ifdef MUTEX_WORKS
#ifdef LIBDAR_DEBUG_MEMORY
	main_alloc.max_percent_full(output);
#endif
	main_alloc.garbage_collect();
#ifdef LIBDAR_NO_OPTIMIZATION
	if(!main_alloc.is_empty())
	    main_alloc.dump(output);
#endif
#endif
    }

} // end of namespace

#endif
