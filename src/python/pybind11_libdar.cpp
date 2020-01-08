/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2019 Denis Corbin
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


#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
#include <tuple>

// this should be passed from Makefile
#define LIBDAR_MODE 64
#define HAVE_CONFIG_H 1
//

#include "../my_config.h"
#include "../libdar/libdar.hpp"
#include "../libdar/tools.hpp"
#include "../libdar/cat_nomme.hpp"
#include "../libdar/entrepot_local.hpp"
#include "../libdar/entrepot_libcurl.hpp"

PYBIND11_MODULE(libdar, mod)
{
	// mod is of type pybind11::module

    mod.doc() = "libdar python binding";

    	///////////////////////////////////////////
	// get_version
	//

    mod.def("get_version",
	    [](bool libgcrypt, bool gpgme){ libdar::U_I maj, med, min; libdar::get_version(maj, med, min, libgcrypt, gpgme); return std::make_tuple(maj,med,min); },
	    pybind11::arg("init_libgcrypt") = true,
	    pybind11::arg("init_gpgme") = true);

    mod.def("close_and_clean", &libdar::close_and_clean);

#ifdef MUTEX_WORKS
    mod
	.def("cancel_thread", &libdar::cancel_thread)
	.def("cancel_status", &libdar::cancel_status)
	.def("cancel_clear", &libdar::cancel_clear)
	.def("get_thread_count", &libdar::get_thread_count);
#endif

	///////////////////////////////////////////
	// exceptions classes (from erreurs.hpp)
	//



	//////

    	// the following is useless as pybind11
	// cannot provide a class description
	// when this one is used as exception

/*

    class pyEgeneric_pub : public libdar::Egeneric
    {
    public:
	using libdar::Egeneric::Egeneric;

	using libdar::Egeneric::exceptionID;
    };

    class pyEgeneric : public pyEgeneric_pub
    {
    public:
	using pyEgeneric_pub::pyEgeneric_pub;

	virtual std::string exceptionID() const override
	{
	    PYBIND11_OVERLOAD_PURE(
		std::string,       // return type
		pyEgeneric_pub,    // parent class
		exceptionID,       // name of the method in C++ (must match Python name)
		);                 // trailing comma needed for portability
	};
    };

    pybind11::class_<libdar::Egeneric, pyEgeneric> egeneric(mod, "Egeneric");
    egeneric
	.def(pybind11::init<const std::string &, const std::string &>())
	.def("get_message", &pyEgeneric_pub::get_message)
	.def("dump_str", &pyEgeneric_pub::dump_str)
	.def("exceptionID", &pyEgeneric_pub::exceptionID);

	    // the following makes a conflict on symboc Egeneric in python
	    // as the symbol is already used from the py::class_ directive above
    // static pybind11::exception<libdar::Egeneric> exc_Egeneric(mod, "Egeneric");

    pybind11::class_<libdar::Ememory>(mod, "Ememory", egeneric)
	.def(pybind11::init<const std::string &>());

    pybind11::class_<libdar::Esecu_memory, libdar::Ememory>(mod, "Esecu_memory")
	.def(pybind11::init<const std::string &>());

    pybind11::class_<libdar::Ebug>(mod, "Ebug", egeneric)
	.def(pybind11::init<const std::string &, libdar::S_I>());

    pybind11::class_<libdar::Einfinint>(mod, "Einfinint", egeneric)
	.def(pybind11::init<const std::string &, const std::string &>());

    pybind11::class_<libdar::Elimitint>(mod, "Elimitint", egeneric)
	.def(pybind11::init<>());

    pybind11::class_<libdar::Erange>(mod, "Erange", egeneric)
	.def(pybind11::init<const std::string, const std::string>());

    pybind11::class_<libdar::Edeci>(mod, "Edeci", egeneric)
	.def(pybind11::init<const std::string &, const std::string &>());

    pybind11::class_<libdar::Efeature>(mod, "Efeature", egeneric)
	.def(pybind11::init<const std::string &>());

    pybind11::class_<libdar::Ehardware>(mod, "Ehardware", egeneric)
	.def(pybind11::init<const std::string &, const std::string &>());

    pybind11::class_<libdar::Euser_abort>(mod, "Euser_abort", egeneric)
	.def(pybind11::init<const std::string &>());

    pybind11::class_<libdar::Edata>(mod, "Edata", egeneric)
	.def(pybind11::init<const std::string &>());

    pybind11::class_<libdar::Escript>(mod, "Escript", egeneric)
	.def(pybind11::init<const std::string &, const std::string &>());

    pybind11::class_<libdar::Elibcall>(mod, "Elibcall", egeneric)
	.def(pybind11::init<const std::string &, const std::string &>());

    pybind11::class_<libdar::Ecompilation>(mod, "Ecompilation", egeneric)
	.def(pybind11::init<const std::string &>());

    pybind11::class_<libdar::Ethread_cancel>(mod, "Ethread_cancel", egeneric)
	.def(pybind11::init<bool, libdar::U_64>());

    pybind11::class_<libdar::Esystem> esystem(mod, "Esystem", egeneric);

    pybind11::enum_<libdar::Esystem::io_error>(esystem, "io_error")
	.value("io_exist", libdar::Esystem::io_exist)
	.value("io_absent", libdar::Esystem::io_absent)
	.value("io_access", libdar::Esystem::io_access)
	.value("io_ro_fs", libdar::Esystem::io_ro_fs)
	.export_values();

    esystem.def(pybind11::init<const std::string &, const std::string &, libdar::Esystem::io_error>())
	.def("get_code", &libdar::Esystem::get_code);


    pybind11::class_<libdar::Enet_auth>(mod, "Enet_auth", egeneric)
	.def(pybind11::init<const std::string &>());

*/

	// for the reason stated above we will use
	// a new class toward which we will translate libdar
	// exception

    class darexc : public std::exception
    {
    public:
	darexc(const std::string & mesg): x_mesg(mesg) {};

	virtual const char * what() const noexcept override { return x_mesg.c_str(); };

    private:
	std::string x_mesg;

    };

	// this will only expose the what() method
	// to python side. This is better than nothing
    pybind11::register_exception<darexc>(mod, "darexc");

	// now the translation from libdar::Egeneric to darexc
    pybind11::register_exception_translator([](std::exception_ptr p)
					    {
						try
						{
						    if(p)
							std::rethrow_exception(p);
						}
						catch(libdar::Ebug & e)
						{
						    throw darexc(e.dump_str().c_str());
						}
						catch(libdar::Egeneric & e)
						{
						    throw darexc(e.get_message().c_str());
						}
					    }
	);



	///////////////////////////////////////////
	// class path (from path.hpp)
	//

    pybind11::class_<libdar::path>(mod, "path")
	.def(pybind11::init<const std::string &, bool>(), pybind11::arg("s"), pybind11::arg("undisclosed") = false)
	.def(pybind11::init<const libdar::path &>())
	.def("basename", &libdar::path::basename)
	.def("reset_read", &libdar::path::reset_read)
	.def("read_subdir", [](libdar::path const & self) { std::string val; bool ret = self.read_subdir(val); return std::make_tuple(ret, val);})
	.def("is_relative", &libdar::path::is_relative)
	.def("is_absolute", &libdar::path::is_absolute)
	.def("is_undisclosed", &libdar::path::is_undisclosed)
	.def("pop", [](libdar::path & self) { std::string x; bool ret = self.pop(x); return make_tuple(ret, x);})
	.def("pop_front", [](libdar::path & self) { std::string x; bool ret = self.pop_front(x); return make_tuple(ret, x);})
	.def("append", &libdar::path::append)
	.def(pybind11::self + pybind11::self) // operator +
	.def(pybind11::self += pybind11::self) // operator += libdar::path
	.def(pybind11::self += std::string()) // operator += std::string
	.def("is_subdir_of", &libdar::path::is_subdir_of)
	.def("display", &libdar::path::display)
	.def("display_without_root", &libdar::path::display_without_root)
	.def("degre", &libdar::path::degre)
	.def("explode_undisclosed", &libdar::path::explode_undisclosed);


   	///////////////////////////////////////////
	// infinint (from real_infinint.hpp / limitint.hpp)
	//


#ifndef LIBDAR_MODE

    pybind11::class_<libdar::real_infinint>(mod, "infininit")
	.def(pybind11::init<off_t>(), pybind11::arg("a") = 0)
	.def(pybind11::init<time_t>(), pybind11::arg("a") = 0)
	.def(pybind11::init<size_t>(), pybind11::arg("a") = 0)
	.def(pybind11::init<const real_infinint &>())
	.def("dump", &libdar::real_infinint::dump)
	.def("read", &libdar::real_infinint::read)
	.def(pybind11::self += pybind11::self)
	.def(pybind11::self -= pybind11::self)
	.def(pybind11::self *= pybind11::self)
	.def(pybind11::self /= pybind11::self)
	.def(pybind11::self &= pybind11::self)
	.def(pybind11::self |= pybind11::self)
	.def(pybind11::self ^= pybind11::self)
	.def(pybind11::self >>= pybind11::self)
	.def(pybind11::self <<= pybind11::self)
	.def(pybind11::self %= pybind11::self)
	.def(pybind11::self % libdar::U_32())
	.def(pybind11::self < pybind11::self)
	.def(pybind11::self == pybind11::self)
	.def(pybind11::self > pybind11::self)
	.def(pybind11::self <= pybind11::self)
	.def(pybind11::self != pybind11::self)
	.def(pybind11::self >= pybind11::self)
	.def("is_zero", &libdar::real_infinint::is_zero);

    mod.def("euclide", [](const libdar::infinint & a, const libdar::infinint & b) { libdar::infinint q, r; euclide(a, b, q, r); return std::make_tuple(q, r); });

#else

    pybind11::class_<libdar::limitint<libdar::INFININT_BASE_TYPE> >(mod, "infinint")
	.def(pybind11::init<off_t>(), pybind11::arg("a") = 0)
	.def(pybind11::init<time_t>(), pybind11::arg("a") = 0)
	.def(pybind11::init<size_t>(), pybind11::arg("a") = 0)
	.def(pybind11::init<const libdar::limitint<libdar::INFININT_BASE_TYPE> &>())
	.def("dump", &libdar::limitint<libdar::INFININT_BASE_TYPE>::dump)
	.def("read", &libdar::limitint<libdar::INFININT_BASE_TYPE>::read)
	.def(pybind11::self += pybind11::self)
	.def(pybind11::self -= pybind11::self)
	.def(pybind11::self *= pybind11::self)
	.def(pybind11::self /= pybind11::self)
	.def(pybind11::self &= pybind11::self)
	.def(pybind11::self |= pybind11::self)
	.def(pybind11::self ^= pybind11::self)
	.def(pybind11::self >>= pybind11::self)
	.def(pybind11::self <<= pybind11::self)
	.def(pybind11::self %= pybind11::self)
	.def(pybind11::self % libdar::U_32())
	.def(pybind11::self < pybind11::self)
	.def(pybind11::self == pybind11::self)
	.def(pybind11::self > pybind11::self)
	.def(pybind11::self <= pybind11::self)
	.def(pybind11::self != pybind11::self)
	.def(pybind11::self >= pybind11::self)
	.def("is_zero", &libdar::limitint<libdar::INFININT_BASE_TYPE>::is_zero);

    mod.def("euclide", [](const libdar::infinint & a, const libdar::infinint & b) { libdar::infinint q, r; euclide(a, b, q, r); return std::make_tuple(q, r); });

#endif



	///////////////////////////////////////////
	// class deci (from deci.hpp / limitint.hpp)
	//

    pybind11::class_<libdar::deci>(mod, "deci")
	.def(pybind11::init<std::string>())
	.def(pybind11::init<libdar::infinint &>())
	.def("computer", &libdar::deci::computer)
	.def("human", &libdar::deci::human);

	///////////////////////////////////////////
	// tools routines related to infinint
	//



    mod.def("tools_display_interger_in_metric_system", &libdar::tools_display_integer_in_metric_system,
	    pybind11::arg("number"),
	    pybind11::arg("unit"),
	    pybind11::arg("binary Ki (else K)"));

    mod.def("tools_get_extended_size", &libdar::tools_get_extended_size,
	    pybind11::arg("input numerical string with unit suffix"),
	    pybind11::arg("unit base: 1000 for SI, 1024 for computer science"));

    	///////////////////////////////////////////
	// class statistics
	//


    pybind11::class_<libdar::statistics, std::shared_ptr<libdar::statistics> >(mod, "statistics")
	.def(pybind11::init<bool>(), pybind11::arg("lock") = true)
	.def("clear", &libdar::statistics::clear)
	.def("total", &libdar::statistics::total)
	.def("get_treated", &libdar::statistics::get_treated)
	.def("get_hard_links", &libdar::statistics::get_hard_links)
	.def("get_skipped", &libdar::statistics::get_skipped)
	.def("get_inode_only", &libdar::statistics::get_inode_only)
	.def("get_ignored", &libdar::statistics::get_ignored)
	.def("get_tooold", &libdar::statistics::get_tooold)
	.def("get_errored", &libdar::statistics::get_errored)
	.def("get_deleted", &libdar::statistics::get_deleted)
	.def("get_ea_treated", &libdar::statistics::get_ea_treated)
	.def("get_byte_amount", &libdar::statistics::get_byte_amount)
	.def("get_fsa_treated", &libdar::statistics::get_fsa_treated)
	    //
	.def("get_treated_str", &libdar::statistics::get_treated_str)
	.def("get_hard_links_str", &libdar::statistics::get_hard_links_str)
	.def("get_skipped_str", &libdar::statistics::get_skipped_str)
	.def("get_inode_only_str", &libdar::statistics::get_inode_only_str)
	.def("get_ignored_str", &libdar::statistics::get_ignored_str)
	.def("get_tooold_str", &libdar::statistics::get_tooold_str)
	.def("get_errored_str", &libdar::statistics::get_errored_str)
	.def("get_deleted_str", &libdar::statistics::get_deleted_str)
	.def("get_ea_treated_str", &libdar::statistics::get_ea_treated_str)
	.def("get_byte_amount_str", &libdar::statistics::get_byte_amount_str)
	.def("get_fsa_treated_str", &libdar::statistics::get_fsa_treated_str);


    	///////////////////////////////////////////
	// data structures in crypto.hpp
	//

    pybind11::enum_<libdar::crypto_algo>(mod, "crypto_algo")
	.value("none", libdar::crypto_algo::none)
	.value("scrambling", libdar::crypto_algo::scrambling)
	.value("blowfish", libdar::crypto_algo::blowfish)
	.value("aes256", libdar::crypto_algo::aes256)
	.value("twofish256", libdar::crypto_algo::twofish256)
	.value("serpent256", libdar::crypto_algo::serpent256)
	.value("camellia256", libdar::crypto_algo::camellia256)
	.export_values();


    pybind11::class_<libdar::signator> pysignator(mod, "signator");

    pybind11::enum_<libdar::signator::result_t>(pysignator, "result_t")
	.value("good", libdar::signator::good)
	.value("bad", libdar::signator::bad)
	.value("unknown_key", libdar::signator::unknown_key)
	.value("error", libdar::signator::error)
	.export_values();

    pybind11::enum_<libdar::signator::key_validity_t>(pysignator, "key_validity_t")
	.value("valid", libdar::signator::valid)
	.value("expired", libdar::signator::expired)
	.value("revoked", libdar::signator::revoked)
	.export_values();

    pysignator
	.def_readonly("result", &libdar::signator::result)
	.def_readonly("key_validity", &libdar::signator::key_validity)
	.def_readonly("fingerprint", &libdar::signator::fingerprint)
	.def_readonly("signing_date", &libdar::signator::signing_date)
	.def_readonly("signature_expiration_date", &libdar::signator::signature_expiration_date)
	.def(pybind11::self  < pybind11::self)
	.def(pybind11::self == pybind11::self);

    mod.def("crypto_algo_2_string", &libdar::crypto_algo_2_string, pybind11::arg("algo"));
    mod.def("same_signatories", &libdar::same_signatories);


	///////////////////////////////////////////
	// masks_* classes
	//

        // trampoline class needed to address pure virtual methods
	// making a template of the trampoline to be able to derive
	// inherited class from libdar::mask in python environment

    class pymask : public libdar::mask
    {
    public:
	    // using the same constructors
	using libdar::mask::mask;

	virtual bool is_covered(const std::string & expression) const override
	{
	    PYBIND11_OVERLOAD_PURE(
		bool,
		libdar::mask,
		is_covered,
		expression);
	};

	virtual bool is_covered(const libdar::path & chemin) const override
	{
	    PYBIND11_OVERLOAD(
		bool,
		libdar::mask,
		is_covered,
		chemin);
	};

	virtual std::string dump(const std::string & prefix) const override
	{
	    PYBIND11_OVERLOAD_PURE(
		std::string,
		libdar::mask,
		dump,
		prefix);
	};

	virtual libdar::mask *clone() const override
	{
	    PYBIND11_OVERLOAD_PURE(
		libdar::mask *,
		libdar::mask,
		clone, // trailing comma needed for portability
		);
	};
    };


	// the trampoline for libdar::mask
    pybind11::class_<libdar::mask, pymask>(mod, "mask")
	.def(pybind11::init<>())
	.def("is_covered", (bool (libdar::mask::*)(const std::string &) const) &libdar::mask::is_covered, "Mask based on string")
	.def("is_covered", (bool (libdar::mask::*)(const libdar::path &) const) &libdar::mask::is_covered, "Mask based on libdar::path")
	.def("dump", &libdar::mask::dump)
	.def("clone", &libdar::mask::clone);

	// inhertied class from libdar::mask will not have their virtual method overridable in python
	// only class libdar::mask can be that way inherited.
	// doing that way is simple. It does not worth effort to make things more complicated as there is no reason
	// to inherit from existing inherited libdar::mask classes

    pybind11::class_<libdar::bool_mask, libdar::mask>(mod, "bool_mask")
	.def(pybind11::init<bool>())
	.def("is_covered", (bool (libdar::bool_mask::*)(const std::string &) const) &libdar::bool_mask::is_covered, "Mask based on string")
	.def("is_covered", (bool (libdar::bool_mask::*)(const libdar::path &) const) &libdar::bool_mask::is_covered, "Mask based on libdar::path")
	.def("dump", &libdar::bool_mask::dump)
	.def("clone", &libdar::bool_mask::clone);


    pybind11::class_<libdar::simple_mask, libdar::mask>(mod, "simple_mask")
	.def(pybind11::init<const std::string &, bool>())
	.def("is_covered", (bool (libdar::simple_mask::*)(const std::string &) const) &libdar::simple_mask::is_covered, "Mask based on string")
	.def("is_covered", (bool (libdar::simple_mask::*)(const libdar::path &) const) &libdar::simple_mask::is_covered, "Mask based on libdar::path")
	.def("dump", &libdar::simple_mask::dump)
	.def("clone", &libdar::simple_mask::clone);

    pybind11::class_<libdar::regular_mask, libdar::mask>(mod, "regular_mask")
	.def(pybind11::init<const std::string &, bool>())
	.def("is_covered", (bool (libdar::regular_mask::*)(const std::string &) const) &libdar::regular_mask::is_covered, "Mask based on string")
	.def("is_covered", (bool (libdar::regular_mask::*)(const libdar::path &) const) &libdar::regular_mask::is_covered, "Mask based on libdar::path")
	.def("dump", &libdar::regular_mask::dump)
	.def("clone", &libdar::regular_mask::clone);

    pybind11::class_<libdar::not_mask, libdar::mask>(mod, "not_mask")
	.def(pybind11::init<const libdar::mask &>())
	.def("is_covered", (bool (libdar::not_mask::*)(const std::string &) const) &libdar::not_mask::is_covered, "Mask based on string")
	.def("is_covered", (bool (libdar::not_mask::*)(const libdar::path &) const) &libdar::not_mask::is_covered, "Mask based on libdar::path")
	.def("dump", &libdar::not_mask::dump)
	.def("clone", &libdar::not_mask::clone);

    pybind11::class_<libdar::et_mask, libdar::mask>(mod, "et_mask")
	.def(pybind11::init<>())
	.def("is_covered", (bool (libdar::et_mask::*)(const std::string &) const) &libdar::et_mask::is_covered, "Mask based on string")
	.def("is_covered", (bool (libdar::et_mask::*)(const libdar::path &) const) &libdar::et_mask::is_covered, "Mask based on libdar::path")
	.def("dump", &libdar::et_mask::dump)
	.def("clone", &libdar::et_mask::clone)
	.def("add_mask", &libdar::et_mask::add_mask)
	.def("size", &libdar::et_mask::size)
	.def("clear", &libdar::et_mask::clear);

    pybind11::class_<libdar::ou_mask, libdar::mask>(mod, "ou_mask")
	.def(pybind11::init<>())
	.def("is_covered", (bool (libdar::ou_mask::*)(const std::string &) const) &libdar::ou_mask::is_covered, "Mask based on string")
	.def("is_covered", (bool (libdar::ou_mask::*)(const libdar::path &) const) &libdar::ou_mask::is_covered, "Mask based on libdar::path")
	.def("dump", &libdar::ou_mask::dump)
	.def("clone", &libdar::ou_mask::clone)
	.def("add_mask", &libdar::ou_mask::add_mask)
	.def("size", &libdar::ou_mask::size)
	.def("clear", &libdar::ou_mask::clear);

    pybind11::class_<libdar::simple_path_mask, libdar::mask>(mod, "simple_path_mask")
	.def(pybind11::init<const std::string &, bool>())
	.def("is_covered", (bool (libdar::simple_path_mask::*)(const std::string &) const) &libdar::simple_path_mask::is_covered, "Mask based on string")
	.def("is_covered", (bool (libdar::simple_path_mask::*)(const libdar::path &) const) &libdar::simple_path_mask::is_covered, "Mask based on libdar::path")
	.def("dump", &libdar::simple_path_mask::dump)
	.def("clone", &libdar::simple_path_mask::clone);

    pybind11::class_<libdar::same_path_mask, libdar::mask>(mod, "same_path_mask")
	.def(pybind11::init<const std::string &, bool>())
	.def("is_covered", (bool (libdar::same_path_mask::*)(const std::string &) const) &libdar::same_path_mask::is_covered, "Mask based on string")
	.def("is_covered", (bool (libdar::same_path_mask::*)(const libdar::path &) const) &libdar::same_path_mask::is_covered, "Mask based on libdar::path")
	.def("dump", &libdar::same_path_mask::dump)
	.def("clone", &libdar::same_path_mask::clone);

    pybind11::class_<libdar::exclude_dir_mask, libdar::mask>(mod, "exclude_dir_mask")
	.def(pybind11::init<const std::string &, bool>())
	.def("is_covered", (bool (libdar::exclude_dir_mask::*)(const std::string &) const) &libdar::exclude_dir_mask::is_covered, "Mask based on string")
	.def("is_covered", (bool (libdar::exclude_dir_mask::*)(const libdar::path &) const) &libdar::exclude_dir_mask::is_covered, "Mask based on libdar::path")
	.def("dump", &libdar::exclude_dir_mask::dump)
	.def("clone", &libdar::exclude_dir_mask::clone);

    	///////////////////////////////////////////
	// mask_list_* class
	//

    pybind11::class_<libdar::mask_list, libdar::mask>(mod, "mask_list")
	.def(pybind11::init<const std::string &, bool, const libdar::path &, bool>())
	.def("is_covered", (bool (libdar::mask_list::*)(const std::string &) const) &libdar::mask_list::is_covered, "Mask based on string")
	.def("is_covered", (bool (libdar::mask_list::*)(const libdar::path &) const) &libdar::mask_list::is_covered, "Mask based on libdar::path")
	.def("dump", &libdar::mask_list::dump)
	.def("clone", &libdar::mask_list::clone)
	.def("size", &libdar::mask_list::size);

    	///////////////////////////////////////////
	// binding for what's criterium.hpp
	//

    class pycriterium : public libdar::criterium
    {
    public:
	virtual bool evaluate(const libdar::cat_nomme & first, const libdar::cat_nomme & second) const override
	{
	    PYBIND11_OVERLOAD_PURE(bool,
				   libdar::criterium,
				   evaluate,
				   first,
				   second);
	}

	virtual criterium *clone() const override
	{
	    PYBIND11_OVERLOAD_PURE(libdar::criterium *,
				   libdar::criterium,
				   clone,); // trailing comma expected as this method has no argument
	};
    };

    pybind11::class_<libdar::criterium, pycriterium>(mod, "criterium")
	.def(pybind11::init<>())
	.def("evaluate", &libdar::criterium::evaluate)
	.def("clone", &libdar::criterium::clone);

	// inherited class from libdar::criterium

    pybind11::class_<libdar::crit_in_place_is_inode, libdar::criterium>(mod, "crit_in_place_is_inode")
	.def(pybind11::init<>())
	.def("evaluate", &libdar::crit_in_place_is_inode::evaluate)
	.def("clone", &libdar::crit_in_place_is_inode::clone);

    pybind11::class_<libdar::crit_in_place_is_dir, libdar::criterium>(mod, "crit_in_place_is_dir")
	.def(pybind11::init<>())
	.def("evaluate", &libdar::crit_in_place_is_dir::evaluate)
	.def("clone", &libdar::crit_in_place_is_dir::clone);

    pybind11::class_<libdar::crit_in_place_is_file, libdar::criterium>(mod, "crit_in_place_is_file")
	.def(pybind11::init<>())
	.def("evaluate", &libdar::crit_in_place_is_file::evaluate)
	.def("clone", &libdar::crit_in_place_is_file::clone);

    pybind11::class_<libdar::crit_in_place_is_hardlinked_inode, libdar::criterium>(mod, "crit_in_place_is_hardlinked_inode")
	.def(pybind11::init<>())
	.def("evaluate", &libdar::crit_in_place_is_hardlinked_inode::evaluate)
	.def("clone", &libdar::crit_in_place_is_hardlinked_inode::clone);

    pybind11::class_<libdar::crit_in_place_is_new_hardlinked_inode, libdar::criterium>(mod, "crit_in_place_is_new_hardlinked_inode")
	.def(pybind11::init<>())
	.def("evaluate", &libdar::crit_in_place_is_new_hardlinked_inode::evaluate)
	.def("clone", &libdar::crit_in_place_is_new_hardlinked_inode::clone);

    pybind11::class_<libdar::crit_in_place_data_more_recent, libdar::criterium>(mod, "crit_in_place_data_more_recent")
	.def(pybind11::init<const libdar::infinint &>(), pybind11::arg("hourshift") = 0)
	.def("evaluate", &libdar::crit_in_place_data_more_recent::evaluate)
	.def("clone", &libdar::crit_in_place_data_more_recent::clone);

    pybind11::class_<libdar::crit_in_place_data_more_recent_or_equal_to, libdar::criterium>(mod, "crit_in_place_data_more_recent_or_equal_to")
	.def(pybind11::init<const libdar::infinint &, const libdar::infinint &>(), pybind11::arg("date"), pybind11::arg("hourshift") = 0)
	.def("evaluate", &libdar::crit_in_place_data_more_recent_or_equal_to::evaluate)
	.def("clone", &libdar::crit_in_place_data_more_recent_or_equal_to::clone);

    pybind11::class_<libdar::crit_in_place_data_bigger, libdar::criterium>(mod, "crit_in_place_data_bigger")
	.def(pybind11::init<>())
	.def("evaluate", &libdar::crit_in_place_data_bigger::evaluate)
	.def("clone", &libdar::crit_in_place_data_bigger::clone);

    pybind11::class_<libdar::crit_in_place_data_saved, libdar::criterium>(mod, "crit_in_place_data_saved")
	.def(pybind11::init<>())
	.def("evaluate", &libdar::crit_in_place_data_saved::evaluate)
	.def("clone", &libdar::crit_in_place_data_saved::clone);

    pybind11::class_<libdar::crit_in_place_data_dirty, libdar::criterium>(mod, "crit_in_place_data_dirty")
	.def(pybind11::init<>())
	.def("evaluate", &libdar::crit_in_place_data_dirty::evaluate)
	.def("clone", &libdar::crit_in_place_data_dirty::clone);

    pybind11::class_<libdar::crit_in_place_data_sparse, libdar::criterium>(mod, "crit_in_place_data_sparse")
	.def(pybind11::init<>())
	.def("evaluate", &libdar::crit_in_place_data_sparse::evaluate)
	.def("clone", &libdar::crit_in_place_data_sparse::clone);

    pybind11::class_<libdar::crit_in_place_has_delta_sig, libdar::criterium>(mod, "crit_in_place_has_delta_sig")
	.def(pybind11::init<>())
	.def("evaluate", &libdar::crit_in_place_has_delta_sig::evaluate)
	.def("clone", &libdar::crit_in_place_has_delta_sig::clone);


    pybind11::class_<libdar::crit_in_place_EA_present, libdar::criterium>(mod, "crit_in_place_EA_present")
	.def(pybind11::init<>())
	.def("evaluate", &libdar::crit_in_place_EA_present::evaluate)
	.def("clone", &libdar::crit_in_place_EA_present::clone);

    pybind11::class_<libdar::crit_in_place_EA_more_recent, libdar::criterium>(mod, "crit_in_place_EA_more_recent")
	.def(pybind11::init<const libdar::infinint &>(), pybind11::arg("hourshift") = 0)
	.def("evaluate", &libdar::crit_in_place_EA_more_recent::evaluate)
	.def("clone", &libdar::crit_in_place_EA_more_recent::clone);

    pybind11::class_<libdar::crit_in_place_EA_more_recent_or_equal_to, libdar::criterium>(mod, "crit_in_place_EA_more_recent_or_equal_to")
	.def(pybind11::init<const libdar::infinint &, const libdar::infinint &>(), pybind11::arg("date"), pybind11::arg("hourshift") = 0)
	.def("evaluate", &libdar::crit_in_place_EA_more_recent_or_equal_to::evaluate)
	.def("clone", &libdar::crit_in_place_EA_more_recent_or_equal_to::clone);

    pybind11::class_<libdar::crit_in_place_more_EA, libdar::criterium>(mod, "crit_in_place_more_EA")
	.def(pybind11::init<>())
	.def("evaluate", &libdar::crit_in_place_more_EA::evaluate)
	.def("clone", &libdar::crit_in_place_more_EA::clone);

    pybind11::class_<libdar::crit_in_place_EA_bigger, libdar::crit_in_place_more_EA>(mod, "crit_in_place_EA_bigger")
	.def(pybind11::init<>())
	.def("evaluate", &libdar::crit_in_place_EA_bigger::evaluate)
	.def("clone", &libdar::crit_in_place_EA_bigger::clone);

    pybind11::class_<libdar::crit_in_place_EA_saved, libdar::criterium>(mod, "crit_in_place_EA_saved")
	.def(pybind11::init<>())
	.def("evaluate", &libdar::crit_in_place_EA_saved::evaluate)
	.def("clone", &libdar::crit_in_place_EA_saved::clone);

    pybind11::class_<libdar::crit_same_type, libdar::criterium>(mod, "crit_same_type")
	.def(pybind11::init<>())
	.def("evaluate", &libdar::crit_same_type::evaluate)
	.def("clone", &libdar::crit_same_type::clone);

    pybind11::class_<libdar::crit_not, libdar::criterium>(mod, "crit_not")
	.def(pybind11::init<const libdar::criterium &>())
	.def("evaluate", &libdar::crit_not::evaluate)
	.def("clone", &libdar::crit_not::clone);

    pybind11::class_<libdar::crit_and, libdar::criterium>(mod, "crit_and")
	.def(pybind11::init<>())
	.def("evaluate", &libdar::crit_and::evaluate)
	.def("clone", &libdar::crit_and::clone)
	.def("add_crit", &libdar::crit_and::add_crit)
	.def("clear", &libdar::crit_and::clear)
	.def("gobe", &libdar::crit_and::gobe);

    pybind11::class_<libdar::crit_or, libdar::crit_and>(mod, "crit_or")
	.def(pybind11::init<>())
	.def("evaluate", &libdar::crit_or::evaluate)
	.def("clone", &libdar::crit_or::clone)
	.def("add_crit", &libdar::crit_or::add_crit)
	.def("clear", &libdar::crit_or::clear)
	.def("gobe", &libdar::crit_or::gobe);

    pybind11::class_<libdar::crit_invert, libdar::crit_not>(mod, "crit_invert")
	.def(pybind11::init<const libdar::criterium &>())
	.def("evaluate", &libdar::crit_invert::evaluate)
	.def("clone", &libdar::crit_invert::clone);


	///////////////////////////////////////////
	// binding for what's in crit_action.hpp
	//

    pybind11::enum_<libdar::saved_status>(mod, "saved_status")
	.value("saved", libdar::saved_status::saved)
	.value("inode_only", libdar::saved_status::inode_only)
	.value("fake", libdar::saved_status::fake)
	.value("not_saved", libdar::saved_status::not_saved)
	.value("delta", libdar::saved_status::delta)
	.export_values();

    pybind11::enum_<libdar::over_action_data>(mod, "over_action_data")
	.value("data_preserve", libdar::data_preserve)
	.value("data_overwrite", libdar::data_overwrite)
	.value("data_preserve_mark_already_saved", libdar::data_preserve_mark_already_saved)
	.value("data_overwrite_mark_already_saved", libdar::data_overwrite_mark_already_saved)
	.value("data_remove", libdar::data_remove)
	.value("data_undefined", libdar::data_undefined)
	.value("data_ask", libdar::data_ask)
	.export_values();

    pybind11::enum_<libdar::over_action_ea>(mod, "over_action_ea")
	.value("EA_preserve", libdar::EA_preserve)
	.value("EA_overwrite", libdar::EA_overwrite)
	.value("EA_clear", libdar::EA_clear)
	.value("EA_preserve_mark_already_saved", libdar::EA_preserve_mark_already_saved)
	.value("EA_overwrite_mark_already_saved", libdar::EA_overwrite_mark_already_saved)
	.value("EA_merge_preserve", libdar::EA_merge_preserve)
	.value("EA_merge_overwrite", libdar::EA_merge_overwrite)
	.value("EA_undefined", libdar::EA_undefined)
	.value("EA_ask", libdar::EA_ask)
	.export_values();

    class pycrit_action : public libdar::crit_action
    {
    public:
	virtual void get_action(const libdar::cat_nomme & first, const libdar::cat_nomme & second, libdar::over_action_data & data, libdar::over_action_ea & ea) const override
	{
	    PYBIND11_OVERLOAD_PURE(void,
				   libdar::crit_action,
				   get_action,
				   first,
				   second,
				   data,
				   ea);
	};

	virtual libdar::crit_action *clone() const override
	{
	    PYBIND11_OVERLOAD_PURE(libdar::crit_action *,
				   libdar::crit_action,
				   clone,); // training comma is expected as there is no argument to this method
	};
    };

    pybind11::class_<libdar::crit_action, pycrit_action>(mod, "crit_action")
	.def(pybind11::init<>())
	.def("get_action", &libdar::crit_action::get_action)
	.def("clone", &libdar::crit_action::clone);

    pybind11::class_<libdar::crit_constant_action, libdar::crit_action>(mod, "crit_constant_action")
	.def(pybind11::init<libdar::over_action_data, libdar::over_action_ea>())
	.def("get_action", &libdar::crit_constant_action::get_action)
	.def("clone", &libdar::crit_constant_action::clone);

    pybind11::class_<libdar::testing, libdar::crit_action>(mod, "testing")
	.def(pybind11::init<const libdar::criterium &, const libdar::crit_action &, const libdar::crit_action &>())
	.def("get_action", &libdar::testing::get_action)
	.def("clone", &libdar::testing::clone);

    pybind11::class_<libdar::crit_chain, libdar::crit_action>(mod, "crit_chain")
	.def(pybind11::init<>())
	.def("add", &libdar::crit_chain::add)
	.def("clear", &libdar::crit_chain::clear)
	.def("gobe", &libdar::crit_chain::gobe)
	.def("get_action", &libdar::crit_chain::get_action)
	.def("clone", &libdar::crit_chain::clone);


	///////////////////////////////////////////
	// secu_string classes
	//

    pybind11::class_<libdar::secu_string>(mod, "secu_string")
	.def_static("is_string_secured", &libdar::secu_string::is_string_secured)
	.def(pybind11::init<libdar::U_I>(), pybind11::arg("storage_size") = 0)
	.def(pybind11::init<const char *, libdar::U_I>())
	.def(pybind11::self != pybind11::self)
	.def(pybind11::self == pybind11::self)
	.def("set", &libdar::secu_string::set)
	.def("append_at", (void (libdar::secu_string::*)(libdar::U_I, const char *, libdar::U_I))&libdar::secu_string::append_at)
	.def("append_at", (void (libdar::secu_string::*)(libdar::U_I, int, libdar::U_I))&libdar::secu_string::append_at)
	.def("append", (void (libdar::secu_string::*)(const char *, libdar::U_I)) &libdar::secu_string::append)
	.def("append", (void (libdar::secu_string::*)(int, libdar::U_I)) &libdar::secu_string::append)
	.def("reduce_string_size_to", &libdar::secu_string::reduce_string_size_to)
	.def("clear", &libdar::secu_string::clear)
	.def("resize", &libdar::secu_string::resize)
	.def("randomize", &libdar::secu_string::randomize)
	.def("c_str", &libdar::secu_string::c_str, pybind11::return_value_policy::reference_internal)
	.def("get_array", &libdar::secu_string::get_array, pybind11::return_value_policy::reference_internal)
	.def("at", [](libdar::secu_string & self, libdar::U_I index) { return std::string(1, self[index]); }) // not a temporary secure storage but no char type in python... so
	.def("get_size", &libdar::secu_string::get_size)
	.def("empty", &libdar::secu_string::empty)
	.def("get_allocated_size", &libdar::secu_string::get_allocated_size);


	///////////////////////////////////////////
	// archive_options_listing_shell classes
	//

    pybind11::class_<libdar::archive_options_listing_shell> aols(mod, "archive_options_listing_shell");

    pybind11::enum_<libdar::archive_options_listing_shell::listformat>(aols, "listformat")
	.value("normal", libdar::archive_options_listing_shell::normal)
	.value("tree", libdar::archive_options_listing_shell::tree)
	.value("xml", libdar::archive_options_listing_shell::xml)
	.value("slicing", libdar::archive_options_listing_shell::slicing);

    aols
	.def("clear", &libdar::archive_options_listing_shell::clear)
	.def("set_list_mode", &libdar::archive_options_listing_shell::set_list_mode)
	.def("set_sizes_in_bytes", &libdar::archive_options_listing_shell::set_sizes_in_bytes)
	.def("get_list_mode", &libdar::archive_options_listing_shell::get_list_mode)
	.def("get_sizes_in_bytes", &libdar::archive_options_listing_shell::get_sizes_in_bytes);

	///////////////////////////////////////////
	// user_interaction classes
	//

    class py_user_interaction : public libdar::user_interaction
    {
    public:
	using libdar::user_interaction::user_interaction;

    protected:
	virtual void inherited_message(const std::string & message) override
	{
	    PYBIND11_OVERLOAD_PURE(
		void,
		libdar::user_interaction,
		inherited_message,
		message);
	};

	virtual bool inherited_pause(const std::string & message) override
	{
	    PYBIND11_OVERLOAD_PURE(
		bool,
		libdar::user_interaction,
		inherited_pause,
		message);
	};

	virtual std::string inherited_get_string(const std::string & message, bool echo) override
	{
	    PYBIND11_OVERLOAD_PURE(
		std::string,
		libdar::user_interaction,
		inherited_get_string,
		message,
		echo);
	}

	virtual libdar::secu_string inherited_get_secu_string(const std::string & message, bool echo) override
	{
	    PYBIND11_OVERLOAD_PURE(
		libdar::secu_string,
		libdar::user_interaction,
		inherited_get_secu_string,
		message,
		echo);
	}

    private:
	using libdar::user_interaction::printf;
    };

    pybind11::class_<libdar::user_interaction, std::shared_ptr<libdar::user_interaction>, py_user_interaction>(mod, "user_interaction")
	.def(pybind11::init<>())
	.def("message", &libdar::user_interaction::message)
	.def("pause", &libdar::user_interaction::pause)
	.def("get_string", &libdar::user_interaction::get_string)
	.def("get_secu_string", &libdar::user_interaction::get_secu_string);

/* example of python class inheriting from libdar::user_interaction
impoty libdar

class myui(libdar.user_interaction):
    def __init__(self):
        libdar.user_interaction.__init__(self)
	# this is mandatory to initialize the parent class!!!

	# the following methods are protected in C++
	# but can defined in python (public scope)
        # the inner code will be bound to the protected
	# method of the same name

    def inherited_message(self, msg):
        print("LIBDAR MESSAGE:{0}".format(msg))

    def inherited_pause(self, msg):
        while True:
            res = input("LIBDAR QUESTION:{0} y/n ".format(msg))
            if res == "y":
                return True
            else:
                if res == "n":
                    return False
                else:
                    print("answer 'y' or 'n'")

    def inherited_get_string(self, msg, echo):
        return input(msg)

    def inherited_get_secu_string(self, msg, echo):
        return input(msg)
*/

    	///////////////////////////////////////////
	// entrepot_* classes
	//

    pybind11::class_<libdar::entrepot_local, std::shared_ptr<libdar::entrepot_local> >(mod, "entrepot_local")
	.def(pybind11::init<const std::string &, const std::string &, bool>())
	.def("get_url", &libdar::entrepot_local::get_url)
	.def("read_dir_reset", &libdar::entrepot_local::read_dir_reset)
    	.def("read_dir_next", &libdar::entrepot_local::read_dir_next)
	.def("clone", &libdar::entrepot_local::clone)
	.def("set_location", &libdar::entrepot_local::set_location)
	.def("set_root", &libdar::entrepot_local::set_root)
	.def("get_full_path", &libdar::entrepot_local::get_full_path)
	.def("get_location", &libdar::entrepot_local::get_location)
	.def("get_root", &libdar::entrepot_local::get_root);

    pybind11::enum_<libdar::mycurl_protocol>(mod, "mycurl_protocol")
	.value("proto_ftp", libdar::mycurl_protocol::proto_ftp)
	.value("proto_sftp", libdar::mycurl_protocol::proto_sftp);

    pybind11::class_<libdar::entrepot_libcurl, std::shared_ptr<libdar::entrepot_libcurl> >(mod, "entrepot_libcurl")
	.def(pybind11::init<
	     const std::shared_ptr<libdar::user_interaction> &,
	     libdar::mycurl_protocol,
	     const std::string &,
	     const libdar::secu_string &,
	     const std::string &,
	     const std::string &,
	     bool,
	     const std::string &,
	     const std::string &,
	     const std::string &,
	     libdar::U_I>())
	.def("get_url", &libdar::entrepot_libcurl::get_url)
	.def("read_dir_reset", &libdar::entrepot_libcurl::read_dir_reset)
    	.def("read_dir_next", &libdar::entrepot_libcurl::read_dir_next)
	.def("clone", &libdar::entrepot_libcurl::clone)
	.def("set_location", &libdar::entrepot_libcurl::set_location)
	.def("set_root", &libdar::entrepot_libcurl::set_root)
	.def("get_full_path", &libdar::entrepot_libcurl::get_full_path)
	.def("get_location", &libdar::entrepot_libcurl::get_location)
	.def("get_root", &libdar::entrepot_libcurl::get_root);


    	///////////////////////////////////////////
	// fsa_* classes / enums
	//

    pybind11::enum_<libdar::fsa_family>(mod, "fsa_family")
	.value("fsaf_hfs_plus", libdar::fsa_family::fsaf_hfs_plus)
	.value("fsaf_linux_extX", libdar::fsa_family::fsaf_linux_extX);

    pybind11::enum_<libdar::fsa_nature>(mod, "fsa_nature")
	.value("fsan_unset", libdar::fsa_nature::fsan_unset)
	.value("fsan_creation_date", libdar::fsa_nature::fsan_creation_date)
	.value("fsan_append_only", libdar::fsa_nature::fsan_append_only)
	.value("fsan_compressed", libdar::fsa_nature::fsan_compressed)
	.value("fsan_no_dump", libdar::fsa_nature::fsan_no_dump)
	.value("fsan_immutable", libdar::fsa_nature::fsan_immutable)
	.value("fsan_data_journaling", libdar::fsa_nature::fsan_data_journaling)
	.value("fsan_secure_deletion", libdar::fsa_nature::fsan_secure_deletion)
	.value("fsan_no_tail_merging", libdar::fsa_nature::fsan_no_tail_merging)
	.value("fsan_undeletable", libdar::fsa_nature::fsan_undeletable)
	.value("fsan_noatime_update", libdar::fsa_nature::fsan_noatime_update)
	.value("fsan_sychronous_directory", libdar::fsa_nature::fsan_synchronous_directory)
	.value("fsan_sychronous_update", libdar::fsa_nature::fsan_synchronous_update)
	.value("fsan_top_of_dir_hierachy", libdar::fsa_nature::fsan_top_of_dir_hierarchy);

    mod.def("fsa_family_to_string", &libdar::fsa_family_to_string);
    mod.def("fsa_nature_to_string", &libdar::fsa_nature_to_string);
    mod.def("all_fsa_families", &libdar::all_fsa_families);
    mod.def("fsa_scope_to_infinint", &libdar::fsa_scope_to_infinint);
    mod.def("infinint_to_fsa_scope", &libdar::infinint_to_fsa_scope);
    mod.def("fsa_scope_to_string", &libdar::fsa_scope_to_string);


	///////////////////////////////////////////
	// compile_time routines
	//

    class compile_time
    {
    public:
	static bool ea() noexcept { return libdar::compile_time::ea(); };
	static bool largefile() noexcept { return libdar::compile_time::largefile(); };
	static bool nodump() noexcept { return libdar::compile_time::nodump(); };
	static bool special_alloc() noexcept { return libdar::compile_time::special_alloc(); };
	static libdar::U_I bits() noexcept { return libdar::compile_time::bits(); };
	static bool thread_safe() noexcept { return libdar::compile_time::thread_safe(); };
	static bool libz() noexcept { return libdar::compile_time::libz(); };
	static bool libbz2() noexcept { return libdar::compile_time::libbz2(); };
	static bool liblzo() noexcept { return libdar::compile_time::liblzo(); };
	static bool libxz() noexcept { return libdar::compile_time::libxz(); };
	static bool libgcrypt() noexcept { return libdar::compile_time::libgcrypt(); };
	static bool furtive_read() noexcept { return libdar::compile_time::furtive_read(); };
	static bool system_endian() noexcept { return libdar::compile_time::system_endian(); };
	static bool posix_fadvise() noexcept { return libdar::compile_time::posix_fadvise(); };
	static bool fast_dir() noexcept { return libdar::compile_time::fast_dir(); };
	static bool FSA_linux_extX() noexcept { return libdar::compile_time::FSA_linux_extX(); };
	static bool FSA_birthtime() noexcept { return libdar::compile_time::FSA_birthtime(); };
	static bool microsecond_read() noexcept { return libdar::compile_time::microsecond_read(); };
	static bool microsecond_write() noexcept { return libdar::compile_time::microsecond_write(); };
	static bool symlink_restore_dates() noexcept { return libdar::compile_time::symlink_restore_dates(); };
	static bool public_key_cipher() noexcept { return libdar::compile_time::public_key_cipher(); };
	static bool libthreadar() noexcept { return libdar::compile_time::libthreadar(); };
	static std::string libthreadar_version() noexcept { return libdar::compile_time::libthreadar_version(); };
	static bool librsync() noexcept { return libdar::compile_time::librsync(); };
	static bool remote_repository() noexcept { return libdar::compile_time::remote_repository(); };
    };

    pybind11::class_<compile_time> c_t(mod, "compile_time");

    pybind11::enum_<libdar::compile_time::endian>(c_t, "endian")
	.value("big", libdar::compile_time::big)
	.value("little", libdar::compile_time::little)
	.value("error", libdar::compile_time::error);

    c_t
	.def_static("ea", &compile_time::ea)
	.def_static("largefile", &compile_time::largefile)
	.def_static("nodump", &compile_time::nodump)
	.def_static("special_alloc", &compile_time::special_alloc)
	.def_static("bits", &compile_time::bits)
	.def_static("thread_safe", &compile_time::thread_safe)
	.def_static("libz", &compile_time::libz)
	.def_static("libbz2", &compile_time::libbz2)
	.def_static("liblzo", &compile_time::liblzo)
	.def_static("libxz", &compile_time::libxz)
	.def_static("libgcrypt", &compile_time::libgcrypt)
	.def_static("furtive_read", &compile_time::furtive_read)
	.def_static("system_endian", &compile_time::system_endian)
	.def_static("posix_fadvise", &compile_time::posix_fadvise)
	.def_static("fast_dir", &compile_time::fast_dir)
	.def_static("FSA_linux_extX", &compile_time::FSA_linux_extX)
	.def_static("FSA_birthtime", &compile_time::FSA_birthtime)
	.def_static("microsecond_read", &compile_time::microsecond_read)
	.def_static("microsecond_write", &compile_time::microsecond_write)
	.def_static("symlink_restore_dates", &compile_time::symlink_restore_dates)
	.def_static("public_key_cipher", &compile_time::public_key_cipher)
	.def_static("libthreadar", &compile_time::libthreadar)
	.def_static("libthreadar_version", &compile_time::libthreadar_version)
	.def_static("librsync", &compile_time::librsync)
	.def_static("remote_repository", &compile_time::remote_repository);


	///////////////////////////////////////////
	// archive_aux_* structures and routines
	//

    pybind11::enum_<libdar::modified_data_detection>(mod, "modified_data_detection")
	.value("any_inode_change", libdar::modified_data_detection::any_inode_change)
	.value("mtime_size", libdar::modified_data_detection::mtime_size);

    pybind11::enum_<libdar::comparison_fields>(mod, "comparison_fields")
	.value("all", libdar::comparison_fields::all)
	.value("ignore_owner", libdar::comparison_fields::ignore_owner)
	.value("mtime", libdar::comparison_fields::mtime)
	.value("inode_type", libdar::comparison_fields::inode_type);

    pybind11::enum_<libdar::hash_algo>(mod, "hash_algo")
	.value("none", libdar::hash_algo::none)
	.value("md5", libdar::hash_algo::md5)
	.value("sha1", libdar::hash_algo::sha1)
	.value("sha512", libdar::hash_algo::sha512);

    mod.def("hash_algo_to_string", &libdar::hash_algo_to_string);
    mod.def("string_to_hash_algo", &libdar::string_to_hash_algo);

    	///////////////////////////////////////////
	// compression_* structures and routines
	//

    pybind11::enum_<libdar::compression>(mod, "compression")
	.value("none", libdar::compression::none)
	.value("gzip", libdar::compression::gzip)
	.value("bzip2", libdar::compression::bzip2)
	.value("lzo", libdar::compression::lzo)
	.value("xz", libdar::compression::xz)
	.value("lzo1x_1_15", libdar::compression::lzo1x_1_15)
	.value("lzo1x_1", libdar::compression::lzo1x_1);

    mod.def("compression2string", &libdar::compression2string);
    mod.def("string2compression", &libdar::string2compression);

	///////////////////////////////////////////
	// delta_sig_block_size datastructure
	//

    pybind11::class_<libdar::delta_sig_block_size> dsbs(mod, "delta_sig_block_size");

    pybind11::enum_<libdar::delta_sig_block_size::fs_function_t>(dsbs, "fs_function_t")
	.value("fixed", libdar::delta_sig_block_size::fixed)
	.value("linear", libdar::delta_sig_block_size::linear)
	.value("log2", libdar::delta_sig_block_size::log2)
	.value("square2", libdar::delta_sig_block_size::square2)
	.value("square3", libdar::delta_sig_block_size::square3);

    dsbs
	.def_readwrite("fs_function", &libdar::delta_sig_block_size::fs_function)
	.def_readwrite("multiplier", &libdar::delta_sig_block_size::multiplier)
	.def_readwrite("divisor", &libdar::delta_sig_block_size::divisor)
	.def_readwrite("min_block_len", &libdar::delta_sig_block_size::min_block_len)
	.def_readwrite("max_block_len", &libdar::delta_sig_block_size::max_block_len)
	.def(pybind11::self == pybind11::self)
	.def("reset", &libdar::delta_sig_block_size::reset)
	.def("equals_defaults", &libdar::delta_sig_block_size::equals_default)
	.def("check", &libdar::delta_sig_block_size::check)
	.def("calculate", &libdar::delta_sig_block_size::calculate);


    	///////////////////////////////////////////
	// archive_options_* classes
	//

    pybind11::class_<libdar::archive_options_read>(mod, "archive_options_read")
	.def("clear", &libdar::archive_options_read::clear)
	.def("set_crypto_algo", &libdar::archive_options_read::set_crypto_algo)
	.def("set_crypto_pass", &libdar::archive_options_read::set_crypto_pass)
	.def("set_crypto_size", &libdar::archive_options_read::set_crypto_size)
	.def("set_default_crypto_size", &libdar::archive_options_read::set_default_crypto_size)
	.def("set_input_pipe", &libdar::archive_options_read::set_input_pipe)
	.def("set_output_pipe", &libdar::archive_options_read::set_output_pipe)
	.def("set_execute", &libdar::archive_options_read::set_execute)
	.def("set_info_details", &libdar::archive_options_read::set_info_details)
	.def("set_lax", &libdar::archive_options_read::set_lax)
	.def("set_sequential_read", &libdar::archive_options_read::set_sequential_read)
	.def("set_slice_min_digits", &libdar::archive_options_read::set_slice_min_digits)
	.def("set_entrepot", &libdar::archive_options_read::set_entrepot)
	.def("set_ignore_signature_check_failure", &libdar::archive_options_read::set_ignore_signature_check_failure)
	.def("set_multi_threaded", &libdar::archive_options_read::set_multi_threaded)
	.def("set_external_catalogue", &libdar::archive_options_read::set_external_catalogue)
	.def("unset_external_catalogue", &libdar::archive_options_read::unset_external_catalogue)
	.def("set_ref_crypto_algo", &libdar::archive_options_read::set_ref_crypto_algo)
	.def("set_ref_crypto_pass", &libdar::archive_options_read::set_ref_crypto_pass)
	.def("set_ref_crypto_size", &libdar::archive_options_read::set_ref_crypto_size)
	.def("set_ref_execute", &libdar::archive_options_read::set_ref_execute)
	.def("set_ref_slice_min_digits", &libdar::archive_options_read::set_ref_slice_min_digits)
	.def("set_ref_entrepot", &libdar::archive_options_read::set_ref_entrepot)
	.def("set_header_only", &libdar::archive_options_read::set_header_only);


    pybind11::class_<libdar::archive_options_create>(mod, "archive_options_create")
	.def("clear", &libdar::archive_options_create::clear)
	.def("set_reference", &libdar::archive_options_create::set_reference)
	.def("set_selection", &libdar::archive_options_create::set_selection)
	.def("set_subtree", &libdar::archive_options_create::set_subtree)
	.def("set_allow_over", &libdar::archive_options_create::set_allow_over)
	.def("set_warn_over", &libdar::archive_options_create::set_warn_over)
	.def("set_info_details", &libdar::archive_options_create::set_info_details)
	.def("set_display_treated", &libdar::archive_options_create::set_display_treated)
	.def("set_display_skipped", &libdar::archive_options_create::set_display_skipped)
	.def("set_display_finished", &libdar::archive_options_create::set_display_finished)
	.def("set_pause", &libdar::archive_options_create::set_pause)
	.def("set_empty_dir", &libdar::archive_options_create::set_empty_dir)
	.def("set_compression", &libdar::archive_options_create::set_compression)
	.def("set_compression_level", &libdar::archive_options_create::set_compression_level)
	.def("set_slicing", &libdar::archive_options_create::set_slicing)
	.def("set_ea_mask", &libdar::archive_options_create::set_ea_mask)
	.def("set_execute", &libdar::archive_options_create::set_execute)
	.def("set_crypto_algo", &libdar::archive_options_create::set_crypto_algo)
	.def("set_crypto_pass", &libdar::archive_options_create::set_crypto_pass)
	.def("set_crypto_size", &libdar::archive_options_create::set_crypto_size)
	.def("set_gnupg_recipients", &libdar::archive_options_create::set_gnupg_recipients)
	.def("set_gnupg_signatories", &libdar::archive_options_create::set_gnupg_signatories)
	.def("set_compr_mask", &libdar::archive_options_create::set_compr_mask)
	.def("set_min_compr_size", &libdar::archive_options_create::set_min_compr_size)
	.def("set_nodump", &libdar::archive_options_create::set_nodump)
	.def("set_exclude_by_ea", &libdar::archive_options_create::set_exclude_by_ea)
	.def("set_what_to_check", &libdar::archive_options_create::set_what_to_check)
	.def("set_hourshift", &libdar::archive_options_create::set_hourshift)
	.def("set_empty", &libdar::archive_options_create::set_empty)
	.def("set_alter_atime", &libdar::archive_options_create::set_alter_atime)
	.def("set_furtive_read_mode", &libdar::archive_options_create::set_furtive_read_mode)
	.def("set_same_fs", &libdar::archive_options_create::set_same_fs)
	.def("set_snapshot", &libdar::archive_options_create::set_snapshot)
	.def("set_cache_directory_tagging", &libdar::archive_options_create::set_cache_directory_tagging)
	.def("set_fixed_date", &libdar::archive_options_create::set_fixed_date)
	.def("set_slice_permission", &libdar::archive_options_create::set_slice_permission)
	.def("set_slice_user_ownership", &libdar::archive_options_create::set_slice_user_ownership)
	.def("set_slice_group_ownership", &libdar::archive_options_create::set_slice_group_ownership)
	.def("set_retry_on_change", &libdar::archive_options_create::set_retry_on_change)
	.def("set_sequential_marks", &libdar::archive_options_create::set_sequential_marks)
	.def("set_sparse_file_min_size", &libdar::archive_options_create::set_sparse_file_min_size)
	.def("set_security_check", &libdar::archive_options_create::set_security_check)
	.def("set_user_comment", &libdar::archive_options_create::set_user_comment)
	.def("set_hash_algo", &libdar::archive_options_create::set_hash_algo)
	.def("set_slice_min_digits", &libdar::archive_options_create::set_slice_min_digits)
	.def("set_backup_hook", &libdar::archive_options_create::set_backup_hook)
	.def("set_ignore_unknow_inode_type", &libdar::archive_options_create::set_ignore_unknown_inode_type)
	.def("set_entrepot", &libdar::archive_options_create::set_entrepot)
	.def("set_fsa_scope", &libdar::archive_options_create::set_fsa_scope)
	.def("set_multi_threaded", &libdar::archive_options_create::set_multi_threaded)
	.def("set_delta_diff", &libdar::archive_options_create::set_delta_diff)
	.def("set_delta_signature", &libdar::archive_options_create::set_delta_signature)
	.def("set_delta_mask", &libdar::archive_options_create::set_delta_mask)
	.def("set_delta_sig_min_size", &libdar::archive_options_create::set_delta_sig_min_size)
	.def("set_auto_zeroing_neg_dates", &libdar::archive_options_create::set_auto_zeroing_neg_dates)
	.def("set_ignored_as_symlink", &libdar::archive_options_create::set_ignored_as_symlink)
	.def("set_modified_data_detection", &libdar::archive_options_create::set_modified_data_detection)
	.def("set_iteration_count", &libdar::archive_options_create::set_iteration_count)
	.def("set_kdf_hash", &libdar::archive_options_create::set_kdf_hash)
	.def("set_sig_block_len", &libdar::archive_options_create::set_sig_block_len);

    pybind11::class_<libdar::archive_options_isolate>(mod, "archive_options_isolate")
	.def("clear", &libdar::archive_options_isolate::clear)
	.def("set_allow_over", &libdar::archive_options_isolate::set_allow_over)
	.def("set_warn_over", &libdar::archive_options_isolate::set_warn_over)
	.def("set_info_details", &libdar::archive_options_isolate::set_info_details)
	.def("set_pause", &libdar::archive_options_isolate::set_pause)
	.def("set_compression", &libdar::archive_options_isolate::set_compression)
	.def("set_compression_level", &libdar::archive_options_isolate::set_compression_level)
	.def("set_slicing", &libdar::archive_options_isolate::set_slicing)
	.def("set_execute", &libdar::archive_options_isolate::set_execute)
	.def("set_crypto_algo", &libdar::archive_options_isolate::set_crypto_algo)
	.def("set_crypto_pass", &libdar::archive_options_isolate::set_crypto_pass)
	.def("set_crypto_size", &libdar::archive_options_isolate::set_crypto_size)
	.def("set_gnupg_recipients", &libdar::archive_options_isolate::set_gnupg_recipients)
	.def("set_gnupg_signatories", &libdar::archive_options_isolate::set_gnupg_signatories)
	.def("set_empty", &libdar::archive_options_isolate::set_empty)
	.def("set_slice_permission", &libdar::archive_options_isolate::set_slice_permission)
	.def("set_slice_user_ownership", &libdar::archive_options_isolate::set_slice_user_ownership)
	.def("set_slice_group_ownership", &libdar::archive_options_isolate::set_slice_group_ownership)
	.def("set_user_comment", &libdar::archive_options_isolate::set_user_comment)
	.def("set_hash_algo", &libdar::archive_options_isolate::set_hash_algo)
	.def("set_slice_min_digits", &libdar::archive_options_isolate::set_slice_min_digits)
	.def("set_sequential_marks", &libdar::archive_options_isolate::set_sequential_marks)
	.def("set_entrepot", &libdar::archive_options_isolate::set_entrepot)
	.def("set_multi_threaded", &libdar::archive_options_isolate::set_multi_threaded)
	.def("set_delta_signature", &libdar::archive_options_isolate::set_delta_signature)
	.def("set_delta_mask", &libdar::archive_options_isolate::set_delta_mask)
	.def("set_delta_sig_min_size", &libdar::archive_options_isolate::set_delta_sig_min_size)
	.def("set_iteration_count", &libdar::archive_options_isolate::set_iteration_count)
	.def("set_kdf_hash", &libdar::archive_options_isolate::set_kdf_hash)
	.def("set_sig_block_len", &libdar::archive_options_isolate::set_sig_block_len);


    pybind11::class_<libdar::archive_options_merge>(mod, "archive_options_merge")
	.def("clear", &libdar::archive_options_merge::clear)
	.def("set_auxiliary_ref", &libdar::archive_options_merge::set_auxiliary_ref)
	.def("set_selection", &libdar::archive_options_merge::set_selection)
	.def("set_subtree", &libdar::archive_options_merge::set_subtree)
	.def("set_allow_over", &libdar::archive_options_merge::set_allow_over)
	.def("set_warn_over", &libdar::archive_options_merge::set_warn_over)
	.def("set_overwriting_rules", &libdar::archive_options_merge::set_overwriting_rules)
	.def("set_info_details", &libdar::archive_options_merge::set_info_details)
	.def("set_display_treated", &libdar::archive_options_merge::set_display_treated)
	.def("set_display_skipped", &libdar::archive_options_merge::set_display_skipped)
	.def("set_pause", &libdar::archive_options_merge::set_pause)
	.def("set_empty_dir", &libdar::archive_options_merge::set_empty_dir)
	.def("set_compression", &libdar::archive_options_merge::set_compression)
	.def("set_compression_level", &libdar::archive_options_merge::set_compression_level)
	.def("set_slicing", &libdar::archive_options_merge::set_slicing)
	.def("set_ea_mask", &libdar::archive_options_merge::set_ea_mask)
	.def("set_execute", &libdar::archive_options_merge::set_execute)
	.def("set_crypto_algo", &libdar::archive_options_merge::set_crypto_algo)
	.def("set_crypto_pass", &libdar::archive_options_merge::set_crypto_pass)
	.def("set_crypto_size", &libdar::archive_options_merge::set_crypto_size)
	.def("set_gnupg_recipients", &libdar::archive_options_merge::set_gnupg_recipients)
	.def("set_gnupg_signatories", &libdar::archive_options_merge::set_gnupg_signatories)
	.def("set_compr_mask", &libdar::archive_options_merge::set_compr_mask)
	.def("set_min_compr_size", &libdar::archive_options_merge::set_min_compr_size)
	.def("set_empty", &libdar::archive_options_merge::set_empty)
	.def("set_keep_compressed", &libdar::archive_options_merge::set_keep_compressed)
	.def("set_slice_permission", &libdar::archive_options_merge::set_slice_permission)
	.def("set_slice_user_ownership", &libdar::archive_options_merge::set_slice_user_ownership)
	.def("set_slice_group_ownership", &libdar::archive_options_merge::set_slice_group_ownership)
	.def("set_decremental_mode", &libdar::archive_options_merge::set_decremental_mode)
	.def("set_sequential_marks", &libdar::archive_options_merge::set_sequential_marks)
	.def("set_sparse_file_min_size", &libdar::archive_options_merge::set_sparse_file_min_size)
	.def("set_user_comment", &libdar::archive_options_merge::set_user_comment)
	.def("set_hash_algo", &libdar::archive_options_merge::set_hash_algo)
	.def("set_slice_min_digits", &libdar::archive_options_merge::set_slice_min_digits)
	.def("set_entrepot", &libdar::archive_options_merge::set_entrepot)
	.def("set_fsa_scope", &libdar::archive_options_merge::set_fsa_scope)
	.def("set_mutli_threaded", &libdar::archive_options_merge::set_multi_threaded)
	.def("set_delta_signature", &libdar::archive_options_merge::set_delta_signature)
	.def("set_delta_mask", &libdar::archive_options_merge::set_delta_mask)
	.def("set_delta_sig_min_size", &libdar::archive_options_merge::set_delta_sig_min_size)
	.def("set_iteration_count", &libdar::archive_options_merge::set_iteration_count)
	.def("set_kdf_hash", &libdar::archive_options_merge::set_kdf_hash)
	.def("set_sig_block_len", &libdar::archive_options_merge::set_sig_block_len);


    pybind11::class_<libdar::archive_options_extract> py_archive_options_extract(mod, "archive_options_extract");

    pybind11::enum_<libdar::archive_options_extract::t_dirty>(py_archive_options_extract, "t_dirty")
	.value("dirty_ignore",libdar::archive_options_extract::t_dirty::dirty_ignore)
	.value("dirty_warn",libdar::archive_options_extract::t_dirty::dirty_warn)
	.value("dirty_ok", libdar::archive_options_extract::t_dirty::dirty_ok);

    py_archive_options_extract
	.def("clear", &libdar::archive_options_extract::clear)
	.def("set_selection", &libdar::archive_options_extract::set_selection)
	.def("set_subtree", &libdar::archive_options_extract::set_subtree)
	.def("set_warn_over", &libdar::archive_options_extract::set_warn_over)
	.def("set_info_details", &libdar::archive_options_extract::set_info_details)
	.def("set_display_treated", &libdar::archive_options_extract::set_display_treated)
	.def("set_display_skipped", &libdar::archive_options_extract::set_display_skipped)
	.def("set_ea_mask", &libdar::archive_options_extract::set_ea_mask)
	.def("set_flat", &libdar::archive_options_extract::set_flat)
	.def("set_what_to_check", &libdar::archive_options_extract::set_what_to_check)
	.def("set_warn_remove_no_match", &libdar::archive_options_extract::set_warn_remove_no_match)
	.def("set_empty", &libdar::archive_options_extract::set_empty)
	.def("set_empty_dir", &libdar::archive_options_extract::set_empty_dir)
	.def("set_dirty_behavior", (void (libdar::archive_options_extract::*)(bool, bool)) &libdar::archive_options_extract::set_dirty_behavior)
	.def("set_dirty_behavior", (void (libdar::archive_options_extract::*)(libdar::archive_options_extract::t_dirty)) &libdar::archive_options_extract::set_dirty_behavior)
	.def("set_overwriting_rules", &libdar::archive_options_extract::set_overwriting_rules)
	.def("set_only_deleted", &libdar::archive_options_extract::set_only_deleted)
	.def("set_ignore_deleted", &libdar::archive_options_extract::set_ignore_deleted)
	.def("set_fsa_scope", &libdar::archive_options_extract::set_fsa_scope);


    pybind11::class_<libdar::archive_options_listing>(mod, "archive_options_listing")
	.def("clear", &libdar::archive_options_listing::clear)
	.def("set_info_details", &libdar::archive_options_listing::set_info_details)
	.def("set_selection", &libdar::archive_options_listing::set_selection)
	.def("set_subtree", &libdar::archive_options_listing::set_subtree)
	.def("set_filter_unsaved", &libdar::archive_options_listing::set_filter_unsaved)
	.def("set_slicing_location", &libdar::archive_options_listing::set_slicing_location)
	.def("set_user_slicing", &libdar::archive_options_listing::set_user_slicing)
	.def("set_display_ea", &libdar::archive_options_listing::set_display_ea);

    pybind11::class_<libdar::archive_options_diff>(mod, "archive_options_diff")
	.def("clear", &libdar::archive_options_diff::clear)
	.def("set_selection", &libdar::archive_options_diff::set_selection)
	.def("set_subtree", &libdar::archive_options_diff::set_subtree)
	.def("set_info_details", &libdar::archive_options_diff::set_info_details)
	.def("set_display_treated", &libdar::archive_options_diff::set_display_treated)
	.def("set_display_skipped", &libdar::archive_options_diff::set_display_skipped)
	.def("set_ea_mask", &libdar::archive_options_diff::set_ea_mask)
	.def("set_what_to_check", &libdar::archive_options_diff::set_what_to_check)
	.def("set_alter_atime", &libdar::archive_options_diff::set_alter_atime)
	.def("set_furtive_read_mode", &libdar::archive_options_diff::set_furtive_read_mode)
	.def("set_hourshift", &libdar::archive_options_diff::set_hourshift)
	.def("set_compare_symlink_date", &libdar::archive_options_diff::set_compare_symlink_date)
	.def("set_fsa_scope", &libdar::archive_options_diff::set_fsa_scope);


    pybind11::class_<libdar::archive_options_test>(mod, "archive_options_test")
	.def("clear", &libdar::archive_options_test::clear)
	.def("set_selection", &libdar::archive_options_test::set_selection)
	.def("set_subtree", &libdar::archive_options_test::set_subtree)
	.def("set_info_details", &libdar::archive_options_test::set_info_details)
	.def("set_display_skipped", &libdar::archive_options_test::set_display_skipped)
	.def("set_display_treated", &libdar::archive_options_test::set_display_treated)
	.def("set_empty", &libdar::archive_options_test::set_empty);


    pybind11::class_<libdar::archive_options_repair>(mod, "archive_options_repair")
	.def("clear", &libdar::archive_options_repair::clear)
	.def("set_allow_over", &libdar::archive_options_repair::set_allow_over)
	.def("set_warn_over", &libdar::archive_options_repair::set_warn_over)
	.def("set_info_details", &libdar::archive_options_repair::set_info_details)
	.def("set_display_treated", &libdar::archive_options_repair::set_display_treated)
	.def("set_display_skipped", &libdar::archive_options_repair::set_display_skipped)
	.def("set_display_finished", &libdar::archive_options_repair::set_display_finished)
	.def("set_pause", &libdar::archive_options_repair::set_pause)
	.def("set_slicing", &libdar::archive_options_repair::set_slicing)
	.def("set_execute", &libdar::archive_options_repair::set_execute)
	.def("set_crypto_algo", &libdar::archive_options_repair::set_crypto_algo)
	.def("set_crypto_pass", &libdar::archive_options_repair::set_crypto_pass)
	.def("set_crypto_size", &libdar::archive_options_repair::set_crypto_size)
	.def("set_gnupg_recipients", &libdar::archive_options_repair::set_gnupg_recipients)
	.def("set_gnupg_signatories", &libdar::archive_options_repair::set_gnupg_signatories)
	.def("set_empty", &libdar::archive_options_repair::set_empty)
	.def("set_slice_permission", &libdar::archive_options_repair::set_slice_permission)
	.def("set_slice_user_ownership", &libdar::archive_options_repair::set_slice_user_ownership)
	.def("set_slice_group_ownership", &libdar::archive_options_repair::set_slice_group_ownership)
	.def("set_user_comment", &libdar::archive_options_repair::set_user_comment)
	.def("set_hash_algo", &libdar::archive_options_repair::set_hash_algo)
	.def("set_slice_min_digits", &libdar::archive_options_repair::set_slice_min_digits)
	.def("set_entrepot", &libdar::archive_options_repair::set_entrepot)
	.def("set_multi_threaded", &libdar::archive_options_repair::set_multi_threaded);



	//////////////////////////////////////////
	// entree_stats classes
	//

    pybind11::class_<libdar::entree_stats>(mod, "entree_stats")
	.def_readonly("num_x", &libdar::entree_stats::num_x)
	.def_readonly("num_d", &libdar::entree_stats::num_d)
	.def_readonly("num_f", &libdar::entree_stats::num_f)
	.def_readonly("num_c", &libdar::entree_stats::num_c)
	.def_readonly("num_b", &libdar::entree_stats::num_b)
	.def_readonly("num_p", &libdar::entree_stats::num_p)
	.def_readonly("num_s", &libdar::entree_stats::num_s)
	.def_readonly("num_l", &libdar::entree_stats::num_l)
	.def_readonly("num_D", &libdar::entree_stats::num_D)
	.def_readonly("num_hard_linked_inodes", &libdar::entree_stats::num_hard_linked_inodes)
	.def_readonly("num_hard_link_entries", &libdar::entree_stats::num_hard_link_entries)
	.def_readonly("saved", &libdar::entree_stats::saved)
	.def_readonly("patched", &libdar::entree_stats::patched)
	.def_readonly("inode_only", &libdar::entree_stats::inode_only)
	.def_readonly("total", &libdar::entree_stats::total)
	.def("clear", &libdar::entree_stats::clear)
	.def("listing", &libdar::entree_stats::listing);

    	///////////////////////////////////////////
	// archive_summary classes
	//

    pybind11::class_<libdar::archive_summary>(mod, "aerchive_summary")
	.def("get_slice_size", &libdar::archive_summary::get_slice_size)
	.def("get_first_slice_size", &libdar::archive_summary::get_first_slice_size)
	.def("get_last_slice_size", &libdar::archive_summary::get_last_slice_size)
	.def("get_slice_number", &libdar::archive_summary::get_slice_number)
	.def("get_archive_size", &libdar::archive_summary::get_archive_size)
	.def("get_catalog_size", &libdar::archive_summary::get_catalog_size)
	.def("get_storage_size", &libdar::archive_summary::get_storage_size)
	.def("get_data_size", &libdar::archive_summary::get_data_size)
	.def("get_contents", &libdar::archive_summary::get_contents)
	.def("get_edition", &libdar::archive_summary::get_edition)
	.def("get_compression_algo", &libdar::archive_summary::get_compression_algo)
	.def("get_user_comment", &libdar::archive_summary::get_user_comment)
	.def("get_cipher", &libdar::archive_summary::get_cipher)
	.def("get_asym", &libdar::archive_summary::get_asym)
	.def("get_signed", &libdar::archive_summary::get_signed)
	.def("get_tape_marks", &libdar::archive_summary::get_tape_marks)
	.def("clear", &libdar::archive_summary::clear);


	///////////////////////////////////////////
	// list_entry classes
	//

    pybind11::class_<libdar::list_entry>(mod, "list_entry")
	.def("is_eod", &libdar::list_entry::is_eod)
	.def("get_name", &libdar::list_entry::get_name)
	.def("get_type", &libdar::list_entry::get_type)
	.def("is_dir", &libdar::list_entry::is_dir)
	.def("is_file", &libdar::list_entry::is_file)
	.def("is_symlink", &libdar::list_entry::is_symlink)
	.def("is_char_device", &libdar::list_entry::is_char_device)
	.def("is_block_device", &libdar::list_entry::is_block_device)
	.def("is_unix_socket", &libdar::list_entry::is_unix_socket)
	.def("is_named_pipe", &libdar::list_entry::is_named_pipe)
	.def("is_hard_linked", &libdar::list_entry::is_hard_linked)
	.def("is_removed_entry", &libdar::list_entry::is_removed_entry)
	.def("is_door_inode", &libdar::list_entry::is_door_inode)
	.def("is_empty_dir", &libdar::list_entry::is_empty_dir)
	.def("get_removed_type", &libdar::list_entry::get_removed_type)
	.def("has_data_present_in_the_archive", &libdar::list_entry::has_data_present_in_the_archive)
	.def("get_data_flag", &libdar::list_entry::get_data_flag)
	.def("get_data_status", &libdar::list_entry::get_data_status)
	.def("has_EA", &libdar::list_entry::has_EA)
	.def("has_EA_saved_in_the_archive", &libdar::list_entry::has_EA_saved_in_the_archive)
	.def("get_ea_flag", &libdar::list_entry::get_ea_flag)
	.def("get_ea_status", &libdar::list_entry::get_ea_status)
	.def("has_FSA", &libdar::list_entry::has_FSA)
	.def("has_FSA_saved_in_the_archive", &libdar::list_entry::has_FSA_saved_in_the_archive)
	.def("get_fsa_flag", &libdar::list_entry::get_fsa_flag)
	.def("get_uid", &libdar::list_entry::get_uid)
	.def("get_gid", &libdar::list_entry::get_gid)
	.def("get_perm", &libdar::list_entry::get_perm)
	.def("get_last_access", (std::string (libdar::list_entry::*)() const) &libdar::list_entry::get_last_access)
	.def("get_last_modif", (std::string (libdar::list_entry::*)() const) &libdar::list_entry::get_last_modif)
	.def("get_last_change", (std::string (libdar::list_entry::*)() const) &libdar::list_entry::get_last_change)
	.def("get_removal_date", &libdar::list_entry::get_removal_date)
	.def("get_last_access_s", &libdar::list_entry::get_last_access_s)
	.def("get_last_modif_s", &libdar::list_entry::get_last_modif_s)
	.def("get_last_change_s", &libdar::list_entry::get_last_change_s)
	.def("get_removal_date_s", &libdar::list_entry::get_removal_date_s)
	.def("get_file_size", &libdar::list_entry::get_file_size)
	.def("get_compression_ratio", &libdar::list_entry::get_compression_ratio)
	.def("get_compression_ratio_flag", &libdar::list_entry::get_compression_ratio_flag)
	.def("is_sparse", &libdar::list_entry::is_sparse)
	.def("get_sparse_flag", &libdar::list_entry::get_sparse_flag)
	.def("get_compression_algo", &libdar::list_entry::get_compression_algo)
	.def("is_dirty", &libdar::list_entry::is_dirty)
	.def("get_link_target", &libdar::list_entry::get_link_target)
	.def("get_major", &libdar::list_entry::get_major)
	.def("get_minor", &libdar::list_entry::get_minor)
	.def("has_delta_signature", &libdar::list_entry::has_delta_signature)
	.def("get_delta_flag", &libdar::list_entry::get_delta_flag)
	.def("get_archive_offset_for_data", (std::string (libdar::list_entry::*)() const) &libdar::list_entry::get_archive_offset_for_data)
	.def("get_archive_offset_for_EA", (std::string (libdar::list_entry::*)() const) &libdar::list_entry::get_archive_offset_for_EA)
	.def("get_archive_offset_for_FSA", (std::string (libdar::list_entry::*)() const) &libdar::list_entry::get_archive_offset_for_FSA)
	.def("get_storage_size_for_FSA", (std::string (libdar::list_entry::*)() const) &libdar::list_entry::get_storage_size_for_FSA)
	.def("get_ea_reaset_read", &libdar::list_entry::get_ea_reset_read)
	.def("get_ea_read_next", &libdar::list_entry::get_ea_read_next)
	.def("get_etiquette", &libdar::list_entry::get_etiquette)
	.def("get_fsa_scope", &libdar::list_entry::get_fsa_scope)
	.def("get_data_crc", &libdar::list_entry::get_data_crc)
	.def("get_delta_patch_base_crc", &libdar::list_entry::get_delta_patch_base_crc)
	.def("get_delta_patch_result_crc", &libdar::list_entry::get_delta_patch_result_crc)
	.def("clear", &libdar::list_entry::clear);


    	///////////////////////////////////////////
	// archive classes
	//

    pybind11::class_<libdar::archive>(mod, "archive")
	.def(pybind11::init<
	     const std::shared_ptr<libdar::user_interaction> &,
	     const libdar::path,
	     const std::string &,
	     const std::string &,
	     const libdar::archive_options_read &
	     >())
	.def(pybind11::init<
	     const std::shared_ptr<libdar::user_interaction> &,
	     const libdar::path &,
	     const libdar::path &,
	     const std::string &,
	     const std::string &,
	     const libdar::archive_options_create &,
	     libdar::statistics *
	     >())
	.def(pybind11::init<
	     const std::shared_ptr<libdar::user_interaction> &,
	     const libdar::path &,
	     std::shared_ptr<libdar::archive>,
	     const std::string &,
	     const std::string &,
	     const libdar::archive_options_merge &,
	     libdar::statistics *
	     >())
	.def(pybind11::init<
	     const std::shared_ptr<libdar::user_interaction> &,
	     const libdar::path &,
	     const std::string &,
	     const std::string &,
	     const libdar::archive_options_read &,
	     const libdar::path &,
	     const std::string &,
	     const std::string &,
	     const libdar::archive_options_repair &
	     >())

	.def("op_extract", &libdar::archive::op_extract)
	.def("summary", &libdar::archive::summary)
	.def("summary_data", &libdar::archive::summary_data)
	.def("op_listing", &libdar::archive::op_listing)
	.def("op_diff", &libdar::archive::op_diff)
	.def("op_test", &libdar::archive::op_test)
	.def("op_isolate", &libdar::archive::op_isolate)
	.def("get_children_in_table", &libdar::archive::get_children_in_table)
	.def("has_subdirectory", &libdar::archive::has_subdirectory)
	.def("get_stats", &libdar::archive::get_stats)
	.def("get_signatories", &libdar::archive::get_signatories)
	.def("init_catalogue", &libdar::archive::init_catalogue)
	.def("drop_all_filedescriptors", &libdar::archive::drop_all_filedescriptors)
	.def("set_to_unsaved_data_and_FSA", &libdar::archive::set_to_unsaved_data_and_FSA);


	///////////////////////////////////////////
	// archives_num classes
	//
    pybind11::class_<libdar::archive_num>(mod, "archive_num")
	.def(pybind11::init<libdar::U_16>(), pybind11::arg("arg") = 0);

    	///////////////////////////////////////////
	// database_archives classes
	//

    pybind11::class_<libdar::database_archives>(mod, "database_archives")
	.def("get_path", &libdar::database_archives::get_path)
	.def("get_basename", &libdar::database_archives::get_basename);

	///////////////////////////////////////////
	// database_*_options classes
	//

    pybind11::class_<libdar::database_open_options>(mod, "database_open_options")
	.def("clear", &libdar::database_open_options::clear)
	.def("set_partial", &libdar::database_open_options::set_partial)
	.def("set_partial_read_only", &libdar::database_open_options::set_partial_read_only)
	.def("set_warn_order", &libdar::database_open_options::set_warn_order);

    pybind11::class_<libdar::database_dump_options>(mod, "database_dump_options")
	.def("clear", &libdar::database_dump_options::clear)
	.def("set_overwrite", &libdar::database_dump_options::set_overwrite);

    pybind11::class_<libdar::database_add_options>(mod, "database_add_options")
	.def("clear", &libdar::database_add_options::clear);

    pybind11::class_<libdar::database_remove_options>(mod, "database_remove_options")
	.def("clear", &libdar::database_remove_options::clear)
	.def("set_revert_archive_numbering", &libdar::database_remove_options::set_revert_archive_numbering);

    pybind11::class_<libdar::database_change_basename_options>(mod, "database_change_basename_options")
	.def("clear", &libdar::database_change_basename_options::clear)
	.def("set_revert_archive_numbering", &libdar::database_change_basename_options::set_revert_archive_numbering);

    pybind11::class_<libdar::database_change_path_options>(mod, "database_change_path_options")
	.def("clear", &libdar::database_change_path_options::clear)
	.def("set_revert_archive_numbering", &libdar::database_change_path_options::set_revert_archive_numbering);

    pybind11::class_<libdar::database_restore_options>(mod, "database_restore_options")
	.def("clear", &libdar::database_restore_options::clear)
	.def("set_early_release", &libdar::database_restore_options::set_early_release)
	.def("set_info_details", &libdar::database_restore_options::set_info_details)
	.def("set_extra_options_for_dar", &libdar::database_restore_options::set_extra_options_for_dar)
	.def("set_ignore_dar_options_in_database", &libdar::database_restore_options::set_ignore_dar_options_in_database)
	.def("set_date", &libdar::database_restore_options::set_date)
	.def("set_even_when_removed", &libdar::database_restore_options::set_even_when_removed);

    pybind11::class_<libdar::database_used_options>(mod, "database_used_options")
	.def("clear", &libdar::database_used_options::clear)
	.def("set_revert_archive_numbering", &libdar::database_used_options::set_revert_archive_numbering);


    	///////////////////////////////////////////
	// database classes
	//

    pybind11::class_<libdar::database>(mod, "database")
	.def(pybind11::init<const std::shared_ptr<libdar::user_interaction> &>())
	.def(pybind11::init<
	     const std::shared_ptr<libdar::user_interaction> &,
	     const std::string &,
	     const libdar::database_open_options &
	     >())
	.def("dump", &libdar::database::dump)
	.def("add_archive", &libdar::database::add_archive)
	.def("remove_archive", &libdar::database::remove_archive)
	.def("set_permutation", &libdar::database::set_permutation)
	.def("change_name", &libdar::database::change_name)
	.def("set_path", &libdar::database::set_path)
	.def("set_options", &libdar::database::set_options)
	.def("set_dar_path", &libdar::database::set_dar_path)
	.def("set_compression", &libdar::database::set_compression)
	.def("get_contents", &libdar::database::get_contents)
	.def("get_options", &libdar::database::get_options)
	.def("get_dar_path", &libdar::database::get_dar_path)
	.def("get_compression", &libdar::database::get_compression)
	.def("get_database_version", &libdar::database::get_database_version)
	.def("get_files", &libdar::database::get_files)
	.def("get_version", &libdar::database::get_version)
	.def("show_most_recent_stats", &libdar::database::show_most_recent_stats)
	.def("restore", &libdar::database::restore)
	.def("check_order", &libdar::database::check_order);


    	///////////////////////////////////////////
	// libdar_xform classes
	//

    pybind11::class_<libdar::libdar_xform>(mod, "libdar_xform")
	.def(pybind11::init<
	     const std::shared_ptr<libdar::user_interaction> &,
	     const std::string &,
	     const std::string &,
	     const std::string &,
	     const libdar::infinint &,
	     const std::string &
	     >())
	.def(pybind11::init<
	     const std::shared_ptr<libdar::user_interaction> &,
	     const std::string &
	     >())
	.def("xform_to", (void (libdar::libdar_xform::*)(const std::string &,
							 const std::string &,
							 const std::string &,
							 bool,
							 bool,
							 const libdar::infinint &,
							 const libdar::infinint &,
							 const libdar::infinint &,
							 const std::string &,
							 const std::string &,
							 const std::string &,
							 libdar::hash_algo,
							 const libdar::infinint &,
							 const std::string &))
	     &libdar::libdar_xform::xform_to)
	.def("xform_to", (void (libdar::libdar_xform::*)(int, const std::string &))
	     &libdar::libdar_xform::xform_to);


	///////////////////////////////////////////
	// libdar_slave classes
	//

    pybind11::class_<libdar::libdar_slave>(mod, "libdar_slave")
	.def(pybind11::init<
	     std::shared_ptr<libdar::user_interaction> &,
	     const std::string &,
	     const std::string &,
	     const std::string &,
	     bool,
	     const std::string &,
	     bool,
	     const std::string &,
	     const std::string &,
	     const libdar::infinint &
	     >())
	.def("run", &libdar::libdar_slave::run);

}
