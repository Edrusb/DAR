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

// this should be passed from Makefile
#define LIBDAR_MODE 64
#define HAVE_CONFIG_H 1
//

#include "../my_config.h"
#include "../libdar/libdar.hpp"
#include "../libdar/tools.hpp"


PYBIND11_MODULE(libdar, mod)
{
	// mod is of type pybind11::module

    mod.doc() = "libdar python binding";


	///////////////////////////////////////////
	// exceptions classes (from erreurs.hpp)
	//

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
		exceptionID);      // name of the method in C++ (must match Python name)
		                   // arguments, (none here)
	};
    };

    pybind11::class_<libdar::Egeneric, pyEgeneric> egeneric(mod, "Egeneric");
    egeneric
	.def(pybind11::init<const std::string &, const std::string &>())
	.def("get_message", &pyEgeneric_pub::get_message)
	.def("dump_str", &pyEgeneric_pub::dump_str)
	.def("exceptionID", &pyEgeneric_pub::exceptionID);

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


	///////////////////////////////////////////
	// class path (from path.hpp)
	//

    pybind11::class_<libdar::path>(mod, "path")
	.def(pybind11::init<const std::string &, bool>(), pybind11::arg("s"), pybind11::arg("undisclosed") = false)
	.def("basename", &libdar::path::basename)
	.def("reset_read", &libdar::path::reset_read)
	.def("read_subdir", &libdar::path::read_subdir)
	.def("is_relative", &libdar::path::is_relative)
	.def("is_absolute", &libdar::path::is_absolute)
	.def("append", &libdar::path::append)
	.def(pybind11::self + pybind11::self) // operator +
	.def(pybind11::self += pybind11::self) // operator += libdar::path
	.def(pybind11::self += std::string()) // operator += std::string
	.def("display", &libdar::path::display)
	.def("display_without_root", &libdar::path::display_without_root);


   	///////////////////////////////////////////
	// infinint (from real_infinint.hpp / limitint.hpp)
	//


#ifndef LIBDAR_MODE

    pybind11::class_<libdar::real_infinint>(mod, "infininit")
	.def(pybind11::init<off_t>(), pybind11::arg("a") = 0)
	.def(pybind11::init<time_t>(), pybind11::arg("a") = 0)
	.def(pybind11::init<size_t>(), pybind11::arg("a") = 0)
	.def("dump", &libdar::real_infinint::dump)
	.def("read", &libdar::real_infinint::read)
	.def("is_zero", &libdar::real_infinint::is_zero);

#else

    pybind11::class_<libdar::limitint<libdar::INFININT_BASE_TYPE> >(mod, "infinint")
	.def(pybind11::init<off_t>(), pybind11::arg("a") = 0)
	.def(pybind11::init<time_t>(), pybind11::arg("a") = 0)
	.def(pybind11::init<size_t>(), pybind11::arg("a") = 0)
	.def("dump", &libdar::limitint<libdar::INFININT_BASE_TYPE>::dump)
	.def("read", &libdar::limitint<libdar::INFININT_BASE_TYPE>::read)
	.def("is_zero", &libdar::limitint<libdar::INFININT_BASE_TYPE>::is_zero);

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


    pybind11::class_<libdar::statistics>(mod, "statistics")
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

}


