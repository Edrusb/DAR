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

    /// \file escape_catalogue.hpp
    /// \brief class escape_catalogue definition. Used for sequential writing to archives, as well as several other inherited classes from catalogue.hpp
    /// \ingroup Private
    ///
    /// This class inherits from the class catalogue and implements
    /// the pre_add(...) method, which role is to add an escape sequence followed
    /// by an entry dump (usually used at the end of archive is the so called catalogue part
    /// of the archive). This sequence followed by entry dump is added
    /// before each file's data all along the archive.
    /// Other inherited classes, implement the escape specific part, used when performing
    /// sequential reading of the catalogue

#ifndef ESCAPE_CATALOGUE_HPP
#define ESCAPE_CATALOGUE_HPP

#include "../my_config.h"

#include "catalogue.hpp"
#include "escape.hpp"
#include "pile_descriptor.hpp"
#include "smart_pointer.hpp"
#include "header_version.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

    class escape_catalogue : public catalogue
    {
    public:

	    /// constructor to setup a escape_catalogue that will drop marks all along the archive and drop its content at end of archive
        escape_catalogue(const std::shared_ptr<user_interaction> & dialog,
			 const pile_descriptor & x_pdesc,
			 const datetime & root_last_modif,
			 const label & data_name);

	    /// constructor to setup a escape_catalogue that will be fed by sequentially reading the archive
        escape_catalogue(const std::shared_ptr<user_interaction> & dialog,        ///< user interaction
			 const pile_descriptor & x_pdesc,  ///< stack descriptor where to write to
			 const header_version & ver,       ///< archive header version read
			 const std::list<signator> & known_signatories, ///< signatories that signed the archive header, to be compared with internal catalogue when reaching the end of the archive
			 bool lax = false);                ///< whether to use lax mode
        escape_catalogue(const escape_catalogue & ref) : catalogue(ref) { copy_from(ref); };
	escape_catalogue(escape_catalogue && ref) = delete;
        escape_catalogue & operator = (const escape_catalogue &ref);
	escape_catalogue & operator = (escape_catalogue && ref) = delete;
	~escape_catalogue() { destroy(); };

	    // inherited from catalogue
	virtual void pre_add(const cat_entree *ref, const pile_descriptor* dest) const override;
	virtual void pre_add_ea(const cat_entree *ref, const pile_descriptor* dest) const override;
	virtual void pre_add_crc(const cat_entree *ref, const pile_descriptor* dest) const override;
	virtual void pre_add_dirty(const pile_descriptor* dest) const override;
	virtual void pre_add_ea_crc(const cat_entree *ref, const pile_descriptor* dest) const override;
	virtual void pre_add_waste_mark(const pile_descriptor* dest) const override;
	virtual void pre_add_failed_mark(const pile_descriptor* dest) const override;
	virtual void pre_add_fsa(const cat_entree *ref, const pile_descriptor* dest) const override;
	virtual void pre_add_fsa_crc(const cat_entree *ref, const pile_descriptor* dest) const override;
	virtual void pre_add_delta_sig(const pile_descriptor* dest) const override;
	virtual escape *get_escape_layer() const override { return pdesc->esc; };

	virtual void reset_read() const override;
	virtual void end_read() const override;
	virtual void skip_read_to_parent_dir() const override;
	virtual bool read(const cat_entree * & ref) const override;
	virtual bool read_if_present(std::string *name, const cat_nomme * & ref) const override;
	virtual void tail_catalogue_to_current_read() override;
	virtual bool read_second_time_dir() const override { return status == ec_detruits; };

    private:
	enum state
	{
	    ec_init,   ///< state in which no one file has yet been searched in the archive
	    ec_marks,  ///< state in which we find the next file using escape sequence marks
	    ec_eod,    ///< state in which the archive is missing trailing EOD entries, due to user interruption, thus returning EOD in enough number to get back to the root directory
	    ec_signature, ///< state in which we compare inline and internal catalogues
	    ec_detruits,  ///< state in which which detruits objects are returned from the catalogue
	    ec_completed  ///< state in which the escape_catalogue object is completed and has all information in memory as a normal catalogue
	};

	smart_pointer<pile_descriptor> pdesc;
	header_version x_ver;
	std::list<signator> known_sig;
	bool x_lax;
	std::map <infinint, cat_etoile *> corres;
        state status;
	catalogue *cat_det;         ///< holds the final catalogue's detruit objects when no more file can be read from the archive
	infinint min_read_offset;   ///< next offset in archive should be greater than that to identify a mark
	infinint depth;             ///< directory depth of archive being read sequentially
	infinint wait_parent_depth; ///< ignore any further entry while depth is less than wait_parent_depth. disabled is set to zero

	void set_esc_and_stack(const pile_descriptor & x_pdesc);
	void copy_from(const escape_catalogue & ref);
	void destroy();
	void merge_cat_det();
	void reset_reading_process();
    };

	/// @}

} // end of namespace

#endif
