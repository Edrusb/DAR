/*********************************************************************
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
*********************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define TMP ".###tmp_file"
#define MAX 10240
char c='\n';

int main(int argc,char *argv[])
{
    char buffer[MAX];
    char sortie[2*MAX];
    char *file;
    int p;
    register int i,lu,d;

    if(argc < 2)
    {
        printf("usage : %s liste de fichiers\n",argv[0]);
        return 1;
    }
    close(0);close(1);
    for(p=1;p<argc;)
    {
        file = argv[p++];

        if(open(file,O_RDONLY) != 0)
        {
            fprintf(stderr,"%s : fichier %s inaccessible\n",argv[0],file);
            return 2;
        }
        if(open(TMP,O_WRONLY|O_CREAT,0600) != 1)
        {
            fprintf(stderr,"%s : ouverture du fichier temporaire %s impossible\n",argv[0],TMP);
            return 3;
        }

        for(;;)
        {
            lu=read(0,buffer,MAX);
            if(lu <= 0)
                break;
            for(i=0,d=0;i<lu;i++)
                if(buffer[i] == c)
                {
                    sortie[i+d]='\r';
                    d++;
                    sortie[i+d]=buffer[i];
                }
                else
                    sortie[i+d]=buffer[i];
            write(1,sortie,lu+d);
        }
        close(1);close(0);
	if(unlink(file) == 0)
	{
	    if(rename(TMP, file) != 0)
	    {
		fprintf(stderr, "%s : failed moving tmp file to %s: %s",
			argv[0],
			file,
			strerror(errno));
		return 4;
	    }
	}
	else
	{
	    fprintf(stderr, "%s : failed deleting %s to remplace it by updated content: %s",
		    argv[0],
		    file,
		    strerror(errno));
	    return 4;
	}
    }
    return 0;
}

