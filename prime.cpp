#include <iostream.h>
#include "infinint.hpp"
#include "deci.hpp"
#include "integers.hpp"

S_I main(S_I argc, char *argv[])
{
    if(argc != 2)
    {
	cout << "usage : " << argv[0] <<  " <number>" << endl;
	exit(1);
    }
    
    deci x = string(argv[1]);
    cout << "converting string to infinint... " << endl;
    infinint num = x.computer();
    cout << "checking if the number is a prime factor... " << endl;
    infinint max = (num / 2) + 1;
    infinint i = 2;
    while(i < max)
	if(num%i == 0)
	    break;
	else 
	    i++;

    if(i < max)
	cout << argv[1] << " is NOT prime" << endl;
    else
	cout << argv[1] << " is PRIME" << endl;
}

		
