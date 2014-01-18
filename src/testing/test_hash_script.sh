#!/bin/sh

clean()
{
   rm -f src.txt dst.txt src.txt.cmd.sha1 src.txt.sha1
}

routine()
{
   ./make_sparse_file src.txt "$1"
#   dd if=/dev/zero of=src.txt bs=1024 count="$1"
   ./test_hash_fichier src.txt dst.txt sha1
   sha1sum src.txt > src.txt.cmd.sha1
   if diff src.txt.sha1 src.txt.cmd.sha1 ; then
       return 0
   else
       return 1
   fi
   clean
}

if [ -z "$1" ]; then
    echo "usage: $0 <max file size in byte>"
    exit 1
fi

clean

dichotomy()
{
local max=$1
local min=1
local val=$max

until [ $max -le $(( $min + 1 )) ] ; do
    if routine $val ; then
	min=$val
    else
	max=$val
    fi

    val=$(( ($min + $max)/2 ))
    echo "min=$min val=$val max=$max"
done

echo "min = $min \n max = $max (KiB)"
}

linear()
{
local max=$1
local val=10

until [ $val -gt $max ] ; do
    if routine $val ; then
	echo "success at $val"
    else
	echo "FAILURE at $val"
    fi

    val=$(( $val + 1073741824 ))
done

}

# dichotomy $1
linear $1
