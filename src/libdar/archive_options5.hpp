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

    /// \file archive_options5.hpp
    /// \brief API v5 compatible archive_options_* classes
    /// \ingroup API5

#ifndef ARCHIVE_OPTIONS5_HPP
#define ARCHIVE_OPTIONS5_HPP

#include "archive_options.hpp"
#include "archive_options_listing_shell.hpp"

namespace libdar5
{

	/////////////////////////////////////////////////////////
	////////////// OPTIONS FOR OPENNING AN ARCHIVE //////////
	/////////////////////////////////////////////////////////

	/// \addtogroup API5
	/// @{

    using libdar::entrepot;

    class archive_options_read: public libdar::archive_options_read
    {
    public:
	void set_entrepot(const entrepot & entr)
	{
	    std::shared_ptr<libdar::entrepot> tmp(entr.clone());
	    if(!tmp)
		throw Ememory("libdar5::archive_options_create::set_entrepot");
	    libdar::archive_options_read::set_entrepot(tmp);
	}

	void set_ref_entrepot(const entrepot & entr)
	{
	    std::shared_ptr<libdar::entrepot> tmp(entr.clone());
	    if(!tmp)
		throw Ememory("libdar5::archive_options_create::set_ref_entrepot");
	    libdar::archive_options_read::set_ref_entrepot(tmp);
	}
    };


	/////////////////////////////////////////////////////////
	///////// OPTIONS FOR CREATING AN ARCHIVE ///////////////
	/////////////////////////////////////////////////////////

    class archive_options_create: public libdar::archive_options_create
    {
    public:
	using libdar::archive_options_create::archive_options_create;

	    /// backward API v5 compatible method

	    /// \note in API v5, set_reference() was given a pointer
	    /// which was kept under the responsibility of the caller
	    /// the API was not deleting it.
	    /// In API v6 it was decided to remove ambiguity by
	    /// using a std::shared_ptr<archive>, which bioth shows
	    /// that archive object will not be deleted by libdar and
	    /// also clearly managed the release of the object when
	    /// no more needed/used.
	    /// Here to adapt APIv5, we have to build a std::shared_ptr
	    /// but as the object must survive the last shared_ptr
	    /// pointing to (APIv5 caller will delete it), the shared_ptr
	    /// is setup with a custome deleter which is a lambda expression
	    /// that does nothing [](libdar::archive *ptr) {}
  	void set_reference(libdar::archive *ref_arch)
	{
	    libdar::archive_options_create::set_reference(
		std::shared_ptr<libdar::archive>(ref_arch,
						 [](libdar::archive* ptr) {} )
		    // the custom deleter must not delete the object pointed to by
		    // by ref, this is why it does nothing
		);
	};

	void set_entrepot(const entrepot & entr)
	{
	    std::shared_ptr<libdar::entrepot> tmp(entr.clone());
	    if(!tmp)
		throw Ememory("libdar5::archive_options_create::set_entrepot");
	    libdar::archive_options_create::set_entrepot(tmp);
	}

    };

	/////////////////////////////////////////////////////////
	//////////// OPTIONS FOR ISOLATING A CATALOGUE //////////
	/////////////////////////////////////////////////////////


    class archive_options_isolate: public libdar::archive_options_isolate
    {
	void set_entrepot(const entrepot & entr)
	{
	    std::shared_ptr<libdar::entrepot> tmp(entr.clone());
	    if(!tmp)
		throw Ememory("libdar5::archive_options_create::set_entrepot");
	    libdar::archive_options_isolate::set_entrepot(tmp);
	}
    };

	/////////////////////////////////////////////////////////
	////////// OPTONS FOR MERGING ARCHIVES //////////////////
	/////////////////////////////////////////////////////////

    class archive_options_merge: public libdar::archive_options_merge
    {
    public:

	using libdar::archive_options_merge::archive_options_merge;

	void set_auxilliary_ref(libdar::archive *ref)
	{
	    libdar::archive_options_merge::set_auxilliary_ref(
		std::shared_ptr<libdar::archive>(ref,
						 [](libdar::archive* ptr) {})
		    // the custom deleter must not delete the object pointed to by
		    // by ref, this is why it does nothing
		);
	};

	void set_entrepot(const entrepot & entr)
	{
	    std::shared_ptr<libdar::entrepot> tmp(entr.clone());
	    if(!tmp)
		throw Ememory("libdar5::archive_options_create::set_entrepot");
	    libdar::archive_options_merge::set_entrepot(tmp);
	}
    };


	/////////////////////////////////////////////////////////
	////////// OPTONS FOR EXTRACTING FILES FROM ARCHIVE /////
	/////////////////////////////////////////////////////////

    using libdar::archive_options_extract;


	/////////////////////////////////////////////////////////
	////////// OPTIONS FOR LISTING AN ARCHIVE ///////////////
	/////////////////////////////////////////////////////////

    class archive_options_listing: public libdar::archive_options_listing_shell
    {
    public:
	using libdar::archive_options_listing_shell::archive_options_listing_shell;
    };


	/////////////////////////////////////////////////////////
	////////// OPTONS FOR DIFFING AN ARCHIVE ////////////////
	/////////////////////////////////////////////////////////

    using libdar::archive_options_diff;


	/////////////////////////////////////////////////////////
	////////// OPTONS FOR TESTING AN ARCHIVE ////////////////
	/////////////////////////////////////////////////////////

    using libdar::archive_options_test;


	/////////////////////////////////////////////////////////
	///////// OPTIONS FOR REPAIRING AN ARCHIVE //////////////
	/////////////////////////////////////////////////////////

    using libdar::archive_options_repair;

	/// @}

} // end of namespace

#endif
