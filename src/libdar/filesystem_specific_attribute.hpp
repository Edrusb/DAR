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

    /// \file filesystem_specific_attribute.hpp
    /// \brief filesystem specific attributes
    /// \ingroup Private

#ifndef FILESYSTEM_SPECIFIC_ATTRIBUTE_HPP
#define FILESYSTEM_SPECIFIC_ATTRIBUTE_HPP

#include <string>
#include <vector>
#include <new>

#include "integers.hpp"
#include "crc.hpp"
#include "fsa_family.hpp"
#include "special_alloc.hpp"

namespace libdar
{
	/// \addtogroup Private
	/// @{


	/// Filesystem Specific Attributes (FSA) class
	///
	/// this class handle the storage of attributes into and from the archive
	/// the have not filesystem specific knownledge. This aspect is managed
	/// by filesystem_specific_attribute_list that upon system call will create
	/// the liste of FSA and given the list of FSA will try to set them back to the
	/// filesystem

    class filesystem_specific_attribute
    {
    public:

	    /// constructor used to before reading the FSA from filesystem

	    /// \note when the underlying filesystem does not support the requested EA the constructor
	    /// of the inherited class should throw an exception of type Erange. Only valid object should
	    /// be built, that is object containing the value of the FSA found on the filesystem.
	filesystem_specific_attribute(fsa_family f) { fam = f; nat = fsan_unset; };

	    /// constructor used to read a FSA from a libdar archive
	filesystem_specific_attribute(generic_file & f, fsa_family xfam, fsa_nature xnat) { fam = xfam; nat = xnat; };

	    /// virtual destructor for inherited classes
	virtual ~filesystem_specific_attribute() {};

	    /// provide a mean to compare objects types
	bool is_same_type_as(const filesystem_specific_attribute & ref) const;

	    /// provides a mean to compare objects values
	virtual bool operator == (const filesystem_specific_attribute & ref) const { return is_same_type_as(ref) && equal_value_to(ref); };
	bool operator != (const filesystem_specific_attribute & ref) const { return ! (*this == ref); };

	    /// used to provided a sorted list of FSA
	bool operator < (const filesystem_specific_attribute & ref) const;
	bool operator >= (const filesystem_specific_attribute & ref) const { return !(*this < ref); };
	bool operator > (const filesystem_specific_attribute & ref) const { return ref < *this; };
	bool operator <= (const filesystem_specific_attribute & ref) const { return !(*this > ref); };


	    /// obtain the family of the FSA
	fsa_family get_family() const { return fam; };

	    /// obtain the nature of the FSA
	fsa_nature get_nature() const { return nat; };

	    /// provides a human readable value of the FSA
	virtual std::string show_val() const = 0;

	    /// write down  to libdar archive
	virtual void write(generic_file & f) const = 0;

	    /// give the storage size for the FSA
	virtual infinint storage_size() const = 0;

	    /// provides a way to copy objects without having to know the more specific class of the object
	virtual filesystem_specific_attribute *clone() const = 0;

#ifdef LIBDAR_SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(filesystem_specific_attribute);
#endif

    protected:
	void set_family(const fsa_family & val) { fam = val; };
	void set_nature(const fsa_nature & val) { nat = val; };

	    /// should return true if the value of the argument is equal to the one of 'this' false in any other case (even for object of another inherited class)
	virtual bool equal_value_to(const filesystem_specific_attribute & ref) const = 0;

    private:
	fsa_family fam;
	fsa_nature nat;
    };

///////////////////////////////////////////////////////////////////////////

    class filesystem_specific_attribute_list
    {
    public:
	filesystem_specific_attribute_list() {};
	filesystem_specific_attribute_list(const filesystem_specific_attribute_list & ref) { copy_from(ref); };
	const filesystem_specific_attribute_list & operator = (const filesystem_specific_attribute_list & ref) { clear(); copy_from(ref); return *this; };
	~filesystem_specific_attribute_list() { clear(); };

	    /// clear all attributes
	void clear();

	    /// gives the set of FSA family present in the list
	fsa_scope get_fsa_families() const { return familes; };

	    /// compare two lists of FSA to see whether they have equal FSA with identical values within the given family scope
	bool is_included_in(const filesystem_specific_attribute_list & ref, const fsa_scope & scope) const;

	    /// read FSA list from archive
	void read(generic_file & f);

	    /// write FSA list to archive
	void write(generic_file & f) const;

	    /// read FSA list from filesystem
	void get_fsa_from_filesystem_for(const std::string & target,
					 const fsa_scope & scope);

	    /// set FSA list to filesystem
	    /// \param [in] target path of file to restore FSA to
	    /// \param [in] scope list of FSA families to only consider
	    /// \param [in] ui user interaction object
	    /// \return true if some FSA have effectively been set, false if no FSA
	    /// could be set either because list was empty of all FSA in the list where out of scope
	bool set_fsa_to_filesystem_for(const std::string & target,
				       const fsa_scope & scope,
				       user_interaction & ui) const;

	    /// whether the list has at least one FSA
	bool empty() const { return fsa.empty(); };


	    /// access to members of the list
	U_I size() const { return fsa.size(); };


	    /// provide reference to FSA given its index
	const filesystem_specific_attribute & operator [] (U_I arg) const;

	    /// give the storage size for the EA
	infinint storage_size() const;

	    /// addition operator
	    ///
	    /// \note this operator is not reflexive (or symetrical if you prefer). Here "a + b" may differ from "b + a"
	    /// all FSA of the arg are added with overwriting to the FSA of 'this'
	filesystem_specific_attribute_list operator + (const filesystem_specific_attribute_list & arg) const;

	    /// look for the FSA of given familly and nature
	    /// \param[in] fam family of the FSA to look for
	    /// \param[in] nat nature of the FSA to look for
	    /// \param[in, out] ptr points to the FSA if found
	    /// \return true if such an FSA has been found and set ptr accordingly else return false
	bool find(fsa_family fam, fsa_nature nat, const filesystem_specific_attribute *&ptr) const;

#ifdef LIBDAR_SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(filesystem_specific_attribute_list);
#endif

    private:
	std::vector<filesystem_specific_attribute *> fsa; //< sorted list of FSA
	fsa_scope familes;

	void copy_from(const filesystem_specific_attribute_list & ref);
	void update_familes();
	void add(const filesystem_specific_attribute & ref); // add an entry without updating the "familes" field
	void sort_fsa();

	void fill_extX_FSA_with(const std::string & target);
	void fill_HFS_FSA_with(const std::string & target);

	    /// \note return true if some FSA could be set
	bool set_extX_FSA_to(user_interaction & ui, const std::string & target) const;

	    /// \note return true if some FSA could be set
	bool set_hfs_FSA_to(user_interaction & ui, const std::string & target) const;

	static std::string family_to_signature(fsa_family f);
	static std::string nature_to_signature(fsa_nature n);
	static fsa_family signature_to_family(const std::string & sig);
	static fsa_nature signature_to_nature(const std::string & sig);
    };

///////////////////////////////////////////////////////////////////////////

    template <class T> T *cloner(const T *x)
    {
	if(x == NULL)
	    throw SRC_BUG;
	T *ret = new (std::nothrow) T(*x);
	if(ret == NULL)
	    throw Ememory("cloner template");

	return ret;
    }

///////////////////////////////////////////////////////////////////////////

    class fsa_bool : public filesystem_specific_attribute
    {
    public:
	fsa_bool(fsa_family f, fsa_nature n, bool xval) : filesystem_specific_attribute(f), val(xval) { set_nature(n); };
	fsa_bool(generic_file & f, fsa_family fam, fsa_nature nat);

	bool get_value() const { return val; };

	    /// inherited from filesystem_specific_attribute
	virtual std::string show_val() const { return val ? gettext("true") : gettext("false"); };
	virtual void write(generic_file & f) const { f.write(val ? "T" : "F", 1); };
	virtual infinint storage_size() const { return 1; };
	virtual filesystem_specific_attribute *clone() const { return cloner(this); };

#ifdef LIBDAR_SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(fsa_bool);
#endif


    protected:
	virtual bool equal_value_to(const filesystem_specific_attribute & ref) const;

    private:
	bool val;
    };

///////////////////////////////////////////////////////////////////////////

    class fsa_infinint : public filesystem_specific_attribute
    {
    public:
	fsa_infinint(fsa_family f, fsa_nature n, bool xval) : filesystem_specific_attribute(f), val(xval) { set_nature(n); mode = integer; };
	fsa_infinint(generic_file & f, fsa_family fam, fsa_nature nat);

	const infinint & get_value() const { return val; };

	enum show_mode { integer, date };
	void set_show_mode(show_mode m) { mode = m; };

	    /// inherited from filesystem_specific_attribute
	virtual std::string show_val() const;
	virtual void write(generic_file & f) const { val.dump(f); };
	virtual infinint storage_size() const { return val.get_storage_size(); };
	virtual filesystem_specific_attribute *clone() const { return cloner(this); };

#ifdef LIBDAR_SPECIAL_ALLOC
        USE_SPECIAL_ALLOC(fsa_infinint);
#endif


    protected:
	virtual bool equal_value_to(const filesystem_specific_attribute & ref) const;

    private:
	infinint val;
	show_mode mode;
    };

	/// @}

} // end of namespace

#endif
