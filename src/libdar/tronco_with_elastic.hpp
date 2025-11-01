/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2025 Denis Corbin
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

    /// \file tronco_with_elastic.hpp
    /// \brief encryption layer with embedded elastic buffer

    /// it relies on tronconneuse or parallele_tronconneuse for the encryption
    /// and adds/fetches an elastic buffer at the beginning and at the end of the
    /// encrypted data to provide what's in between, which is the real encrypted data
    /// \ingroup Private
    ///

#ifndef TRONCO_WITH_ELASTIC_HPP
#define TRONCO_WITH_ELASTIC_HPP

#include "../my_config.h"
#include <string>
#include <memory>

#include "crypto.hpp"
#include "archive_aux.hpp"
#include "proto_tronco.hpp"
#include "elastic.hpp"
#include "mem_ui.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{


	/// this class abstracts encryption (tronconneuse & parallel_tronconneuse) adding elastic buffers a the extremities

    class tronco_with_elastic : public generic_file, public mem_ui
    {
    public:
	    /// This is the constructor

	    /// \param[in] dialog for user interaction
	    /// \param[in] workers is the number of worker threads
	    /// \param[in] block_size is the size of block encryption (the size of clear data encrypted together).
	    /// \param[in] encrypted_side where encrypted data are read from or written to.
	    /// \param[in] password cipher password
	    /// \param[in] reading_ver archive format version
	    /// \param[in] algo ciphering algorithm to use
	    /// \param[in] salt if set empty, the salt is generated when use_pcsk5 argument is set
	    /// \param[in] kdf_hash only used if use_pkcs5 is true
	    /// \param[in] iteration_count only used if use_pkcs5 is true
	    /// \param[in] use_pkcs5 must be set if provided password is human defined to apply KDF on it
	    /// \param[in] info_details whether to display information messages on what's going on
	    ///
	    /// \note that encrypted_side is not owned and destroyed by tronco_with_elastic, it must exist during all the life of the
	    /// tronco_with_elastic object, and is not destroyed by the tronco_with_elastic's destructor
	tronco_with_elastic(const std::shared_ptr<user_interaction> & dialog,
			    U_I workers,
			    U_32 block_size,
			    generic_file & encrypted_side,
			    const secu_string & password,
			    const archive_version & reading_ver,
			    crypto_algo algo,
			    const std::string & salt,
			    const infinint & iteration_count,
			    hash_algo kdf_hash,
			    bool use_pkcs5,
			    bool info_details);


	    /// copy constructor
	tronco_with_elastic(const tronco_with_elastic & ref) = delete;

	    /// move constructor
	tronco_with_elastic(tronco_with_elastic && ref) noexcept = delete;

	    /// assignment operator
	tronco_with_elastic & operator = (const tronco_with_elastic & ref) = delete;

	    /// move operator
	tronco_with_elastic & operator = (tronco_with_elastic && ref) noexcept = default;

	    /// destructor
	virtual ~tronco_with_elastic() noexcept = default;

	    /// obtain the salt
	const std::string & get_salt() const { return sel; };


	    /// necessary before calling write,

	    /// \param[in] initial_shift defines the offset before which clear data is stored encrypted_side

	    /// \note this call adds an elastic buffer at the provided offset and shifts the writing
	    /// position right after it.
	void get_ready_for_writing(const infinint & initial_shift);

	    /// get ready for reading fetching information from the end

	    /// \note in this mode the terminal elastic_buffer is fetch reading
	    /// data backward which takes care of possible clear data located
	    /// after it (and other encrypted data before it), but the initial_shift
	    /// cannot be guessed and has to be provided
	void get_ready_for_reading(const infinint & initial_shift);

	    /// get ready for reading fetching information from the beginning

	    /// \param[in] callback callback function to detect clear data after encrypted one at end of file
	    /// \param[in] skip_after_initial_elastic if set, reads the initial elastic data structure and position
	    /// the read cursor right after it
	    /// \note this expected the encrypted_side to be located at the beginning
	    /// of encrypted data, but a callback function must be necessary to detect
	    /// clear data located after the terminal elastic_buffer
	void get_ready_for_reading(trailing_clear_data_callback callback,
				   bool skip_after_initial_elastic);

	    /// in write_only mode indicate that end of file is reached

	    /// this call must be called in write mode to add final elastic buffer then purge the
	    /// internal cache before deleting the object (else some data may be lost)
	    /// no further write call is allowed
	    /// \note this call cannot be used from the destructor, because it relies on pure virtual methods
	virtual void write_end_of_file();

	    /// inherited from generic_file
	virtual bool skippable(skippability direction, const infinint & amount) override;

	    /// inherited from generic_file
	virtual bool skip(const infinint & pos) override;

	    /// inherited from generic_file
	virtual bool skip_to_eof() override;

	    /// inherited from generic_file
	virtual bool skip_relative(S_I x) override;

	    /// inherited from generic_file
	virtual bool truncatable(const infinint & pos) const override;

	    /// inherited from generic_file
	virtual infinint get_position() const override;



    protected:

	    /// inherited from generic_file
	virtual void inherited_read_ahead(const infinint & amount) override;

	    /// inherited from generic_file
	virtual U_I inherited_read(char *a, U_I size) override;

	    /// inherited from generic_file
	virtual void inherited_write(const char *a, U_I size) override;

	    /// inherited from generic_file
	virtual void inherited_truncate(const infinint & pos) override;

	    /// inherited from generic_file
	virtual void inherited_sync_write() override;

	    /// inherited from generic_file
	virtual void inherited_flush_read() override;

	    /// inherited from generic_file
	virtual void inherited_terminate() override;


    private:
	enum { init, reading, writing, closed } status;

	std::unique_ptr<proto_tronco> behind;
	generic_file* encrypted;
	archive_version reading_version;
	std::string sel;
	bool info;

	    // static fields and methods

	static constexpr U_I GLOBAL_ELASTIC_BUFFER_SIZE = 51200;

	    /// append an elastic buffer of given size to the file

	    /// \param[in,out] f file to append elastic buffer to
	    /// \param[in] max_size size of the elastic buffer to add
	    /// \param[in] modulo defines the size to choose (see note)
	    /// \param[in] offset defines the offset to apply (see note)
	    /// \note the size of the elastic buffer should not exceed max_size but
	    /// should be chosen in order to reach a size which is zero modulo "modulo"
	    /// assuming the offset we add the elastic buffer at is "offset". If modulo is zero
	    /// this the elastic buffer is randomly chosen from 1 to max_size without any
	    /// concern about being congruent to a given modulo.
	    /// Example if module is 5 and offset is 2, the elastic buffer possible size
	    /// can be 3 (2+3 is congruent to 0 modulo 5), 8 (2+8 is congruent to modulo 5), 12, etc.
	    /// but not exceed max_size+modulo-1
	    /// \note this is to accomodate the case when encrypted data is followed by clear data
	    /// at the end of an archive. There is no way to known when we read clear data, but we
	    /// know the clear data size is very inferior to crypted block size, thus when reading
	    /// a uncompleted block of data we can be sure we have reached end of file and that
	    /// the data is clear without any encrypted part because else we would have read an entire
	    /// block of data.
	static void add_elastic_buffer(generic_file & f,
				       U_32 max_size,
				       U_32 modulo,
				       U_32 offset);



    };

	/// @}

} // end of namespace

#endif
