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

if [ ! -x ../dar_suite/dar_slave ] ; then
  echo "********************************************"
  echo ""
  echo "Dar_slave is not built, aborting"
  echo ""
  echo "********************************************"
  exit 2
else
  export DAR_SLAVE=../dar_suite/dar_slave
fi

if [ ! -x ../dar_suite/dar_xform ] ; then
  echo "********************************************"
  echo ""
  echo "Dar_xform is not built, aborting"
  echo ""
  echo "********************************************"
  exit 2
else
  export DAR_XFORM=../dar_suite/dar_xform
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

if [ -z "$DAR_KEY" ] ; then
  echo "You need to set the environmental varialble"
  echo "DAR_KEY to an email for which you have public"
  echo "and private key available for encryption and"
  echo "signature."
  echo "You can use the GNUPGHOME variable to"
  echo "point to another keyring than ~root/.gnupg"
  echo ""
  echo "Example of use with bash:"
  echo "export DAR_KEY=your.email@your.domain"
  echo "export GNUPGHOME=~me/.gnupg"
  echo ""
  echo "Example of use with tcsh:"
  echo "setenv DAR_KEY your.email@your.domain"
  echo "setenv GNUPGHOME ~me/.gnupg"
  exit 3
fi

if ./all_features ; then
  echo "OK, all required features are available for testing"
else
  exit 3
fi
# for multi_thread in n y ; do
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
                         ./main.sh "$multi_thread" "$hash" "$crypto" "$zip" "$slice" "$Slice" "$tape" "$seq_read" "$digit" "$sparse_size" "$keep_compr" "$recheck_hole" "$asym" || exit 1
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

echo "+-------------------------------+"
echo "| ALL TESTS PASSED SUCCESSFULLY |"
echo "+-------------------------------+"

if [ -x ~denis/Scripts/send_sms ] ; then
    ~denis/Scripts/send_sms "make check successful !"
fi
