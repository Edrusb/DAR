#!/bin/bash

if [ $# -lt 1 ]; then
   echo "usage: $0 <list of test family>"
   echo "example: $0 A1 B1 B2 B3"
   exit 1
fi

## environement required:
# OPT environment variable (DCF file for common parameters)
if [ "$OPT" = "" ]; then
   echo '$OPT not set'
   exit 2
fi

if [ "$DAR" = "" ]; then
   echo '$DAR not set'
   exit 2
fi

# DAR environment variable points to dar
#
## relies on:
# ./build_tree.sh <dir> <discr>
# ./modif_tree.sh <dir> <discr>

src=SRC
dst=DST
src2=SRC2
full=full
catf=catf
catf_fly=catf_fly
double_catf=double_catf
double_catf_fly=double_catf_fly
diff=diff
catd=catd
catd_fly=catd_fly
double_catd=double_catd
double_catd_fly=double_catd_fly
diff_fly=diff_fly
diff_double=diff_double
diff_double_fly=diff_double_fly
merge_full=merge_full
merge_diff=merge_diff
full2=full2
diff2=diff2
full3=full3
decr=decr

hash="`sed -r -n -e 's/--hash (.*)/\1/p' tmp.file | tail -n 1`"
if [ "$hash" = "" ] ; then
   hash="none"
fi

function my_diff {
  if [ "$1" = "" -o "$2" = "" ] ; then
    echo "usage: $0 <dir> <dir>"
    return 1
  fi
  cd "$1"
  tar -cf "../$1.tmp.tar" .
  cd "../$2"
  tar -df "../$1.tmp.tar"
  local ret1=$?
  tar -cf "../$2.tmp.tar" .
  cd "../$1"
  tar -df "../$2.tmp.tar"
  local ret2=$?
  cd ..
  rm "$1.tmp.tar" "$2.tmp.tar"
  return `[ $ret1 -eq 0 -a $ret2 -eq 0 ]`
}

function GO {
  if [ "$1" == "" ] ; then
     echo "usage: $0 <label> [debug] command"
     exit 1
  fi
  local label="$1"
  if [ "$2" = "debug" ]; then
     local debug=yes
     local disable=false
     shift 2
  else
    if [ "$2" = "disable" ] ; then
       local disable=yes
    else
       local disable=no
       shift 1
    fi
  fi

  if [ "$disable" = "yes" ] ; then
    echo "$label = DISABLED"
  else
    echo -n "$label = "
    if [ "$debug" != "yes" ];  then
      { $* >/dev/null && echo "OK" ; } || { echo "$label FAILED" && echo "--------> $1 FAILED : $*" && exit 1 ; }
    else
      echo "--- debug -----"
      { $* && echo "OK" ; } || { echo "$label FAILED" && echo "--------> $1 FAILED : $*" && exit 1 ; }
      echo "---------------"
    fi
  fi
}

function check_hash {
  if [ "$1" == "" ] ; then
    echo "usage: $0 <hash> <file> <file> ..."
    exit 1
  fi
  local hash="$1"
  shift

  case $hash in
      ( none )
        return 0
        ;;
      ( md5 )
        for fic in $* ; do
          if md5sum -c "$fic.$hash" ; then
            echo "hash for $fic is OK"
          else
            echo "hash for $fic FAILED"
            return 1
          fi
        done
        ;;
      ( sha1 )
        for fic in $* ; do
          if sha1sum -c "$fic.$hash" ; then
            echo "hash $hash for $fic is OK"
          else
            echo "hash $hash for $fic FAILED"
            return 1
          fi
        done
        ;;
      ( * )
        echo "Unknown hash given"
        exit 2;
        ;;
  esac
}


function clean {
local verbose=""
rm -rf $verbose $src $dst $src2
rm -f $verbose $full*.dar
rm -f $verbose $catf*.dar
rm -f $verbose $catf_fly*.dar
rm -f $verbose $double_catf*.dar
rm -f $verbose $double_catf_fly*.dar
rm -f $verbose $diff*.dar
rm -f $verbose $catd*.dar
rm -f $verbose $catd_fly*.dar
rm -f $verbose $double_catd*.dar
rm -f $verbose $double_catd_fly*.dar
rm -f $verbose $diff_fly*.dar
rm -f $verbose $diff_double*.dar
rm -f $verbose $diff_double_fly*.dar
rm -f $verbose $merge_full*.dar
rm -f $verbose $merge_diff*.dar
rm -f $verbose $full2*.dar
rm -f $verbose $diff2*.dar
rm -f $verbose *.md5 *.sha1
rm -f $verbose $full3*.dar
rm -f $verbose $decr*.dar
}

clean

#
#   A1 -  operations on a single archive
#
if echo $* | grep "A1" > /dev/null ; then
./build_tree.sh $src "U"
GO "A1-1"  $DAR -N -c $full -R $src "-@" $catf_fly -B $OPT
GO "A1-2" check_hash $hash $full.*.dar
GO "A1-3"  $DAR -N -l $full -B $OPT
GO "A1-4"  $DAR -N -t $full -B $OPT
GO "A1-5"  $DAR -N -d $full -R $src -B $OPT
GO "A1-6"  $DAR -N -C $catf -A $full -B $OPT
GO "A1-7" check_hash $hash $catf.*.dar
mkdir $dst
GO "A1-8"  $DAR -N -x $full -R $dst -B $OPT
GO "A1-9"  my_diff  $src $dst
rm -rf $dst
fi

#
#   B1 -  operations on an isolated catalogue alone
#
if echo $* | grep "B1" > /dev/null ; then
GO "B1-1"  $DAR -N -l $catf -B $OPT
GO "B1-2"  $DAR -N -t $catf -B $OPT
GO "B1-3"  $DAR -N -C $double_catf -A $catf -B $OPT
GO "B1-4" check_hash $hash $double_catf.*.dar
fi

#
#  B2 -  operations on an on-fly isolated catalogue
#
if echo $* | grep "B2" > /dev/null ; then
GO "B2-1"  $DAR -N -l $catf_fly -B $OPT
GO "B2-2"  $DAR -N -t $catf_fly -B $OPT
GO "B2-3"  $DAR -N -C $double_catf_fly -A $catf_fly  -B $OPT
GO "B2-4" check_hash $hash $double_catf_fly.*.dar
fi

#
#  B3 -  operations on a double isolated catalogue
#
if echo $* | grep "B3" > /dev/null ; then
GO "B3-1"  $DAR -N -l $double_catf  -B $OPT
GO "B3-2"  $DAR -N -t $double_catf  -B $OPT
fi

#
#  B4 -  operations on a double on-fly isolated catalogue
#
if echo $* | grep "B4" > /dev/null ; then
GO "B4-1"  $DAR -N -l $double_catf_fly  -B $OPT
GO "B4-2"  $DAR -N -t $double_catf_fly  -B $OPT
fi

#
#  C1 -  operations on a single archive but with assistance of an isolated catalogue
#
if echo $* | grep "C1" > /dev/null ; then
GO "C1-1"  $DAR -N -t $full -A $catf -B $OPT
GO "C1-2"  $DAR -N -d $full -A $catf -R $src  -B $OPT
mkdir $dst
GO "C1-3"  $DAR -N -x $full -A $catf -R $dst  -B $OPT
GO "C1-4"  my_diff $src $dst
rm -rf $dst
fi

#
#  C2 -   operation on a single archive but with assistance of on-fly isolated catalogue
#
if echo $* | grep "C2" > /dev/null ; then
GO "C2-1"  $DAR -N -t $full -A $catf_fly  -B $OPT
GO "C2-2"  $DAR -N -d $full -A $catf_fly -R $src  -B $OPT
mkdir $dst
GO "C2-3"  $DAR -N -x $full -A $catf_fly -R $dst  -B $OPT
GO "C2-4"  my_diff $src $dst
rm -rf $dst
fi

#
#  C3 -   operation on a single archive but with assistance of a double isolated catalogue
#
if echo $* | grep "C3" > /dev/null ; then
GO "C3-1"  $DAR -N -t $full -A $double_catf  -B $OPT
GO "C3-2"  $DAR -N -d $full -A $double_catf -R $src  -B $OPT
mkdir $dst
GO "C3-3"  $DAR -N -x $full -A $double_catf -R $dst  -B $OPT
GO "C3-4"  my_diff $src $dst
rm -rf $dst
fi

#
#  C4 -   operation on a single archive but with assistance of a double on-fly isolated catalogue
#
if echo $* | grep "C4" > /dev/null ; then
GO "C4-1"  $DAR -N -t $full -A $double_catf_fly  -B $OPT
GO "C4-2"  $DAR -N -d $full -A $double_catf_fly -R $src  -B $OPT
mkdir $dst
GO "C4-3"  $DAR -N -x $full -A $double_catf_fly -R $dst  -B $OPT
GO "C4-4"  my_diff $src $dst
rm -rf $dst
fi

#
#  D1 -    operation with differential backup
#
if echo $* | grep "D1" > /dev/null ; then
./modif_tree.sh "$src" "U"
GO "D1-1"  $DAR -N -c $diff -R $src -A $full "-@" $catd_fly  -B $OPT
GO "D1-2" check_hash $hash $diff.*.dar
GO "D1-3"  $DAR -N -l $diff -B $OPT
GO "D1-4"  $DAR -N -t $diff -B $OPT
GO "D1-5"  $DAR -N -d $diff -R $src  -B $OPT
GO "D1-6"  $DAR -N -C $catd -A $full  -B $OPT
GO "D1-7" check_hash $hash $catf.*.dar
mkdir $dst
GO "D1-8"  $DAR -N -x $full -R $dst  -B $OPT
GO "D1-9"  $DAR -N -x $diff -R $dst -w  -B $OPT
GO "D1-A"  my_diff $src $dst
rm -rf $dst
fi


#
#  E1 -   operation on an diff isolated catalogue alone
#
if echo $* | grep "E1" > /dev/null ; then
GO "E1-1"  $DAR -N -l $catd  -B $OPT
GO "E1-2"  $DAR -N -t $catd  -B $OPT
GO "E1-3"  $DAR -N -C $double_catd -A $catd  -B $OPT
GO "E1-4" check_hash $hash $double_catd.*.dar
fi

#
#  E2 -   operation on an double diff isolated catalogue alone
#
if echo $* | grep "E2" > /dev/null ; then
GO "E2-1"  $DAR -N -l $double_catd  -B $OPT
GO "E2-2"  $DAR -N -t $double_catd  -B $OPT
fi

#
#  E3 -   operation on an on-fly diff isolated catalogue alone
#
if echo $* | grep "E3" > /dev/null ; then
GO "E3-1"  $DAR -N -l $catd_fly  -B $OPT
GO "E3-2"  $DAR -N -t $catd_fly  -B $OPT
fi

###

#
# F1 - merging full archives
#
if echo $* | grep "F1" > /dev/null ; then
./build_tree.sh $src2 "V"
GO "F1-1"  $DAR -N -c $full2 -R $src2  -B $OPT
GO "F1-2" check_hash $hash $full2.*.dar
GO "F1-3" $DAR -N -+ $merge_full -A $full "-@" $full2 -B $OPT
GO "F1-4" check_hash $hash $merge_full.*.dar
GO "F1-5"  $DAR -N -t $merge_full  -B $OPT
mkdir $dst
GO "F1-6"  $DAR -N -x $full -R $dst -B $OPT
GO "F1-7"  $DAR -N -x $full2 -R $dst -n -B $OPT
GO "F1-8"  $DAR -N --alter=do-not-compare-symlink-mtime -d $merge_full -R $dst  -B $OPT
rm -rf $dst
mkdir $dst
GO "F1-9" $DAR -N -x merge_full -R $dst  -B $OPT
rm -rf $dst
fi

#
# F2 - merging diff archives
#
if echo $* | grep "F2" > /dev/null ; then
./modif_tree.sh $src2 "V"
GO "F2-1"  $DAR -N -c $diff2 -R $src2 -A $full2 -B $OPT
GO "F2-2" check_hash $hash $diff2.*.dar
GO "F2-3"  $DAR -N -+ $merge_diff -A $diff "-@" $diff2 -B $OPT
GO "F2-4" check_hash $hash $merge_diff.*.dar
GO "F2-5"  $DAR -N -t $merge_diff -B $OPT
mkdir $dst
GO "F2-6"  $DAR -N -x $full2 -R $dst -B $OPT
GO "F2-7"  $DAR -N -x $diff2 -R $dst -w -B $OPT
GO "F2-8"  $DAR -N -x $full -R $dst -w -B $OPT
GO "F2-9"  $DAR -N -x $diff -R $dst -w -B $OPT
GO "F2-A"  $DAR -N -d $merge_diff -R $dst --alter=do-not-compare-symlink-mtime -B $OPT
rm -rf $dst
mkdir $dst
GO "F2-B"  $DAR -N -x $merge_full -R $dst -B $OPT
GO "F2-C"  $DAR -N -x $merge_diff -R $dst -w -B $OPT
rm -rf $dst
fi

#
# F3 - decremental backup
#
if echo $* | grep "F3" > /dev/null ; then
rm -rf $src
mkdir $src
GO "F3-1" $DAR -N -x $full -R $src -B $OPT
GO "F3-2" $DAR -N -x $diff -R $src -B $OPT -w
GO "F3-3" $DAR -N -c $full3 -R $src -B $OPT
GO "F3-4" $DAR -N -+ $decr -A $full -@ $full3 -B $OPT -w -ad
rm -rf $src
mkdir $src
GO "F3-5" $DAR -N -x $full -R $src -B $OPT
rm -rf $dst
mkdir $dst
GO "F3-6" $DAR -N -x $full3 -R $dst -B $OPT
GO "F3-7" $DAR -N -x $decr -R $dst -B $OPT -w
GO "F3-8" my_diff $src $dst
rm -rf $src $dst
fi

###
# final cleanup
#

clean

