#!/bin/sh

if [ ! -x ../dar_suite/dar ] ; then
  echo "********************************************"
  echo ""
  echo "Dar is not built, aborting"
  echo ""
  echo "********************************************"
  exit 2
else
  export DAR=../dar_suite/dar
fi

if [ ! -x ./all_features ] ; then
  echo "********************************************"
  echo ""
  echo "all_features program not built, aborting"
  echo ""
  echo "********************************************"
  exit 2
fi

if [ `id -u` -ne 0 ]; then
  echo "********************************************"
  echo ""
  echo "need to be run as root"
  echo ""
  echo "********************************************"
  exit 3
fi

if ./all_features ; then
  echo "OK, all required features are available for testing"
else
  exit 3
fi
for hash in  md5 none sha1 ; do
  for crypto in bf none scram aes twofish serpent camellia ; do
    for zip in  gzip none bzip2 lzo ; do
      for slice in 1k none ; do
        for Slice in  500 none ; do
          for tape in y n ; do
            for seq_read in y n ; do
              for digit in 3 none ; do
                for sparse_size in 100 0 ; do
                  for keep_compr in y n ; do
                    for recheck_hole in y n ; do
                       ./main.sh $crypto "$zip" "$slice" "$Slice" "$tape" "$seq_read" "$digit" "$sparse_size" "$keep_compr" "$recheck_hole" "$hash" || exit 1
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

echo "+-------------------------------+"
echo "| ALL TESTS PASSED SUCCESSFULLY |"
echo "+-------------------------------+"
