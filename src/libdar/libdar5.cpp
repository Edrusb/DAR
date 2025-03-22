/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2025 Denis Corbin
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// to contact the author, see the AUTHOR file
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if LIBLZO2_AVAILABLE
#if HAVE_LZO_LZO1X_H
#include <lzo/lzo1x.h>
#endif
#endif

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#if HAVE_GCRYPT_H
#ifndef GCRYPT_NO_DEPRECATED
#define GCRYPT_NO_DEPRECATED
#endif
#include <gcrypt.h>
#endif

#if HAVE_TIME_H
#include <time.h>
#endif

#if HAVE_GPGME_H
#include <gpgme.h>
#endif

} // end extern "C"

#include <string>
#include <iostream>
#include "libdar5.hpp"
#include "erreurs.hpp"
#include "infinint.hpp"
#include "generic_file.hpp"
#include "user_interaction5.hpp"
#include "archive5.hpp"
#include "nls_swap.hpp"
#ifdef __DYNAMIC__
#include "user_group_bases.hpp"
#endif
#include "get_version.hpp"

#include <list>

using namespace std;

#define WRAPPER_IN try {  //
#define WRAPPER_OUT(code,msg)						\
    code = LIBDAR_NOEXCEPT;						\
    }									\
				    catch(Ememory & e)			\
				    {					\
					code = LIBDAR_EMEMORY;		\
					msg = e.get_message();		\
				    }					\
				    catch(Ebug & e)			\
				    {					\
					code = LIBDAR_EBUG;		\
					msg = e.get_message();		\
				    }					\
				    catch(Einfinint & e)		\
				    {					\
					code = LIBDAR_EINFININT;	\
					msg = e.get_message();		\
				    }					\
				    catch(Elimitint & e)		\
				    {					\
					code = LIBDAR_ELIMITINT;	\
					msg = e.get_message();		\
				    }					\
				    catch(Erange & e)			\
				    {					\
					code = LIBDAR_ERANGE;		\
					msg = e.get_message();		\
				    }					\
				    catch(Edeci & e)			\
				    {					\
					code = LIBDAR_EDECI;		\
					msg = e.get_message();		\
				    }					\
				    catch(Efeature & e)			\
				    {					\
					code = LIBDAR_EFEATURE;		\
					msg = e.get_message();		\
				    }					\
				    catch(Ehardware & e)		\
				    {					\
					code = LIBDAR_EHARDWARE;	\
					msg = e.get_message();		\
				    }					\
				    catch(Euser_abort & e)		\
				    {					\
					code = LIBDAR_EUSER_ABORT;	\
					msg = e.get_message();		\
				    }					\
				    catch(Edata & e)			\
				    {					\
					code = LIBDAR_EDATA;		\
					msg = e.get_message();		\
				    }					\
				    catch(Escript & e)			\
				    {					\
					code = LIBDAR_ESCRIPT;		\
					msg = e.get_message();		\
				    }					\
				    catch(Elibcall & e)			\
				    {					\
					code = LIBDAR_ELIBCALL;		\
					msg = e.get_message();		\
				    }					\
				    catch(Ecompilation & e)		\
				    {					\
					code = LIBDAR_ECOMPILATION;	\
					msg = e.get_message();		\
				    }					\
				    catch(Ethread_cancel & e)		\
				    {					\
					code = LIBDAR_THREAD_CANCEL;	\
					msg = e.get_message();		\
				    }					\
				    catch(Egeneric & e)			\
				    {/*unknown Egeneric exception*/	\
					code = LIBDAR_EBUG;		\
					msg = string(gettext("Caught an unknown Egeneric exception: ")) + e.get_message(); \
				    }					\
				    catch(...)				\
				    {/* unknown Egeneric exception*/	\
					code = LIBDAR_UNKNOWN;		\
					msg = gettext("Caught a none libdar exception"); \
				    }                              //


namespace libdar5
{

    void get_version(U_I & major, U_I & medium, U_I & minor, bool init_libgcrypt)
    {
	libdar::get_version(major, medium, minor, init_libgcrypt);
	major = LIBDAR_COMPILE_TIME_MAJOR;
	medium = LIBDAR_COMPILE_TIME_MEDIUM;
	minor = LIBDAR_COMPILE_TIME_MINOR;
    }

    void get_version_noexcept(U_I & major, U_I & medium, U_I & minor, U_16 & exception, std::string & except_msg, bool init_libgcrypt)
    {
	NLS_SWAP_IN;
	WRAPPER_IN
	    get_version(major, medium, minor, init_libgcrypt);
	WRAPPER_OUT(exception, except_msg)
	NLS_SWAP_OUT;
    }

    void close_and_clean()
    {
	libdar::close_and_clean();
    }

    archive* open_archive_noexcept(user_interaction & dialog,
				   const path & chem,
				   const std::string & basename,
				   const std::string & extension,
				   const archive_options_read & options,
				   U_16 & exception,
				   std::string & except_msg)
    {
	archive *ret = nullptr;
        NLS_SWAP_IN;
	WRAPPER_IN
	    ret = new (nothrow) archive(dialog, chem,  basename,  extension,
					options);
  	    if(ret == nullptr)
		throw Ememory("open_archive_noexcept");
	WRAPPER_OUT(exception, except_msg)
	NLS_SWAP_OUT;
	return ret;
    }

    archive *create_archive_noexcept(user_interaction & dialog,
				     const path & fs_root,
				     const path & sauv_path,
				     const std::string & filename,
				     const std::string & extension,
				     const archive_options_create & options,
				     statistics * progressive_report,
				     U_16 & exception,
				     std::string & except_msg)
    {
	archive *arch_ret = nullptr;
	NLS_SWAP_IN;
	WRAPPER_IN
	    arch_ret = new (nothrow) archive(dialog,
					     fs_root,
					     sauv_path,
					     filename,
					     extension,
					     options,
					     progressive_report);
  	    if(arch_ret == nullptr)
		throw Ememory("open_archive_noexcept");
	WRAPPER_OUT(exception, except_msg)
        NLS_SWAP_OUT;
	return arch_ret;
    }


    void op_isolate_noexcept(user_interaction & dialog,
			     archive *ptr,
			     const path &sauv_path,
			     const std::string & filename,
			     const std::string & extension,
			     const archive_options_isolate & options,
			     U_16 & exception,
			     std::string & except_msg)
    {
	NLS_SWAP_IN;
	WRAPPER_IN
	    if(ptr == nullptr)
		throw Elibcall("op_isolate_noexcept", gettext("Invald nullptr argument given to 'ptr'"));
	ptr->op_isolate(dialog,
			sauv_path,
			filename,
			extension,
			options);
	WRAPPER_OUT(exception, except_msg)
	    NLS_SWAP_OUT;
    }

    archive *merge_archive_noexcept(user_interaction & dialog,
				    const path & sauv_path,
				    archive *ref_arch1,
				    const std::string & filename,
				    const std::string & extension,
				    const archive_options_merge & options,
				    statistics * progressive_report,
				    U_16 & exception,
				    std::string & except_msg)
    {
	archive *ret = nullptr;
	NLS_SWAP_IN;
	WRAPPER_IN
	    ret = new (nothrow) archive(dialog,
					sauv_path,
					ref_arch1,
					filename,
					extension,
					options,
					progressive_report);
  	    if(ret == nullptr)
		throw Ememory("open_archive_noexcept");
	WRAPPER_OUT(exception, except_msg)
	    NLS_SWAP_OUT;
	return ret;
    }




    void close_archive_noexcept(archive *ptr,
				U_16 & exception,
				std::string & except_msg)
    {
	NLS_SWAP_IN;
	WRAPPER_IN
	    if(ptr == nullptr)
		throw Elibcall("close_archive_noexcept", gettext("Invalid nullptr pointer given to close_archive"));
	    else
	    {
		delete ptr;
		ptr = nullptr;
	    }
	WRAPPER_OUT(exception, except_msg)
	    NLS_SWAP_OUT;
    }


    statistics op_extract_noexcept(user_interaction & dialog,
				   archive *ptr,
				   const path &fs_root,
				   const archive_options_extract & options,
				   statistics * progressive_report,
				   U_16 & exception,
				   std::string & except_msg)
    {
	statistics ret;
	NLS_SWAP_IN;
	WRAPPER_IN
	    if(ptr == nullptr)
		throw Elibcall("op_extract_noexcept", gettext("Invalid nullptr argument given to 'ptr'"));
	ret = ptr->op_extract(dialog,
			      fs_root,
			      options,
			      progressive_report);
	WRAPPER_OUT(exception, except_msg)
	    NLS_SWAP_OUT;
	return ret;
    }


    void op_listing_noexcept(user_interaction & dialog,
			     archive *ptr,
			     const archive_options_listing & options,
			     U_16 & exception,
			     std::string & except_msg)
    {
	NLS_SWAP_IN;
	WRAPPER_IN
	    if(ptr == nullptr)
		throw Elibcall("op_extract_noexcept", gettext("Invalid nullptr argument given to 'ptr'"));
	ptr->op_listing(dialog,
			options);
	WRAPPER_OUT(exception, except_msg)
	    NLS_SWAP_OUT;
    }

    statistics op_diff_noexcept(user_interaction & dialog,
				archive *ptr,
				const path & fs_root,
				const archive_options_diff & options,
				statistics * progressive_report,
				U_16 & exception,
				std::string & except_msg)
    {
	statistics ret;
	NLS_SWAP_IN;
	WRAPPER_IN
	    if(ptr == nullptr)
		throw Elibcall("op_extract_noexcept", gettext("Invalid nullptr argument given to 'ptr'"));
	ret = ptr->op_diff(dialog,
			   fs_root,
			   options,
			   progressive_report);
	WRAPPER_OUT(exception, except_msg)
	    NLS_SWAP_OUT;
	return ret;
    }


    statistics op_test_noexcept(user_interaction & dialog,
				archive *ptr,
				const archive_options_test & options,
				statistics * progressive_report,
				U_16 & exception,
				std::string & except_msg)
    {
	statistics ret;
	NLS_SWAP_IN;
	WRAPPER_IN
	    if(ptr == nullptr)
		throw Elibcall("op_extract_noexcept", gettext("Invalid nullptr argument given to 'ptr'"));
	ret = ptr->op_test(dialog,
			   options,
			   progressive_report);
	WRAPPER_OUT(exception, except_msg)
	    NLS_SWAP_OUT;
	return ret;
    }


    bool get_children_of_noexcept(user_interaction & dialog,
				  archive *ptr,
				  const std::string & dir,
				  U_16 & exception,
				  std::string & except_msg)
    {
	bool ret = false;
	NLS_SWAP_IN;
	WRAPPER_IN
	    if(ptr == nullptr)
		throw Elibcall("op_extract_noexcept", gettext("Invalid nullptr argument given to 'ptr'"));
	ret = ptr->get_children_of(dialog,
				   dir);
	WRAPPER_OUT(exception, except_msg)
	    NLS_SWAP_OUT;
	return ret;
    }

    char *libdar_str2charptr_noexcept(const std::string & x, U_16 & exception, std::string & except_msg)
    {
        char *ret = nullptr;
        NLS_SWAP_IN;
        WRAPPER_IN
	    ret = libdar::tools_str2charptr(x);
	WRAPPER_OUT(exception, except_msg)
	    NLS_SWAP_OUT;
	return ret;
    }

#if MUTEX_WORKS
    void cancel_thread(pthread_t tid, bool immediate, U_64 flag)
    {
	libdar::cancel_thread(tid, immediate, flag);
    }

    bool cancel_status(pthread_t tid)
    {
	return libdar::cancel_status(tid);
    }

    bool cancel_clear(pthread_t tid)
    {
	return libdar::cancel_clear(tid);
    }
#endif


} // end of namespace
