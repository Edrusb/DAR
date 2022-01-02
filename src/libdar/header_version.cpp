/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2022 Denis Corbin
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

#include "../my_config.h"

extern "C"
{
#if HAVE_STRING_H
#include <string.h>
#endif
} // end of extern "C"


#include "header_version.hpp"
#include "integers.hpp"
#include "tools.hpp"
#include "deci.hpp"
#include "header_flags.hpp"


#define LIBDAR_URL_VERSION "http://dar.linux.free.fr/pre-release/doc/Notes.html#Dar_version_naming"

using namespace std;

namespace libdar
{

    	    // FLAG VALUES

	    // flags of the historical bytes (oldest), read/wrote last (or alone if no extra bits are set)
    static constexpr U_I FLAG_SAVED_EA_ROOT = 0x80;       ///< no more used since version "05"
    static constexpr U_I FLAG_SAVED_EA_USER = 0x40;       ///< no more used since version "05"
    static constexpr U_I FLAG_SCRAMBLED     = 0x20;       ///< scrambled or strong encryption used
    static constexpr U_I FLAG_SEQUENCE_MARK = 0x10;       ///< escape sequence marks present for sequential reading
    static constexpr U_I FLAG_INITIAL_OFFSET = 0x08;      ///< whether the header contains the initial offset (size of clear data before encrypted)
    static constexpr U_I FLAG_HAS_CRYPTED_KEY = 0x04;     ///< the header contains a symmetrical key encrypted with asymmetrical algorithm
    static constexpr U_I FLAG_HAS_REF_SLICING = 0x02;     ///< the header contains the slicing information of the archive of reference (used for isolated catalogue)
    static constexpr U_I FLAG_IS_RESERVED_1 = 0x01;       ///< this flag is reserved meaning two bytes length bitfield
    static constexpr U_I FLAG_IS_RESERVED_2 = 0x0100;     ///< reserved for future use
    static constexpr U_I FLAG_ARCHIVE_IS_SIGNED = 0x0200; ///< archive is signed
    static constexpr U_I FLAG_HAS_KDF_PARAM = 0x0400;     ///< archive header contains salt and non default interaction count
    static constexpr U_I FLAG_HAS_COMPRESS_BS = 0x0800;   ///< archive header contains a compression block size (else it is assumed equal to zero)
	// when adding a new flag, all_flags_known() must be updated to pass sanity checks

    static const U_I HEADER_CRC_SIZE = 2; //< CRC width (deprecated, now only used when reading old archives)

	// check we are not facing a more recent format than what we know, this would fail CRC calculation before reporting the real reason of the issue
    static bool all_flags_known(header_flags flag);

	//


    header_version::header_version()
    {
	algo_zip = compression::none;
	cmd_line = "";
	initial_offset = 0;
	sym = crypto_algo::none;
	crypted_key = nullptr;
	ref_layout = nullptr;
	ciphered = false;
	arch_signed = false;
	iteration_count = PRE_FORMAT_10_ITERATION;
	kdf_hash = hash_algo::sha1; // used by default
	salt = "";
	compr_bs = 0;
	has_kdf_params = false;
    }

    void header_version::read(generic_file & f, user_interaction & dialog, bool lax_mode)
    {
	crc *ctrl = nullptr;
	char tmp;
        header_flags flag;

	f.reset_crc(HEADER_CRC_SIZE);
	try
	{
	    edition.read(f);
	}
	catch(Egeneric & e)
	{
	    if(lax_mode)
	    {
		string answ;
		U_I equivalent;
		bool ok = false;

		dialog.message(gettext("LAX MODE: Failed to read the archive header's format version."));
		do
		{
		    answ = dialog.get_string(tools_printf(gettext("LAX MODE: Please provide the archive format: You can use the table at %s to find the archive format depending on the release version, (for example if this archive has been created using dar release 2.3.4 to 2.3.7 answer \"6\" without the quotes here): "), LIBDAR_URL_VERSION), true);
		    if(tools_my_atoi(answ.c_str(), equivalent))
			edition = equivalent;
		    else
		    {
			dialog.pause(tools_printf(gettext("LAX MODE: \"%S\" is not a valid archive format"), &answ));
			continue;
		    }

		    try
		    {
			dialog.pause(tools_printf(gettext("LAX MODE: Using archive format \"%d\"?"), equivalent));
			ok = true;
		    }
		    catch(Euser_abort & e)
		    {
			ok = false;
		    }
		}
		while(!ok);
	    }
	    else
		throw;
	}

	if(f.read(&tmp, 1) == 1) // compression algo
	{
	    bool ok = false;
	    do
	    {
		try
		{
		    algo_zip = char2compression(tmp);
		    ok = true;
		}
		catch(Erange & e)
		{
		    bool loop = false;

		    if(!lax_mode)
			throw;

		    do
		    {
		    string answ = dialog.get_string(gettext("LAX MODE: Unknown compression algorithm used, assuming data corruption occurred. Please help me, answering with one of the following words \"none\", \"gzip\", \"bzip2\", \"lzo\", \"xz\", \"zstd\" or \"lz4\" at the next prompt:"), true);

			if(answ == gettext("none"))
			{
			    tmp = compression2char(compression::none);
			    loop = false;
			}
			else if(answ == gettext("gzip"))
			{
			    tmp = compression2char(compression::gzip);
			    loop = false;
			}
			else if(answ == gettext("bzip2"))
			{
			    tmp = compression2char(compression::bzip2);
			    loop = false;
			}
			else if(answ == gettext("lzo"))
			{
			    tmp = compression2char(compression::lzo);
			    loop = false;
			}
			else if(answ == gettext("xz"))
			{
			    tmp = compression2char(compression::xz);
			    loop = false;
			}
			else if(answ == gettext("zstd"))
			{
			    tmp = compression2char(compression::zstd);
			    loop = false;
			}
			else if(answ == gettext("lz4"))
			{
			    tmp = compression2char(compression::lz4);
			    loop = false;
			}
			else
			    loop = true;
		    }
		    while(loop);
		}
	    }
	    while(!ok);
	}
	else
	    throw Erange("header_version::read", gettext("Reached End of File while reading archive header_version data structure"));

	tools_read_string(f, cmd_line);

	if(edition > 1)
	    flag.read(f);
	else
	    flag.clear(); // flag has been at edition 2

	if(flag.is_set(FLAG_INITIAL_OFFSET))
	{
	    initial_offset.read(f);
	}
	else
	    initial_offset = 0;

	if(flag.is_set(FLAG_SCRAMBLED))
	{
	    ciphered = true;
	    if(edition >= 9)
	    {
		if(f.read(&tmp, sizeof(tmp)) != 1)
		    throw Erange("header_version::read", gettext("Reached End of File while reading archive header_version data structure"));

		try
		{
		    sym = char_2_crypto_algo(tmp);
		}
		catch(Erange & e)
		{
		    if(!lax_mode)
			throw;
		    dialog.printf("Unknown crypto algorithm used in archive, ignoring that field and simply assuming the archive has been encrypted, if not done you will need to specify the crypto algorithm to use in order to read this archive");
		    sym = crypto_algo::none;
		}
	    }
	    else
		    // unknown ciphering algorithm used (old archive format) or no encryption
		    // not coherent with flag which has the FLAG_SCRAMBLED bit set
		    // but that this way we record that the crypto algo has
		    // to be provided by the user
		sym = crypto_algo::none;
	}
	else
	{
	    ciphered = false;
	    sym = crypto_algo::none; // no crypto used, coherent with flag
	}

	has_tape_marks = flag.is_set(FLAG_SEQUENCE_MARK);
	if(edition < 8 && has_tape_marks)
	{
	    if(lax_mode)
		has_tape_marks = false; // Escape sequence marks appeared at revision 08
	    else
		throw Erange("header_version::read", gettext("Corruption met while reading header_version data structure"));
	}

	if(crypted_key != nullptr)
	{
	    delete crypted_key;
	    crypted_key = nullptr;
	}

	if(flag.is_set(FLAG_HAS_CRYPTED_KEY))
	{
	    infinint key_size = f; // reading key_size from the header_version in archive

	    crypted_key = new (nothrow) memory_file();
	    if(crypted_key == nullptr)
		throw Ememory("header_version::read");
	    if(f.copy_to(*crypted_key, key_size) != key_size)
		throw Erange("header_version::read", gettext("Missing data for encrypted symmetrical key"));
	}

	if(flag.is_set(FLAG_HAS_REF_SLICING))
	{
	    try
	    {
		if(ref_layout == nullptr)
		    ref_layout = new (nothrow) slice_layout();
		if(ref_layout == nullptr)
		    throw Ememory("header_version::read");
		ref_layout->read(f);
	    }
	    catch(Egeneric & e)
	    {
		if(lax_mode)
		{
		    dialog.message(gettext("Error met while reading archive of reference slicing layout, ignoring this field and continuing"));
		    clear_slice_layout();
		}
		else
		    throw;
	    }
	}
	else
	    clear_slice_layout();

	arch_signed = flag.is_set(FLAG_ARCHIVE_IS_SIGNED);

	if(flag.is_set(FLAG_HAS_KDF_PARAM))
	{
	    unsigned char tmp_hash;
	    infinint salt_size(f); // reading salt_size from file

	    has_kdf_params = true;
	    tools_read_string_size(f, salt, salt_size);
	    iteration_count.read(f);
	    f.read((char *)&tmp_hash, 1);
	    try
	    {
		kdf_hash = char_to_hash_algo(tmp_hash);
		if(kdf_hash == hash_algo::none)
		    throw Erange("header_version::read", gettext("valid hash algoritm needed for key derivation function"));
	    }
	    catch(Erange & e)
	    {
		if(lax_mode)
		{
		    string msg = e.get_message(); // msg has two roles: 1)display error message to user 2) get answer from user
		    bool ok = false;

		    do
		    {
			dialog.message(msg);
			msg = dialog.get_string(gettext("please indicate the hash algoritm to use for key derivation function '1' for sha1, '5' for sha512, 'm' for md5, or 'q' to abort: "), true);
			try
			{
			    if(msg.size() == 1)
			    {
				tmp_hash = msg.c_str()[0];
				if(tmp_hash == 'q')
				{
				    kdf_hash = hash_algo::none;
				    ok = true;
				}
				else
				{
				    kdf_hash = char_to_hash_algo(tmp_hash);
				    ok = true;
				}
			    }
			    else
				dialog.message(gettext("please answer with a single character"));
			}
			catch(Erange & e)
			{
			    msg = e.get_message();
			}
		    }
		    while(!ok);

		    if(kdf_hash == hash_algo::none)
			throw;
		}
		else
		    throw;
	    }
	}
	else
	{
	    salt = "";
	    iteration_count = PRE_FORMAT_10_ITERATION;
	    kdf_hash = hash_algo::sha1;
	}

	if(flag.is_set(FLAG_HAS_COMPRESS_BS))
	    compr_bs.read(f);
	else
	    compr_bs = 0;

	ctrl = f.get_crc();
	if(ctrl == nullptr)
	    throw SRC_BUG;

	try
	{
	    if(edition == empty_archive_version())
	    {
		if(lax_mode)
		    dialog.message(gettext("Consistency check failed for archive header"));
		else
		    throw Erange("header_version::read", gettext("Consistency check failed for archive header"));
	    }

	    if(edition > 7)
	    {
		try
		{
		    crc *coh = create_crc_from_file(f);

		    if(coh == nullptr)
			throw SRC_BUG;
		    try
		    {
			if(typeid(*coh) != typeid(*ctrl))
			{
			    if(coh->get_size() != ctrl->get_size())
				throw SRC_BUG;
			    else
				throw SRC_BUG; // both case lead to a bug, but we need to know which one is met
			}

			if(*coh != *ctrl)
			{
			    if(lax_mode)
				dialog.message(gettext("Consistency check failed for archive header"));
			    else
				throw Erange("header_version::read", gettext("Consistency check failed for archive header"));
			}
		    }
		    catch(...)
		    {
			if(coh != nullptr)
			    delete coh;
			throw;
		    }
		    if(coh != nullptr)
			delete coh;
		}
		catch(...)
		{
		    if(!all_flags_known(flag))
			dialog.message(gettext("Unknown flag found in archive header/trailer, ignoring the CRC mismatch possibly due to the unknown corresponding field"));
		    else
			throw;
		}

	    }
	    if(initial_offset.is_zero())
		initial_offset = f.get_position();
	}
	catch(...)
	{
	    if(ctrl != nullptr)
		delete ctrl;
	    throw;
	}

	if(ctrl != nullptr)
	    delete ctrl;
    }

    void header_version::write(generic_file &f) const
    {
	crc *ctrl = nullptr;
	char tmp;
        header_flags flag;

	    // preparing the data

	if(!initial_offset.is_zero())
	    flag.set_bits(FLAG_INITIAL_OFFSET); // adding it to the flag

	if(crypted_key != nullptr)
	    flag.set_bits(FLAG_HAS_CRYPTED_KEY);

	if(ref_layout != nullptr)
	    flag.set_bits(FLAG_HAS_REF_SLICING);

	if(has_tape_marks)
	    flag.set_bits(FLAG_SEQUENCE_MARK);

	if(sym != crypto_algo::none)
	    flag.set_bits(FLAG_SCRAMBLED);
	    // Note: we cannot set this flag (even if ciphered is true) if we do not know the crypto algo
	    // as since version 9 the presence of this flag implies the existence
	    // of the crypto algorithm in the header/trailer and we will always
	    // write down a header/version of the latest known format (thus greater or
	    // equal to 9).

	if(arch_signed)
	    flag.set_bits(FLAG_ARCHIVE_IS_SIGNED);

	if(salt.size() > 0)
	    flag.set_bits(FLAG_HAS_KDF_PARAM);

	if(compr_bs > 0)
	    flag.set_bits(FLAG_HAS_COMPRESS_BS);

	if(!all_flags_known(flag))
	    throw SRC_BUG; // all_flag_known has not been updated with new flags

	    // writing down the data

	f.reset_crc(HEADER_CRC_SIZE);
	edition.dump(f);
	tmp = compression2char(algo_zip);
	f.write(&tmp, sizeof(tmp));
	tools_write_string(f, cmd_line);
	flag.dump(f);
	if(initial_offset != 0)
	    initial_offset.dump(f);
	if(sym != crypto_algo::none)
	{
	    tmp = crypto_algo_2_char(sym);
	    f.write(&tmp, sizeof(tmp));
	}

	if(crypted_key != nullptr)
	{
	    crypted_key->size().dump(f);
	    crypted_key->skip(0);
	    crypted_key->copy_to(f);
	}

	if(ref_layout != nullptr)
	    ref_layout->write(f);

	if(salt.size() > 0)
	{
	    unsigned char tmp_hash = hash_algo_to_char(kdf_hash);
	    infinint salt_size = salt.size();

	    salt_size.dump(f);
	    tools_write_string_all(f, salt);
	    iteration_count.dump(f);
	    f.write((char *)&tmp_hash, 1);
	}

	if(compr_bs > 0)
	    compr_bs.dump(f);

	ctrl = f.get_crc();
	if(ctrl == nullptr)
	    throw SRC_BUG;
	try
	{
	    ctrl->dump(f);
	}
	catch(...)
	{
	    if(ctrl != nullptr)
		delete ctrl;
	    throw;
	}
	if(ctrl != nullptr)
	    delete ctrl;
    }

    void header_version::set_kdf_hash(hash_algo algo)
    {
	if(algo == hash_algo::none)
	    throw Erange("header_version::set_kdf_hash", gettext("invalid hash algorithm provided for key derivation function"));
	kdf_hash = algo;
	has_kdf_params = true;
    }

    string header_version::get_sym_crypto_name() const
    {
	if(get_edition() >= 9)
	    return crypto_algo_2_string(get_sym_crypto_algo());
	else
	    return is_ciphered() ? gettext("yes") : gettext("no");
    }

    string header_version::get_asym_crypto_name() const
    {
	if((get_edition() >= 9)
	   && (get_crypted_key() != nullptr))
	    return "gnupg";
	else
	    return gettext("none");
    }

    void header_version::display(user_interaction & dialog) const
    {
	string algo = compression2string(get_compression_algo());
	string sym_str = get_sym_crypto_name();
	string asym = get_asym_crypto_name();
	string xsigned = is_signed() ? gettext("yes") : gettext("no");
	string kdf_iter = deci(iteration_count).human();
	string hashing = hash_algo_to_string(kdf_hash);

	dialog.printf(gettext("Archive version format               : %s"), get_edition().display().c_str());
	dialog.printf(gettext("Compression algorithm used           : %S"), &algo);
	dialog.printf(gettext("Compression block size used          : %i"), &compr_bs);
	dialog.printf(gettext("Symmetric key encryption used        : %S"), &sym_str);
	dialog.printf(gettext("Asymmetric key encryption used       : %S"), &asym);
	dialog.printf(gettext("Archive is signed                    : %S"), &xsigned);
	dialog.printf(gettext("Sequential reading marks             : %s"), (get_tape_marks() ? gettext("present") : gettext("absent")));
	dialog.printf(gettext("User comment                         : %S"), &(get_command_line()));
	if(has_kdf_params)
	{
	    dialog.printf(gettext("KDF iteration count                  : %S"), &kdf_iter);
	    dialog.printf(gettext("KDF hash algorithm                   : %S"), &hashing);
	    dialog.printf(gettext("Salt size                            : %d byte%c"), salt.size(), salt.size() > 1 ? 's' : ' ');
	}
    }

    void header_version::clear()
    {
	edition = archive_version();
	algo_zip = compression::none;
	cmd_line = "";
	initial_offset = 0;
	sym = crypto_algo::none;
	clear_crypted_key();
	clear_slice_layout();
	has_tape_marks = false;
	ciphered = false;
	arch_signed = false;
	iteration_count = PRE_FORMAT_10_ITERATION;
	kdf_hash = hash_algo::sha1;
	compr_bs = 0;
    }

    void header_version::copy_from(const header_version & ref)
    {
	edition = ref.edition;
	algo_zip = ref.algo_zip;
	cmd_line = ref.cmd_line;
	initial_offset = ref.initial_offset;
	sym = ref.sym;
	if(ref.crypted_key != nullptr)
	{
	    crypted_key = new (nothrow) memory_file(*ref.crypted_key);
	    if(crypted_key == nullptr)
		throw Ememory("header_version::copy_from");
	}
	else
	    crypted_key = nullptr;
	if(ref.ref_layout != nullptr)
	{
	    ref_layout = new (nothrow) slice_layout(*ref.ref_layout);
	    if(ref_layout == nullptr)
		throw Ememory("header_version::copy_from");
	}
	else
	    ref_layout = nullptr;
	has_tape_marks = ref.has_tape_marks;
	ciphered = ref.ciphered;
	arch_signed = ref.arch_signed;
	has_kdf_params = ref.has_kdf_params;
	salt = ref.salt;
	iteration_count = ref.iteration_count;
	kdf_hash = ref.kdf_hash;
	compr_bs = ref.compr_bs;
    }

    void header_version::move_from(header_version && ref) noexcept
    {
	edition = move(ref.edition);
	algo_zip = move(ref.algo_zip);
	cmd_line = move(ref.cmd_line);
	initial_offset = move(ref.initial_offset);
	sym = move(ref.sym);
	swap(crypted_key, ref.crypted_key);
	swap(ref_layout, ref.ref_layout);
	has_tape_marks = move(ref.has_tape_marks);
	ciphered = move(ref.ciphered);
	arch_signed = move(ref.arch_signed);
	has_kdf_params = move(ref.has_kdf_params);
	salt = move(ref.salt);
	iteration_count = move(ref.iteration_count);
	kdf_hash = move(ref.kdf_hash);
	compr_bs = move(ref.compr_bs);
    }

    void header_version::detruit()
    {
	clear_crypted_key();
	clear_slice_layout();
    }

    static bool all_flags_known(header_flags flag)
    {
	U_I bf = 0;

	bf |= FLAG_SAVED_EA_ROOT;
	bf |= FLAG_SAVED_EA_USER;
	bf |= FLAG_SCRAMBLED;
	bf |= FLAG_SEQUENCE_MARK;
	bf |= FLAG_INITIAL_OFFSET;
	bf |= FLAG_HAS_CRYPTED_KEY;
	bf |= FLAG_HAS_REF_SLICING;
	bf |= FLAG_ARCHIVE_IS_SIGNED;
	bf |= FLAG_HAS_KDF_PARAM;
	bf |= FLAG_HAS_COMPRESS_BS;
	flag.unset_bits(bf);

	return flag.is_all_cleared();
    }

} // end of namespace
