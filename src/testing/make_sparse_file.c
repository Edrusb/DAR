#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

void error(const char *argv0);
int create_file(const char *filename, unsigned long int size_byte);

int main(int argc, char *argv[])
{
    if(argc != 3)
    {
	error(argv[0]);
	return 1;
    }
    else
	return create_file(argv[1], atol(argv[2]));
}

void error(const char *argv0)
{
    fprintf(stderr, "usage: %s <filename> <size in byte>\n", argv0);
}

int create_file(const char *filename, unsigned long int size_byte)
{
    int fd = open(filename, O_WRONLY|O_TRUNC|O_CREAT, 0644);

    if(fd < 0)
    {
	fprintf(stderr, "open() failed: %s\n", strerror(errno));
	return 2;
    }

    --size_byte;
    if(lseek(fd, size_byte, SEEK_SET) != size_byte)
    {
	fprintf(stderr, "lseek() failed: %s\n", strerror(errno));
	close(fd);
	return 2;
    }

    switch(write(fd, "\0", 1))
    {
    case -1:
	fprintf(stderr, "write() failed: %s\n", strerror(errno));
	close(fd);
	return 2;
    case 0:
	fprintf(stderr, "write() failed writing one byte, returning 0\n");
	close(fd);
	return 2;
    case 1:
	close(fd);
	return 0;
    default:
	fprintf(stderr, "write() returned unexpected value\n");
	close(fd);
	return 3;
    }
}

