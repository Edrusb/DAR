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

#include "../my_config.h"

extern "C"
{
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_STRINGS_H
#include <strings.h>
#endif

#if STDC_HEADERS
# include <string.h>
#else
# if !HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
char *strchr (), *strrchr ();
# if !HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

}

#include "libdar.hpp"
#include "tronconneuse.hpp"
#include "shell_interaction.hpp"
#include "deci.hpp"
#include "cygwin_adapt.hpp"
#include "macro_tools.hpp"
#include "fichier_local.hpp"

class test : public tronconneuse
{
public:
    test(user_interaction & dialog, U_32 block_size, generic_file & encrypted_size, bool no_is = false): tronconneuse(block_size, encrypted_size, no_is, macro_tools_supported_version)
    {};

protected:
    U_32 encrypted_block_size_for(U_32 clear_block_size) { return clear_block_size + 1; };
    U_32 clear_block_allocated_size_for(U_32 clear_block_size) { return clear_block_size + 2; };
    U_32 encrypt_data(const infinint & block_num, const char *clear_buf, const U_32 clear_size, const U_32 clear_allocated, char *crypt_buf, U_32 crypt_size);
    U_32 decrypt_data(const infinint & block_num, const char *crypt_buf, const U_32 crypt_size, char *clear_buf, U_32 clear_size);
};

U_32 test::encrypt_data(const infinint & block_num, const char *clear_buf, const U_32 clear_size, const U_32 clear_allocated, char *crypt_buf, U_32 crypt_size)
{
    if(crypt_size < clear_size + 1)
	throw SRC_BUG;
    memcpy(crypt_buf, clear_buf, clear_size);
    crypt_buf[clear_size] = '#';

    return clear_size + 1;
}

U_32 test::decrypt_data(const infinint & block_num, const char *crypt_buf, const U_32 crypt_size, char *clear_buf, U_32 clear_size)
{
    if(crypt_size > clear_size + 1)
	throw SRC_BUG;
    if(crypt_size == 0)
	return 0;
    memcpy(clear_buf, crypt_buf, crypt_size - 1);

    return crypt_size - 1;
}


void f1(user_interaction *dialog);
void f2(user_interaction *dialog);
void f3(user_interaction *dialog);

int main()
{
    U_I maj, med, min;
    user_interaction *dialog = NULL;

    get_version(maj, med, min);
    dialog = shell_interaction_init(&cout, &cerr, false);
    try
    {
	f1(dialog);
	f2(dialog);
	f3(dialog);
    }
    catch(Egeneric & e)
    {
	cout << "exception caught : " + e.get_message() << endl;
    }
    catch(...)
    {
	cout << "unknown exception caught" << endl;
    }
    shell_interaction_close();
    if(dialog != NULL)
	delete dialog;
}

void f1(user_interaction *dialog)
{
    int fd = open("toto", O_WRONLY|O_TRUNC|O_CREAT|O_BINARY, 0644);
    fichier_local fic = fichier_local(*dialog, fd);

    test *toto = new test(*dialog, 10, fic);
    if(toto == NULL)
	throw Ememory("test");

#define TEST_WRITE(x) toto->write(x, strlen(x))
#define WRITE_TO(x, y) x.write(y, strlen(y))

    TEST_WRITE("[|]");
    cout << string("pos = ") << toto->get_position() << endl;
    TEST_WRITE("bonjour les amis comment ca va ?");
    cout << string("pos = ") << toto->get_position() << endl;
    TEST_WRITE("a");
    cout << string("pos = ") << toto->get_position() << endl;

    toto->write_end_of_file();
    delete toto;
}

void f2(user_interaction *dialog)
{
    int fd = open("toto", O_RDONLY|O_BINARY);
    fichier_local fic = fichier_local(*dialog, fd);

    test *toto = new test(*dialog, 10, fic);
    if(toto == NULL)
	throw Ememory("test");

    const int taille = 100;
    char buffer[taille];

    buffer[toto->read(buffer, 5)] = '\0';
    cout << buffer << " | " << toto->get_position() << endl;

    buffer[toto->read(buffer, taille)] = '\0';
    cout << buffer << " | " << toto->get_position() << endl;

    toto->skip(5);
    cout << toto->get_position() << endl;
    buffer[toto->read(buffer, 5)] = '\0';
    cout << buffer << " | " << toto->get_position() << endl;

    toto->skip(11);
    cout << toto->get_position() << endl;
    buffer[toto->read(buffer, 5)] = '\0';
    cout << buffer << " | " << toto->get_position() << endl;

    toto->skip_to_eof();
    cout << toto->get_position() << endl;
    buffer[toto->read(buffer, 5)] = '\0';
    cout << buffer << " | " << toto->get_position() << endl;

    int i;

    for(i = 0; i < taille && toto->read_back(buffer[i]) == 1; ++i)
	cout << toto->get_position() << endl;

    buffer[i] = '\0';
    cout << buffer << " | " << toto->get_position() << endl;

    delete toto;
}


void f3(user_interaction *dialog)
{
    int fd = open("toto", O_WRONLY|O_TRUNC|O_CREAT|O_BINARY, 0666);
    fichier_local foc = fichier_local(*dialog, fd);

    fd = open("titi", O_WRONLY|O_TRUNC|O_CREAT|O_BINARY, 0666);
    fichier_local fic = fichier_local(*dialog, fd);

    WRITE_TO(foc, "Hello les amis");
    WRITE_TO(fic, "Hello les amis");

    cout << "pos = " << fic.get_position() << endl;
    cout << "pos = " << foc.get_position() << endl;

    test fuc = test(*dialog, 10, fic);
    cout << "pos = " << fuc.get_position() << endl;

    WRITE_TO(foc, "Il fait chaud il fait beau les mouches pettent et les cailloux fleurissent");
    WRITE_TO(fuc, "Il fait chaud il fait beau les mouches pettent et les cailloux fleurissent");

    cout << "pos = " << fuc.get_position() << endl;
    cout << "pos = " << foc.get_position() << endl;

    fuc.write_end_of_file();
    cout << "pos = " << fuc.get_position() << endl;
}
