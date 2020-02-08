#include <stdio.h>

int main(int argc, char *argv[])
{
    int i;

    printf("Argument received from the parent process, one by line surrounded by double quotes:\n");
    for(i = 0; i < argc; ++i)
	printf("argument %3d: \"%s\"\n", i, argv[i]);
    printf("End of argument list\n");

    return 0;
}

