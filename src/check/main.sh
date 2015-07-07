#!/bin/sh

if [ "$1" = "" ] ; then
  echo "usage $0  <multi-thread> <hash> <crypto> <zip> <slice> <Slice> <tape mark[y|n]> <sequential read[y|n]> <min-digit> <sparse-size> <keepcompressed[y|n]> <recheck-hole[y|n]> <asym[y|n]>"
  exit 1
fi

multi_thread="$1"
hash="$2"
crypto="$3"
zip="$4"
slice="$5"
Slice="$6"
tape_mark="$7"
seq_read="$8"
digit="$9"
sparse="${10}"
keep_compr="${11}"
re_hole="${12}"
asym="${13}"

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
  if [ "$asym" != "y" ]; then
    crypto_K="-K $crypto:toto"
    crypto_J="-J $crypto:toto"
    crypto_A="'-$' $crypto:toto"
    sign=""
  else
    crypto_K="-K gnupg:$crypto:$DAR_KEY"
    crypto_J=""
    crypto_A="'-$' gnupg:$crypto:$DAR_KEY"
    sign="$DAR_KEY"
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
    # when asym != y
  fi
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
 --sign $sign

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
 --sign $sign

listing:

testing:

diffing:

EOF
echo "----------------------------------------------------------------"
echo "----------------------------------------------------------------"
echo "TESTS PARAMETERS NATURE: thread\thash\t\tcrypto\tzip\tslice\tSlice\ttape-mark\tseqread\tdigit\tsparse\tkeepcpr\tre-hole\tasym"
echo "TESTS PARAMETERS VALUE : $multi_thread\t$hash\t$crypto\t$zip\t$slice\t$Slice\t$tape_mark\t\t$seq_read\t$digit\t$sparse\t$keep_compr\t$re_hole\t$asym"
echo "TEST SET: $ALL_TESTS"
echo "----------------------------------------------------------------"
exec ./routine.sh $ALL_TESTS
