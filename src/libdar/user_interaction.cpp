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
// to contact the author : dar.linux@free.fr
/*********************************************************************/
// $Id: user_interaction.cpp,v 1.9 2003/10/18 14:43:07 edrusb Rel $
//
/*********************************************************************/

#include "../my_config.h"

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

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#if HAVE_ERRNO_H
#include <errno.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if STDC_HEADERS
#include <stdarg.h>
#endif


#include <iostream>
#include "user_interaction.hpp"
#include "erreurs.hpp"
#include "tools.hpp"
#include "integers.hpp"
#include "deci.hpp"

using namespace std;

namespace libdar
{

    static void (*warning_callback)(const string & x) = NULL;
    static bool (*answer_callback)(const string & x) = NULL;

    void set_warning_callback(void (*callback)(const string & x))
    {
        warning_callback = callback;
    }

    void set_answer_callback(bool (*callback)(const string &x))
    {
        answer_callback = callback;
    }

    void user_interaction_pause(const string & message)
    {
        if(answer_callback == NULL)
            cerr << "answer_callback not set, use set_answer_callback() first" << endl;
        else
            if(! (*answer_callback)(message))
                throw Euser_abort(message);
    }

    void user_interaction_warning(const string & message)
    {
        if(warning_callback == NULL)
            cerr << "warning_callback not set, use set_warning_callback first" << endl;
        else
            (*warning_callback)(message + '\n');
    }

    static void dummy_call(char *x)
    {
        static char id[]="$Id: user_interaction.cpp,v 1.9 2003/10/18 14:43:07 edrusb Rel $";
        dummy_call(id);
    }

    void ui_printf(const char *format, ...)
    {
        va_list ap;
        bool end;
        U_32 taille = strlen(format)+1;
        char *copie;
        string output = "";

        U_I test;

        copie = new char[taille];
        if(copie == NULL)
            throw Ememory("ui_printf");

        va_start(ap, format);
        try
        {
            char *ptr = copie, *start = copie;

            strcpy(copie, format);
            copie[taille-1] = '\0';

            do
            {
                while(*ptr != '%' && *ptr != '\0')
                    ptr++;
                if(*ptr == '%')
                {
                    *ptr = '\0';
                    end = false;
                }
                else
                    end = true;
                output += start;
                if(!end)
                {
                    ptr++;
                    switch(*ptr)
                    {
                    case '%':
                        output += "%";
                        break;
                    case 'd':
                        output += tools_int2str(va_arg(ap, S_I));
                        break;
                    case 'u':
                        test = va_arg(ap, U_I);
                        output += deci(test).human();
                        break;
                    case 's':
                        output += va_arg(ap, char *);
                        break;
                    case 'c':
                        output += static_cast<char>(va_arg(ap, S_I));
                        break;
                    case 'i':
                        output += deci(*(va_arg(ap, infinint *))).human();
                        break;
                    case 'S':
                        output += *(va_arg(ap, string *));
                        break;
                    default:
                        throw Efeature(string("%") + (*ptr) + " is not implemented in ui_printf format argument");
                    }
                    ptr++;
                    start = ptr;
                }
            }
            while(!end);
        }
        catch(...)
        {
            va_end(ap);
            delete copie;
            throw;
        }
        delete copie;
        va_end(ap);

        if(warning_callback == NULL)
            cerr << "warning_callback not set, use set_warning_callback first" << endl;
        else
            (*warning_callback)(output);
    }

} // end of namespace
