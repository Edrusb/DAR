#!/bin/bash

MT=$(sed '/^MemTotal/!d;s/.*  //' /proc/meminfo)
echo -e "\n\tyou have $MT total memory"

ST=$(sed '/^SwapTotal/!d;s/.*  //' /proc/meminfo)
echo -e "\n\tyou have $ST total swap"

P=$(mount | sed '/^none/d' | awk '{print $3}')

for p in $P
do
	fc=$(find $p -xdev \
		-path '/tmp' -prune -o \
		-path '/var/tmp' -prune -o \
		-print |
		wc -l)
	echo -e "\n\tpartition \"$p\" contains $fc files"
	(( iioh = ($fc * 1300)/1024 ))
	echo -e "\tdar differential backup with infinint requires $iioh kB memory"
done

echo

# /proc and /sys (and /dev if it's udev) are excluded by "-xdev"
