#!/bin/sh

#######################################################################
# dar - disk archive - a backup/restoration program
# Copyright (C) 2002-2022 Denis Corbin
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
# to contact the author, see the AUTHOR file
#######################################################################

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
