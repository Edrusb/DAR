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
// $Id: compressor.cpp,v 1.19 2002/05/28 20:17:29 denis Rel $
//
/*********************************************************************/

#pragma implementation

#include <zlib.h>
#include <signal.h>
#include <pthread.h>
#include "compressor.hpp"

#define BUFFER_SIZE 102400

compressor::compressor(compression algo, generic_file & compressed_side) : generic_file(compressed_side.get_mode())
{
    init(algo, &compressed_side);
    compressed_owner = false;
}

compressor::compressor(compression algo, generic_file *compressed_side) : generic_file(compressed_side->get_mode())
{
    init(algo, compressed_side);
    compressed_owner = true;
}

void compressor::init(compression algo, generic_file *compressed_side)
{
    compr = decompr = NULL;
    switch(algo)
    {
    case none :
	read_ptr = &compressor::none_read;
	write_ptr = &compressor::none_write;
	break;
    case zip :
	throw Efeature("zip compression not implemented");
	break;
    case gzip :
	read_ptr = &compressor::gzip_read;
	write_ptr = &compressor::gzip_write;
	compr = new xfer(BUFFER_SIZE);
	if(compr == NULL)
	    throw Ememory("compressor::compressor");
	decompr = new xfer(BUFFER_SIZE);
	if(decompr == NULL)
	{
	    delete compr;
	    throw Ememory("compressor::compressor");
	}

	switch(deflateInit(compr->strm, 9))
	{
	case Z_OK:
	    break;
	case Z_MEM_ERROR:
	    delete compr;
	    delete decompr;
	    throw Ememory("compressor::compressor");
	case Z_VERSION_ERROR:
	    delete compr;
	    delete decompr;
	    throw Erange("compressor::compressor", "incompatible Zlib version");
	case Z_STREAM_ERROR:
	default:
	    delete compr;
	    delete decompr;
	    throw SRC_BUG;
	}

	switch(inflateInit(decompr->strm))
	{
	case Z_OK:
	    decompr->strm->avail_in = 0;
	    break;
	case Z_MEM_ERROR:
	    deflateEnd(compr->strm);
	    delete compr;
	    delete decompr;
	    throw Ememory("compressor::compressor");
	case Z_VERSION_ERROR:
	    deflateEnd(compr->strm);
	    delete compr;
	    delete decompr;
	    throw Erange("compressor::compressor", "incompatible Zlib version");
	case Z_STREAM_ERROR:
	default:
	    deflateEnd(compr->strm);
	    delete compr;
	    delete decompr;
	    throw SRC_BUG;
	}

	break;
    case bzip2 :
	throw Efeature("bzip2 compression not implemented");
	break;
    default :
	throw SRC_BUG;
    }

    compressed = compressed_side;
}

compressor::~compressor()
{
    if(compr != NULL)
    {
	int ret;

	    // flushing the pending data
	flush_write();

	ret = deflateEnd(compr->strm);
	delete compr;

	switch(ret)
	{
	case Z_OK:
	    break;
	case Z_DATA_ERROR: // some data remains in the compression pipe (data loss)
	    throw SRC_BUG;
	case Z_STREAM_ERROR:
	    throw Erange("compressor::~compressor", "compressed data is corrupted");
	default :
	    throw SRC_BUG;
	}
    }

    if(decompr != NULL)
    {
	    // flushing data
	flush_read();

	int ret = inflateEnd(decompr->strm);
	delete decompr;
	switch(ret)
	{
	case Z_OK:
	    break;
	default:
	    throw SRC_BUG;
	}
    }
    if(compressed_owner)
	delete compressed;
}

compressor::xfer::xfer(unsigned int sz)
{
    try 
    {
	buffer = new char[sz];
	if(buffer == NULL)
	    throw Ememory("");
	size = sz;

	strm = new z_stream;
	if(strm == NULL)
	{
	    delete buffer;
	    throw Ememory("");
	}
	strm->zalloc = NULL;
	strm->zfree = NULL;
	strm->opaque = NULL;
    }
    catch(Ememory &e)
    {
	throw Ememory("compressor::xfer::xfer");
    }
}

compressor::xfer::~xfer()
{
    delete buffer;
    delete strm;
}

int compressor::none_read(char *a, size_t size)
{
    return compressed->read(a, size);
}

int compressor::none_write(char *a, size_t size)
{
    return compressed->write(a, size);
}

int compressor::gzip_read(char *a, size_t size)
{
    int ret;
    int flag = Z_NO_FLUSH;

    if(size == 0)
	return 0;

    decompr->strm->next_out = (Bytef *)a;
    decompr->strm->avail_out = size;
  
    do
    {
	    // feeding the input buffer if necessary
	if(decompr->strm->avail_in == 0)
	{
	    decompr->strm->next_in = (Bytef *)decompr->buffer;
	    decompr->strm->avail_in = compressed->read(decompr->buffer, 
						       decompr->size);
	}

	ret = inflate(decompr->strm, flag);

	switch(ret)
	{
	case Z_OK:
	case Z_STREAM_END:
	    break;
	case Z_DATA_ERROR:
	    throw Erange("compressor::gzip_read", "gzip data CRC error");
	case Z_MEM_ERROR:
	    throw Ememory("compressor::gzip_read");
	case Z_BUF_ERROR:
		// no process is possible:
	    if(decompr->strm->avail_in == 0) // because we reached EOF
		ret = Z_STREAM_END; // zlib did not returned Z_STREAM_END (why ?) 
	    else // nothing explains why no process is possible:
		if(decompr->strm->avail_out == 0)
		    throw SRC_BUG; // bug from DAR: no output possible
		else
		    throw SRC_BUG; // unexpected comportment from zlib
	    break;
	default:
	    throw SRC_BUG;
	}
    }
    while(decompr->strm->avail_out > 0 && ret != Z_STREAM_END);

    return (char *)decompr->strm->next_out - a;
}

int compressor::gzip_write(char *a, size_t size)
{
    compr->strm->next_in = (Bytef *)a;
    compr->strm->avail_in = size;

    if(a == NULL)
	throw SRC_BUG;

    while(compr->strm->avail_in > 0)
    {
	    // making room for output
	compr->strm->next_out = (Bytef *)compr->buffer;
	compr->strm->avail_out = compr->size;

	switch(deflate(compr->strm, Z_NO_FLUSH))
	{
	case Z_OK:
	case Z_STREAM_END:
	    break;
	case Z_STREAM_ERROR:
	    throw SRC_BUG;
	case Z_BUF_ERROR:
	    throw SRC_BUG;
	default :
	    throw SRC_BUG;
	}

	if(compr->strm->next_out != (Bytef *)compr->buffer)
	    compressed->write(compr->buffer, (char *)compr->strm->next_out - compr->buffer);
    }

    return size;
}

void compressor::flush_write()
{
    int ret;

    if(compr != NULL && compr->strm->total_in != 0)  // zlib
    {
	    // no more input
	compr->strm->avail_in = 0;
	do 
	{
		// setting the buffer for reception of data
	    compr->strm->next_out = (Bytef *)compr->buffer;
	    compr->strm->avail_out = compr->size;

	    ret = deflate(compr->strm, Z_FINISH);
		
	    switch(ret)
	    {
	    case Z_OK :
	    case Z_STREAM_END:
		if((char *)compr->strm->next_out != compr->buffer)
		    compressed->write(compr->buffer, (char *)compr->strm->next_out - compr->buffer);
		break;
	    case Z_BUF_ERROR :
		throw SRC_BUG;
	    case Z_STREAM_ERROR :
		throw SRC_BUG;
	    default :
		throw SRC_BUG;
	    }
	}
	while(ret != Z_STREAM_END);   

	if(deflateReset(compr->strm) != Z_OK)
	    throw SRC_BUG;
    }
}

void compressor::flush_read()
{
    if(decompr != NULL) // zlib
	if(inflateReset(decompr->strm) != Z_OK)
	    throw SRC_BUG;
	// keep in the buffer the bytes already read, theses are discarded in case of a call to skip
}

void compressor::clean_read()
{
    if(decompr != NULL)
	decompr->strm->avail_in = 0;
}

static void dummy_call(char *x)
{
    static char id[]="$Id: compressor.cpp,v 1.19 2002/05/28 20:17:29 denis Rel $";
    dummy_call(id);
}

void compressor::clean_write()
{
    if(compr != NULL)
    {
	int ret;

	do
	{
	    compr->strm->next_out = (Bytef *)compr->buffer;
	    compr->strm->avail_out = compr->size;
	    compr->strm->avail_in = 0;
	    
	    ret = deflate(compr->strm, Z_FINISH);
	}
	while(ret == Z_OK);
    }
}

compression char2compression(char a)
{
    switch(a)
    {
    case 'n':
	return none;
    case 'p':
	return zip;
    case 'z':
	return gzip;
    case 'y':
	return bzip2;
    default :
	throw Erange("char2compression", "unknown compression");
    }
}

char compression2char(compression c)
{
    switch(c)
    {
    case none:
	return 'n';
    case zip:
	return 'p';
    case gzip:
	return 'z';
    case bzip2:
	return 'y';
    default :
	throw Erange("char2compression", "unknown compression");
    }
}

string compression2string(compression c)
{
    switch(c)
    {
    case none :
	return "none";
    case zip :
	return "zip";
    case gzip :
	return "gzip";
    case bzip2 :
	return "bzip2";
    default :
	throw Erange("compresion2char", "unknown compression");
    }
}

