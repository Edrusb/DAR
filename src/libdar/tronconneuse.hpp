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

    /// \file tronconneuse.hpp
    /// \brief defines a block structured file.
    /// \ingroup Private
    ///
    /// Mainly used for strong encryption.

#ifndef TRONCONNEUSE_HPP
#define TRONCONNEUSE_HPP

#include "../my_config.h"
#include <string>

#include "infinint.hpp"
#include "generic_file.hpp"
#include "archive_version.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{


	/// this is a partial implementation of the generic_file interface to cypher/decypher data block by block.

	/// This class is a pure virtual one, as several calls have to be defined by inherited classes
	/// - encrypted_block_size_for
	/// - clear_block_allocated_size_for
	/// - encrypt_data
	/// - decrypt_data
	/// .
	/// tronconneuse is either read_only or write_only, read_write is not allowed.
	/// The openning mode is defined by encrypted_side's mode.
	/// In write_only no skip() is allowed, writing is sequential from the beginning of the file to the end
	/// (like writing to a pipe).
	/// In read_only all skip() functions are available.
    class tronconneuse : public generic_file
    {
    public:
	    /// This is the constructor

	    /// \param[in] block_size is the size of block encryption (the size of clear data encrypted toghether).
	    /// \param[in] encrypted_side where encrypted data are read from or written to.
  	    /// \param[in] no_initial_shift assume that no unencrypted data is located at the begining of the underlying file, else this is the
	    /// position of the encrypted_side at the time of this call that is used as initial_shift
	    /// \param[in] reading_ver version of the archive format
	    /// \note that encrypted_side is not owned and destroyed by tronconneuse, it must exist during all the life of the
	    /// tronconneuse object, and is not destroyed by the tronconneuse's destructor
	tronconneuse(U_32 block_size, generic_file & encrypted_side, bool no_initial_shift, const archive_version & reading_ver);

	    /// copy constructor
	tronconneuse(const tronconneuse & ref) : generic_file(ref) { copy_from(ref); };

	    /// move constructor
	tronconneuse(tronconneuse && ref) noexcept: generic_file(std::move(ref)) { nullifyptr(); move_from(std::move(ref)); };

	    /// assignment operator
	tronconneuse & operator = (const tronconneuse & ref);

	    /// move operator
	tronconneuse & operator = (tronconneuse && ref) noexcept { generic_file::operator = (std::move(ref)); move_from(std::move(ref)); return *this; };

	    /// destructor
	virtual ~tronconneuse() override { detruit(); }; // must not write pure virtual method from here, directly or not

	    /// inherited from generic_file
	virtual bool skippable(skippability direction, const infinint & amount) override;
	    /// inherited from generic_file
	virtual bool skip(const infinint & pos) override;
	    /// inherited from generic_file
	virtual bool skip_to_eof() override;
	    /// inherited from generic_file
	virtual bool skip_relative(S_I x) override;
	    /// inherited from generic_file
	virtual bool truncatable(const infinint & pos) const override { return false; };
	    /// inherited from generic_file
	virtual infinint get_position() const override { if(is_terminated()) throw SRC_BUG; return current_position; };

	    /// in write_only mode indicate that end of file is reached

	    /// this call must be called in write mode to purge the
	    /// internal cache before deleting the object (else some data may be lost)
	    /// no further write call is allowed
	    /// \note this call cannot be used from the destructor, because it relies on pure virtual methods
	void write_end_of_file() { if(is_terminated()) throw SRC_BUG; flush(); weof = true; };


	    /// this method to modify the initial shift. This overrides the constructor "no_initial_shift" of the constructor

	void set_initial_shift(const infinint & x) { initial_shift = x; };

	    /// let the caller give a callback function that given a generic_file with cyphered data, is able
	    /// to return the offset of the first clear byte located *after* all the cyphered data, this
	    /// callback function is used (if defined by the following method), when reaching End of File.
	void set_callback_trailing_clear_data(infinint (*call_back)(generic_file & below, const archive_version & reading_ver)) { trailing_clear_data = call_back; };

	    /// returns the block size give to constructor
	U_32 get_clear_block_size() const { return clear_block_size; };

    private:

	    /// inherited from generic_file

	    /// this protected inherited method is now private for inherited classes of tronconneuse
	virtual void inherited_read_ahead(const infinint & amount) override;

	    /// this protected inherited method is now private for inherited classes of tronconneuse
	virtual U_I inherited_read(char *a, U_I size) override;

	    /// inherited from generic_file

	    /// this protected inherited method is now private for inherited classes of tronconneuse
	virtual void inherited_write(const char *a, U_I size) override;

	    /// this prorected inherited method is now private for inherited classed of tronconneuse
	virtual void inherited_truncate(const infinint & pos) override { throw SRC_BUG; }; // no skippability in write mode, so no truncate possibility neither

	    /// this protected inherited method is now private for inherited classes of tronconneuse
	virtual void inherited_sync_write() override { flush(); };


	    /// this protected inherited method is now private for inherited classes of tronconneuse
	virtual void inherited_flush_read() override { buf_byte_data = 0; };

	    /// this protected inherited method is now private for inherited classes of tronconneuse
	virtual void inherited_terminate() override {};

    protected:
	    /// defines the size necessary to encrypt a given amount of clear data

	    /// \param[in] clear_block_size is the size of the clear block to encrypt.
	    /// \return the size of the memory to allocate to receive corresponding encrypted data.
	    /// \note this implies that encryption algorithm must always generate a fixed size encrypted block of data for
	    /// a given fixed size block of data. However, the size of the encrypted block of data may differ from
	    /// the size of the clear block of data
	virtual U_32 encrypted_block_size_for(U_32 clear_block_size) = 0;

	    /// it may be necessary by the inherited class have few more bytes allocated after the clear data given for encryption

	    /// \param[in] clear_block_size is the size in byte of the clear data that will be asked to encrypt.
	    /// \return the requested allocated buffer size (at least the size of the clear data).
	    /// \note when giving clear buffer of data of size "clear_block_size" some inherited class may requested
	    /// that a bit more of data must be allocated.
	    /// this is to avoid copying data when the algorithm needs to add some data after the
	    /// clear data before encryption.
	virtual U_32 clear_block_allocated_size_for(U_32 clear_block_size) = 0;

	    /// this method encrypts the clear data given

	    /// \param block_num is the number of the block to which correspond the given data, This is an informational field for inherited classes.
	    /// \param[in] clear_buf points to the first byte of clear data to encrypt.
	    /// \param[in] clear_size is the length in byte of data to encrypt.
	    /// \param[in] clear_allocated is the size of the allocated memory (modifiable bytes) in clear_buf: clear_block_allocated_size_for(clear_size)
	    /// \param[in,out] crypt_buf is the area where to put corresponding encrypted data.
	    /// \param[in] crypt_size is the allocated memory size for crypt_buf: encrypted_block_size_for(clear_size)
	    /// \return is the amount of data put in crypt_buf (<= crypt_size).
	    /// \note it must respect that : returned value = encrypted_block_size_for(clear_size argument)
	virtual U_32 encrypt_data(const infinint & block_num,
				  const char *clear_buf, const U_32 clear_size, const U_32 clear_allocated,
				  char *crypt_buf, U_32 crypt_size) = 0;

	    /// this method decyphers data

	    /// \param[in] block_num block number of the data to decrypt.
	    /// \param[in] crypt_buf pointer to the first byte of encrypted data.
	    /// \param[in] crypt_size size of encrypted data to decrypt.
	    /// \param[in,out] clear_buf pointer where to put clear data.
	    /// \param[in] clear_size allocated size of clear_buf.
	    /// \return is the amount of data put in clear_buf (<= clear_size)
	virtual U_32 decrypt_data(const infinint & block_num,
				  const char *crypt_buf, const U_32 crypt_size,
				  char *clear_buf, U_32 clear_size) = 0;

    protected:
	const archive_version & get_reading_version() const { return reading_ver; };


    private:
	infinint initial_shift;    ///< the initial_shift first bytes of the underlying file are not encrypted
	    //
	infinint buf_offset;       ///< offset of the first byte in buf
	U_32 buf_byte_data;        ///< number of byte of information in buf (buf_byte_data <= buf_size)
	U_32 buf_size;             ///< size of allocated memory for clear data in buf
	char *buf;                 ///< decrypted data (or data to encrypt)
	    //
	U_32 clear_block_size;     ///< max amount of data that will be encrypted at once (must stay less than buf_size)
	infinint current_position; ///< position of the next character to read or write from the upper layer perspective, offset zero is the first encrypted byte, thus the first byte after initial_shift
	infinint block_num;        ///< block number we next read or write
	generic_file *encrypted;   ///< generic_file where is put / get the encrypted data
	    //
	U_32 encrypted_buf_size;   ///< allocated size of encrypted_buf
	U_32 encrypted_buf_data;   ///< amount of byte of information in encrypted_buf
	char *encrypted_buf;       ///< buffer of encrypted data (read or to write)
	    //
	infinint extra_buf_offset; ///< offset of the first byte of extra_buf
	U_32 extra_buf_size;       ///< allocated size of extra_buf
	U_32 extra_buf_data;       ///< amount of byte of information in extra_buf
	char *extra_buf;           ///< additional read encrypted that follow what is in encrypted_buf used to check for clear data after encrypted data
	    //
	bool weof;                 ///< whether write_end_of_file() has been called
	bool reof;                 ///< whether we reached eof while reading
	archive_version reading_ver; ///< archive format we currently read
	infinint (*trailing_clear_data)(generic_file & below, const archive_version & reading_ver); ///< callback function that gives the amount of clear data found at the end of the given file


	void nullifyptr() noexcept;
	void detruit() noexcept;
	void copy_from(const tronconneuse & ref);
	void move_from(tronconneuse && ref) noexcept;
	U_32 fill_buf();       ///< returns the position (of the next read op) inside the buffer and fill the buffer with clear data
	void flush();          ///< flush any pending data (write mode only) to encrypted device
	void init_buf();       ///< initialize if necessary the various buffers that relies on inherited method values


	    /// convert clear position to corresponding position in the encrypted data

	    /// \param[in] pos is the position in the clear data
	    /// \param[out] file_buf_start is the position of the beginning of the crypted block where can be found the data
	    /// \param[out] clear_buf_start is the position of the beginning of the corresponding clear block
	    /// \param[out] pos_in_buf is the position in the clear block of the 'pos' offset
	    /// \param[out] block_num is the block number we have our requested position inside
	void position_clear2crypt(const infinint & pos,
				  infinint & file_buf_start,
				  infinint & clear_buf_start,
				  infinint & pos_in_buf,
				  infinint & block_num);

	    /// gives the position of the next character
	    /// of clear data that corresponds to the encrypted data which index is pos
	void position_crypt2clear(const infinint & pos, infinint & clear_pos);

	    /// return true if a there is a byte of information at the given offset
	bool check_current_position() { return fill_buf() < buf_byte_data; };

	    /// remove clear data at the end of the encrypted_buf
	    /// \param[in] crypt_offset is the offset of the first byte of encrypted_buf not
	    /// considering initial_shift bytes before the begining of the encrypted data
	void remove_trailing_clear_data_from_encrypted_buf(const infinint & crypt_offset);

    };

	/// @}

} // end of namespace

#endif
