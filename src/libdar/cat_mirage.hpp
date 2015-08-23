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

    /// \file cat_mirage.hpp
    /// \brief smart pointer to an etoile object. Used to store hard link information inside a catalogue
    /// \ingroup Private

#ifndef CAT_MIRAGE_HPP
#define CAT_MIRAGE_HPP

#include "../my_config.h"

extern "C"
{
} // end extern "C"

#include "cat_nomme.hpp"
#include "cat_etoile.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

	/// the hard link implementation, cat_mirage is the named entry owned by a directory it points to a common "cat_etoile class"

	/// well, a mirage is this fake apparition of water in a desert... I guess you get the picture now... :-)
    class cat_mirage : public cat_nomme
    {
    public:
	enum mirage_format {fmt_mirage,           //< new format
			    fmt_hard_link,        //< old dual format
			    fmt_file_etiquette }; //< old dual format

	cat_mirage(const std::string & name, cat_etoile *ref) : cat_nomme(name) { star_ref = ref; if(ref == nullptr) throw SRC_BUG; star_ref->add_ref(this); };
	cat_mirage(user_interaction & dialog,
		   const pile_descriptor & pdesc,
		   const archive_version & reading_ver,
		   saved_status saved,
		   entree_stats & stats,
		   std::map <infinint, cat_etoile *> & corres,
		   compression default_algo,
		   mirage_format fmt,
		   bool lax,
		   bool small);
	cat_mirage(user_interaction & dialog,
		   const pile_descriptor & pdesc,
		   const archive_version & reading_ver,
		   saved_status saved,
		   entree_stats & stats,
		   std::map <infinint, cat_etoile *> & corres,
		   compression default_algo,
		   bool lax,
		   bool small);
	cat_mirage(const cat_mirage & ref) : cat_nomme (ref) { star_ref = ref.star_ref; if(star_ref == nullptr) throw SRC_BUG; star_ref->add_ref(this); };
	const cat_mirage & operator = (const cat_mirage & ref);
	~cat_mirage() { star_ref->drop_ref(this); };

	bool operator == (const cat_entree & ref) const;

	unsigned char signature() const { return 'm'; };
	cat_entree *clone() const { return new (get_pool()) cat_mirage(*this); };

	cat_inode *get_inode() const { if(star_ref == nullptr) throw SRC_BUG; return star_ref->get_inode(); };
	infinint get_etiquette() const { return star_ref->get_etiquette(); };
	infinint get_etoile_ref_count() const { return star_ref->get_ref_count(); };
	cat_etoile *get_etoile() const { return star_ref; };

	bool is_inode_counted() const { return star_ref->is_counted(); };
	bool is_inode_wrote() const { return star_ref->is_wrote(); };
	bool is_inode_dumped() const { return star_ref->is_dumped(); };
	void set_inode_counted(bool val) const { star_ref->set_counted(val); };
	void set_inode_wrote(bool val) const { star_ref->set_wrote(val); };
	void set_inode_dumped(bool val) const { star_ref->set_dumped(val); };

	void post_constructor(const pile_descriptor & pdesc);

	    /// whether we are the mirage that triggered this hard link creation
	bool is_first_mirage() const { return star_ref->get_first_ref() == this; };

	    // overwriting virtual method from cat_entree
	virtual void change_location(const pile_descriptor & pdesc) { get_inode()->change_location(pdesc); };

    protected:
	void inherited_dump(const pile_descriptor & pdesc, bool small) const;

    private:
	cat_etoile *star_ref;

	void init(user_interaction & dialog,
		  const pile_descriptor & pdesc,
		  const archive_version & reading_ver,
		  saved_status saved,
		  entree_stats & stats,
		  std::map <infinint, cat_etoile *> & corres,
		  compression default_algo,
		  mirage_format fmt,
		  bool lax,
		  bool small);
    };

	/// @}

} // end of namespace

#endif
