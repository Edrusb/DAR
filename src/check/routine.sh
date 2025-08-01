#!/bin/bash

#######################################################################
# dar - disk archive - a backup/restoration program
# Copyright (C) 2002-2025 Denis Corbin
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

if [ -z "$OPT_NOSEQ" ] ; then
    echo '$OPT_NOSEQ not set'
    exit 2
fi

if [ "$DAR" = "" ]; then
   echo '$DAR not set'
   exit 2
fi

if [ ! -z "$ROUTINE_DEBUG" -a "$ROUTINE_DEBUG" != "debug" ] ; then
   echo 'unvalid value given to environment variable ROUTINE_DEBUG'
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
dst2=DST2
full=full
catf=catf
catf_fly=catf_fly
full_delta=full_delta
cat_full_delta=cat_full_delta
catf_delta=catf_delta
double_catf=double_catf
double_catf_fly=double_catf_fly
diff=diff
diff_delta=diff_delta
diff_delta2=diff_delta2
catd=catd
catd_fly=catd_fly
double_catd=double_catd
double_catd_fly=double_catd_fly
diff_fly=diff_fly
diff_double=diff_double
diff_double_fly=diff_double_fly
merge_full=merge_full
merge_diff=merge_diff
merge_delta=merge_delta
full2=full2
diff2=diff2
full3=full3
decr=decr
todar=todar
toslave=toslave
piped=piped
piped_fly=piped_fly
xformed1=xformed1
xformed2=xformed2
xformed3=xformed3

if [ "$1" != "NONE" ] ; then
  hash="`sed -r -n -e 's/--hash (.*)/\1/p' tmp.file | tail -n 1`"
fi
hash_xform="-3 $hash"
if [ "$hash" = "" ] ; then
   hash="none"
   hash_xform=""
fi

function my_diff
{
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

function RETRIER
{
    # run the command given in argument and restart it if it failed
    # while the value of the $search_pattern variable below is found
    # on stdout (this function cannot be used when dar use stdout to
    # produce an archive).

    if [ "$1" != "normal" -a "$1" != "piped-in" -a "$1" != "piped-out" ] ; then
	echo "usage: RETRIER { normal | piped-in | piped-out } command and arguments to run" >&2
	exit 1
    fi

    local tmp_out_file=./retrier_tmp_out
    local tmp_err_file=./retrier_tmp_err
    local total_out=./retrier_total_out
    local total_err=./retrier_total_err
    local total_in=./retrier_total_in
    local search_pattern="Unexpected error reported by GPGME"
    local ret=0
    local looping=1
    local mode="$1"

    shift 1

    rm -f "$total_out" "$total_err"

    # copying stdin to be able to retry the call if needed

    if [ "$mode" = "piped-in" ] ; then
	echo "RETRIER copying stdin... " >&2
	cat > "$total_in"
	echo "RETRIER stdin copied: $(stat $total_in)" >&2
    fi

    echo "RETRIER: mode=[$mode]   command=[$*]" >&2

    while [ $looping -ge 1 ] ; do

	#---
	# runinng the requested command catching stdout, stderr and exit status
	#---

	if [ "$mode" = "piped-in" ] ; then
	    $* < "$total_in" 1> "$tmp_out_file" 2> "$tmp_err_file"
	else
	    if [ "$mode" = "piped-out" ] ; then
		echo "now running $*" >&2 "$tmp_err_file"
	    fi
	    $* 1> "$tmp_out_file" 2> "$tmp_err_file"
	fi
	ret=$?

	if [ "$mode" != "piped-out" ] ; then
	    cat < "$tmp_out_file" >> "$total_out"
	else
	    cat < "$tmp_out_file" > "$total_out"
	fi
	cat < "$tmp_err_file" >> "$total_err"

	#---
	# analysing stdout and stderr for the search_pattern and deciding whether to loop or not
	#---

	if [ "$mode" != "piped-out" ] ; then
	    looping=$(grep "$search_pattern" "$tmp_out_file" | wc -l)
	else
	    looping=$(grep "$search_pattern" "$tmp_err_file" | wc -l)
	fi

	if [ $looping -ge 1 ] ; then
	    if [ "$mode" != "piped-out" ] ; then
		echo "Looping due to: $search_pattern" >> "$total_out"
	    else
		echo "Looping due to: $search_pattern" >> "$total_err"
	    fi
	    sleep 1
	fi
    done

    #---
    # providing transparent stdout, stderr and exit status
    #---

    cat "$total_out"
    cat "$total_err" >&2
    rm -f "$tmp_out_file" "$tmp_err_file" "$total_out" "$total_err" "$total_in" 2> /dev/null
    return $ret
}

function GO
{
  if [ "$1" == "" ] ; then
     echo "usage: $0 <label> validexitcode[,validexitcode,...] [debug|disable|piped-in|piped-out] command" >&2
     exit 1
  fi

  local label="$1"
  local exit_codes=`echo "$2"| sed -re 's/,+/ /g'`
  if [ "$3" = "debug" ]; then
     local disable="no"
     local debug="yes"
     local piped="normal"
     # yes, because we cannot debug piped-in or piped-out mode
     shift 3
  else
    if [ "$3" = "disable" ] ; then
       local disable="yes"
    else
	if [ "$3" = "piped-in" -o "$3" = "piped-out" ] ; then
	    local disable="no"
	    local debug="no"
	    local piped="$3"
	    shift 3
	else
            local disable="no"
	    local debug="no"
	    local piped="normal"
            shift 2
       fi
    fi
  fi

  if [ "$disable" = "yes" ] ; then
    echo "$label = DISABLED"
  else
    if [ "$piped" != "piped-out" ] ; then
	printf "$label = "
    else
	printf "$label = " >&2
    fi

    if [ "$debug" != "yes" ] ; then
	echo "About to run [RETRIER $piped $*]" >&2
	RETRIER "$piped" $*
    else
	# never piped-in nor piped-out in debug mode

	echo "--- debug -----"
	echo $*
	RETRIER "$piped" $*
    fi
    local ret=$?

    local is_valid="no"
    for valid in $exit_codes ; do
	if [ $ret = $valid ] ; then is_valid="yes" ; fi
    done

    if [ $is_valid = "yes" ] ; then
	if [ "$piped" != "piped-out" ] ; then
	    echo "OK"
        else
	    echo "OK" >&2
	fi
    else
	if [ "$piped" != "piped-out" ] ; then
	    echo "$label FAILED"
	    echo "--------> $1 FAILED : $*"
	else
	    echo "--------> $1 FAILED : $*" >&2
	fi
	exit 1
    fi

    if [ "$debug" = "yes" ] ;  then
	echo "---------------"
    fi
  fi
}

function check_hash
{
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


function clean
{
    local verbose=""
    rm -rf $verbose $src $dst $src2 $dst2
    rm -f $verbose *.md5 *.sha1
    rm -f $verbose $full*.dar
    rm -f $verbose $catf*.dar
    rm -f $verbose $catf_fly*.dar
    rm -f $verbose $full_delta*.dar
    rm -f $verbose $cat_full_delta*.dar
    rm -f $verbose $catf_delta*.dar
    rm -f $verbose $double_catf*.dar
    rm -f $verbose $double_catf_fly*.dar
    rm -f $verbose $diff*.dar
    rm -f $verbose $diff_delta*.dar
    rm -f $verbose $diff_delta2*.dar
    rm -f $verbose $catd*.dar
    rm -f $verbose $catd_fly*.dar
    rm -f $verbose $double_catd*.dar
    rm -f $verbose $double_catd_fly*.dar
    rm -f $verbose $diff_fly*.dar
    rm -f $verbose $diff_double*.dar
    rm -f $verbose $diff_double_fly*.dar
    rm -f $verbose $merge_full*.dar
    rm -f $verbose $merge_diff*.dar
    rm -f $verbose $merge_delta*.dar
    rm -f $verbose $full2*.dar
    rm -f $verbose $diff2*.dar
    rm -f $verbose $full3*.dar
    rm -f $verbose $decr*.dar
    rm -f $verbose $piped*.dar
    rm -f $verbose $piped_fly*.dar
    rm -f $verbose $xformed1*.dar
    rm -f $verbose $xformed2*.dar
    rm -f $verbose $xformed3*.dar
    rm -f $todar $toslave
}

clean

#
#   A1 -  operations on a single archive
#
if echo $* | grep "A1" > /dev/null ; then
../build_tree.sh $src "U"
GO "A1-1" 0 $ROUTINE_DEBUG $DAR -N -Q -c $full -R $src "-@" $catf_fly -B $OPT
GO "A1-2" 0 $ROUTINE_DEBUG check_hash $hash $full.*.dar
GO "A1-3" 0 $ROUTINE_DEBUG $DAR -N -Q -l $full -B $OPT
GO "A1-4" 0 $ROUTINE_DEBUG $DAR -N -Q -q -l $full -B $OPT
GO "A1-5" 0 $ROUTINE_DEBUG $DAR -N -Q -t $full -B $OPT
GO "A1-6" 0 $ROUTINE_DEBUG $DAR -N -Q -d $full -R $src -B $OPT
GO "A1-7" 0 $ROUTINE_DEBUG $DAR -N -Q -C $catf -A $full -B $OPT
GO "A1-8" 0 $ROUTINE_DEBUG check_hash $hash $catf.*.dar
mkdir $dst
GO "A1-9" 0 $ROUTINE_DEBUG $DAR -N -Q -x $full -R $dst -B $OPT
GO "A1-A" 0 $ROUTINE_DEBUG my_diff  $src $dst
GO "A1-B" 0 $ROUTINE_DEBUG $DAR -N -Q -d $full -R $dst -B $OPT
rm -rf $dst
fi

#
# A2 - delta signature generation on an archive alone
#

if echo $* | grep "A2" > /dev/null ; then
GO "A2-1" 0 $ROUTINE_DEBUG $DAR -N -Q -c $full_delta -R $src -B $OPT --delta sig --delta-sig-min-size 1
GO "A2-2" 0 $ROUTINE_DEBUG check_hash $hash $full_delta.*.dar
GO "A2-3" 0 $ROUTINE_DEBUG $DAR -N -Q -t $full_delta -B $OPT
GO "A2-4" 0 $ROUTINE_DEBUG $DAR -N -Q -d $full_delta -R $src -B $OPT
GO "A2-5" 0 $ROUTINE_DEBUG $DAR -N -Q -C $cat_full_delta -A $full_delta -B $OPT --delta sig
GO "A2-6" 0 $ROUTINE_DEBUG check_hash $hash $cat_full_delta.*.dar
mkdir $dst2
GO "A2-7" 0 $ROUTINE_DEBUG $DAR -N -Q -x $full_delta -R $dst2 -B $OPT
GO "A2-8" 0 $ROUTINE_DEBUG my_diff $src $dst2
GO "A2-9" 0 $ROUTINE_DEBUG $DAR -N -Q -d $full_delta -R $dst2 -B $OPT
rm -rf $dst2
fi

#
# A3 - delta signature generation while isolating
#

if echo $* | grep "A3" > /dev/null ; then
GO "A3-1" 0 $ROUTINE_DEBUG $DAR -N -Q -C $catf_delta -A $full -B $OPT --delta sig --include-delta-sig '"*"' --delta-sig-min-size 1
GO "A3-2" 0 $ROUTINE_DEBUG $DAR -N -Q -t $catf_delta -B $OPT
GO "A3-3" 0 $ROUTINE_DEBUG $DAR -N -Q -t $full -A $catf_delta -B $OPT
fi

#
#   B1 -  operations on an isolated catalogue alone
#
if echo $* | grep "B1" > /dev/null ; then
GO "B1-1" 0 $ROUTINE_DEBUG $DAR -N -Q -l $catf -B $OPT
GO "B1-2" 0 $ROUTINE_DEBUG $DAR -N -Q -q -l $catf -B $OPT
GO "B1-3" 0 $ROUTINE_DEBUG $DAR -N -Q -t $catf -B $OPT
GO "B1-4" 0 $ROUTINE_DEBUG $DAR -N -Q -d $catf -R $src -B $OPT
GO "B1-5" 0 $ROUTINE_DEBUG $DAR -N -Q -C $double_catf -A $catf -B $OPT
GO "B1-6" 0 $ROUTINE_DEBUG check_hash $hash $double_catf.*.dar
GO "B1-7" 0 $ROUTINE_DEBUG $DAR -N -Q -t $double_catf -B $OPT
GO "B1-8" 0 $ROUTINE_DEBUG $DAR -N -Q -l $cat_full_delta -B $OPT
GO "B1-9" 0 $ROUTINE_DEBUG $DAR -N -Q -t $cat_full_delta -B $OPT
GO "B1-A" 0 $ROUTINE_DEBUG $DAR -N -Q -d $cat_full_delta -R $src -B $OPT
fi

#
#  B2 -  operations on an on-fly isolated catalogue
#
if echo $* | grep "B2" > /dev/null ; then
GO "B2-1" 0 $ROUTINE_DEBUG $DAR -N -Q -l $catf_fly -B $OPT
GO "B2-2" 0 $ROUTINE_DEBUG $DAR -N -Q -q -l $catf_fly -B $OPT
GO "B2-3" 0 $ROUTINE_DEBUG $DAR -N -Q -t $catf_fly -B $OPT
GO "B2-4" 0 $ROUTINE_DEBUG $DAR -N -Q -C $double_catf_fly -A $catf_fly  -B $OPT
GO "B2-5" 0 $ROUTINE_DEBUG check_hash $hash $double_catf_fly.*.dar
GO "B2-6" 0 $ROUTINE_DEBUG $DAR -N -Q -l $double_catf_fly -B $OPT
GO "B2-7" 0 $ROUTINE_DEBUG $DAR -N -Q -d $catf_fly -R $src -B $OPT
fi

#
#  B3 -  operations on a double isolated catalogue
#
if echo $* | grep "B3" > /dev/null ; then
GO "B3-1" 0 $ROUTINE_DEBUG $DAR -N -Q -l $double_catf  -B $OPT
GO "B3-2" 0 $ROUTINE_DEBUG $DAR -N -Q -q -l $double_catf  -B $OPT
GO "B3-3" 0 $ROUTINE_DEBUG $DAR -N -Q -t $double_catf  -B $OPT
fi

#
#  B4 -  operations on a double on-fly isolated catalogue
#
if echo $* | grep "B4" > /dev/null ; then
GO "B4-1" 0 $ROUTINE_DEBUG $DAR -N -Q -l $double_catf_fly  -B $OPT
GO "B4-2" 0 $ROUTINE_DEBUG $DAR -N -Q -q -l $double_catf_fly  -B $OPT
GO "B4-3" 0 $ROUTINE_DEBUG $DAR -N -Q -t $double_catf_fly  -B $OPT
fi

#
#  C1 -  operations on a single archive but with assistance of an isolated catalogue
#
if echo $* | grep "C1" > /dev/null ; then
GO "C1-1" 0 $ROUTINE_DEBUG $DAR -N -Q -t $full -A $catf -B $OPT
GO "C1-2" 0 $ROUTINE_DEBUG $DAR -N -Q -d $full -A $catf -R $src  -B $OPT
mkdir $dst
GO "C1-3" 0 $ROUTINE_DEBUG $DAR -N -Q -x $full -A $catf -R $dst  -B $OPT
GO "C1-4" 0 $ROUTINE_DEBUG my_diff $src $dst
rm -rf $dst
GO "C1-5" 0 $ROUTINE_DEBUG $DAR -N -Q -t $full_delta -A $cat_full_delta -B $OPT
GO "C1-6" 0 $ROUTINE_DEBUG $DAR -N -Q -d $full_delta -A $cat_full_delta -R $src -B $OPT

GO "C1-7" 0 $ROUTINE_DEBUG $DAR -N -Q -t $full_delta -A $cat_full_delta -B $OPT
GO "C1-8" 0 $ROUTINE_DEBUG $DAR -N -Q -d $full_delta -A $cat_full_delta -R $src -B $OPT
mkdir $dst2
GO "C1-9" 0 $ROUTINE_DEBUG $DAR -N -Q -x $full_delta -A $cat_full_delta -R $dst2 -B $OPT
GO "C1-A" 0 $ROUTINE_DEBUG my_diff $src $dst2
rm -rf $dst2
fi

#
#  C2 -   operation on a single archive but with assistance of on-fly isolated catalogue
#
if echo $* | grep "C2" > /dev/null ; then
GO "C2-1" 0 $ROUTINE_DEBUG $DAR -N -Q -t $full -A $catf_fly  -B $OPT
GO "C2-2" 0 $ROUTINE_DEBUG $DAR -N -Q -d $full -A $catf_fly -R $src  -B $OPT
mkdir $dst
GO "C2-3" 0 $ROUTINE_DEBUG $DAR -N -Q -x $full -A $catf_fly -R $dst  -B $OPT
GO "C2-4" 0 $ROUTINE_DEBUG my_diff $src $dst
rm -rf $dst
fi

#
#  C3 -   operation on a single archive but with assistance of a double isolated catalogue
#
if echo $* | grep "C3" > /dev/null ; then
GO "C3-1" 0 $ROUTINE_DEBUG $DAR -N -Q -t $full -A $double_catf  -B $OPT
GO "C3-2" 0 $ROUTINE_DEBUG $DAR -N -Q -d $full -A $double_catf -R $src  -B $OPT
mkdir $dst
GO "C3-3" 0 $ROUTINE_DEBUG $DAR -N -Q -x $full -A $double_catf -R $dst  -B $OPT
GO "C3-4" 0 $ROUTINE_DEBUG my_diff $src $dst
rm -rf $dst
fi

#
#  C4 -   operation on a single archive but with assistance of a double on-fly isolated catalogue
#
if echo $* | grep "C4" > /dev/null ; then
GO "C4-1" 0 $ROUTINE_DEBUG $DAR -N -Q -t $full -A $double_catf_fly  -B $OPT
GO "C4-2" 0 $ROUTINE_DEBUG $DAR -N -Q -d $full -A $double_catf_fly -R $src  -B $OPT
mkdir $dst
GO "C4-3" 0 $ROUTINE_DEBUG $DAR -N -Q -x $full -A $double_catf_fly -R $dst  -B $OPT
GO "C4-4" 0 $ROUTINE_DEBUG my_diff $src $dst
rm -rf $dst
fi

#
#  D1 -    operation with differential backup
#
if echo $* | grep "D1" > /dev/null ; then
../modif_tree.sh "$src" "U"
GO "D1-1" 0 $ROUTINE_DEBUG $DAR -N -Q -c $diff -R $src -A $full "-@" $catd_fly  -B $OPT_NOSEQ
GO "D1-2" 0 $ROUTINE_DEBUG check_hash $hash $diff.*.dar
GO "D1-3" 0 $ROUTINE_DEBUG $DAR -N -Q -l $diff -B $OPT
GO "D1-4" 0 $ROUTINE_DEBUG $DAR -N -Q -q -l $diff -B $OPT
GO "D1-5" 0 $ROUTINE_DEBUG $DAR -N -Q -t $diff -B $OPT
GO "D1-6" 0 $ROUTINE_DEBUG $DAR -N -Q -d $diff -R $src  -B $OPT
GO "D1-7" 0 $ROUTINE_DEBUG $DAR -N -Q -C $catd -A $full  -B $OPT
GO "D1-8" 0 $ROUTINE_DEBUG check_hash $hash $catf.*.dar
mkdir $dst
GO "D1-9" 0 $ROUTINE_DEBUG $DAR -N -Q -x $full -R $dst  -B $OPT
GO "D1-A" 0 $ROUTINE_DEBUG $DAR -N -Q -x $diff -R $dst -w  -B $OPT
GO "D1-B" 0 $ROUTINE_DEBUG my_diff $src $dst
rm -rf $dst
fi

#
#  D2 -    operation with differential backup using delta signature
#
if echo $* | grep "D2" > /dev/null ; then
GO "D2-1" 0 $ROUTINE_DEBUG $DAR -N -Q -c $diff_delta -R $src -A $full_delta -B $OPT_NOSEQ --delta sig --delta-sig-min-size 1
GO "D2-2" 0 $ROUTINE_DEBUG check_hash $hash $diff_delta.*.dar
GO "D2-3" 0 $ROUTINE_DEBUG $DAR -N -Q -l $diff_delta -B $OPT
GO "D2-4" 0 $ROUTINE_DEBUG $DAR -N -Q -q -l $diff_delta -B $OPT
GO "D2-5" 0 $ROUTINE_DEBUG $DAR -N -Q -t $diff_delta -B $OPT
GO "D2-6" 0 $ROUTINE_DEBUG $DAR -N -Q -d $diff_delta -R $src -B $OPT
mkdir $dst2
GO "D2-7" 0 $ROUTINE_DEBUG $DAR -N -Q -x $full_delta -R $dst2 -B $OPT
GO "D2-8" 0 $ROUTINE_DEBUG $DAR -N -Q -x $diff_delta -R $dst2 -w -B $OPT
GO "D2-9" 0 $ROUTINE_DEBUG my_diff $src $dst2
../modif_tree_again.sh "$src" "U"
GO "D2-A" 0 $ROUTINE_DEBUG $DAR -N -Q -c $diff_delta2 -R $src -A $diff_delta -B $OPT_NOSEQ --delta sig --delta-sig-min-size 1
GO "D2-B" 0 $ROUTINE_DEBUG check_hash $hash $diff_delta2.*.dar
GO "D2-C" 0 $ROUTINE_DEBUG $DAR -N -Q -x $diff_delta2 -R $dst2 -w -B $OPT
GO "D2-D" 0 $ROUTINE_DEBUG my_diff "$src" "$dst2"
rm -rf $dst2
fi

#
#  E1 -   operation on an diff isolated catalogue alone
#
if echo $* | grep "E1" > /dev/null ; then
GO "E1-1" 0 $ROUTINE_DEBUG $DAR -N -Q -l $catd  -B $OPT
GO "E1-2" 0 $ROUTINE_DEBUG $DAR -N -Q -q -l $catd  -B $OPT
GO "E1-3" 0 $ROUTINE_DEBUG $DAR -N -Q -t $catd  -B $OPT
GO "E1-4" 0 $ROUTINE_DEBUG $DAR -N -Q -C $double_catd -A $catd  -B $OPT
GO "E1-5" 0 $ROUTINE_DEBUG check_hash $hash $double_catd.*.dar
fi

#
#  E2 -   operation on an double diff isolated catalogue alone
#
if echo $* | grep "E2" > /dev/null ; then
GO "E2-1" 0 $ROUTINE_DEBUG $DAR -N -Q -l $double_catd  -B $OPT
GO "E2-2" 0 $ROUTINE_DEBUG $DAR -N -Q -q -l $double_catd  -B $OPT
GO "E2-3" 0 $ROUTINE_DEBUG $DAR -N -Q -t $double_catd  -B $OPT
fi

#
#  E3 -   operation on an on-fly diff isolated catalogue alone
#
if echo $* | grep "E3" > /dev/null ; then
GO "E3-1" 0 $ROUTINE_DEBUG $DAR -N -Q -l $catd_fly  -B $OPT
GO "E3-2" 0 $ROUTINE_DEBUG $DAR -N -Q -q -l $catd_fly  -B $OPT
GO "E3-3" 0 $ROUTINE_DEBUG $DAR -N -Q -t $catd_fly  -B $OPT
fi

###

#
# F1 - merging full archives
#
if echo $* | grep "F1" > /dev/null ; then
../build_tree.sh $src2 "V"
GO "F1-1" 0 $ROUTINE_DEBUG $DAR -N -Q -c $full2 -R $src2  -B $OPT
GO "F1-2" 0 $ROUTINE_DEBUG check_hash $hash $full2.*.dar
GO "F1-3" 0 $ROUTINE_DEBUG $DAR -N -Q -+ $merge_full -A $full "-@" $full2 -B $OPT
GO "F1-4" 0 $ROUTINE_DEBUG check_hash $hash $merge_full.*.dar
GO "F1-5" 0 $ROUTINE_DEBUG $DAR -N -Q -t $merge_full  -B $OPT
mkdir $dst
GO "F1-6" 0 $ROUTINE_DEBUG $DAR -N -Q -x $full -R $dst -B $OPT
GO "F1-7" 0 $ROUTINE_DEBUG $DAR -N -Q -x $full2 -R $dst -n -B $OPT
GO "F1-8" 0 $ROUTINE_DEBUG $DAR -N -Q --alter=do-not-compare-symlink-mtime -d $merge_full -R $dst  -B $OPT
rm -rf $dst
mkdir $dst
GO "F1-9" 0 $ROUTINE_DEBUG $DAR -N -Q -x merge_full -R $dst  -B $OPT
rm -rf $dst
fi

#
# F2 - merging diff archives
#
if echo $* | grep "F2" > /dev/null ; then
../modif_tree.sh $src2 "V"
GO "F2-1" 0 $ROUTINE_DEBUG $DAR -N -Q -c $diff2 -R $src2 -A $full2 -B $OPT
GO "F2-2" 0 $ROUTINE_DEBUG check_hash $hash $diff2.*.dar
GO "F2-3" 0 $ROUTINE_DEBUG $DAR -N -Q -+ $merge_diff -A $diff "-@" $diff2 -B $OPT
GO "F2-4" 0 $ROUTINE_DEBUG check_hash $hash $merge_diff.*.dar
GO "F2-5" 0 $ROUTINE_DEBUG $DAR -N -Q -t $merge_diff -B $OPT
mkdir $dst
GO "F2-6" 0 $ROUTINE_DEBUG $DAR -N -Q -x $full2 -R $dst -B $OPT
GO "F2-7" 0 $ROUTINE_DEBUG $DAR -N -Q -x $diff2 -R $dst -w -B $OPT
GO "F2-8" 0 $ROUTINE_DEBUG $DAR -N -Q -x $full -R $dst -w -B $OPT
GO "F2-9" 0 $ROUTINE_DEBUG $DAR -N -Q -x $diff -R $dst -w -B $OPT
GO "F2-A" 0,5 $ROUTINE_DEBUG $DAR -N -Q -d $merge_diff -R $dst --alter=do-not-compare-symlink-mtime -B $OPT
rm -rf $dst
mkdir $dst
GO "F2-B" 0 $ROUTINE_DEBUG $DAR -N -Q -x $merge_full -R $dst -B $OPT
GO "F2-C" 0 $ROUTINE_DEBUG $DAR -N -Q -x $merge_diff -R $dst -w -B $OPT
rm -rf $dst
fi

#
# F3 - decremental backup
#
if echo $* | grep "F3" > /dev/null ; then
rm -rf $src
mkdir $src
GO "F3-1" 0 $ROUTINE_DEBUG $DAR -N -Q -x $full -R $src -B $OPT
GO "F3-2" 0 $ROUTINE_DEBUG $DAR -N -Q -x $diff -R $src -B $OPT -w
GO "F3-3" 0 $ROUTINE_DEBUG $DAR -N -Q -c $full3 -R $src -B $OPT
GO "F3-4" 0 $ROUTINE_DEBUG $DAR -N -Q -+ $decr -A $full -@ $full3 -B $OPT -w -ad
rm -rf $src
mkdir $src
GO "F3-5" 0 $ROUTINE_DEBUG $DAR -N -Q -x $full -R $src -B $OPT
rm -rf $dst
mkdir $dst
GO "F3-6" 0 $ROUTINE_DEBUG $DAR -N -Q -x $full3 -R $dst -B $OPT
GO "F3-7" 0 $ROUTINE_DEBUG $DAR -N -Q -x $decr -R $dst -B $OPT -w
GO "F3-8" 0 $ROUTINE_DEBUG my_diff $src $dst
rm -rf $src $dst
fi

#
# F4 - merging with delta signature - without signature recomputation
#
if echo $* | grep "F4" > /dev/null ; then
GO "F4-1" 0 $ROUTINE_DEBUG $DAR -+ merge_delta -A full_delta -B $OPT --delta sig
fi

#
# F5 - merging with delta signature - with signature recomputation
#
if echo $* | grep "F5" > /dev/null ; then
rm $merge_delta*.*
GO "F5-1" 0 $ROUTINE_DEBUG $DAR -+ merge_delta -A full -B $OPT --delta sig --include-delta-sig "'*'" --delta-sig-min-size 1
fi


#
# G1 - using pipe to output the archive
#
rm -rf $src
../build_tree.sh $src "W"
if echo $* | grep "G1" > /dev/null ; then
GO "G1-1" 0 piped-out $DAR -N -Q -c - -R $src "-@" $piped_fly -B $OPT > $piped.$padded_one.dar
GO "G1-2" 0 $ROUTINE_DEBUG $DAR -N -Q -d $piped -R $src -B $OPT
GO "G1-3" 0 $ROUTINE_DEBUG $DAR -N -Q -d $piped -R $src -A $piped_fly -B $OPT
mkdir $dst
GO "G1-4" 0 $ROUTINE_DEBUG $DAR -N -Q -x $piped -R $dst -B $OPT
GO "G1-5" 0 $ROUTINE_DEBUG my_diff  $src $dst
GO "G1-6" 0 $ROUTINE_DEBUG $DAR -N -Q -d $piped -R $dst -B $OPT
rm -rf $dst
fi

#
# G2 - using dar_slave for reading
#
if echo $* | grep "G2" > /dev/null ; then
mkfifo $todar
mkfifo $toslave
GO "G2-1(a)" 0 $ROUTINE_DEBUG $DAR_SLAVE -Q -i $toslave -o $todar $slave_digits $piped &
GO "G2-1(b)" 0 $ROUTINE_DEBUG $DAR -N -Q -d - -R $src -B $OPT -i $todar -o $toslave
fi

#
# G3 - using pipe to read the archive
#
if echo $* | grep "G3" > /dev/null ; then
GO "G3-1" 0 piped-in $DAR -N -Q -d - -R $src -B $OPT < $piped.$padded_one.dar
fi

#
# H1 - modifying an archive with dar_xform
#
if echo $* | grep "H1" > /dev/null ; then
GO "H1-1" 0 $ROUTINE_DEBUG $DAR_XFORM -Q $hash_xform $xform_digits -s 1G $full $xformed1
GO "H1-2" 0 $ROUTINE_DEBUG check_hash $hash $xformed1.*.dar
GO "H1-3" 0 $ROUTINE_DEBUG $DAR -N -Q -t $xformed1 -B $OPT
GO "H1-4" 0 $ROUTINE_DEBUG $DAR -N -Q -t $xformed1 -A $catf_fly -B $OPT
GO "H1-5" 0 $ROUTINE_DEBUG $DAR_XFORM -Q $hash_xform $xform_digits $xformed1 $xformed2
GO "H1-6" 0 $ROUTINE_DEBUG check_hash $hash $xformed2.*.dar
GO "H1-7" 0 $ROUTINE_DEBUG $DAR -N -Q -t $xformed2 -A $catf_fly -B $OPT
if [ ! -z "$slicing" ] ; then
GO "H1-8" 0 $ROUTINE_DEBUG $DAR_XFORM -Q $hash_xform $slicing $xform_digits $xformed2 $xformed3
GO "H1-9" 0 $ROUTINE_DEBUG check_hash $hash $xformed3.*.dar
GO "H1-A" 0 $ROUTINE_DEBUG $DAR -N -Q -t $xformed3 -A $catf_fly -B $OPT
GO "H1-B" 0 $ROUTINE_DEBUG $DAR -N -Q -t $xformed3 -B $OPT
fi
fi

#
# I1 - SFTP repository
#
if echo $* | grep "I1" > /dev/null ; then
GO "I1-1" 0 $ROUTINE_DEBUG $DAR -N -Q -w -c $DAR_SFTP_REPO/$prefix-sftp-$full -R $src "-@" $prefix$catf_fly -B $OPT
GO "I1-2" 0 $ROUTINE_DEBUG $DAR -N -Q -l $DAR_SFTP_REPO/$prefix-sftp-$full -B $OPT
GO "I1-3" 0 $ROUTINE_DEBUG $DAR -N -Q -t $DAR_SFTP_REPO/$prefix-sftp-$full -B $OPT
../sftp_mdelete "$DAR_SFTP_REPO" "$prefix-sftp-*"
fi

#
# I2 - FTP repository
#
if echo $* | grep "I2" > /dev/null ; then
GO "I2-1" 0 $ROUTINE_DEBUG $DAR -N -Q -w -c $DAR_FTP_REPO/$prefix-ftp-$full -R $src "-@" $prefix$catf_fly -B $OPT
GO "I2-2" 0 $ROUTINE_DEBUG $DAR -N -Q -l $DAR_FTP_REPO/$prefix-ftp-$full -B $OPT
GO "I2-3" 0 $ROUTINE_DEBUG $DAR -N -Q -t $DAR_FTP_REPO/$prefix-ftp-$full -B $OPT
#../ftp_mdelete "$DAR_FTP_REPO" "$prefix-ftp-*"
../sftp_mdelete "$DAR_SFTP_REPO" "$prefix-ftp-*"
fi
###
# final cleanup
#

clean
