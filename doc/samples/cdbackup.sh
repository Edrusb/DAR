#!/bin/sh

#script for doing sliced full and incremental backups to cdr
#stef at hardco.de, 2003

#full backup: "cdbackup.sh full"
#incremental backup: "cdbackup.sh </path/referencearchivebasename>"

#Reference archive name is the filename of the first slice without .number.dar
#Dar will also search/ask for the last reference archive slice.
#A plain catalogue file can also be used as an incremental reference.

#backups everything starting from / (see DAR_PARAMS) to iso/rr cdrs
#Archive slices are stored temporarily in ./ (see TDIR) and get deleted
#if written successfully to cdr.
#The first cdr will also contain the dar_static executable.

#If anything goes wrong while trying to write to cdr, you can try again
#or keep the current archive slice as a file in ./ (see TDIR).
#For backing up to files only, simply accept the cdr write error and
#answer with 'keep file' (or even better: use dar directly).

#Slice size is for 700MB cdr blanks, see (and maybe change) DAR_PARAMS below.
#For (slow!) compression, add a -y or -z<num> parameter to DAR_PARAMS.

#The archive slice file names are:
#- for full backups: YYYY-MM-DD.<slice number>.dar
#- for incrementals: YYYY-MM-DD_YYYY-MM-DD.<slice number>.dar
#  The second date is the name of the reference archive, so you can end
#  up with names like YYYY-MM-DD_YYYY-MM-DD_YYYY-MM-DD_YYYY-MM-DD.1.dar
#  for a four level stacked incremental backup.

#Files which don't get backed up: (see DAR_PARAMS below)
#- the slice files of the current archive
#- the slice files of the reference archive
#- files called "darswap" (for manually adding more swap space for incrementals)
#- directory contents of /mnt, /cdrom, /proc, /dev/pts

#hints:
#- You need at least 700MB of free disk space in ./ (or in TDIR, if changed).
#- For incrementals, you need about 1KB of memory per tested file.
#  Create a large file "darswap" and add this as additional swap space.
#- If you are doing more than one backup per day, the filenames may interfere.
#- Carefully read the dar man page as well as the excellent TUTORIAL and NOTES.

#uncompressed, for 700MB cdr blanks:
DAR_PARAMS="-s 699M -S 691M -R / -P dev/pts -P proc -P mnt -P cdrom -D"

#temporary or target directory:
TDIR="."

#I'm using a USB CDR drive, so i don't know which 'scsi'-bus it is on.
#Cdrecord -scanbus is grepped for the following string:
DRIVENAME="PLEXTOR"

#Also because of USB i have to limit drive speed:
DRIVESPEED=4

#used external programs:
DAR_EXEC="/root/app/dar-1.3.0/dar"		#tested: dar-1.3.0
DAR_STATIC="/root/app/dar-1.3.0/dar_static"	#copied to the first cdr
MKISOFS="/root/app/cdrtools-2.0/bin/mkisofs"	#tested: cdrtools-2.0
CDRECORD="/root/app/cdrtools-2.0/bin/cdrecord"	#tested: cdrtools-2.0
GREP="/usr/bin/grep"				#tested: gnu grep 2.2
BASENAME="/usr/bin/basename"
DATECMD="/bin/date"
MKDIR="/bin/mkdir"
MV="/bin/mv"
CP="/bin/cp"
RM="/bin/rm"

#initial call of this script (just executes dar with the proper parameters):
DATE=`$DATECMD -I`
START=`$DATECMD`
if [ "$1" != "" ] && [ "$2" == "" ] ; then
	if [ "$1" == "full" ] ; then
		echo "starting full backup"
		$DAR_EXEC -c "$TDIR/$DATE" \
		-X "$DATE.*.dar" -X "darswap" \
		-N $DAR_PARAMS -E "$0 %p %b %N"
	else
		echo "starting incremental backup based on $1"
		LDATE=`$BASENAME $1`
		$DAR_EXEC -c "$TDIR/${DATE}_$LDATE" -A $1 \
		-X "${DATE}_$LDATE.*.dar" -X "$LDATE.*.dar" -X "darswap" \
		-N $DAR_PARAMS -E "$0 %p %b %N"
	fi
	echo "backup done"
	echo "start: $START"
	echo "end:   `$DATECMD`"

#called by dar's -E parameter after each slice:
elif [ -r "$1/$2.$3.dar" ] ; then

	echo -n "creating cdr $3 volume dir containing $2.$3.dar"
	$MKDIR "$1/$2.$3.cdr"
	$MV "$1/$2.$3.dar" "$1/$2.$3.cdr"
	if [ "$3" == "1" ] ; then
		echo -n " and dar_static"
		$CP $DAR_STATIC "$1/$2.$3.cdr"
	fi
	echo
	SCANBUS=`$CDRECORD  -scanbus 2>/dev/null | $GREP $DRIVENAME`
	DEV=${SCANBUS:1:5}
	CDBLOCKS=`$MKISOFS -R -print-size -quiet $1/$2.$3.cdr`
	echo "writing cdr $3 (${CDBLOCKS}s)..."
	KEEPFILE="n"
	until $MKISOFS -R "$1/$2.$3.cdr" | \
	$CDRECORD -eject -s dev=$DEV speed=$DRIVESPEED tsize=${CDBLOCKS}s -
	do
		read -p "write error, try [A]gain or [k]eep $2.$3.dar? " ERR
		if [ "$ERR" == "k" ] ; then
			KEEPFILE="y"
			break
		fi
	done
	if [ "$KEEPFILE" == "y" ] ; then
		echo "cdr not written, keeping $2.$3.dar as file"
		$MV "$1/$2.$3.cdr/$2.$3.dar" "$1/$2.$3.dar"
	fi
	echo "removing volume dir"
	$RM -rf "$1/$2.$3.cdr"
	echo "backup continues"
else
	echo "usage: $0 <full|/path/referencearchivebasename>"
fi

exit 0
