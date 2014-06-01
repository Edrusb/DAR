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
        sar(user_interaction & dialog,
	    const std::string & base_name,
	    const std::string & extension,
	    const entrepot & where,
	    bool by_the_end,
	    const infinint & x_min_digits,
	    bool lax = false,
	    const std::string & execute = "");


	    /// this constructor creates a new set of slices

	    /// \param[in,out] dialog is used for user interaction
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
        sar(user_interaction  & dialog,
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
        infinint get_position();

            // informational routines
        infinint get_sub_file_size() const { return size; };
        infinint get_first_sub_file_size() const { return first_size; };
        bool get_total_file_number(infinint &num) const { num = of_last_file_num; return of_last_file_known; };
        bool get_last_file_size(infinint &num) const { num = of_last_file_size; return of_last_file_known; };

	    // disable execution of user command when destroying the current object
	void disable_natural_destruction() { natural_destruction = false; };

	    // enable back execution of user command when destroying the current object
	void enable_natural_destruction() { natural_destruction = true; };

	    // true if sar's header is from an old archive format (<= "07")
	bool is_an_old_start_end_archive() const { return old_sar; };

	    // return the internal_name used to link slices toghether
	const label & get_internal_name_used() const { return of_internal_name; };

	    // return the data_name used to link slices toghether
	const label & get_data_name() const { return of_data_name; };

	const entrepot *get_entrepot() const { return entr; };

    protected :
        U_I inherited_read(char *a, U_I size);
        void inherited_write(const char *a, U_I size);
	void inherited_sync_write() {}; // nothing to do
	void inherited_terminate();

    private :
	struct coordinate
	{
	    infinint slice_num;
	    infinint offset_in_slice;
	};

	entrepot *entr;              //< where are stored slices
        std::string base;            //< archive base name
	std::string ext;             //< archive extension
        std::string hook;            //< command line to execute between slices
        infinint size;               //< size of slices
        infinint first_size;         //< size of first slice
        infinint first_file_offset;  //< where data start in the first slice
	infinint other_file_offset;  //< where data start in the slices other than the first
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
	bool old_sar;                //< in read-mode, is true if the read sar has an old header (format <= "07"), in write mode, is true if it is requested to build old slice headers
	bool lax;                    //< whether to try to go further reading problems

	coordinate get_slice_and_offset(infinint pos) const;       //< convert absolute position (seen by the upper layer) to slice number and offset in slice
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


	/// "trivial sar" proposes the same interface a sar but does the work slightly differently using different constructors
	///
	/// depending on the constructor used trivial sar can:
	/// - in write mode send the data to a arbitrary long slice (plain file)
	/// - in read mode let read a single slice from a named pipe
	/// - in write mode let write a single sliced archive to an anonymous pipe

    class trivial_sar : public generic_file , public contextual, protected mem_ui
    {
    public:
	    /// constructor to build a new single sliced archive
	trivial_sar(user_interaction & dialog,         //< how to interact with the user
		    const std::string & base_name,     //< archive basename to create
		    const std::string & extension,     //< archive extension
 		    const entrepot & where,            //< where to store the archive
		    const label & internal_nale,       //< tag common to all slices of the archive
		    const label & data_name,           //< tag that follows the data when archive is dar_xform'ed
		    const std::string & execute,       //< command line to execute at end of slice creation
		    bool allow_over,                   //< whether to allow overwriting
		    bool warn_over,                    //< whether to warn before overwriting
		    bool force_permission,             //< whether to enforce slice permission or not
		    U_I permission,                    //< value of permission to use if permission enforcement is used
		    hash_algo x_hash,                  //< whether to build a hash of the slice, and which algo to use for that
	    	    const infinint & min_digits,       //< is the minimum number of digits the slices number is stored with in the filename
		    bool format_07_compatible);        //< build a slice header backward compatible with 2.3.x


	    /// constructor to read a (single sliced) archive from a pipe
	trivial_sar(user_interaction & dialog,         //< how to interact with the user
		    const std::string & pipename,      //< if set to '-' the data are read from standard input, else the given file is expected to be named pipe to read data from
		    bool lax);                         //< whether to be laxist or follow the normal and strict controlled procedure


	    /// constructor to write a (single sliced) archive to a anonymous pipe
	trivial_sar(user_interaction & dialog,
		    generic_file * f, //< in case of exception the generic_file "f" is not released, this is the duty of the caller to do so, else (success), the object becomes owned by the trivial_sar and must not be released by the caller.
		    const label & internal_name,
		    const label & data_name,
		    bool format_07_compatible,
		    const std::string & execute);

	    /// copy constructor (disabled)
	trivial_sar(const trivial_sar & ref) : generic_file(ref), mem_ui(ref) { throw SRC_BUG; };

	    /// destructor
	~trivial_sar();

	const trivial_sar & operator = (const trivial_sar & ref) { throw SRC_BUG; };
	bool skippable(skippability direction, const infinint & amount) { return true; };
        bool skip(const infinint & pos) { if(is_terminated()) throw SRC_BUG; return reference->skip(pos + offset); };
        bool skip_to_eof() { if(is_terminated()) throw SRC_BUG; return reference->skip_to_eof(); };
        bool skip_relative(S_I x);
        infinint get_position();

	    // contextual inherited method
	bool is_an_old_start_end_archive() const { return old_sar; };
	const label & get_data_name() const { return of_data_name; };

    protected:
        U_I inherited_read(char *a, U_I size);
        void inherited_write(const char *a, U_I size) { reference->write(a, size); };
	void inherited_sync_write() {};
	void inherited_terminate();

    private:
        generic_file *reference;  //< points to the underlying data, owned by "this"
        infinint offset;          //< offset to apply to get the first byte of data out of SAR headers
	infinint end_of_slice;    //< when end of slice/archive is met, there is an offset by 1 compared to the offset of reference. end_of_slice is set to 1 in that situation, else it is always equal to zero
	std::string hook;         //< command to execute after slice writing (not used in read-only mode)
	std::string base;         //< basename of the archive (used for string susbstitution in hook)
	std::string ext;          //< extension of the archive (used for string substitution in hook)
	label of_data_name;       //< archive's data name
	bool old_sar;             //< true if the read sar has an old header (format <= "07") or the to be written is must keep a version 07 format.
	infinint min_digits;      //< minimum number of digits in slice name
	std::string hook_where;   //< what value to use for %p subsitution in hook

	void init(const label & internal_name); //< write the slice header and set the offset field (write mode), or (read-mode),  reads the slice header an set offset field
    };


	/// return the name of a slice given the base_name, slice number and extension

    extern std::string sar_make_filename(const std::string & base_name, const infinint & num, const infinint & min_digits, const std::string & ext);

	/// @}

} // end of namespace

#endif
