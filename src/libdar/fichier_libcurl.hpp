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

    /// \file fichier_libcurl.hpp
    /// \brief class fichier_libcurl definition. This is a full implementation/inherited class of class fichier_global
    /// this type of object are generated by entrepot_libcurl.
    /// \ingroup Private

#ifndef FICHIER_LIBCURL_HPP
#define FICHIER_LIBCURL_HPP


#include "../my_config.h"

extern "C"
{
#if LIBCURL_AVAILABLE
#if HAVE_CURL_CURL_H
#include <curl/curl.h>
#endif
#endif
} // end extern "C"

#include <string>

#include "integers.hpp"
#include "thread_cancellation.hpp"
#include "label.hpp"
#include "user_interaction.hpp"
#include "fichier_global.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

#if LIBCURL_AVAILABLE

	/// libcurl remote files

    class fichier_libcurl : public fichier_global
    {
    public:

	    /// constructor
	fichier_libcurl(user_interaction & dialog,  //< for user interaction requested by fichier_global
			const std::string & chemin, //< full path of the file to open
			CURL *ref_handle,           //< the handle passes to the responsibility of the fichier_libcurl object once fully constructed
			gf_mode m,                  //< open mode
			U_I waiting,                //< retry timeout in case of network error
			bool force_permission,      //< whether file permission should be modified
			U_I permission,             //< file permission to enforce if force_permission is set
			bool erase);                //< whether to erase the file before writing to it

	fichier_libcurl(const fichier_libcurl & ref) : fichier_global(ref) { copy_from(ref); };

	    /// assignment operator
	const fichier_libcurl & operator = (const fichier_libcurl & ref) { detruit(); copy_parent_from(ref); copy_from(ref); return *this; };

	    /// destructor
	~fichier_libcurl() { detruit(); };

	    /// change the permission of the file
	virtual void change_permission(U_I perm);

	    /// set the ownership of the file
	virtual void change_ownership(const std::string & user, const std::string & group) { throw Efeature(gettext("user/group ownership not supported for this repository")); }; // not supported

	    /// return the size of the file
        infinint get_size() const;

	    /// set posix_fadvise for the whole file
	virtual void fadvise(advise adv) const {}; // not supported and ignored

            // inherited from generic_file
	bool skippable(skippability direction, const infinint & amount);
        bool skip(const infinint & pos);
        bool skip_to_eof();
        bool skip_relative(S_I x);
        infinint get_position() const { return current_offset; };

    protected:
	    // inherited from generic_file grand-parent class
	void inherited_read_ahead(const infinint & amount);
	void inherited_sync_write();
	void inherited_flush_read();
	void inherited_terminate();

	    // inherited from fichier_global parent class
	U_I fichier_global_inherited_write(const char *a, U_I size);
        bool fichier_global_inherited_read(char *a, U_I size, U_I & read, std::string & message);

    private:
	static const U_I tampon_size = CURL_MAX_WRITE_SIZE;

	CURL *easyhandle;                 //< easy handle that we modify when necessary
	CURLM *multihandle;               //< multi handle to which is added the easyhandle for step by step libcurl interaction
	bool easy_in_multi;               //< whether easyhandle is added to multihandle
	bool metadatamode;                //< wether we are acting on metadata rather than file's data
	infinint current_offset;          //< current offset we are reading / writing at
	bool has_maxpos;                  //< true if maxpos is set
	infinint maxpos;                  //< in read mode this is the filesize, in write mode this the offset where to append data (not ovewriting)
	char tampon[tampon_size];         //< data in transit to / from libcurl
	U_I inbuf;                        //< amount of byte available in "tampon"
	bool eof;                         //< whether we have reached end of file (read mode)
	bool append_write;                //< whether we should append to data (and not replace) when uploading
	char meta_tampon[tampon_size];    //< trash in transit data used to carry metadata
	U_I meta_inbuf;                   //< amount of byte available in "meta_tampon"
	char *ptr_tampon;                 //< points to tampon or meta_tampon depending on the metadata mode
	U_I *ptr_inbuf;                   //< points to inbuf or meta_in_buf depending on the metadata mod
	U_I ptr_tampon_size;              //< the allocated size in bytes of buffer pointed to by ptr_tampon
	U_I wait_delay;                   //< time in second to wait before retrying in case of network error


	void add_easy_to_multi();
	void remove_easy_from_multi();
	void switch_to_metadata(bool mode);//< set to true to get or set file's metadata, false to read/write file's data
	void run_multi(const std::string & context_msg) const;           //< blindly run multihandle up to the end of transfer
	void copy_from(const fichier_libcurl & ref);
	void copy_parent_from(const fichier_libcurl & ref);
	void my_multi_perform(int & running, const std::string & context_msg); //< robust wrapper to curl_multi_perform to face temporary network error conditions
	bool check_info_after_multi_perform(const std::string & context_msg) const; //< depending on error type, wait and retry or throw an exception, return false if no error was met
	void detruit();

	static size_t write_data_callback(char *buffer, size_t size, size_t nmemb, void *userp);
	static size_t read_data_callback(char *bufptr, size_t size, size_t nitems, void *userp);
	static size_t null_callback(char *bufptr, size_t size, size_t nmemb, void *userp) { return size*nmemb; };
    };

	/// helper function to handle libcurl error code
	/// wait or throw an exception depending on error condition
	///
	/// \param[in] dialog used to report the reason we are waiting for and how much time we wait
	/// \param[in] err is the curl easy code to examin
	/// \param[in] wait_seconds is the time to wait for recoverable error
	/// \param[in] err_context is the error context message use to prepend waiting message or exception throw
    extern void fichier_libcurl_check_wait_or_throw(user_interaction & dialog,
						    CURLcode err,
						    U_I wait_seconds,
						    const std::string & err_context);

#endif
	/// @}

} // end of namespace

#endif
