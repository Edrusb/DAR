#!/bin/bash

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

if [ "$1" = "" ] ; then
  echo "usage $0 <prefix> <multi-thread> <hash> <crypto> <zip> <zip block size> <slice> <Slice> <tape mark[y|n]> <sequential read[y|n]> <min-digit> <sparse-size> <keepcompressed[y|n]> <recheck-hole[y|n]> <asym[y|n]>"
  exit 1
fi

if [ -f my_env ] ; then
    source my_env
fi

prefix="$1"
multi_thread="$2"
hash="$3"
crypto="$4"
zip="$5"
zipbs="$6"
slice="$7"
Slice="$8"
tape_mark="$9"
seq_read="$10"
digit="${11}"
sparse="${12}"
keep_compr="${13}"
re_hole="${14}"
asym="${15}"


#echo "crypto = $crypto"
#echo "zip = $zip"
#echo "zipbs = $zipbs"
#echo "slice = $slice"
#echo "Slice = $Slice"
#echo "tape_mark = $tape_mark"
#echo "seq_read = $seq_read"
#echo "digit = $digit"
#echo "sparse = $sparse"
#echo "keep_compr = $keep_compr"
#echo "re_hole = $re_hole"
#echo "hash = [$hash]"
#echo "multi-thread = [$multi_thread]"

ALL_TESTS="A1 A2 A3 B1 B2 B3 B4 C1 C2 C3 C4 D1 D2 E1 E2 E3 F1 F2 F3 F4 F5 G1 G2 G3 H1 I1 I2"

export prefix
prefix_dir="$prefix.dir"

if [ -e "$prefix_dir" ] ; then
    rm -rf "$prefix_dir"
fi
mkdir "$prefix_dir"
cd "$prefix_dir"

LOG=log.out
printf " ----------------------------------------------------------------\n" > "$LOG"
printf "TESTS PARAMETERS NATURE: thread\thash\t\tcrypto\tzip\tslice\tSlice\ttape-mark\tseqread\tdigit\tsparse\tkeepcpr\tre-hole\tasy\n" >> "$LOG"
printf "TESTS PARAMETERS VALUE : $multi_thread\t$hash\t$crypto\t$zip\t$slice\t$Slice\t$tape_mark\t\t$seq_read\t$digit\t$sparse\t$keep_compr\t$re_hole\t$asym\n" >> "$LOG"
printf "TEST SET: $ALL_TESTS\n" >> "$LOG"
printf " ----------------------------------------------------------------\n" >> "$LOG"

export OPT=tmp.file
export OPT_NOSEQ=tmp_noseq.file

if [ -z "$DAR_KEY" ] ; then
    echo "*****************"
    echo ""
    echo "DAR_KEY not set"
    echo ""
    echo "*****************"
    exit 1
fi

if [ -z "$DAR_FTP_REPO" ] ; then
    echo "*****************"
    echo ""
    echo "DAR_FTP_REPO not set"
    echo ""
    echo "*****************"
    exit 1
fi

if [ -z "$DAR_SFTP_REPO" ] ; then
    echo "*****************"
    echo ""
    echo "DAR_SFTP_REPO not set"
    echo ""
    echo "*****************"
    exit 1
fi

password="did7hj&!sksjs#@_/-8<>!sk_ajqwplHYT"

if [ "$crypto" != "none" ]; then
  if [ "$asym" != "y" ]; then
    crypto_K="-K $crypto:$password"
    crypto_J="-J $crypto:$password"
    crypto_A="'-$' $crypto:$password"
    sign=""
  else
      if [ "$crypto" != "scram" ] ; then
	  crypto_K="-K gnupg:$crypto:$DAR_KEY"
	  crypto_J=""
	  crypto_A="'-$' gnupg:$crypto:$DAR_KEY"
	  sign="--sign $DAR_KEY"
      else
	  ALL_TESTS="A0"
	  # scrambling with public key is useless
	  # and not accepted by dar
      fi
  fi
else
  if [ "$asym" != "y" ]; then
    crypto_K=""
    crypto_J=""
    crypto_A=""
    sign=""
  else
    ALL_TESTS="A0"
    # do no test (A0 does not exist) as same test done
    # when asym = y
  fi
fi

if [ "$zip" != "none" ]; then
  zip=-z$zip:1:$zipbs
else
  zip=""
fi

if [ "$slice" != "none" ]; then
  slicing="-s $slice"
  if [ "$Slice" != "none" ]; then
    slicing="$slicing -S $Slice"
  fi
  ALL_TESTS=`echo $ALL_TESTS | sed -r -e 's/(G1|G2|G3)//g'`
else
  slicing=""
fi

if [ "$seq_read" = "y" ] ; then
  ALL_TESTS=`echo $ALL_TESTS | sed -r -e 's/G2//g'`
else
  ALL_TESTS=`echo $ALL_TESTS | sed -r -e 's/G3//g'`
fi

if [ "$tape_mark" = "y" ]; then
  tape=""
else
  tape="-at"
fi

if [ "$seq_read" = "y" ]; then
  sequential="--sequential-read"
  ALL_TESTS=`echo $ALL_TESTS | sed -r -e 's/(F1|F2|F3|F4|F5)//g'`
  if [ "$tape_mark" != "y" ] ; then
    ALL_TESTS="none"
  fi
else
  sequential=""
fi

if [ "$keep_compr" = "y" ] ; then
   ALL_TESTS=`echo $ALL_TESTS | sed -r -e 's/F5//g'`
fi

if [ "$digit" != "none" ]; then
  min_digits="--min-digit $digit,$digit,$digit"
  export xform_digits="-9 $digit,$digit"
  export slave_digits="-9 $digit"
  export padded_one=`../padder $digit 1`
else
  min_digits=""
  export xform_digits=""
  export slave_digits=""
  export padded_one="1"
fi

sparse="-1 $sparse"

if [ "$keep_compr" = "y" ]; then
  keepcompressed="-ak"
else
  keepcompressed=""
fi

if [ "$re_hole" = "y" ]; then
  recheck_hole="-ah"
else
  recheck_hole=""
fi

if [ "$hash" != "none" ]; then
  hash="--hash $hash"
else
  hash=""
fi

if [ "$multi_thread" != "1" ]; then
    multi_thread="-G $multi_thread"
else
    multi_thread=""
fi

if [ ! -z "$keepcompressed" -a ! -z "$recheck_hole" ] ; then
    ALL_TESTS=`echo $ALL_TESTS | sed -r -e 's/(F5)//g'`
fi

cat > $OPT <<EOF
all:
 $sequential
 $min_digits
 $crypto_K
 $multi_thread

reference:
 $crypto_J

auxiliary:
 $crypto_A

isolate:
 $zip
 $slicing
 $tape
 $hash
 $sign

merge:
 $zip
 $slicing
 $tape
 $hash
 $keepcompressed
 $recheck_hole

create:
 $zip
 $slicing
 $tape
 $hash
 $sparse
 $sign

listing:

testing:

diffing:

EOF

grep -v -- "--sequential-read" $OPT > $OPT_NOSEQ

if ../routine.sh $ALL_TESTS 1>> "$LOG" 2> log.err ; then
  cd ..
  rm -rf "$prefix_dir"
  touch "$prefix"
else
  echo "================================"
  echo "Failure of test $prefix"
  echo "--------------------------------"
  echo "multi_thread = $multi_thread"
  echo "hash         = $hash"
  echo "crypto       = $crypto"
  echo "zip          = $zip"
  echo "slice        = $slice"
  echo "Slice        = $Slice"
  echo "tape_mark    = $tape_mark"
  echo "seq_read     = $seq_read"
  echo "digit        = $digit"
  echo "sparse       = $sparse"
  echo "keep_compr   = $keep_compr"
  echo "re_hole      = $re_hole"
  echo "asym         = $asym"
  echo "================================"
  exit 1
fi
