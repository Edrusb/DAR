#!/bin/sh

#######################################################################
# dar - disk archive - a backup/restoration program
# Copyright (C) 2002-2024 Denis Corbin
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

if [ $# -ne 2 ] ; then
  echo "usage: $0 <dir> <discriminant>"
  exit 1
fi

DIRNAME=`pwd`/`dirname $0`

COMP_FILE="$DIRNAME/compressible_file"
UNCOMP_FILE="$DIRNAME/uncompressible_file"

if [ ! -f $COMP_FILE ] ; then
    echo "missing file: $COMP_FILE"
    exit 2
fi



SUB1=S"$2"B1
SUB2=S"$2"B2
SUB3=S"$2"B3

mkdir "$1"
cd "$1"

echo "this is the content of a plain file" > plain_file.txt
if [ -x `which chattr` ] ; then
   chattr +cdS plain_file.txt
fi
setfattr -n user.coucou -v hello plain_file.txt

mkdir "$SUB1"
setfattr -n user.cuicui -v hellu "$SUB1"
cd "$SUB1"
ln ../plain_file.txt hard_linked_inode.txt
cd ..

mkdir "$SUB2"
cd "$SUB2"
ln -s "../$SUB1"/hard_linked_inode.txt symlink.txt
ln symlink.txt hard_to_symlink.txt
dd bs=4096 seek=10 count=1 if=/dev/zero of=sparse.txt 1> /dev/null 2> /dev/null
echo "some chars in the middle of holes" >> sparse.txt
dd bs=4096 conv=notrunc if=/dev/zero  count=10 >> sparse.txt 2> /dev/null
cp sparse.txt sparse2.txt
mkfifo tube1
cd ..

mkdir "$SUB3"
cd "$SUB3"
ln ../plain_file.txt
mkdir T
cd T
touch "another plain file" > titi.txt
cd ..
cd ..

ln "$SUB2/sparse2.txt" "hard_linked_sparse.txt"
mkfifo tube2

cd "$SUB1"
ln ../tube2 hard_linked_pipe
mknod chardev c 1 1
setfacl -m u:bin:rw- chardev
mknod blockdev b 1 1
ln chardev chardev_hard
ln blockdev blockdev_hard
echo "another simple plain file" > plain_file2.txt
chgrp root plain_file2.txt
# need a bigger file to test compression, and a easy one but non-sparse to compress
cp "$COMP_FILE" .
# need an uncompressible file to test skip back upon poor compression
cp "$UNCOMP_FILE" .

# file used to test binary delta
cp "$UNCOMP_FILE" for_binary_delta
