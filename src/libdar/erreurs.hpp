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
// $Id: erreurs.hpp,v 1.24.2.1 2012/02/25 14:43:44 edrusb Exp $
//
/*********************************************************************/

    /// \file erreurs.hpp
    /// \brief contains all the excetion class thrown by libdar
    /// \ingroup API

#ifndef ERREURS_HPP
#define ERREURS_HPP

#include "../my_config.h"
#include <string>
#include <list>
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
	    /// the destructor
        virtual ~Egeneric() {};

	    /// add more detailed couple of information to the exception
        virtual void stack(const std::string & passage, const std::string & message = "") { pile.push_back(niveau(passage, message)); };

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

	    /// dump all information of the exception to the standard error
        void dump() const;

    protected :
        virtual std::string exceptionID() const = 0;

    private :
        struct niveau
        {
            niveau(const std::string &ou, const std::string &quoi) { lieu = ou; objet = quoi; };
            std::string lieu, objet;
        };

        std::list<niveau> pile;

	static const std::string empty_string;
    };


	/// exception used when memory has been exhausted

	/// the inherited get_message() method is probably
	/// the only one you will need to use
    class Ememory : public Egeneric
    {
    public:
        Ememory(const std::string &source) : Egeneric(source, dar_gettext("Lack of Memory")) {};

    protected:
        Ememory(const std::string &source, const std::string & message) : Egeneric(source, message) {};
        std::string exceptionID() const { return "MEMORY"; };
    };

	/// exception used when secure memory has been exhausted

    class Esecu_memory : public Ememory
    {
    public:
        Esecu_memory(const std::string &source) : Ememory(source, dar_gettext("Lack of Secured Memory")) {};

    protected:
        std::string exceptionID() const { return "SECU_MEMORY"; };
    };


#define SRC_BUG Ebug(__FILE__, __LINE__)
// #define XMT_BUG(exception, call) exception.stack(call, __FILE__, __LINE__)

	/// exception used to signal a bug. A bug is triggered when reaching some code that should never be reached
    class Ebug : public Egeneric
    {
    public :
        Ebug(const std::string & file, S_I line);

        void stack(const std::string & passage, const std::string & file, const std::string & line);

    protected :
        std::string exceptionID() const { return "BUG"; };
    };

	/// exception used when arithmetic error is detected when operating on infinint

	/// the inherited get_message() method is probably
	/// the only one you will need to use
    class Einfinint : public Egeneric
    {
    public :
        Einfinint(const std::string & source, const std::string & message) : Egeneric(source, message) {};

    protected :
        std::string exceptionID() const { return "INFININT"; };
    };

	/// exception used when a limitint overflow is detected, the maximum value of the limitint has been exceeded

	/// the inherited get_message() method is probably
	/// the only one you will need to use
    class Elimitint : public Egeneric
    {
    public :
        Elimitint() : Egeneric("", dar_gettext("Cannot handle such a too large integer. Use a full version of libdar (compiled to rely on the \"infinint\" integer type) to solve this problem")) {};

    protected :
        std::string exceptionID() const { return "LIMITINT"; };
    };

	/// exception used to signal range error

	/// the inherited get_message() method is probably
	/// the only one you will need to use
    class Erange : public Egeneric
    {
    public :
        Erange(const std::string & source, const std::string & message) : Egeneric(source, message) {};

    protected :
        std::string exceptionID() const { return "RANGE"; };
    };

	/// exception used to signal convertion problem between infinint and string (decimal representation)

	/// the inherited get_message() method is probably
	/// the only one you will need to use
	/// see also the class deci
    class Edeci : public Egeneric
    {
    public :
        Edeci(const std::string & source, const std::string & message) : Egeneric(source, message) {};

    protected :
        std::string exceptionID() const { return "DECI"; };
    };

	/// exception used when a requested feature is not (yet) implemented

	/// the inherited get_message() method is probably
	/// the only one you will need to use
    class Efeature : public Egeneric
    {
    public :
        Efeature(const std::string & message) : Egeneric("", message) {};

    protected :
        std::string exceptionID() const { return "UNIMPLEMENTED FEATURE"; };
    };

	/// exception used when hardware problem is found

	/// the inherited get_message() method is probably
	/// the only one you will need to use
    class Ehardware : public Egeneric
    {
    public :
        Ehardware(const std::string & source, const std::string & message) : Egeneric(source, message) {};

    protected :
        std::string exceptionID() const { return "HARDWARE ERROR"; };
    };

	/// exception used to signal that the user has aborted the operation

	/// the inherited get_message() method is probably
	/// the only one you will need to use
    class Euser_abort : public Egeneric
    {
    public :
        Euser_abort(const std::string & msg) : Egeneric("",msg) {};

    protected :
        std::string exceptionID() const { return "USER ABORTED OPERATION"; };
    };


	/// exception used when an error concerning the treated data has been met

	/// the inherited get_message() method is probably
	/// the only one you will need to use
    class Edata : public Egeneric
    {
    public :
        Edata(const std::string & msg) : Egeneric("", msg) {};

    protected :
        std::string exceptionID() const { return "ERROR IN TREATED DATA"; };
    };

	/// exception used when error the inter-slice user command returned an error code

	/// the inherited get_message() method is probably
	/// the only one you will need to use
    class Escript : public Egeneric
    {
    public :
        Escript(const std::string & source, const std::string & msg) : Egeneric(source ,msg) {};

    protected :
        std::string exceptionID() const { return "USER ABORTED OPERATION"; };
    };

	/// exception used to signal an error in the argument given to libdar call of the API

	/// the inherited get_message() method is probably
	/// the only one you will need to use
    class Elibcall : public Egeneric
    {
    public :
        Elibcall(const std::string & source, const std::string & msg) : Egeneric(source ,msg) {};

    protected :
        std::string exceptionID() const { return "USER ABORTED OPERATION"; };
    };

	/// exception used when a requested fearture has not beed activated at compilation time

	/// the inherited get_message() method is probably
	/// the only one you will need to use
    class Ecompilation : public Egeneric
    {
    public :
        Ecompilation(const std::string & msg) : Egeneric("" ,msg) {};

    protected :
        std::string exceptionID() const { return "FEATURE DISABLED AT COMPILATION TIME"; };
    };


	/// exception used when the thread libdar is running in is asked to stop

    class Ethread_cancel : public Egeneric
    {
    public:
	Ethread_cancel(bool now, U_64 x_flag) : Egeneric("", now ? dar_gettext("Thread cancellation requested, aborting as soon as possible") : dar_gettext("Thread cancellation requested, aborting as properly as possible")) { immediate = now; flag = x_flag; };

	bool immediate_cancel() const { return immediate; };
	U_64 get_flag() const { return flag; };

    protected:
	std::string exceptionID() const { return "THREAD CANCELLATION REQUESTED, ABORTING"; };

    private:
	bool immediate;
	U_64 flag;
    };


	/// @}

} // end of namespace

#endif
