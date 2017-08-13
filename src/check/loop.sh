#!/bin/sh

for archive in Old_format/* ; do
  basename=`echo "$archive" | sed -r -e "s/\.[0-9]+\.dar//"`
  echo "Testing ascendant compatibility with $basename"
  if ! ../dar_suite/dar -q -Q -t "$basename" ; then
     echo "FAILED TESTING OLD ARCHIVE FORMAT: $basename"
     exit 1
  fi
done

echo "exit" | expect || exit 1

TMP_FILE=my_tmpfile
MY_MAKEFILE=my_makefile

rm -f $TMP_FILE $MY_MAKEFILE

printf "Building the Makefile... "

# for multi_thread in n y ; do
multi_thread="n"
  for hash in  md5 none sha1 ; do
    for crypto in bf none scram aes twofish serpent camellia ; do
      for zip in xz gzip none bzip2 lzo; do
        for slice in 1k none ; do
          for Slice in  500 none ; do
            for tape in y n ; do
              for seq_read in y n ; do
                for digit in 3 none ; do
                  for sparse_size in 100 0 ; do
                    for keep_compr in y n ; do
                      for recheck_hole in y n ; do
			for asym in y n ; do
TARGET=target_$multi_thread$hash$crypto$zip$slice$Slice$tape$seq_read$digit$sparse_size$keep_compr$recheck_hole$asym
cat >> $MY_MAKEFILE <<EOF
$TARGET:
	./main.sh "$TARGET" "$multi_thread" "$hash" "$crypto" "$zip" "$slice" "$Slice" "$tape" "$seq_read" "$digit" "$sparse_size" "$keep_compr" "$recheck_hole" "$asym"

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
# done

printf "all:" >> $MY_MAKEFILE
echo `cat $TMP_FILE` >> $MY_MAKEFILE
rm $TMP_FILE
echo "Done"
