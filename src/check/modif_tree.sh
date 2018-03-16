#!/bin/sh

if [ $# -ne 2 ] ; then
  echo "usage: $0 <dir> <discriminant>"
  exit 1
fi

SUB1=S"$2"B
SUB2=S"$2"B2
SUB3=S"$2"B3

cd "$1"

cd "$SUB1"
ln ../plain_file.txt new_hard_linked_inode.txt
echo "new file alone" > plain2.txt
cd ..

cd "$SUB2"
rm -f symlink.txt
rm -f tube1
cd ..

rm -rf "$SUB3"

cd "$SUB1"
rm  blockdev
chgrp sys plain_file2.txt
