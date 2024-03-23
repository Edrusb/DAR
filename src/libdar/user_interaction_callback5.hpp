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

    /// \file user_interaction_callback5.hpp
    /// \brief API v5 backward compatible user_interaction_callback class
    /// \ingroup API5

#ifndef USER_INTERACTION_CALLBACK5_HPP
#define USER_INTERACTION_CALLBACK5_HPP

#include "../my_config.h"

#include "user_interaction5.hpp"

namespace libdar5
{

	/// \addtogroup API5
	/// @{


	/// full implemented class for user_interaction based on callback functions.

	/// this class is an inherited class of user_interaction it is used by
	/// dar command line programs, but you can use it if you wish.
	/// \ingroup API5

    class user_interaction_callback : public user_interaction
    {
    public:

	    /// constructor which receive the callback functions.

	    /// \param[in] x_warning_callback is used by warning() method
	    /// \param[in] x_answer_callback is used by the pause() method
	    /// \param[in] x_string_callback is used by get_string() method
	    /// \param[in] x_secu_string_callback is used by get_secu_string() method
	    /// \param[in] context_value will be passed as last argument of callbacks when
	    /// called from this object.
	    /// \note The context argument of each callback is set with the context_value given
	    /// in the user_interaction_callback object constructor. The value can
	    /// can be any arbitrary value (nullptr is valid), and can be used as you wish.
	    /// Note that the listing callback is not defined here, but thanks to a specific method
	user_interaction_callback(void (*x_warning_callback)(const std::string &x, void *context),
				  bool (*x_answer_callback)(const std::string &x, void *context),
				  std::string (*x_string_callback)(const std::string &x, bool echo, void *context),
				  secu_string (*x_secu_string_callback)(const std::string &x, bool echo, void *context),
				  void *context_value);
	user_interaction_callback(const user_interaction_callback & ref) = default;
	user_interaction_callback(user_interaction_callback && ref) noexcept = default;
	user_interaction_callback & operator = (const user_interaction_callback & ref) = default;
	user_interaction_callback & operator = (user_interaction_callback && ref) noexcept = default;
	~user_interaction_callback() = default;

	    /// overwritting method from parent class.
       	virtual void pause(const std::string & message) override;
	    /// overwritting method from parent class.
	virtual std::string get_string(const std::string & message, bool echo) override;
	    /// overwritting method from parent class.
	virtual secu_string get_secu_string(const std::string & message, bool echo) override;
	    /// overwritting method from parent class.
        virtual void listing(const std::string & flag,
			     const std::string & perm,
			     const std::string & uid,
			     const std::string & gid,
			     const std::string & size,
			     const std::string & date,
			     const std::string & filename,
			     bool is_dir,
			     bool has_children) override;

	    /// overwritting method from parent class
	virtual void dar_manager_show_files(const std::string & filename,
					    bool available_data,
					    bool available_ea) override;

	    /// overwritting method from parent class
	virtual void dar_manager_contents(U_I number,
					  const std::string & chemin,
					  const std::string & archive_name) override;

	    /// overwritting method from parent class
	virtual void dar_manager_statistics(U_I number,
					    const infinint & data_count,
					    const infinint & total_data,
					    const infinint & ea_count,
					    const infinint & total_ea) override;

	    /// overwritting method from parent class
	virtual void dar_manager_show_version(U_I number,
					      const std::string & data_date,
					      const std::string & data_presence,
					      const std::string & ea_date,
					      const std::string & ea_presence) override;

	    /// You can set a listing callback thanks to this method.

	    /// If set, when file listing will this callback function will
	    /// be used instead of the x_warning_callback given as argument
	    /// of the constructor.
        void set_listing_callback(void (*callback)(const std::string & flag,
						   const std::string & perm,
						   const std::string & uid,
						   const std::string & gid,
						   const std::string & size,
						   const std::string & date,
						   const std::string & filename,
						   bool is_dir,
						   bool has_children,
						   void *context))
	{
	    tar_listing_callback = callback;
	    set_use_listing(true); // this is to inform libdar to use listing()
	};

	    /// You can set a dar_manager_show_files callback thanks to this method
	void set_dar_manager_show_files_callback(void (*callback)(const std::string & filename,
								  bool available_data,
								  bool available_ea,
								  void *context))
	{
	    dar_manager_show_files_callback = callback;
	    set_use_dar_manager_show_files(true); // this is to inform libdar to use the dar_manager_show_files() method
	};

	void set_dar_manager_contents_callback(void (*callback)(U_I number,
								const std::string & chemin,
								const std::string & archive_name,
								void *context))
	{
	    dar_manager_contents_callback = callback;
	    set_use_dar_manager_contents(true); // this is to inform libdar to use the dar_manager_contents() method
	};

	void set_dar_manager_statistics_callback(void (*callback)(U_I number,
								  const infinint & data_count,
								  const infinint & total_data,
								  const infinint & ea_count,
								  const infinint & total_ea,
								  void *context))
	{
	    dar_manager_statistics_callback = callback;
	    set_use_dar_manager_statistics(true); // this is to inform libdar to use the dar_manager_statistics() method
	};

	void set_dar_manager_show_version_callback(void (*callback)(U_I number,
								    const std::string & data_date,
								    const std::string & data_presence,
								    const std::string & ea_date,
								    const std::string & ea_presence,
								    void *context))
	{
	    dar_manager_show_version_callback = callback;
	    set_use_dar_manager_show_version(true);  // this is to inform libdar to use the dar_manager_show_version() method
	};


	    /// overwritting method from parent class.
	virtual user_interaction *clone() const override;

    protected:
	    /// change the context value of the object that will be given to callback functions
	void change_context_value(void *new_value) { context_val = new_value; };

	    /// overwritting method from parent class.
	virtual void inherited_warning(const std::string & message) override;

    private:
	void (*warning_callback)(const std::string & x, void *context);  // pointer to function
	bool (*answer_callback)(const std::string & x, void *context);   // pointer to function
	std::string (*string_callback)(const std::string & x, bool echo, void *context); // pointer to function
	secu_string (*secu_string_callback)(const std::string & x, bool echo, void *context); // pointer to function
	void (*tar_listing_callback)(const std::string & flags,
				     const std::string & perm,
				     const std::string & uid,
				     const std::string & gid,
				     const std::string & size,
				     const std::string & date,
				     const std::string & filename,
				     bool is_dir,
				     bool has_children,
				     void *context);
	void (*dar_manager_show_files_callback)(const std::string & filename,
						bool available_data,
						bool available_ea,
						void *context);
	void (*dar_manager_contents_callback)(U_I number,
					      const std::string & chemin,
					      const std::string & archive_name,
					      void *context);
	void (*dar_manager_statistics_callback)(U_I number,
						const infinint & data_count,
						const infinint & total_data,
						const infinint & ea_count,
						const infinint & total_ea,
						void *context);
	void (*dar_manager_show_version_callback)(U_I number,
						  const std::string & data_date,
						  const std::string & data_presence,
						  const std::string & ea_date,
						  const std::string & ea_presence,
						  void *context);

	void *context_val;
    };

	/// @}

} // end of namespace

#endif
