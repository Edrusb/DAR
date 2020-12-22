//*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2020 Denis Corbin
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

#include "../my_config.h"

extern "C"
{
} // end extern "C"

#include "mycurl_param_element.hpp"
#include "libdar.hpp"

using namespace libdar;
using namespace std;

class foo
{
public:
    foo(U_I arg): val(arg) { cout << "foo(constructor)" << endl; }
    foo(const foo & arg): val(arg.val) { cout << "foo(copy constructor)" << endl; }
    foo(foo && arg) noexcept: val(arg.val) { cout << "foo(move constructor)" << endl; }
    foo & operator = (const foo & arg) { val = arg.val; cout << "foo(copy assignment)" << endl; return *this; }
    foo & operator = (foo && arg) noexcept { val = arg.val; cout << "foo(move asignment)" << endl; return *this; }
    ~foo() { cout << "foo(destructor)" << endl; }

    operator U_I() const { return val; };

private:
    U_I val;
};


void f1(int argc, char *argv[]);
void f2();

int main(int argc, char *argv[])
{
    U_I maj, med, min;

    try
    {
	get_version(maj, med, min);
	f1(argc, argv);
	f2();
    }
    catch(Egeneric & e)
    {
	cout << "Execption caught: " << e.get_message() << endl;
    }
}

void f1(int argc, char *argv[])
{
    mycurl_param_list mylist;
    string type1 = "hello world";
    S_I type2 = 50;
    void const* type3 = (void const*)(2);
    CURLoption opt;

    mylist.add(CURLOPT_VERBOSE, type1);
    mylist.add(CURLOPT_HEADER, type2);
    mylist.add(CURLOPT_NOPROGRESS, type3);
    mylist.add(CURLOPT_NOSIGNAL, (float)5.5);
    mylist.add(CURLOPT_PROXY, foo(3));

    mylist.reset_read();
    const string* type1_ptr;
    const S_I* type2_ptr;
    const void* const* type3_ptr;
    const float* type4_ptr;
    const foo* type5_ptr;

    while(mylist.read_next(opt))
    {
	switch(opt)
	{
	case CURLOPT_VERBOSE:
	    mylist.read_opt(type1_ptr);
	    cout << "val(CURLOPT_VERBOSE) = " << (*type1_ptr) << endl;
	    break;
	case CURLOPT_HEADER:
	    mylist.read_opt(type2_ptr);
	    cout << "val(CURLOPT_HEADER) = " << (*type2_ptr) << endl;
	    break;
	case CURLOPT_NOPROGRESS:
	    mylist.read_opt(type3_ptr);
	    cout << "val(CURLOPT_NOPROGRESS) = " << U_I(*type3_ptr) << endl;
	    break;
	case CURLOPT_NOSIGNAL:
	    mylist.read_opt(type4_ptr);
	    cout << "val(CURLOPT_NOSIGNAL) = " << (*type4_ptr) << endl;
	    break;
	case CURLOPT_PROXY:
	    mylist.read_opt(type5_ptr);
	    cout << "val(CURLOPT_PROXY) = " << U_I(*type5_ptr) << endl;
	    break;
	default:
	    cout << "what's that!?" << endl;
	}
    }

    type1_ptr = nullptr;

    if(mylist.get_val(CURLOPT_VERBOSE, type1_ptr))
	cout << "val(VERBOSE) = " << *type1_ptr << endl;
    mylist.clear(CURLOPT_VERBOSE);
    if(mylist.get_val(CURLOPT_VERBOSE, type1_ptr))
	cout << "val(VERBOSE) = " << *type1_ptr << endl;
    if(mylist.get_val(CURLOPT_HEADER, type2_ptr))
	cout << "val(HEADER) = " << *type2_ptr << endl;
    cout << "size = " << mylist.size() << endl;
    mylist.reset();
    cout << "size = " << mylist.size() << endl;
    if(mylist.get_val(CURLOPT_HEADER, type2_ptr))
	cout << "val(HEADER) = " << *type2_ptr << endl;
}

void f2()
{
    mycurl_param_list current;
    mycurl_param_list wanted;
    const foo* ptr;
    CURLoption opt;
    list<CURLoption> modified;
    list<CURLoption>::iterator modit;

    current.add(CURLOPT_VERBOSE, foo(1));
    current.add(CURLOPT_HEADER, foo(2));

    wanted.add(CURLOPT_VERBOSE, foo(3));
    wanted.add(CURLOPT_NOPROGRESS, foo(4));

    modified = current.update_with(wanted);

    current.reset_read();

    while(current.read_next(opt))
    {
	current.read_opt(ptr);
	cout << opt << " -> " << U_I(*ptr) << endl;
    }

    modit = modified.begin();
    while(modit != modified.end())
    {
	cout << "modified = " << *modit << endl;
	++modit;
    }
}
