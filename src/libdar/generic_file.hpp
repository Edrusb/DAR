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

    /// \file generic_file.hpp
    /// \brief class generic_file is defined here as well as class fichier
    ///
    /// the generic_file interface is widely used in libdar
    /// it defines the standard way of transmitting data between different
    /// part of the library
    /// - compression engine
    /// - encryption engine
    /// - exchange through pipes
    /// - slicing
    /// - dry run operations
    /// - file access
    /// .
    /// \ingroup Private


///////////////////////////////////////////////////////////////////////
// IMPORTANT : THIS FILE MUST ALWAYS BE INCLUDE AFTER infinint.hpp   //
//             (and infinint.hpp must be included too, always)       //
///////////////////////////////////////////////////////////////////////
#include "infinint.hpp"
///////////////////////////////////////////////////////////////////////



#ifndef GENERIC_FILE_HPP
#define GENERIC_FILE_HPP


#include "../my_config.h"

extern "C"
{
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
} // end extern "C"

#include "path.hpp"
#include "integers.hpp"
#include "user_interaction.hpp"
#include "thread_cancellation.hpp"

#include <string>

namespace libdar
{

	/// \addtogroup Private
	/// @{

    const int CRC_SIZE = 2;
    typedef char crc[CRC_SIZE];
    extern void clear(crc & value);
    extern void copy_crc(crc & dst, const crc & src);
    extern bool same_crc(const crc &a, const crc &b);
    extern std::string crc2str(const crc &a);

	/// generic_file openning modes
    enum gf_mode
    {
	gf_read_only,  ///< read only access
	gf_write_only, ///< write only access
	gf_read_write  ///< read and write access
    };


    extern gf_mode generic_file_get_mode(S_I fd);
    extern const char * generic_file_get_name(gf_mode mode);

	/// this is the interface class from which all other data transfer classes inherit

	/// it provides mainly read and write operations,
	/// skip operations and few other functions.
	/// \note
	/// the read and write method are similar to the read and write system calls
	/// except that they never return negative values, but throw exception instead.
	/// returning zero means end of generic_file. The call is blocked if
	/// no data is available for reading.
	/// write returns the number of bytes written, and never make partial writtings.
	/// Thus, it is blocked until all bytes are written or occures an exception
	/// inconsequences the returned value is always the value of the argument
	/// "size".
    class generic_file
    {
    public :
	    /// main constructor
        generic_file(user_interaction & dialog, gf_mode m) { rw = m; clear(value); crc_offset = 0; enable_crc(false); gf_ui = dialog.clone(); };
	    /// copy constructor
	generic_file(const generic_file &ref) { copy_from(ref); };
	    /// assignment operator
	generic_file & operator = (const generic_file & ref) { detruire(); copy_from(ref); return *this; };
	    /// destructor
        virtual ~generic_file() { detruire(); };

	    /// retreive the openning mode for this object
        gf_mode get_mode() const { return rw; };

	    /// read data from the generic_file
        S_I read(char *a, size_t size);
	    /// write data to the generic_file
        S_I write(const char *a, size_t size);
	    /// write a string to the generic_file
        S_I write(const std::string & arg);
	    /// skip back one char, read on char and skip back one char
        S_I read_back(char &a);
	    /// read one char
	S_I read_forward(char &a) { return read(&a, 1); };
	    /// skip at the absolute position
        virtual bool skip(const infinint & pos) = 0;
	    /// skip to the end of file
        virtual bool skip_to_eof() = 0;
	    /// skip relatively to the current position
        virtual bool skip_relative(S_I x) = 0;
	    /// get the current read/write position
        virtual infinint get_position() = 0;

	    /// copy all data from current position to the object in argument
        void copy_to(generic_file &ref);
	    /// copy all data from the current position to the object in argument and computes a CRC value of the transmitted data
        void copy_to(generic_file &ref, crc & value);
	    /// small copy (up to 4GB) with CRC calculation
        U_32 copy_to(generic_file &ref, U_32 size); // returns the number of byte effectively copied
	    /// copy the given amount to the object in argument
        infinint copy_to(generic_file &ref, infinint size); // returns the number of byte effectively copied
	    /// compares the contents with the object in argument
        bool diff(generic_file & f); // return true if arg differs from "this"

            /// reset CRC on read or writen data
        void reset_crc();
	    /// get CRC of the transfered date since last reset
        void get_crc(crc & val) { enable_crc(false); copy_crc(val, value); };

	    /// get the cached user_interaction for inherited classes in particular (this stay a public method, not a protected one)
	user_interaction & get_gf_ui() const { return *gf_ui; };

    protected :
        void set_mode(gf_mode x) { rw = x; };
        virtual S_I inherited_read(char *a, size_t size) = 0;
            // must provide as much byte as requested up to end of file
            // stay blocked if not enough available
            // returning zero or less than requested means end of file
        virtual S_I inherited_write(const char *a, size_t size) = 0;
            // must write all data or block or throw exceptions
            // thus always returns the second argument

    private :
        gf_mode rw;
        crc value;
        S_I crc_offset;
	user_interaction *gf_ui;
        S_I (generic_file::* active_read)(char *a, size_t size);
        S_I (generic_file::* active_write)(const char *a, size_t size);

        void enable_crc(bool mode);
        void compute_crc(const char *a, S_I size);
        S_I read_crc(char *a, size_t size);
        S_I write_crc(const char *a, size_t size);

	void detruire() { if(gf_ui != NULL) delete gf_ui; };
	void copy_from(const generic_file & ref);
    };

	/// this is a full implementation of a generic_file applied to a plain file
    class fichier : public generic_file, public thread_cancellation
    {
    public :
        fichier(user_interaction & dialog, S_I fd);
        fichier(user_interaction & dialog, const char *name, gf_mode m);
        fichier(user_interaction & dialog, const path & chemin, gf_mode m);
        ~fichier() { close(filedesc); };

        infinint get_size() const;

            // herite de generic_file
        bool skip(const infinint & pos);
        bool skip_to_eof();
        bool skip_relative(S_I x);
        infinint get_position();

    protected :
        S_I inherited_read(char *a, size_t size);
        S_I inherited_write(const char *a, size_t size);

    private :
        S_I filedesc;

        void open(const char *name, gf_mode m);
    };

#define CONTEXT_INIT "init"
#define CONTEXT_OP   "operation"
#define CONTEXT_LAST_SLICE  "last_slice"

	/// the contextual class adds the information of phases in the generic_file

	/// two phases are defined
	/// - INIT phase
	/// - OPERATIONAL phase
	/// .
	/// these are used to help the command launched between slices to
	/// decide the action to do depending on the context when reading an archive
	/// (first slice / last slice read, ...)
	/// the context must also be transfered to dar_slave through the pair of tuyau objects
    class contextual : public generic_file
    {
    public :
	contextual(user_interaction & dialog, gf_mode m) : generic_file(dialog, m) {};

	virtual void set_info_status(const std::string & s) = 0;
        virtual std::string get_info_status() const = 0;
    };

	/// @}

} // end of namespace

#endif
