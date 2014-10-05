#!/bin/sh

if [ "$1" = "" ] ; then
  echo "usage $0 <crypto> <zip> <slice> <Slice> <tape mark[y|n]> <sequential read[y|n]> <min-digit> <sparse-size> <keepcompressed[y|n]> <recheck-hole[y|n]> <hash> <multi-thread>"
  exit 1
fi

crypto="$1"
zip="$2"
slice="$3"
Slice="$4"
tape_mark="$5"
seq_read="$6"
digit="$7"
sparse="$8"
keep_compr="$9"
re_hole="${10}"
hash="${11}"
multi_thread="${12}"

#echo "crypto = $crypto"
#echo "zip = $zip"
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

ALL_TESTS="A1 B1 B2 B3 B4 C1 C2 C3 C4 D1 E1 E2 E3 F1 F2 F3 G1 G2 G3 H1"

export OPT=tmp.file

if [ "$crypto" != "none" ]; then
  crypto_K="-K $crypto:toto"
  crypto_J="-J $crypto:toto"
  crypto_A="'-$' $crypto:toto"
else
  crypto_K=""
  crypto_J=""
  crypto_A=""
fi

if [ "$zip" != "none" ]; then
  zip=-z$zip:1
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
  ALL_TESTS=`echo $ALL_TESTS | sed -r -e 's/(F1|F2|F3)//g'`
  if [ "$tape_mark" != "y" ] ; then
    ALL_TESTS="none"
  fi
else
  sequential=""
fi

if [ "$digit" != "none" ]; then
  min_digits="--min-digit $digit,$digit,$digit"
  export xform_digits="-9 $digit,$digit"
  export slave_digits="-9 $digit"
  export padded_one=`./padder $digit 1`
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

if [ "$multi_thread" != "y" ]; then
  multi_thread="-G"
else
  multi_thread=""
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

listing:

testing:

diffing:

EOF
echo "----------------------------------------------------------------"
echo "----------------------------------------------------------------"
echo "TESTS PARAMETERS NATURE: crypto zip slice Slice tape-mark seq-read digit sparse keepcompr re-hole hash multi-thread"
echo "TESTS PARAMETERS VALUE: $*"
echo "TEST SET: $ALL_TESTS"
echo "----------------------------------------------------------------"
exec ./routine.sh $ALL_TESTS
