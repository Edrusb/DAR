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

    /// \file erreurs.hpp
    /// \brief contains all the excetion class thrown by libdar
    /// \ingroup API

#ifndef ERREURS_HPP
#define ERREURS_HPP

#include "../my_config.h"

#include <string>
#include <deque>
#include "integers.hpp"

namespace libdar
{

	/// \addtogroup API
	/// @{

	/// a routine to change NLS domaine forth and back for inline routines
    extern const char *dar_gettext(const char *);

	/// this is the parent class of all exception classes.

	/// this is a pure virtual class that provide some simple
	/// mechanisme to carry the information about the cause of the exception,
	/// as well as some a complex mechanim which not used often in libdar
	/// that keep trace, for each exception throwing process, of the different
	/// calls by which the current exception has been exiting.
    class Egeneric
    {
    public :
	    /// the constructor
        Egeneric(const std::string &source, const std::string &message);

	    /// copy constructor
	Egeneric(const Egeneric & ref) = default;

	    /// move constructor
	Egeneric(Egeneric && ref) = default;

	    /// assignment operator
	Egeneric & operator = (const Egeneric & ref) = default;

	    /// move operator
	Egeneric & operator = (Egeneric && ref) noexcept = default;

	    /// the destructor
        virtual ~Egeneric() = default;

	    /// add more detailed couple of information to the exception
        void stack(const std::string & passage, const std::string & message = "") { pile.push_back(niveau(passage, message)); };
	void stack(const std::string && passage, const std::string && message = "") { pile.push_back(niveau(std::move(passage), std::move(message))); };

	    /// get the message explaing the nature of the exception

	    /// This is probably the only method you will use for all the
	    /// the exception, as you will not have to create such objects
	    /// and will only need to get the error message thanks to this
	    /// method
        const std::string & get_message() const { return pile.front().objet; };

	    /// get the call function which has thrown this exception
	const std::string & get_source() const { return pile.front().lieu; };

	    /// retrieve the objet (object) associated to a given "lieu" (location) from the stack

	    /// \param[in] location key to look for the value of
	    /// \return returns an empty string if key is not found in the stack
	const std::string & find_object(const std::string & location) const;

	    /// prepend error message by the given string
	void prepend_message(const std::string & context);

	    /// return a string result of the exception information dump
	std::string dump_str() const;

    protected :
        virtual std::string exceptionID() const = 0;

    private :
        struct niveau
        {
            niveau(const std::string & ou, const std::string & quoi) { lieu = ou; objet = quoi; };
	    niveau(std::string && ou, std::string && quoi) { lieu = std::move(ou); objet = std::move(quoi); };
	    niveau(const niveau & ref) = default;
	    niveau(niveau && ref) noexcept = default;
	    niveau & operator = (const niveau & ref) = default;
	    niveau & operator = (niveau && ref) noexcept = default;
	    ~niveau() = default;

            std::string lieu, objet;
        };

        std::deque<niveau> pile;

	static const std::string empty_string;
    };


	/// exception used when memory has been exhausted

	/// the inherited get_message() method is probably
	/// the only one you will need to use
    class Ememory : public Egeneric
    {
    public:
        Ememory(const std::string &source) : Egeneric(source, dar_gettext("Lack of Memory")) {};
	Ememory(const Ememory & ref) = default;
	Ememory(Ememory && ref) = default;
	Ememory & operator = (const Ememory & ref) = default;
	Ememory & operator = (Ememory && ref) = default;
	~Ememory() = default;

    protected:
        Ememory(const std::string &source, const std::string & message) : Egeneric(source, message) {};
        virtual std::string exceptionID() const override { return "MEMORY"; };
    };

	/// exception used when secure memory has been exhausted

    class Esecu_memory : public Ememory
    {
    public:
        Esecu_memory(const std::string &source) : Ememory(source, dar_gettext("Lack of Secured Memory")) {};
	Esecu_memory(const Esecu_memory & ref) = default;
	Esecu_memory(Esecu_memory && ref) = default;
	Esecu_memory & operator = (const Esecu_memory & ref) = default;
	Esecu_memory & operator = (Esecu_memory && ref) = default;
	~Esecu_memory() = default;

    protected:
        virtual std::string exceptionID() const override { return "SECU_MEMORY"; };
    };


#define SRC_BUG Ebug(__FILE__, __LINE__)
// #define XMT_BUG(exception, call) exception.stack(call, __FILE__, __LINE__)

	/// exception used to signal a bug. A bug is triggered when reaching some code that should never be reached
    class Ebug : public Egeneric
    {
    public :
        Ebug(const std::string & file, S_I line);
	Ebug(const Ebug & ref) = default;
	Ebug(Ebug && ref) = default;
	Ebug & operator = (const Ebug & ref) = default;
	Ebug & operator = (Ebug && ref) = default;
	~Ebug() = default;

	using Egeneric::stack; // to avoid warning with clang
        void stack(const std::string & passage, const std::string & file, const std::string & line);

    protected :
        virtual std::string exceptionID() const override { return "BUG"; };
    };

	/// exception used when arithmetic error is detected when operating on infinint

	/// the inherited get_message() method is probably
	/// the only one you will need to use
    class Einfinint : public Egeneric
    {
    public :
        Einfinint(const std::string & source, const std::string & message) : Egeneric(source, message) {};
	Einfinint(const Einfinint & ref) = default;
	Einfinint(Einfinint && ref) = default;
	Einfinint & operator = (const Einfinint & ref) = default;
	Einfinint & operator = (Einfinint && ref) = default;
	~Einfinint() = default;

    protected :
        virtual std::string exceptionID() const override { return "INFININT"; };
    };

	/// exception used when a limitint overflow is detected, the maximum value of the limitint has been exceeded

	/// the inherited get_message() method is probably
	/// the only one you will need to use
    class Elimitint : public Egeneric
    {
    public :
        Elimitint() : Egeneric("", dar_gettext("Cannot handle such a too large integer. Use a full version of libdar (compiled to rely on the \"infinint\" integer type) to solve this problem")) {};
	Elimitint(const Elimitint & ref) = default;
	Elimitint(Elimitint && ref) = default;
	Elimitint & operator = (const Elimitint & ref) = default;
	Elimitint & operator = (Elimitint && ref) = default;
	~Elimitint() = default;

    protected :
        virtual std::string exceptionID() const override { return "LIMITINT"; };
    };

	/// exception used to signal range error

	/// the inherited get_message() method is probably
	/// the only one you will need to use
    class Erange : public Egeneric
    {
    public :
        Erange(const std::string & source, const std::string & message) : Egeneric(source, message) {};
	Erange(const Erange & ref) = default;
	Erange(Erange && ref) = default;
	Erange & operator = (const Erange & ref) = default;
	Erange & operator = (Erange && ref) = default;
	~Erange() = default;

    protected :
        virtual std::string exceptionID() const override { return "RANGE"; };
    };

	/// exception used to signal convertion problem between infinint and string (decimal representation)

	/// the inherited get_message() method is probably
	/// the only one you will need to use
	/// see also the class deci
    class Edeci : public Egeneric
    {
    public :
        Edeci(const std::string & source, const std::string & message) : Egeneric(source, message) {};
	Edeci(const Edeci & ref) = default;
	Edeci(Edeci && ref) = default;
	Edeci & operator = (const Edeci & ref) = default;
	Edeci & operator = (Edeci && ref) = default;
	~Edeci() = default;

    protected :
        virtual std::string exceptionID() const override { return "DECI"; };
    };

	/// exception used when a requested feature is not (yet) implemented

	/// the inherited get_message() method is probably
	/// the only one you will need to use
    class Efeature : public Egeneric
    {
    public :
        Efeature(const std::string & message) : Egeneric("", message) {};
	Efeature(const Efeature & ref) = default;
	Efeature(Efeature && ref) = default;
	Efeature & operator = (const Efeature & ref) = default;
	Efeature & operator = (Efeature && ref) = default;
	~Efeature() = default;

    protected :
        virtual std::string exceptionID() const override { return "UNIMPLEMENTED FEATURE"; };
    };

	/// exception used when hardware problem is found

	/// the inherited get_message() method is probably
	/// the only one you will need to use
    class Ehardware : public Egeneric
    {
    public :
        Ehardware(const std::string & source, const std::string & message) : Egeneric(source, message) {};
	Ehardware(const Ehardware & ref) = default;
	Ehardware(Ehardware && ref) = default;
	Ehardware & operator = (const Ehardware & ref) = default;
	Ehardware & operator = (Ehardware && ref) = default;
	~Ehardware() = default;

    protected :
        virtual std::string exceptionID() const override { return "HARDWARE ERROR"; };
    };

	/// exception used to signal that the user has aborted the operation

	/// the inherited get_message() method is probably
	/// the only one you will need to use
    class Euser_abort : public Egeneric
    {
    public :
        Euser_abort(const std::string & msg) : Egeneric("",msg) {};
	Euser_abort(const Euser_abort & ref) = default;
	Euser_abort(Euser_abort && ref) = default;
	Euser_abort & operator = (const Euser_abort & ref) = default;
	Euser_abort & operator = (Euser_abort && ref) = default;
	~Euser_abort() = default;

    protected :
        virtual std::string exceptionID() const override { return "USER ABORTED OPERATION"; };
    };


	/// exception used when an error concerning the treated data has been met

	/// the inherited get_message() method is probably
	/// the only one you will need to use
    class Edata : public Egeneric
    {
    public :
        Edata(const std::string & msg) : Egeneric("", msg) {};
	Edata(const Edata & ref) = default;
	Edata(Edata && ref) = default;
	Edata & operator = (const Edata & ref) = default;
	Edata & operator = (Edata && ref) = default;
	~Edata() = default;

    protected :
        virtual std::string exceptionID() const override { return "ERROR IN TREATED DATA"; };
    };

	/// exception used when error the inter-slice user command returned an error code

	/// the inherited get_message() method is probably
	/// the only one you will need to use
    class Escript : public Egeneric
    {
    public :
        Escript(const std::string & source, const std::string & msg) : Egeneric(source ,msg) {};
	Escript(const Escript & ref) = default;
	Escript(Escript && ref) = default;
	Escript & operator = (const Escript & ref) = default;
	Escript & operator = (Escript && ref) = default;
	~Escript() = default;

    protected :
        virtual std::string exceptionID() const override { return "USER ABORTED OPERATION"; };
    };

	/// exception used to signal an error in the argument given to libdar call of the API

	/// the inherited get_message() method is probably
	/// the only one you will need to use
    class Elibcall : public Egeneric
    {
    public :
        Elibcall(const std::string & source, const std::string & msg) : Egeneric(source ,msg) {};
	Elibcall(const Elibcall & ref) = default;
	Elibcall(Elibcall && ref) = default;
	Elibcall & operator = (const Elibcall & ref) = default;
	Elibcall & operator = (Elibcall && ref) = default;
	~Elibcall() = default;

    protected :
        virtual std::string exceptionID() const override { return "USER ABORTED OPERATION"; };
    };

	/// exception used when a requested fearture has not beed activated at compilation time

	/// the inherited get_message() method is probably
	/// the only one you will need to use
    class Ecompilation : public Egeneric
    {
    public :
        Ecompilation(const std::string & msg) : Egeneric("" ,msg) {};
	Ecompilation(const Ecompilation & ref) = default;
	Ecompilation(Ecompilation && ref) = default;
	Ecompilation & operator = (const Ecompilation & ref) = default;
	Ecompilation & operator = (Ecompilation && ref) = default;
	~Ecompilation() = default;

    protected :
        virtual std::string exceptionID() const override { return "FEATURE DISABLED AT COMPILATION TIME"; };
    };


	/// exception used when the thread libdar is running in is asked to stop

    class Ethread_cancel : public Egeneric
    {
    public:
	Ethread_cancel(bool now, U_64 x_flag) : Egeneric("", now ? dar_gettext("Thread cancellation requested, aborting as soon as possible") : dar_gettext("Thread cancellation requested, aborting as properly as possible")) { immediate = now; flag = x_flag; };
	Ethread_cancel(const Ethread_cancel & ref) = default;
	Ethread_cancel(Ethread_cancel && ref) = default;
	Ethread_cancel & operator = (const Ethread_cancel & ref) = default;
	Ethread_cancel & operator = (Ethread_cancel && ref) = default;
	~Ethread_cancel() = default;

	bool immediate_cancel() const { return immediate; };
	U_64 get_flag() const { return flag; };

    protected:
	virtual std::string exceptionID() const override { return "THREAD CANCELLATION REQUESTED, ABORTING"; };

    private:
	bool immediate;
	U_64 flag;
    };

	/// exception used to carry system error

    class Esystem : public Egeneric
    {
    public:
	enum io_error
	{
	    io_exist,  ///< file already exists (write mode)
	    io_absent, ///< file does not exist (read mode)
	    io_access, ///< permission denied (any mode)
	    io_ro_fs   ///< read-only filesystem (write mode/read-write mode)
	};

	Esystem(const std::string & source, const std::string & message, io_error code);
	Esystem(const Esystem & ref) = default;
	Esystem(Esystem && ref) = default;
	Esystem & operator = (const Esystem & ref) = default;
	Esystem & operator = (Esystem && ref) = default;
	~Esystem() = default;

	io_error get_code() const { return x_code; };

    protected:
	virtual std::string exceptionID() const override { return "SYSTEM ERROR MET"; };

    private:
	io_error x_code;
    };

	/// exception used to report authentication error

    class Enet_auth : public Egeneric
    {
    public:
	Enet_auth(const std::string & message): Egeneric("on the network", message) {};
	Enet_auth(const Enet_auth & ref) = default;
	Enet_auth(Enet_auth && ref) = default;
	Enet_auth & operator = (const Enet_auth & ref) = default;
	Enet_auth & operator = (Enet_auth && ref) = default;
	~Enet_auth() = default;

    protected:
	virtual std::string exceptionID() const override { return "NETWORK AUTHENTICATION ERROR"; };
    };


	/// @}

} // end of namespace

#endif
