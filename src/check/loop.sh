#!/bin/sh

#######################################################################
# dar - disk archive - a backup/restoration program
# Copyright (C) 2002-2021 Denis Corbin
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
# to contact the author : http://dar.linux.free.fr/email.html
#######################################################################

for archive in Old_format/* ; do
  basename=`echo "$archive" | sed -r -e "s/\.[0-9]+\.dar//"`
  echo "Testing ascendant compatibility with $basename"
  crypto=`echo "$basename.1.dar" | grep _crypto_| wc -l`
  if [ $crypto -gt 0 ] ; then
      KEY="-K bf:test"
  else
      KEY="-q"
  fi
  if ! ../dar_suite/dar -q -Q -t "$basename" $KEY ; then
     echo "FAILED TESTING OLD ARCHIVE FORMAT: $basename"
     exit 1
  fi
done

echo "exit" | expect || exit 1

TMP_FILE=my_tmpfile
MY_MAKEFILE=my_makefile

rm -f $TMP_FILE $MY_MAKEFILE

printf "Building the Makefile (patience, this takes some minutes)... "

  for multi_thread in 2 1 ; do
    for hash in md5 none sha1 ; do
      for crypto in bf none scram aes twofish serpent camellia ; do
        for zip in zstd lz4 xz gzip none bzip2 lzo ; do
	  for zipbs in 1234 0 ; do
            for slice in 1k none ; do
              for Slice in 500 none ; do
                for tape in y n ; do
                  for seq_read in y n ; do
                    for digit in 3 none ; do
                      for sparse_size in 100 0 ; do
                        for keep_compr in y n ; do
                          for recheck_hole in y n ; do
                            for asym in y n ; do
                              TARGET=target-$multi_thread-$hash-$crypto-$zip-$zipbs-$slice-$Slice-$tape-$seq_read-$digit-$sparse_size-$keep_compr-$recheck_hole-$asym
cat >> $MY_MAKEFILE <<EOF
$TARGET:
	./main.sh "$TARGET" "$multi_thread" "$hash" "$crypto" "$zip" "$zipbs" "$slice" "$Slice" "$tape" "$seq_read" "$digit" "$sparse_size" "$keep_compr" "$recheck_hole" "$asym"

EOF
cat >> $TMP_FILE <<EOF
 $TARGET
EOF
                            done
                          done
			done
                      done
                    done
                  done
                done
              done
            done
          done
        done
      done
    done
  done

printf "all:" >> $MY_MAKEFILE
echo `cat $TMP_FILE` >> $MY_MAKEFILE
rm $TMP_FILE
echo "Done"
