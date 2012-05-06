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
// $Id: compressor.hpp,v 1.15 2002/05/28 20:17:29 denis Rel $
//
/*********************************************************************/

#ifndef COMPRESSOR_HPP
#define COMPRESSOR_HPP

#pragma interface

#include <zlib.h>
#include "generic_file.hpp"

enum compression { none = 'n', zip = 'p', gzip = 'z', bzip2 = 'y' };

extern compression char2compression(char a);
extern char compression2char(compression c);
extern string compression2string(compression c);

class compressor : public generic_file
{
public :
    compressor(compression algo, generic_file & compressed_side);
	// compressed_side is not owned by the object and will remains
	// after the objet destruction
    compressor(compression algo, generic_file *compressed_side);
	// compressed_side is owned by the object and will be 
	// deleted a destructor time
    ~compressor();
    
    void flush_write(); // flush all data to compressed_side, and reset the compressor 
	// for that additional write can be uncompresssed starting at this point.
    void flush_read(); // reset decompression engine to be able to read the next block of compressed data 
	// if not called, furthur read return EOF
    void clean_read(); // discard any byte buffered and not yet returned by read()
    void clean_write(); // discard any byte buffered and not yet wrote to compressed_side;
    
	// inherited from generic file	 
    bool skip(infinint position) { flush_write(); flush_read(); clean_read(); return compressed->skip(position); };
    bool skip_to_eof()  { flush_write(); flush_read(); clean_read(); return compressed->skip_to_eof(); };
    bool skip_relative(signed int x) { flush_write(); flush_read(); clean_read(); return compressed->skip_relative(x); };
    infinint get_position() { return compressed->get_position(); };

protected :
    int inherited_read(char *a, size_t size) { return (this->*read_ptr)(a, size); };
    int inherited_write(char *a, size_t size) { return (this->*write_ptr)(a, size); };

private :
    struct xfer 
    {
	z_streamp strm;
	char *buffer;
	unsigned int size;

	xfer(unsigned int sz);
	~xfer();
    };
    
    xfer *compr, *decompr;
    generic_file *compressed;
    int tube[2];
    bool compressed_owner;

    void init(compression algo, generic_file *compressed_side);
    int (compressor::*read_ptr) (char *a, size_t size);
    int none_read(char *a, size_t size);
    int gzip_read(char *a, size_t size);
	// int zip_read(char *a, size_t size);
	// int bzip2_read(char *a, size_t size);
    
    int (compressor::*write_ptr) (char *a, size_t size);
    int none_write(char *a, size_t size);
    int gzip_write(char *a, size_t size);
	// int zip_write(char *a, size_t size);
	// int bzip2_write(char *a, size_t size);
};

#endif

