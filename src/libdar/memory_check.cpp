//*********************************************************************/
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

#include "erreurs.hpp"
#include "memory_check.hpp"

#ifdef LIBDAR_DEBUG_MEMORY
extern "C"
{
#include <stdlib.h>
#include <stdio.h>
}

#define DEBUG_MEM_OUTPUT_FILE "/tmp/dar_debug_mem_allocation.txt"
#define DEBUG_MEM_SIZE 10240000

using namespace std; // for output flow
using namespace libdar; // required to use SRC_BUG

struct debug_mem
{
    void *ptr;
    size_t size;
};

static bool debug_mem_initialized = false; //< whether module has been initialized
static debug_mem debug_mem_alloc[DEBUG_MEM_SIZE]; //< stores all current memory allocation requests
static long long debug_mem_next = 0; //< pointer to the next most probable place to find an free slot
static long long debug_mem_allocated = 0, debug_mem_max_allocated = 0, debug_mem_block = 0, debug_mem_max_block = 0;
static FILE *debug_mem_output = NULL;

#define ERROR(MSG)  debug_mem_error(__FILE__, __LINE__, MSG)

static void debug_mem_error(const char *file, unsigned int line, const char *msg)
{
    fprintf(stderr, "DEBUG MEMORY ERROR: File %s line %u : %s\n", file, line, msg);
    exit(1);
}

static void debug_mem_init()
{
    for(long long i = 0; i < DEBUG_MEM_SIZE; ++i)
    {
	debug_mem_alloc[i].ptr = NULL;
	debug_mem_alloc[i].size = 0;
    }
    debug_mem_output = fopen(DEBUG_MEM_OUTPUT_FILE, "w");
    if(debug_mem_output == NULL)
	   ERROR("failed openning output file to log debug memory events");
    debug_mem_initialized = true;
}

static long long debug_mem_find_free_slot()
{
    long long ret = debug_mem_next;

    while(ret < DEBUG_MEM_SIZE && debug_mem_alloc[ret].ptr != NULL)
	++ret;

    if(ret >= DEBUG_MEM_SIZE)
	ret = 0;

    while(ret < DEBUG_MEM_SIZE && debug_mem_alloc[ret].ptr != NULL)
	++ret;

    if(ret < DEBUG_MEM_SIZE)
	debug_mem_next = ret + 1;
    else
	 ERROR("Cannot handle that many blocks, need an increase of the table size in memory_check module");

    return ret;
}

static long long debug_mem_find_slot(void *ptr)
{
    long long ret = 0;

    while(ret < DEBUG_MEM_SIZE && debug_mem_alloc[ret].ptr != ptr)
	++ret;

    if(ret >= DEBUG_MEM_SIZE)
	ERROR("unknown memory block asked to be released");

    return ret;
}

static void debug_mem_report(void *ptr, const char *msg)
{
    static long long openum = 0;
    fprintf(debug_mem_output,
	    "[%04lld %s %p | %lld currently allocated blocks (max = %lld). Total size = %lld bytes (max reached so far = %lld bytes)\n",
	    ++openum,
	    msg,
	    ptr,
	    debug_mem_block,
	    debug_mem_max_block,
	    debug_mem_allocated,
	    debug_mem_max_allocated);
}

static void *alloc(size_t size)
{
    void *ret = malloc(size);

    if(ret != NULL)
    {
	long long slot;

	if(!debug_mem_initialized)
	    debug_mem_init();

	    // recording new entry in the map
	slot = debug_mem_find_free_slot();
	debug_mem_alloc[slot].size = size;
	debug_mem_alloc[slot].ptr = ret;


	    // computing current and max allocated space
	debug_mem_allocated += size;
	if(debug_mem_allocated > debug_mem_max_allocated)
	    debug_mem_max_allocated = debug_mem_allocated;
	++debug_mem_block;
	if(debug_mem_block > debug_mem_max_block)
	    debug_mem_max_block = debug_mem_block;
	debug_mem_report(ret, "NEW");
    }
    return ret;
}

void desalloc(void *p)
{
    long long slot = debug_mem_find_slot(p);

    debug_mem_allocated -= debug_mem_alloc[slot].size;
    --debug_mem_block;
    debug_mem_alloc[slot].ptr = NULL;
    debug_mem_alloc[slot].size = 0;
    free(p);
    debug_mem_report(p, "DEL");
}

void * operator new (size_t size) throw (std::bad_alloc)
{
    void *ret = alloc(size);
    if(ret == NULL)
	throw std::bad_alloc();
    return ret;
}

void * operator new (size_t size, const std::nothrow_t& nothrow_constant) throw ()
{
    return alloc(size);
}

void * operator new[] (size_t size) throw (std::bad_alloc)
{
    void *ret = alloc(size);
    if(ret == NULL)
	throw std::bad_alloc();
    return ret;
}

void * operator new[] (size_t size, const std::nothrow_t& nothrow_constant) throw ()
{
    return alloc(size);
}


void operator delete (void * p) throw()
{
    desalloc(p);
}

void operator delete (void * p, const std::nothrow_t& nothrow_constant) throw()
{
    desalloc(p);
}

void operator delete[] (void * p) throw()
{
    desalloc(p);
}

void operator delete[] (void * p, const std::nothrow_t& nothrow_constant) throw()
{
    desalloc(p);
}

#endif

void memory_check_snapshot()
{
#ifdef LIBDAR_DEBUG_MEMORY
    long long curs = 0;

    fprintf(debug_mem_output, "DUMP START OF THE CURRENT ALLOCATED BLOCKS\n");
    while(curs < DEBUG_MEM_SIZE)
    {
	if(debug_mem_alloc[curs].ptr != NULL)
	    fprintf(debug_mem_output, "address = %p   size = %ld\n",
		    debug_mem_alloc[curs].ptr,
		    debug_mem_alloc[curs].size);
	++curs;
    }
    fprintf(debug_mem_output, "DUMP end OF THE CURRENT ALLOCATED BLOCKS\n");
#endif
}

void memory_check_special_report_new(const void *ptr, unsigned long taille)
{
#ifdef LIBDAR_DEBUG_MEMORY
    fprintf(debug_mem_output, "SPECIAL NEW: %p\t size = %ld\n",
	    ptr,
	    taille);
#endif
}

void memory_check_special_report_delete(const void *ptr)
{
#ifdef LIBDAR_DEBUG_MEMORY
    fprintf(debug_mem_output, "SPECIAL DEL: %p\n",
	    ptr);
#endif
}

void memory_check_special_new_sized(unsigned long size)
{
#ifdef LIBDAR_DEBUG_MEMORY
    fprintf(debug_mem_output, "SPECIAL SIZED created: %ld\n",size);
#endif
}
