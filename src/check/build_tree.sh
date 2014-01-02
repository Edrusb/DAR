#!/bin/sh

if [ $# -ne 2 ] ; then
  echo "usage: $0 <dir> <discriminant>"
  exit 1
fi

SUB1=S"$2"B
SUB2=S"$2"B2
SUB3=S"$2"B3

mkdir "$1"
cd "$1"

echo "this is the content of a plain file" > plain_file.txt
if [ -x `which chattr` ] ; then
   chattr +cdS plain_file.txt
fi

mkdir "$SUB1"
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
mknod blockdev b 1 1
ln chardev chardev_hard
ln blockdev blockdev_hard
