/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2024 Denis Corbin
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

    /// \file trivial_sar.hpp
    /// \brief the trivial_sar classes manages the slicing layer when single slice is used
    /// \ingroup Private

#ifndef TRIVIAL_SAR_HPP
#define TRIVIAL_SAR_HPP

#include "../my_config.h"

#include <string>
#include "infinint.hpp"
#include "generic_file.hpp"
#include "integers.hpp"
#include "entrepot.hpp"
#include "contextual.hpp"
#include "mem_ui.hpp"

namespace libdar
{
	// contextual is defined in generic_file module

	/// \addtogroup Private
	/// @{

	/// "trivial sar" proposes the same interface a sar but does the work slightly differently using different constructors
	///
	/// depending on the constructor used trivial sar can:
	/// - in write mode send the data to a arbitrary long slice (plain file)
	/// - in read mode let read a single slice from a named pipe
	/// - in write mode let write a single sliced archive to an anonymous pipe

    class trivial_sar : public generic_file, public contextual, protected mem_ui
    {
    public:
	    /// constructor to build a new single sliced archive
	trivial_sar(const std::shared_ptr<user_interaction> & dialog,   ///< how to interact with the user
		    gf_mode open_mode,                 ///< read_write or write_only are the only acceptable values
		    const std::string & base_name,     ///< archive basename to create
		    const std::string & extension,     ///< archive extension
 		    const entrepot & where,            ///< where to store the archive
		    const label & internal_nale,       ///< tag common to all slices of the archive
		    const label & data_name,           ///< tag that follows the data when archive is dar_xform'ed
		    const std::string & execute,       ///< command line to execute at end of slice creation
		    bool allow_over,                   ///< whether to allow overwriting
		    bool warn_over,                    ///< whether to warn before overwriting
		    bool force_permission,             ///< whether to enforce slice permission or not
		    U_I permission,                    ///< value of permission to use if permission enforcement is used
		    hash_algo x_hash,                  ///< whether to build a hash of the slice, and which algo to use for that
	    	    const infinint & min_digits,       ///< is the minimum number of digits the slices number is stored with in the filename
		    bool format_07_compatible          ///< build a slice header backward compatible with 2.3.x
	    );


	    /// constructor to read a (single sliced) archive from a pipe
	trivial_sar(const std::shared_ptr<user_interaction> & dialog,   ///< how to interact with the user
		    const std::string & pipename,      ///< if set to '-' the data are read from standard input, else the given file is expected to be named pipe to read data from
		    bool lax                           ///< whether to be laxist or follow the normal and strict controlled procedure
	    );

	trivial_sar(const std::shared_ptr<user_interaction> & dialog,  ///< how to interact with the user
		    int filedescriptor,                ///< if set to '-' the data are read from standard input, else the given file is expected to be named pipe to read data from
		    bool lax                           ///< whether to be laxist or follow the normal and strict controlled procedure
	    );

	    /// constructor to write a (single sliced) archive to a anonymous pipe
	trivial_sar(const std::shared_ptr<user_interaction> & dialog, ///< user interaction
		    generic_file * f, ///< in case of exception the generic_file "f" is not released, this is the duty of the caller to do so, else (success), the object becomes owned by the trivial_sar and must not be released by the caller.
		    const label & internal_name, ///< internal name ti use
		    const label & data_name,     ///< data name
		    bool format_07_compatible,   ///< whether we have to avoid creating a slice trailer
		    const std::string & execute  ///< command to execute after each slice
	    );

	    /// copy constructor (disabled)
	trivial_sar(const trivial_sar & ref) = delete;

	    /// move constructor
	trivial_sar(trivial_sar && ref) noexcept = delete;

	    /// assignment operator (disabled)
	trivial_sar & operator = (const trivial_sar & ref) = delete;

	    /// move operator
	trivial_sar & operator = (trivial_sar && ref) noexcept = delete;

	    /// destructor
	~trivial_sar();

	virtual bool skippable(skippability direction, const infinint & amount) override { return reference->skippable(direction, amount); };
        virtual bool skip(const infinint & pos) override;
        virtual bool skip_to_eof() override { if(is_terminated()) throw SRC_BUG; return reference->skip_to_eof(); };
        virtual bool skip_relative(S_I x) override;
	virtual bool truncatable(const infinint & pos) const override { return reference->truncatable(offset + pos); };
        virtual infinint get_position() const override { return cur_pos; };

	    // contextual inherited method
	virtual bool is_an_old_start_end_archive() const override { return old_sar; };
	virtual const label & get_data_name() const override { return of_data_name; };

	    /// size of the slice header
	const infinint & get_slice_header_size() const { return offset; };

	    /// disable execution of user command when destroying the current object
	void disable_natural_destruction() { natural_destruction = false; };

	    /// enable back execution of user command when destroying the current object
	void enable_natural_destruction() { natural_destruction = true; };

    protected:
	virtual void inherited_read_ahead(const infinint & amount) override { reference->read_ahead(amount); };
        virtual U_I inherited_read(char *a, U_I size) override;
        virtual void inherited_write(const char *a, U_I size) override;
	virtual void inherited_truncate(const infinint & pos) override { reference->truncate(pos + offset); cur_pos = pos; };
	virtual void inherited_sync_write() override { if(reference != nullptr) reference->sync_write(); };
	virtual void inherited_flush_read() override { if(reference != nullptr) reference->flush_read(); };
	virtual void inherited_terminate() override;

    private:
        generic_file *reference;  ///< points to the underlying data, owned by "this"
        infinint offset;          ///< offset to apply to get the first byte of data out of SAR headers
	infinint cur_pos;         ///< current position as returned by get_position()
	infinint end_of_slice;    ///< when end of slice/archive is met, there is an offset by 1 compared to the offset of reference. end_of_slice is set to 1 in that situation, else it is always equal to zero
	std::string hook;         ///< command to execute after slice writing (not used in read-only mode)
	std::string base;         ///< basename of the archive (used for string susbstitution in hook)
	std::string ext;          ///< extension of the archive (used for string substitution in hook)
	label of_data_name;       ///< archive's data name
	bool old_sar;             ///< true if the read sar has an old header (format <= "07") or the to be written must keep a version 07 format.
	infinint min_digits;      ///< minimum number of digits in slice name
	std::string hook_where;   ///< what value to use for %p substitution in hook
	std::string base_url;     ///< what value to use for %u substitution in hook
	bool natural_destruction; ///< whether user command is executed once the single sliced archive is completed (disable upon user interaction)

	void init(const label & internal_name); ///< write the slice header and set the offset field (write mode), or (read-mode),  reads the slice header an set offset field

	void where_am_i();
    };


	/// return the name of a slice given the base_name, slice number and extension
    extern std::string sar_make_filename(const std::string & base_name, const infinint & num, const infinint & min_digits, const std::string & ext);

	/// @}

} // end of namespace

#endif
