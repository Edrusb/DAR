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
#include "fsa_familly.hpp"

namespace libdar
{
	/// \addtogroup Private
	/// @{


	/// Filesystem Specific Attributes (FSA) class

    class filesystem_specific_attribute
    {
    public:

	    // public methods

	    /// inherited class must provide their equivalent constructor to the following one:
	    /// \note when the underlying filesystem does not support the requested EA the constructor
	    /// of the inherited class should throw an exception of type Erange. Only valid object should
	    /// be built, that is object containing the value of the FSA found on the filesystem.
	filesystem_specific_attribute(fsa_familly f, const std::string & target) { fam = f; nat = fsan_unset; };

	    /// \note the same note as for the first constructor do apply here
	filesystem_specific_attribute(generic_file & f, fsa_familly xfam) { fam = xfam; nat = fsan_unset; };

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


	    /// obtain the familly of the FSA
	fsa_familly get_familly() const { return fam; };

	    /// obtain the nature of the FSA
	fsa_nature get_nature() const { return nat; };

	    /// provides a human readable value of the FSA
	virtual std::string show_val() const = 0;

	    /// write down  to libdar archive
	virtual void write(generic_file & f) const = 0;

	    ///set attribute to an existing inode in filesystem
	virtual void set_to_fs(const std::string & target) = 0;

	    /// give the storage size for the EA
	virtual infinint storage_size() const = 0;

	    /// provides a way to copy objects without having to know the more specific class of the object
	virtual filesystem_specific_attribute *clone() const = 0;

    protected:
	void set_familly(const fsa_familly & val) { fam = val; };
	void set_nature(const fsa_nature & val) { nat = val; };

	    /// should return true if the value of the argument is equal to the one of 'this' false in any other case (even for object of another inherited class)
	virtual bool equal_value_to(const filesystem_specific_attribute & ref) const = 0;

    private:
	fsa_familly fam;
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

	    /// gives the set of FSA familly present in the list
	fsa_scope get_fsa_famillies() const { return familles; };

	    /// compare two lists of FSA to see whether they have equal FSA with identical values within the given familly scope
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
	    /// \param [in] scope list of FSA famillies to only consider
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

    private:
	std::vector<filesystem_specific_attribute *> fsa; //< sorted list of FSA
	fsa_scope familles;

	void copy_from(const filesystem_specific_attribute_list & ref);
	void update_familles();
	void add(const filesystem_specific_attribute & ref); // add an entry without updating the "familles" field
	void sort_fsa();

	static std::string familly_to_signature(fsa_familly f);
	static std::string nature_to_signature(fsa_nature n);
	static fsa_familly signature_to_familly(const std::string & sig);
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


    class fsa_creation_date : public filesystem_specific_attribute
    {
    public:
	fsa_creation_date(fsa_familly f, const std::string & target);
	fsa_creation_date(generic_file & f, fsa_familly fam);

//>>> see fstat64 field st_birthtimespec renamed st_birthtim by POSIX 2008

	    // inherited from filesystem_specific_attribute
	virtual std::string show_val() const;
	virtual void write(generic_file & f) const;
	virtual void set_to_fs(const std::string & target);
	virtual infinint storage_size() const { return date.get_storage_size(); };
	virtual filesystem_specific_attribute *clone() const { return cloner(this); };

    protected:
	virtual bool equal_value_to(const filesystem_specific_attribute & ref) const;

    private:
	infinint date;
    };

///////////////////////////////////////////////////////////////////////////

    class fsa_compressed : public filesystem_specific_attribute
    {
    public:
       	fsa_compressed(fsa_familly f, const std::string & target): filesystem_specific_attribute(f, target) {};
	fsa_compressed(generic_file & f, fsa_familly fam): filesystem_specific_attribute(f, fam) {};

	    // inherited from filesystem_specific_attribute
	virtual std::string show_val() const { return ""; };
	virtual void write(generic_file & f) const {};
	virtual void set_to_fs(const std::string & target) {};
	virtual infinint storage_size() const { return sizeof(char); };
	virtual filesystem_specific_attribute *clone() const { return cloner(this); };

    protected:
	virtual bool equal_value_to(const filesystem_specific_attribute & ref) const { return true; };
    };


    class fsa_nodump : public filesystem_specific_attribute
    {
    public:
       	fsa_nodump(fsa_familly f, const std::string & target): filesystem_specific_attribute(f, target) {};
	fsa_nodump(generic_file & f, fsa_familly fam): filesystem_specific_attribute(f, fam) {};

	    // inherited from filesystem_specific_attribute
	virtual std::string show_val() const { return ""; };
	virtual void write(generic_file & f) const {};
	virtual void set_to_fs(const std::string & target) {};
	virtual infinint storage_size() const { return sizeof(char); };
	virtual filesystem_specific_attribute *clone() const { return cloner(this); };

    protected:
	virtual bool equal_value_to(const filesystem_specific_attribute & ref) const { return true; };
    };


    class fsa_immutable : public filesystem_specific_attribute
    {
    public:
       	fsa_immutable(fsa_familly f, const std::string & target): filesystem_specific_attribute(f, target) {};
	fsa_immutable(generic_file & f, fsa_familly fam): filesystem_specific_attribute(f, fam) {};

	    // inherited from filesystem_specific_attribute
	virtual std::string show_val() const { return ""; };
	virtual void write(generic_file & f) const {};
	virtual void set_to_fs(const std::string & target) {};
	virtual infinint storage_size() const { return sizeof(char); };
	virtual filesystem_specific_attribute *clone() const { return cloner(this); };

    protected:
	virtual bool equal_value_to(const filesystem_specific_attribute & ref) const { return true; };
    };


    class fsa_undeleted : public filesystem_specific_attribute
    {
    public:
       	fsa_undeleted(fsa_familly f, const std::string & target): filesystem_specific_attribute(f, target) {};
	fsa_undeleted(generic_file & f, fsa_familly fam): filesystem_specific_attribute(f, fam) {};

	    // inherited from filesystem_specific_attribute
	virtual std::string show_val() const { return ""; };
	virtual void write(generic_file & f) const {};
	virtual void set_to_fs(const std::string & target) {};
	virtual infinint storage_size() const { return sizeof(char); };
	virtual filesystem_specific_attribute *clone() const { return  cloner(this); };

    protected:
	virtual bool equal_value_to(const filesystem_specific_attribute & ref) const { return true; };
    };


	/// @}

} // end of namespace

#endif
