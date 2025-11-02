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

#include "../my_config.h"

extern "C"
{
}

#include "tronco_with_elastic.hpp"

#include "tools.hpp"
#include "crypto_sym.hpp"
#include "tronconneuse.hpp"

#ifdef LIBTHREADAR_AVAILABLE
#include "parallel_tronconneuse.hpp"
#endif

using namespace std;

namespace libdar
{


    tronco_with_elastic::tronco_with_elastic(const shared_ptr<user_interaction> & dialog,
					     U_I workers,
					     U_32 block_size,
					     generic_file & encrypted_side,
					     const secu_string & password,
					     const archive_version & reading_ver,
					     crypto_algo algo,
					     const string & salt,
					     const infinint & iteration_count,
					     hash_algo kdf_hash,
					     bool use_pkcs5,
					     bool info_details):
	generic_file(encrypted_side.get_mode() == gf_read_only ? gf_read_only: gf_write_only),
	mem_ui(dialog),
	beh(nullptr),
	status(init),
	encrypted(&encrypted_side),
	reading_version(reading_ver),
	info(info_details)
    {
	if(algo == crypto_algo::scrambling)
	{
	    behind_weak.reset(new (nothrow) scrambler(password, *encrypted));
	    if(behind_weak == nullptr)
		throw Ememory("tronco_with_elastic::tronc_with_elastic");

	    beh = behind_weak.get();
	}
	else
	{
	    unique_ptr<crypto_module> ptr;

	    if(encrypted == nullptr)
		throw SRC_BUG;

	    ptr.reset(new (nothrow) crypto_sym(password,
					       reading_version,
					       algo,
					       salt,
					       iteration_count,
					       kdf_hash,
					       use_pkcs5));
	    if(! ptr)
		throw Ememory("tronco_with_elastic::tronc_with_elastic");
	    else
	    {
		crypto_sym *csym = dynamic_cast<crypto_sym*>(ptr.get());
		if(csym == nullptr)
		    throw SRC_BUG; // for now only a crypto_sym is available as class inherited from crypto_module
		sel = csym->get_salt();
	    }

	    switch(workers)
	    {
	    case 0:
		throw SRC_BUG;
	    case 1:
		behind.reset(new (nothrow) tronconneuse(block_size,
							*encrypted,
							reading_version,
							ptr));
		if(info)
		    get_ui().message(tools_printf(gettext("single-threaded cyphering layer open")));
		break;
	    default:
#if LIBTHREADAR_AVAILABLE
		behind.reset(new (nothrow) parallel_tronconneuse(workers,
								 block_size,
								 *encrypted,
								 reading_version,
								 ptr));
		if(info)
		    get_ui().message(tools_printf(gettext("multi-threaded cyphering layer open, with %d worker thread(s)"), workers));
		break;
#else
		throw Ecompilation(gettext("libthreadar is required at compilation time in order to use more than one thread for cryptography"));
#endif
	    }

	    if(! behind)
		throw Ememory("tronco_with_elastic::tronc_with_elastic");

	    beh = behind.get();
	}

	if(beh == nullptr)
	    throw SRC_BUG;
    }

    void tronco_with_elastic::get_ready_for_writing(const infinint & initial_shift)
    {
	if(status != init)
	    throw SRC_BUG;

	if(! behind_weak)
	{
	    if(! behind)
		throw SRC_BUG;

	    behind->set_initial_shift(initial_shift);
	}

	if(info)
	    get_ui().message(gettext("Writing down the initial elastic buffer through the encryption layer..."));

	if(beh == nullptr)
	    throw SRC_BUG;

	add_elastic_buffer(*beh, GLOBAL_ELASTIC_BUFFER_SIZE, 0, 0);

	status = writing;
    }

    void tronco_with_elastic::get_ready_for_reading(const infinint & initial_shift)
    {
	if(status != init)
	    throw SRC_BUG;

	if(! behind_weak)
	{
	    if(! behind)
		throw SRC_BUG;

	    behind->set_initial_shift(initial_shift);
	}
	    // else nothing to do, scrambling not concerned by initial_shift

	status = reading;
    }

    void tronco_with_elastic::get_ready_for_reading(trailing_clear_data_callback callback,
						    bool skip_after_initial_elastic)
    {
	if(status != init)
	    throw SRC_BUG;

	if(! behind_weak)
	{

	    if(! behind)
		throw SRC_BUG;

	    if(encrypted == nullptr)
		throw SRC_BUG;

	    behind->set_callback_trailing_clear_data(callback);
	    behind->set_initial_shift(encrypted->get_position());

	    if(skip_after_initial_elastic)
		elastic(*behind, elastic_forward, reading_version);
		// this is line creates a temporary anonymous object and destroys it just afterward
		// this is necessary to skip the reading of the initial elastic buffer
		// nothing prevents the elastic buffer from carrying what could
		// be considered an escape mark.
	}
	    // there is no initial elastic_buffer to read when scrambling is used

	status = reading;
    }


    void tronco_with_elastic::write_end_of_file()
    {
	    // obtaining the crypto block size (for clear data)

	U_32 block_size = 0;

	    // this als handles the context where behind_weak is true (scrambling)
	switch(status)
	{
	case init:
	    break;  // we will write the final elastic buffer
	case reading:
	    throw SRC_BUG; // only supported in writing mode
	case writing:
	    break;   // we will write the final elastic buffer
	case closed:
	    return; // final elastic buffer already written, nothing more to do
	default:
	    throw SRC_BUG;
	}

	if(! behind_weak)
	{
	    if(! behind)
		throw SRC_BUG;
	    block_size = behind->get_clear_block_size();
	}
	else
	    block_size = 0;


		// calculating the current offset modulo block_size

	U_32 offset = 0;

	if(encrypted == nullptr)
	    throw SRC_BUG;

	if(block_size > 0)
	{
	    infinint times = 0;
	    infinint reste = 0;

	    euclide(encrypted->get_position(), infinint(block_size), times, reste);
	    reste.unstack(offset);
	    if(! reste.is_zero())
		throw SRC_BUG;
	}

	if(info)
	    get_ui().message(gettext("writing down the final elastic buffer through the encryption layer..."));

	    // a final elastic_buffer is added also when scrambling is used (with offset = 0 and block_size = 0)
	    // for historical reasons,

	if(beh == nullptr)
	    throw SRC_BUG;

	add_elastic_buffer(*beh,
			   GLOBAL_ELASTIC_BUFFER_SIZE,
			   block_size,
			   offset);

	    // terminal elastic buffer (after terminateur to protect against
	    // plain text attack on the terminator string)

	if(behind_weak)
	    behind_weak->sync_write();
	else
	{
	    if(behind)
		behind->sync_write();
	    else
		throw SRC_BUG;
	}

	status = closed;
    }

    bool tronco_with_elastic::skippable(skippability direction, const infinint & amount)
    {
	if(status != reading && status != writing)
	    return false;

	if(beh == nullptr)
	    throw SRC_BUG;
	else
	    return beh->skippable(direction, amount);
    }

    bool tronco_with_elastic::skip(const infinint & pos)
    {
	if(status != reading && status != writing)
	    return false;

	if(beh == nullptr)
	    throw SRC_BUG;

	return beh->skip(pos);
    }

    bool tronco_with_elastic::skip_to_eof()
    {
	if(status != reading && status != writing)
	    return false;

	if(beh == nullptr)
	    throw SRC_BUG;

	return beh->skip_to_eof();
    }


    bool tronco_with_elastic::skip_relative(S_I x)
    {
	if(status != reading && status != writing)
	    return false;

	if(beh == nullptr)
	    throw SRC_BUG;

	return beh->skip_relative(x);
    }

    bool tronco_with_elastic::truncatable(const infinint & pos) const
    {
	if(status != reading && status != writing)
	    return false;

	if(beh == nullptr)
	    throw SRC_BUG;

	return beh->truncatable(pos);
    }


    infinint tronco_with_elastic::get_position() const
    {
	if(status != reading && status != writing)
	    return false;

	if(beh == nullptr)
	    throw SRC_BUG;

	return beh->get_position();
    }


    void tronco_with_elastic::inherited_read_ahead(const infinint & amount)
    {
	if(status != reading)
	    throw SRC_BUG;

	if(beh == nullptr)
	    throw SRC_BUG;

	beh->read_ahead(amount);
    }

    U_I tronco_with_elastic::inherited_read(char *a, U_I size)
    {
	if(status != reading)
	    throw SRC_BUG;

	if(beh == nullptr)
	    throw SRC_BUG;

	return beh->read(a, size);
    }


    void tronco_with_elastic::inherited_write(const char *a, U_I size)
    {
	if(status != writing)
	    throw SRC_BUG;

	if(beh == nullptr)
	    throw SRC_BUG;

	beh->write(a, size);
    }


    void tronco_with_elastic::inherited_truncate(const infinint & pos)
    {
	if(status != writing)
	    throw SRC_BUG;

	if(beh == nullptr)
	    throw SRC_BUG;

	beh->truncate(pos);
    }


    void tronco_with_elastic::inherited_sync_write()
    {
	if(status != writing)
	    throw SRC_BUG;

	if(beh == nullptr)
	    throw SRC_BUG;

	beh->sync_write();
    }


    void tronco_with_elastic::inherited_flush_read()
    {
	if(status != reading)
	    throw SRC_BUG;

	if(beh == nullptr)
	    throw SRC_BUG;

	beh->flush_read();
    }

    void tronco_with_elastic::inherited_terminate()
    {
	switch(status)
	{
	case init:
	    break; // nothing to do at this level
	case reading:
	    break; // nothing to do at this level
	case writing:
	    write_end_of_file();
	    break;
	case closed:
	    break; // nothing to do at this level
	default:
	    throw SRC_BUG;
	}

	if(beh == nullptr)
	    throw SRC_BUG;

	beh->terminate();
	status = closed;
    }


    void tronco_with_elastic::add_elastic_buffer(generic_file & f,
						 U_32 max_size,
						 U_32 modulo,
						 U_32 offset)
    {
	U_32 size = tools_pseudo_random(max_size-1) + 1; // range from 1 to max_size;

	if(modulo > 0)
	{
	    U_32 shift = modulo - (offset % modulo);
	    size = (size/modulo)*modulo + shift;
	}

        elastic tic = size;
        char *buffer = new (nothrow) char[tic.get_size()];

        if(buffer == nullptr)
            throw Ememory("tools_add_elastic_buffer");
        try
        {
            tic.dump((unsigned char *)buffer, tic.get_size());
            f.write(buffer, tic.get_size());
        }
        catch(...)
        {
            delete [] buffer;
            throw;
        }
        delete [] buffer;
    }



} // end of namespace
