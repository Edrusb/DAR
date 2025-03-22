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
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_TIME_H
#include <time.h>
#endif
}

#include "libdar.hpp"
#include "crypto_module.hpp"
#include "tronconneuse.hpp"
#include "parallel_tronconneuse.hpp"
#include "fichier_local.hpp"
#include "macro_tools.hpp"
#include "crypto_sym.hpp"
#include "tools.hpp"

using namespace libdar;
using namespace std;

class pseudo_crypto: public crypto_module
{
public:
	/// data is stored encrypted by inserting a '#' caracter
	/// every chunk (=10) bytes
	/// if the last chunk is uncompleted a % character is used instead and # is used for padding
	/// for simplicity we assume # not being present in the data to encrypt

    virtual U_32 encrypted_block_size_for(U_32 clear_block_size) override { return (clear_block_size / chunk)*(chunk+1) + (clear_block_size % chunk == 0 ? 0 : chunk+1) ; };

	// >>>>>> ATTENTION CAS PARTICULIER ICI, PREVOIR LE CAS OU ON UTILISE
	// >>>>>> DES LA MEMOIRE SUPPLEMENTAIRE DU BUFFER CLEAR (FAIRE COMME SI)
    virtual U_32 clear_block_allocated_size_for(U_32 clear_block_size) override { return clear_block_size; };

    virtual U_32 encrypt_data(const infinint & block_num,
			      const char *clear_buf,
			      const U_32 clear_size,
			      const U_32 clear_allocated,
			      char *crypt_buf,
			      U_32 crypt_size) override
    {
	U_32 lu = 0;
	U_32 cryoff = 0;
	U_32 remain;

	    // sanity checks

	if(clear_allocated < clear_size)
	    throw SRC_BUG;
	if(clear_allocated < clear_block_allocated_size_for(clear_size))
	    throw SRC_BUG;
	if(crypt_size < encrypted_block_size_for(clear_size))
	    throw SRC_BUG;

	while(lu < clear_size)
	{
	    remain = clear_size - lu;
	    if(chunk < remain)
		remain = chunk;
	    memcpy(crypt_buf + cryoff, clear_buf + lu, remain);
	    lu += remain;
	    cryoff += remain;

		// adding the padding now

	    if(remain < chunk) // end of buffer
	    {
		memcpy(crypt_buf + cryoff, padding, chunk - remain + 1);
		cryoff += chunk - remain + 1;
	    }
	    else // single byte # padding
	    {
		memcpy(crypt_buf + cryoff, "#", 1);
		++cryoff;
	    }
	}

	return cryoff;
    };

    virtual U_32 decrypt_data(const infinint & block_num,
			      const char *crypt_buf,
			      const U_32 crypt_size,
			      char *clear_buf,
			      U_32 clear_size) override
    {
	U_32 wrote = 0;
	U_32 cryoff = 0;
	U_32 remain;

	while(cryoff < crypt_size)
	{
	    remain = crypt_size - cryoff;
	    if(remain < chunk)
		throw SRC_BUG;
	    if(remain > chunk)
		remain = chunk;

	    switch(crypt_buf[cryoff + chunk]) // end of chunk to read
	    {
	    case '#':
		break;
	    case '%':
		for(U_32 i = 1; i < chunk && crypt_buf[cryoff + chunk - i] == '%'; ++i)
		    --remain;
		if(remain == 0)
		    throw SRC_BUG; // no data to copy
		break;
	    default:
		throw SRC_BUG; // data corruption
	    };
	    memcpy(clear_buf + wrote, crypt_buf + cryoff, remain);
	    wrote += remain;
	    if(wrote > clear_size)
		throw SRC_BUG;
	    cryoff += chunk + 1;
	}
	return wrote;
    };

    virtual unique_ptr<crypto_module> clone() const override { return unique_ptr<crypto_module>(new pseudo_crypto()); };

private:
    static constexpr U_32 chunk = 10;
    static constexpr const char *padding = "%%%%%%%%%%%";
};


void f1();
void f2(const char *src, const char *dst, bool encrypt, U_I num, const char *pass);


static shared_ptr<user_interaction>ui;

int main(int argc, char* argv[])
{
    U_I maj, med, min;

    get_version(maj, med, min);
    ui.reset(new (nothrow) shell_interaction(cout, cerr, false));
    if(!ui)
	cout << "ERREUR !" << endl;
    try
    {
	    // f1();

	if(argc < 6)
	    ui->message(tools_printf("usage: %s { -e | -d } <src> <dst> <num> <pass>", argv[0]));
	else
	{
	    if(strcmp(argv[1], "-d") != 0 && strcmp(argv[1], "-e") != 0)
		ui->message("use -d to decrypt or -e to encrypt");
	    else
		f2(argv[2], argv[3], strcmp(argv[1], "-e") == 0, atoi(argv[4]), argv[5]);
	}
    }
    catch(Ebug & e)
    {
	ui->message(e.dump_str());
    }
    catch(Egeneric & e)
    {
	ui->message(string("Aborting on exception: ") + e.get_message());
    }
    ui.reset();
}

#define CLEAR "toto.txt"
#define CRYPT "titi.txt"
#define BACK  "tutu.txt"

void f1()
{
    unique_ptr<fichier_local> src = make_unique<fichier_local>(CLEAR, false);
    unique_ptr<fichier_local> dst = make_unique<fichier_local>(ui, CRYPT, gf_write_only, 0644, false, true, false);
    const char *pass = "zero plus zero egale la tete a toto";
    secu_string secu_pass(pass, sizeof(pass));
    unique_ptr<crypto_module> ptr1;
    time_t temp;
    U_32 chain_size;
    const U_I bufsize = 200;
    char buf[bufsize];
    unique_ptr<generic_file> encry;
    unique_ptr<generic_file> decry;
    bool single_encr = false; // whether to use tronconneuse (true) of parallel_tronconneuse (false) for encryption
    bool single_decr = false;  // whether to use tronconneuse (true) of parallel_tronconneuse (false) for decryption
    bool simple = false;       // whether to use pseudo_crypto crypto_module (true) or crypto_sym crypto_module (false)

    if(simple)
    {
	ptr1 = make_unique<pseudo_crypto>();
	chain_size = 10240;
    }
    else
    {
	ptr1 = make_unique<crypto_sym>(secu_pass,
				       macro_tools_supported_version,
				       crypto_algo::aes256,
				       "sans sel c'est fade",
				       2000,
				       hash_algo::sha512,
				       true);
	chain_size = 10240;
    }

    if(!ptr1)
	throw Ememory("pseudo_crypto");
    unique_ptr<crypto_module> ptr2 = ptr1->clone();
    if(!ptr2)
	throw Ememory("pseudo_crypto");

    if(single_encr)
	encry = make_unique<tronconneuse>(chain_size,
					  *dst,
					  macro_tools_supported_version,
					  ptr1);
    else
	encry = make_unique<parallel_tronconneuse>(2, // workers
						   chain_size,
						   *dst,
						   macro_tools_supported_version,
						   ptr1);


    temp = time(NULL);
    cout << "ciphering... " << ctime(&temp) << endl;
    src->copy_to(*encry);
    src->terminate();
    encry->terminate();
    dst->terminate();
    src.reset();
    encry.reset();
    dst.reset();



    src = make_unique<fichier_local>(CRYPT, false);
    dst = make_unique<fichier_local>(ui, BACK, gf_write_only, 0644, false, true, false);

    if(!single_decr)
	decry = make_unique<parallel_tronconneuse>(2, // worker
						   chain_size,
						   *src,
						   macro_tools_supported_version,
						   ptr2);
    else
	decry = make_unique<tronconneuse>(chain_size,
					  *src,
					  macro_tools_supported_version,
					  ptr2);

    temp = time(NULL);
    cout << "unciphering..." << ctime(&temp) << endl;
    decry->copy_to(*dst);
    decry->skip(0);
    decry->read(buf, bufsize);
    decry->skip(1);
    decry->read(buf, bufsize);
    decry->skip_to_eof();
    infinint x = decry->get_position();

    decry->terminate();
    src->terminate();
    dst->terminate();
    decry.reset();
    src.reset();
    dst.reset();
    temp = time(NULL);
    cout << "finished! " << ctime(&temp) << endl;
}

void f2(const char *src, const char *dst, bool encrypt, U_I num, const char *pass)
{
    static const U_I block_size = 10240;

    unique_ptr<fichier_local> src_f = make_unique<fichier_local>(src, false);
    unique_ptr<fichier_local> dst_f = make_unique<fichier_local>(ui, dst, gf_write_only, 0644, true, false, false);
    unique_ptr<proto_tronco> tronco;
    unique_ptr<crypto_module> crypto = make_unique<crypto_sym>(secu_string(pass, strlen(pass)),
							       macro_tools_supported_version,
							       crypto_algo::aes256,
							       "sans sel c'est fade",
							       100,
							       hash_algo::sha512,
							       true);

    if(encrypt)
	if(num > 1)
	    tronco = make_unique<parallel_tronconneuse>(num,
							block_size,
							*dst_f,
							macro_tools_supported_version,
							crypto);
	else
	    tronco = make_unique<tronconneuse>(block_size,
					       *dst_f,
					       macro_tools_supported_version,
					       crypto);
    else
	if(num > 1)
	    tronco = make_unique<parallel_tronconneuse>(num,
							block_size,
							*src_f,
							macro_tools_supported_version,
							crypto);
	else
	    tronco = make_unique<tronconneuse>(block_size,
					       *src_f,
					       macro_tools_supported_version,
					       crypto);

    if(encrypt)
    {
	src_f->copy_to(*tronco);
	tronco->write_end_of_file();
	tronco->sync_write();
	src_f->terminate();
	tronco->terminate();
	dst_f->terminate();
    }
    else
    {
	tronco->copy_to(*dst_f);
	dst_f->sync_write();
	tronco->terminate();
	src_f->terminate();
	dst_f->terminate();
    }
}
