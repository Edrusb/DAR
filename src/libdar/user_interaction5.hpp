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

    /// \file user_interaction5.hpp
    /// \brief API v5 backward compatible class user_interaction
    /// \ingroup API5

#ifndef USER_INTERACTION5_HPP
#define USER_INTERACTION5_HPP

#include "../my_config.h"

#include <string>
#include <memory>
#include "user_interaction.hpp"
#include "secu_string.hpp"

namespace libdar5
{

	/// \addtogroup API5
        /// @{

    using libdar::secu_string;
    using libdar::U_I;
    using libdar::infinint;
    using libdar::Euser_abort;
    using libdar::Elibcall;

    using libdar::Ebug;
    using libdar::Egeneric;
    using libdar::Ememory;

        /// This is a pure virtual class that is used by libdar when interaction with the user is required.

        /// You can base your own class on it using C++ inheritance
        /// or use the class user_interaction_callback which implements
        /// the interaction based on callback functions.
        /// The user_interaction class is used by libdar in the following circumpstances:
        /// - when is required a boolean answer to a question the pause() method is used
        /// - when a warning needs to be displayed to the user the warning() method is used
        /// - when a directory listing needs to be returned to the user the listing() method is used
        /// .
        /// the printf() method is built over the warning() methods to display a formated message
        /// it has not to be redefined in any inherited class.
        /// If you want to define you own class as inherited class of user_interaction
        /// you need to overwrite:
        /// - the clone() method. It is used to make local temporary copies of objets
        ///  in libdar. It acts like the constructor copy but is virtual.
        /// - the pause() method
        /// - the warning() method
        /// - the listing() method (this is not mandatory).  inherited classes *can*
        /// overwrite the listing() method, which will be used if the use_listing
        /// is set to true thanks to the set_use_listing() protected method.
        /// In that case the listing of archive contents is done thanks to this listing()
        /// method instead of the warning() method.
        /// - get_string() method
        /// - get_secu_string() method
        /// .
        /// WARNING !
        /// if your own class has specific fields, you will probably
        /// need to redefine the copy constructor as well as operator =
        /// if you don't understand this and why, don't play trying making your
        /// own class, and/or read good C++ book about canonical form
        /// of a C++ class, as well as how to properly make an inherited class.
        /// And don't, complain if libdar segfault or core dumps. Libdar
        /// *needs* to make local copies of these objects, if the copy constructor
        ///  is not properly defined in your inherited class this will crash the application.
    class user_interaction : public libdar::user_interaction
    {
    public:

            /// class constructor.
        user_interaction();
        user_interaction(const user_interaction & ref) = default;
        user_interaction(user_interaction && ref) noexcept = default;
        user_interaction & operator = (const user_interaction & ref) = default;
        user_interaction & operator = (user_interaction && ref) noexcept = default;
        virtual ~user_interaction() = default;

            /// method added for backward compatibility with API v5

            /// \note warning() has been renamed message() in libdar::user_interaction
            /// to let libdar5::user_interaction intercept the inherted_message() to
            /// inherited_warning() and keep implementing the warning_with_more() feature
            /// as warning() does not exist in the parent class and some API v5 program
            /// may call it we add a warning() methode here:
        void warning(const std::string & msg) { message(msg); };

            /// method used to ask a boolean question to the user.

            /// \param[in] message is the message to be displayed, that is the question.
            /// \exception Euser_abort If the user answer "false" or "no" to the question the method
            /// must throw an exception of type "Euser_abort".
        virtual void pause(const std::string & message)
        {
            if(!pause2(message))
                throw Euser_abort(message);
        };

            /// alternative method to the pause() method

            /// \param[in] message The boolean question to ask to the user
            /// \return the answer of the user (true/yes or no/false)
            /// \note either pause2() or pause() *must* be overwritten, but not both.
            /// libdar always calls pause() which default implementation relies on pause2() where it converts negative
            /// return from pause2() by throwing the appropriated exception. As soon as you overwrite pause(),
            /// pause2() is no more used.
        virtual bool pause2(const std::string & message)
        { throw Elibcall("user_interaction::pause2", "user_interaction::pause() or pause2() must be overwritten !"); };

            /// method used to ask a question that needs an arbitrary answer.

            /// \param[in] message is the question to display to the user.
            /// \param[in] echo is set to false is the answer must not be shown while the user answers.
            /// \return the user's answer.
        virtual std::string get_string(const std::string & message, bool echo) = 0;

            /// same a get_string() but uses secu_string instead

            /// \param[in] message is the question to display to the user.
            /// \param[in] echo is set to false is the answer must not be shown while the user answers.
            /// \return the user's answer.
        virtual secu_string get_secu_string(const std::string & message, bool echo) = 0;


            /// optional method to use if you want file listing splitted in several fields.
            /// If you need to use this feature, you have then to supply an implementation for this method,
            /// in your inherited class which will be called by libdar in place of the warning method
            /// You then also have to call the set_use_listing() method with true as parameter
            /// from the constructor of your inherited class (for example) to tell libdar that the listing() method is
            /// to be used in place of the warning() method for archive listing.

            /// \param[in] flag is the given information about the EA, compression, presence of saved data.
            /// \param[in] perm is the access permission of the file.
            /// \param[in] uid User ID of the file.
            /// \param[in] gid Group ID of the file.
            /// \param[in] size file size.
            /// \param[in] date file modification date.
            /// \param[in] filename file name.
            /// \param[in] is_dir true if file is a directory.
            /// \param[in] has_children true if file is a directory which is not empty.
            /// \note This is not a pure virtual method, this is normal,
            /// so your inherited class is not obliged to overwrite it.
        virtual void listing(const std::string & flag,
                             const std::string & perm,
                             const std::string & uid,
                             const std::string & gid,
                             const std::string & size,
                             const std::string & date,
                             const std::string & filename,
                             bool is_dir,
                             bool has_children);



            /// optional method to use if you want dar_manager database contents listing split in several fields.
            /// if you want to use this feature, you have then to supply an implementation for this method
            /// in your inherited class, method that will be called by libdar in place of the warning method.
            /// You will also have to call the set_use_dar_manager_show_files() protected method with true as argument
            /// from the constructor of your inherited class to tell libdar to use the dar_manager_show_files()
            /// method in place of the warning() method.

            /// \param[in] filename name of the file
            /// \param[in] data_change whether the backup owns the most recent data for the file
            /// \param[in] ea_change whether the backup owns the most recent  Extended Attributes for the file
            /// \note this method can be set for database::show_files() method to call it
        virtual void dar_manager_show_files(const std::string & filename,
                                            bool data_change,
                                            bool ea_change);


            /// optional method to use if you want dar_manager database archive listing split in several fields
            /// if you want to use this feature, you have then to supply an implementation for this method
            /// in your inherited class, method that will be called by libdar in place of the warning method.
            /// You will also have to call the set_use_dar_manager_contents() protected method with true as argument
            /// from the constructor of your inherited class to tell libdar to use the dar_manager_contents()
            /// method in place of the warning() method.

            /// \param[in] number is the number of the archive in the database
            /// \param[in] chemin recorded path where to find this archive
            /// \param[in] archive_name basename of this archive
            /// \note this method can be set for database::show_contents() to call it
        virtual void dar_manager_contents(U_I number,
                                          const std::string & chemin,
                                          const std::string & archive_name);

            /// optional method to use if you want dar_manager statistics listing split in several fields
            /// if you want to use this feature, you have then to supply an implementation for this method
            /// in your inherited class, method that will be called by libdar in place of the warning method.
            /// You will also have to call the set_use_dar_manager_statistics() protected method with true as argument
            /// from the constructor of your inherited class to tell libdar to use the dar_manager_statistics()
            /// method in place of the warning() method.

            /// \param[in] number archive number
            /// \param[in] data_count amount of file which last version is located in this archive
            /// \param[in] total_data total number of file covered in this database
            /// \param[in] ea_count amount of EA which last version is located in this archive
            /// \param[in] total_ea total number of file that have EA covered by this database
            /// \note this method can be set for database::show_most_recent_stats() method to call it
        virtual void dar_manager_statistics(U_I number,
                                            const infinint & data_count,
                                            const infinint & total_data,
                                            const infinint & ea_count,
                                            const infinint & total_ea);

            /// optional method to use if you want dar_manager statistics listing split in several fields
            /// if you want to use this feature, you have then to supply an implementation for this method
            /// in your inherited class, method that will be called by libdar in place of the warning method.
            /// You will also have to call the set_use_dar_manager_show_version() protected method with true as argument
            /// from the constructor of your inherited class to tell libdar to use the dar_manager_show_version()
            /// method in place of the warning() method.

            /// \param[in] number archive number
            /// \param[in] data_date is the last modification date of the requested file in thie archive whose number is "number"
            /// \param[in] data_presence is the nature of this modification, true if the data was saved, false if it was deleted
            /// \param[in] ea_date is the date of the EA for the requested file in the archive whose number is "number"
            /// \param[in] ea_presence is the nature of this modification, true if the EAs were saved, false if they were deleted
            /// \note this method can be set for database::show_version() method to call it
        virtual void dar_manager_show_version(U_I number,
                                              const std::string & data_date,
                                              const std::string & data_presence,
                                              const std::string & ea_date,
                                              const std::string & ea_presence);

            /// for libdar to know if it is interesting to use listing(), dar_manager_show_files(),
            /// dar_manager_contents(), dar_manager_statistics() or to keep reporting listing thanks
            /// to the warning() method,

            /// this is not a virtual method, it has not to be overwritten in inherited classes.
        bool get_use_listing() const { return use_listing; };
            /// this is not a virtual method, it has not to be overwritten in inherited classes.
        bool get_use_dar_manager_show_files() const { return use_dar_manager_show_files; };
            /// this is not a virtual method, it has not to be overwritten in inherited classes.
        bool get_use_dar_manager_contents() const { return use_dar_manager_contents; };
            /// this is not a virtual method, it has not to be overwritten in inherited classes.
        bool get_use_dar_manager_statistics() const { return use_dar_manager_statistics; };
            /// this is not a virtual method, it has not to be overwritten in inherited classes.
        bool get_use_dar_manager_show_version() const { return use_dar_manager_show_version; };

        virtual void printf(const char *format, ...) override;

            /// make a newly allocated object which has the same properties as "this".

            /// This *is* a virtual method, it *must* be overwritten in any inherited class
            /// copy constructor and = operator may have to be overwritten too if necessary
            /// Warning !
            /// clone() must throw exception if necessary (Ememory), but never
            /// return a nullptr pointer !
        virtual user_interaction *clone() const = 0;

            /// make a pause each N line of output when calling the warning method

            /// \param[in] num is the number of line to display at once, zero for unlimited display
            /// \note. Since API 3.1, the warning method is no more a pure virtual function
            /// you need to call the parent warning method in your method for this warning_with_more
            /// method works as expected.
        void warning_with_more(U_I num) { at_once = num; count = 0; };

    protected:

            /// method to be called with true as argument if you have defined a listing() method.

            /// in the constructor of any inherited class that define a listing() method
            /// it is advisable to call set_use_listing() with true as argument for libdar
            /// knows that the listing() call has to be used in place of the warning() call
            /// for file listing.
        void set_use_listing(bool val) { use_listing = val; };

            /// method to be called with true as argument if you have defined a dar_manager_show_files() method.
        void set_use_dar_manager_show_files(bool val) { use_dar_manager_show_files = val; };

            /// method to be called with true as argument if you have defined a dar_manager_contents() method.
        void set_use_dar_manager_contents(bool val) { use_dar_manager_contents = val; };

            /// method to be called with true as argument if you have defined a dar_manager_statistics() method.
        void set_use_dar_manager_statistics(bool val) { use_dar_manager_statistics = val; };

            /// method to be called with true as argument if you have defined a dar_manager_show_version() method.
        void set_use_dar_manager_show_version(bool val) { use_dar_manager_show_version = val; };

            /// need to be overwritten in place of the warning() method since API 3.1.x

            // inherited from libdar::user_interaction
        virtual void inherited_message(const std::string & message) override;
        virtual bool inherited_pause(const std::string & message) override;
        virtual std::string inherited_get_string(const std::string & message, bool echo) override;
        virtual secu_string inherited_get_secu_string(const std::string & message, bool echo) override;

            /// to be defined by inherited classes
        virtual void inherited_warning(const std::string & message) = 0;


    private:
        bool use_listing;
        bool use_dar_manager_show_files;
        bool use_dar_manager_contents;
        bool use_dar_manager_statistics;
        bool use_dar_manager_show_version;
        U_I at_once, count;

    };

        /// convert a user_interaction to a shared_pointer on a clone of that user_interaction

    extern std::shared_ptr<user_interaction> user_interaction5_clone_to_shared_ptr(user_interaction & dialog);

        /// @}

} // end of namespace

#endif
