#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifndef GCRYPT_NO_DEPRECATED
#define GCRYPT_NO_DEPRECATED
#endif
#include <gcrypt.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define EXIT_OK 0
#define EXIT_SYNTAX 1
#define EXIT_LIBGCRYPT 2
#define EXIT_BUG 3
#define EXIT_SYSTEM 4

#define BUFSIZE 102400

#define MIN_VERSION_GCRYPT "1.4.0"

void usage(const char *cmd);
int init_libgcrypt();
int get_hash_algo(const char *algo);
int set_hash_handle(int algo, gcry_md_hd_t *hash_handle);
void feed_hash_from(int fd, gcry_md_hd_t *hash_handle);
void show_hash(int algo, gcry_md_hd_t *hash_handle, const char *filename);
void release_hash(gcry_md_hd_t *hash_handle);


int main(int argc, char *argv[])
{
    int fd = -1;

    if(argc != 3)
    {
	usage(argv[0]);
	return EXIT_SYNTAX;
    }

    if(!init_libgcrypt())
	return EXIT_LIBGCRYPT;

    fd = open(argv[1], O_RDONLY);

    if(fd < 0)
	return EXIT_SYSTEM;
    else
    {
	int algo = get_hash_algo(argv[2]);
	gcry_md_hd_t hash_handle;

	if(algo != 0)
	{
	    if(set_hash_handle(algo, &hash_handle))
	    {
		feed_hash_from(fd, &hash_handle);
		close(fd);
		show_hash(algo, &hash_handle, argv[1]);
		release_hash(&hash_handle);

		return EXIT_OK;
	    }
	    else
	    {
		close(fd);
		return EXIT_LIBGCRYPT;
	    }
	}
	else
	{
	    close(fd);
	    return EXIT_SYNTAX;
	}
    }
}

void usage(const char *cmd)
{
    printf("usage: %s <filename> {sha1|md5}\n", cmd);
}

int init_libgcrypt()
{
    if(!gcry_control(GCRYCTL_INITIALIZATION_FINISHED_P))
    {
	gcry_error_t err;

	    // no multi-thread support activated for gcrypt
	    // this must be done from the application as stated
	    // by the libgcrypt documentation

	if(!gcry_check_version(MIN_VERSION_GCRYPT))
	{
	    fprintf(stderr, "Too old version for libgcrypt, minimum required version is %s\n", MIN_VERSION_GCRYPT);
	    return 0;
	}

	    // initializing default sized secured memory for libgcrypt
	(void)gcry_control(GCRYCTL_INIT_SECMEM, 65536);
	    // if secured memory could not be allocated, further request of secured memory will fail
	    // and a warning will show at that time (we cannot send a warning (just failure notice) at that time).

	err = gcry_control(GCRYCTL_ENABLE_M_GUARD);
	if(err != GPG_ERR_NO_ERROR)
	{
	    fprintf(stderr, "Error while activating libgcrypt's memory guard: %s/%s",
		    gcry_strsource(err),
		    gcry_strerror(err));
	    return 0;
	}

	err = gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);
	if(err != GPG_ERR_NO_ERROR)
	{
	    fprintf(stderr,
		    "Error while telling libgcrypt that initialization is finished: %s/%s",
		    gcry_strsource(err),
		    gcry_strerror(err));
	    return 0;
	}
    }
    else
	if(!gcry_check_version(MIN_VERSION_GCRYPT))
	{
	    fprintf(stderr, "Too old version for libgcrypt, minimum required version is %s\n",
		    MIN_VERSION_GCRYPT);
	    return 0;
	}

    return 1;
}

int get_hash_algo(const char *algo)
{
    if(strcmp("md5", algo) == 0)
	return GCRY_MD_MD5;
    else if(strcmp("sha1", algo) == 0)
	return GCRY_MD_SHA1;
    else
    {
	fprintf(stderr, "Unknown algorithm: %s\n", algo);
	return 0;
    }
}

int set_hash_handle(int algo, gcry_md_hd_t *hash_handle)
{
    gcry_error_t err;

    err = gcry_md_test_algo(algo);
    if(err != GPG_ERR_NO_ERROR)
    {
	fprintf(stderr, "Error while initializing hash: Hash algorithm not available in libgcrypt: %s/%s",
		gcry_strsource(err),
		gcry_strerror(err));
	return 0;
    }

    err = gcry_md_open(hash_handle, algo, 0);
    if(err != GPG_ERR_NO_ERROR)
    {
	fprintf(stderr, "Error while creating hash handle: %s/%s",
		gcry_strsource(err),
		gcry_strerror(err));
	return 0;
    }
    else
	return 1;
}

void feed_hash_from(int fd, gcry_md_hd_t *hash_handle)
{
    char buffer[BUFSIZE];

    int lu;

    do
    {
	lu = read(fd, buffer, BUFSIZE);
	if(lu > 0)
	    gcry_md_write(*hash_handle, (const void *)buffer, lu);
    }
    while(lu > 0 || (lu == -1 && (errno == EINTR)));
}

void show_hash(int algo, gcry_md_hd_t *hash_handle, const char *filename)
{
    const unsigned char *digest = gcry_md_read(*hash_handle, algo);
    const unsigned int digest_size = gcry_md_get_algo_dlen(algo);
    unsigned int i;

    if(digest == NULL)
    {
	fprintf(stderr, "Failed to obtain digest from libgcrypt\n");
	return;
    }

    for(i = 0; i < digest_size; ++i)
	printf("%02x", digest[i]);
    printf("  %s\n", filename);
}

void release_hash(gcry_md_hd_t *hash_handle)
{
    gcry_md_close(*hash_handle);
}

void convert_to_hexa(const char *digest, const unsigned int digest_size, char *hexa, const unsigned int hexa_size)
{





}
