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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

    /// \file sar.hpp
    /// \brief the sar and trivial_sar classes, they manage the slicing layer
    /// \ingroup Private

#ifndef SAR_HPP
#define SAR_HPP

#include "../my_config.h"

#include <string>
#include "infinint.hpp"
#include "generic_file.hpp"
#include "header.hpp"
#include "path.hpp"
#include "integers.hpp"
#include "entrepot.hpp"
#include "tools.hpp"
#include "slice_layout.hpp"

namespace libdar
{
	// contextual is defined in generic_file module

	/// \addtogroup Private
	/// @{

	/// Sar class stands for Segmentation And Reassembly class

	/// sar is used as a normal file but given some parameters at construction time
	/// the object will split the data in several files of given size (aka slices)
	/// sar uses a header to identify slices in a given set and tie slices of different sets
	/// At reading time sar transparently read data from the different slices.
    class sar : public generic_file, public contextual, protected mem_ui
    {
    public:

	    /// this constructor reads data from a set of slices

	    /// \param[in] dialog is for user interation (such a requesting a slice and pausing between slices)
	    /// \param[in] base_name is the basename of all slices of the set (it will be added the ".<slice numer>.extension" to form a filename
	    /// \param[in] extension is the extension of slice's filenames
	    /// \param[in] where defines where to store or where are stored slices
	    /// \param[in] by_the_end if true dar will try to open the slice set starting from the last slice else it will try starting from the first
	    /// \param[in] x_min_digits is the minimum number of digits the slices number is stored with in the filename
	    /// \param[in] lax if set to true will try workaround problems that would otherwise lead the operation to fail
	    /// \param[in] execute is the command to execute before trying to open each slice for reading
	    /// \note if by_the_end is set to true, the last slice must have extended slice header that contain informations about
	    /// the first slice size (used starting archive format "08"), Else, the slice size is not possible to open as the offset
	    /// of the data cannot be determin. If slice header is too old the sar class will fallback openning the first slice and
	    /// directly get the first slice.
        sar(const user_interaction & dialog,
	    const std::string & base_name,
	    const std::string & extension,
	    const entrepot & where,
	    bool by_the_end,
	    const infinint & x_min_digits,
	    bool lax = false,
	    const std::string & execute = "");


	    /// this constructor creates a new set of slices

	    /// \param[in,out] dialog is used for user interaction
	    /// \param[in] open_mode read_write or write_only is accepted only
	    /// \param[in] base_name is the slice set base name
	    /// \param[in] extension is the slices extension
	    /// \param[in] file_size is the size of slices (in byte)
	    /// \param[in] first_file_size is the size of the first slice (in byte) or set it to zero if it has to be equal to other slice's size
	    /// \param[in] x_warn_overwrite if set to true, a warning will be issued before overwriting a slice
	    /// \param[in] x_allow_overwrite if set to false, no slice overwritting will be allowed
	    /// \param[in] pause if set to zero no pause will be done between slice creation. If set to 1 a pause between each slice will be done. If set to N a pause each N slice will be done. Pauses must be acknoledged by user for the process to continue
	    /// \param[in] where defines where to store the slices
	    /// \param[in] internal_name is a tag common to all slice of the archive
	    /// \param[in] data_name is a tag that has to be associated with the data.
	    /// \param[in] force_permission if true slice permission will be forced to the value given in the next argument
	    /// \param[in] permission value to use to set permission of slices
	    /// \param[in] x_hash defines whether a hash file has to be generated for each slice, and wich hash algorithm to use
	    /// \param[in] x_min_digits is the minimum number of digits the slices number is stored with in the filename
	    /// \param[in] format_07_compatible when set to true, creates a slice header in the archive format of version 7 instead of the highest version known
	    /// \param[in] execute is the command to execute after each slice creation (once it is completed)
	    /// \note data_name should be equal to internal_name except when reslicing an archive as dar_xform does in which
	    /// case internal_name is randomly, and data_name is kept from the source archive
        sar(const user_interaction  & dialog,
	    gf_mode open_mode,
	    const std::string & base_name,
	    const std::string & extension,
	    const infinint & file_size,
	    const infinint & first_file_size,
 	    bool x_warn_overwrite,
	    bool x_allow_overwrite,
	    const infinint & pause,
	    const entrepot & where,
	    const label & internal_name,
	    const label & data_name,
	    bool force_permission,
	    U_I permission,
	    hash_algo x_hash,
	    const infinint & x_min_digits,
	    bool format_07_compatible,
	    const std::string & execute = "");

	    /// the destructor

   	sar(const sar & ref) : generic_file(ref), mem_ui(ref) { throw Efeature("class sar's copy constructor is not implemented"); };

	    /// destructor
        ~sar();

            // inherited from generic_file
	bool skippable(skippability direction, const infinint & amount);
        bool skip(const infinint &pos);
        bool skip_to_eof();
        bool skip_relative(S_I x);
        infinint get_position() const;

            // informational routines
	const slice_layout & get_slicing() const { return slicing; };
        bool get_total_file_number(infinint &num) const { num = of_last_file_num; return of_last_file_known; };
        bool get_last_file_size(infinint &num) const { num = of_last_file_size; return of_last_file_known; };

	    // disable execution of user command when destroying the current object
	void disable_natural_destruction() { natural_destruction = false; };

	    // enable back execution of user command when destroying the current object
	void enable_natural_destruction() { natural_destruction = true; };

	    // true if sar's header is from an old archive format (<= "07")
	bool is_an_old_start_end_archive() const { return slicing.older_sar_than_v8; };

	    // return the internal_name used to link slices toghether
	const label & get_internal_name_used() const { return of_internal_name; };

	    // return the data_name used to link slices toghether
	const label & get_data_name() const { return of_data_name; };

	const entrepot *get_entrepot() const { return entr; };

	    /// get the first slice header
	const infinint & get_first_slice_header_size() const { return slicing.first_slice_header; };

	    /// get the non first slice header
	const infinint & get_non_first_slice_header_size() const { return slicing.other_slice_header; };

    protected :
	void inherited_read_ahead(const infinint & amount);
        U_I inherited_read(char *a, U_I size);
        void inherited_write(const char *a, U_I size);
	void inherited_sync_write() {}; // nothing to do
	void inherited_flush_read() {}; // nothing to do
	void inherited_terminate();

    private :
	entrepot *entr;              //< where are stored slices
        std::string base;            //< archive base name
	std::string ext;             //< archive extension
        std::string hook;            //< command line to execute between slices
	slice_layout slicing;        //< slice layout
        infinint file_offset;        //< current reading/writing position in the current slice (relative to the whole slice file, including headers)
	hash_algo hash;              //< whether to build a hashing when creating slices, and if so, which algorithm to use
	infinint min_digits;         //< minimum number of digits the slices number is stored with in the filename
        bool natural_destruction;    //< whether to execute commands between slices on object destruction
            // these following variables are modified by open_file / open_file_init
            // else the are used only for reading
        infinint of_current;         //< number of the open slice
	infinint size_of_current;    //< size of the current slice (used in reading mode only)
        infinint of_max_seen;        //< highest slice number seen so far
        bool of_last_file_known;     //< whether the T terminal slice has been met
        infinint of_last_file_num;   //< number of the last slice (if met)
        infinint of_last_file_size;  //< size of the last slice (if met)
        label of_internal_name;      //< internal name shared in all slice header
	label of_data_name;          //< internal name linked to data (transparent to dar_xform and used by isolated catalogue as reference)
	bool force_perm;             //< true if any future slice has its permission to be set explicitely
	U_I perm;                    //< if force_perm is true, value to use for slice permission
        fichier_global *of_fd;       //< file object currently openned
        char of_flag;                //< flags of the open file
        bool initial;                //< do not launch hook command-line during sar initialization
            // these are the option flags
        bool opt_warn_overwrite;     //<  a warning must be issued before overwriting a slice
        bool opt_allow_overwrite;    //< is slice overwriting allowed
	    //
        infinint pause;              //< do we pause between slices
	bool lax;                    //< whether to try to go further reading problems
	infinint to_read_ahead;      //< amount of data to read ahead for next slices

        bool skip_forward(U_I x);                                  //< skip forward in sar global contents
        bool skip_backward(U_I x);                                 //< skip backward in sar global contents
        void close_file(bool terminal);                            //< close current openned file, adding (in write mode only) a terminal mark (last slice) or not
        void open_readonly(const std::string & fic, const infinint &num);  //< open file of name "filename" for read only "num" is the slice number
        void open_writeonly(const std::string & fic, const infinint &num); //< open file of name "filename" for write only "num" is the slice number
        void open_file_init();            //< initialize some of_* fields
        void open_file(infinint num);     //< close current slice and open the slice 'num'
        void set_offset(infinint offset); //< skip to current slice relative offset
        void open_last_file();            //< open the last slice, ask the user, test, until last slice available
	bool is_current_eof_a_normal_end_of_slice() const;  //< return true if current reading position is at end of slice
	infinint bytes_still_to_read_in_slice() const;  //< returns the number of bytes expected before the end of slice
        header make_write_header(const infinint &num, char flag);

            // function to lauch the eventually existing command to execute after/before each slice
        void hook_execute(const infinint &num);
    };

	/// @}

} // end of namespace

#endif
