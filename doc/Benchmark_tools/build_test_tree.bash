#!/bin/bash

if [ -z "$1" ] ; then
    echo "usage: $0 <directory>"
    exit 1
fi

if [ -e "$1" ] ; then
    echo "$1 already exists, remove it or use another directory name"
    exit 1
fi

if ! dar -V > /dev/null ; then
    echo "need dar to copy unix socket to the test tree"
    exit 1
fi

mkdir "$1"
cd "$1"

# creating
mkdir "SUB"
dd if=/dev/zero of=plain_zeroed bs=1024 count=1024
dd if=/dev/urandom of=random bs=1024 count=1024
dd if=/dev/zero of=sparse_file bs=1 count=1 seek=10239999
ln -s random SUB/symlink-broken
ln -s ../random SUB/symlink-valid
mkfifo pipe
mknod null c 3 1
mknod fd1 b 2 1
dar -c - -R / -g dev/log -N -Q -q | dar -x - --sequential-read -N -q -Q
ln sparse_file SUB/hard_linked_sparse_file
ln dev/log SUB/hard_linked_socket
ln pipe SUB/hard_linked_pipe

# modifying dates and permissions
sleep 2
chown nobody random
chown -h bin SUB/symlink-valid
chgrp -h daemon SUB/symlink-valid
sleep 2
echo hello >> random
sleep 2
cat < random > /dev/null

# adding Extend Attributes, assuming the filesystem as user_xattr and acl option set
setfacl -m u:nobody:rwx plain_zeroed && setfattr -n "user.hello" -v "hello world!!!" plain_zeroed || (echo "FAILED TO CREATE EXTENDED ATTRIBUTES" && exit 1)

# adding filesystem specific attributes
chattr +dis plain_zeroed



