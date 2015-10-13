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

#include <iostream>
#include <string>

#include "infinint.hpp"
#include "deci.hpp"
#include "integers.hpp"
#include "erreurs.hpp"
#include "tools.hpp"
#include "../dar_suite/shell_interaction.hpp"
#include "../dar_suite/dar_suite.hpp"
#include "../dar_suite/crit_action_cmd_line.hpp"
#include "../dar_suite/line_tools.hpp"

using namespace libdar;
using namespace std;

int little_main(shell_interaction & ui, S_I argc, char * const argv[], const char **env);

static infinint calculus(const string & formula); // recusive call

int main(S_I argc, char * const argv[], const char **env)
{
    return dar_suite_global(argc,
			    argv,
			    env,
			    "",
			    nullptr,
			    '\0',
			    &little_main);
}

int little_main(shell_interaction & ui, S_I argc, char * const argv[], const char **env)
{
    if(argc != 2)
    {
        cout << "usage: " << argv[0] <<  " \"arithmetical formula with only positive intergers, the following binary operators +, -, /, *, %, &, ^, | and parenthesis\"" << endl;
	return EXIT_SYNTAX;
    }
    else
    {
	string formula = crit_action_canonize_string(argv[1]);
	libdar::deci tmp = calculus(formula);

	cout << tmp.human() << endl;

	return EXIT_OK;
    }
}


static infinint calculus(const string & formula)
{
    string::const_iterator it;
    string s1, s2;
    const string operators = "+-/*%&^|";
    string::const_iterator op = operators.begin();

    while(op != operators.end() && (it = line_tools_find_last_char_out_of_parenth(formula, *op)) == formula.end())
	++op;

    if(it != formula.end())
    {
	if(formula.size() < 3)
	    throw Erange("calculus", tools_printf("Unknown meaning for string: %S", &formula));

	s1 = string(formula.begin(), it);
	s2 = string(it + 1, formula.end());
	switch(*op)
	{
	case '+':
	    return calculus(s1) + calculus(s2);
	case '-':
	    return calculus(s1) - calculus(s2);
	case '/':
	    return calculus(s1) / calculus(s2);
	case '*':
	    return calculus(s1) * calculus(s2);
	case '%':
	    return calculus(s1) % calculus(s2);
	case '&':
	    return calculus(s1) & calculus(s2);
	case '^':
	    return calculus(s1) ^ calculus(s2);
	case '|':
	    return calculus(s1) | calculus(s2);
	default:
	    throw SRC_BUG;
	}
    }

    if(*(formula.begin()) == '(' && *(formula.end() - 1) == ')')
	return calculus(string(formula.begin() + 1, formula.end() - 1));
    else  // assuming an integer
    {
	libdar::deci tmp = formula;
	return tmp.computer();
    }
}
