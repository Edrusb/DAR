#!/bin/sh

#################################
#				#
#      DAR Archiver script	#
#				#
#################################

# Jakub Holy 25.4.2005
# This file: $HOME/bin/dar-make_user_backup.sh
# IMPORTANT: This script depends upon /etc/darrc (options what not to compress/ archive)
#            But the file is ignored if $HOME/.darrc exists.
#	     Additional options are read from dar_archiver.options (see 
#	     $DAR_OPTIONS_FILE below)


USAGE="echo -e USAGE: \n$0 -full | -inc"

# ----------------------------- OPTIONS TO MODIFY

DIR_TO_ARCHIVE=$HOME

DEST_DIR=/mnt/mandrake/debian-bkp/
DAR_OPTIONS_FILE="$HOME/bin/dar_archiver.options"
ARCHIVE_NAME="`/bin/date -I`_$USER"		  # Ex: 2005-04-25_jholy

DAR_INFO_DIR="$HOME/backup"
DAR_MANAGER_DB=${DAR_INFO_DIR}/dar_manager_database.dmd

LAST_FULL_BACKUP_ID="2005-04-25"  # The last full backup - the unique part of its name
LAST_FULL_BACKUP=${DAR_INFO_DIR}/${LAST_FULL_BACKUP_ID}_aja-full-katalog

MSG=""
LOG_FILE="${DAR_INFO_DIR}/zaloha-aja-dar.log"

# PARSE COMMAND LINE ---------------------------------------------
INC_BKP_OPT=""		# dar options needed to create an incremental backup: empty => full bkp
if [ $# -ne 1 ]; then
  echo "ERROR: Wrong number of parameters"
  $USAGE
  exit 1
elif [ "X$1" != "X-full" -a "X$1" != "X-inc" ]; then
  echo "Unknown parameter"
  $USAGE
  exit 1
else
  if [ "X$1" = "X-full" ]; then 
     echo "DAR: Doing FULL backup."; 
     ARCHIVE_NAME="${ARCHIVE_NAME}-full"
  fi
  if [ "X$1" = "X-inc" ]; then 
     echo "DAR: Doing INCREMENTAL backup with respect to $LAST_FULL_BACKUP."; 
     INC_BKP_OPT=" -A $LAST_FULL_BACKUP "
     ARCHIVE_NAME="${ARCHIVE_NAME}-inc-wrt${LAST_FULL_BACKUP_ID}"
  fi
  echo "See the log in $LOG_FILE"
fi

# ----------------------------- OPTIONS CONT'D

ARCHIVE=${DEST_DIR}/${ARCHIVE_NAME}

CATALOGUE=${DAR_INFO_DIR}/${ARCHIVE_NAME}-katalog

echo "-----------------------" >> "$LOG_FILE"

# -m N 		- soubory pod N [B] nejsou komprimovany
# -Z pattern	- soub. odpovidajici vzoru nejsou komprimovany
# -P subdir	- adresare kt. se nezalohuji; relativni w.r.t. -R
# -X pattern	- exclude files matching pattern; nesmi tam byt file path 
# -R /home/aja	- adresar, ktery zalohujeme
# -s 700M	- na jak velke kusy se archiv rozseka
# -y [level]	- proved bzip2 kompresi
# -c `date -I`_bkp	- vystupni archiv (pribude pripona .dar)
# -G 		- generuj zvlast katalog archivu 
# -D,--empty-dir - vtvor prazdne adresare pro ty excludovane (s -P)
# -M		- skip other filesystems (tj. namountovane FS).
# -v		- verbose output
# --beep	- pipni kdyz je pozadovana uzivatelova akce
# -A basename   - vytvor incremental backupwrt archive se zakladem jmena 'basename'
#		Misto archivu lze pouzit i catalog.
# Soubory kt. nelze komprimovat (upper i lower case):
# bz2 deb ear gif GIF gpg gz chm jar jpeg jpg obj pdf png rar rnd scm svgz swf 
# tar tbz2 tgz tif tiff vlt war wings xpi Z zargo zip trezor

COMMAND="dar -c $ARCHIVE -R $DIR_TO_ARCHIVE -B $DAR_OPTIONS_FILE $INC_BKP_OPT"

echo "Backup started at: `date`" >>  "$LOG_FILE"
echo "Making backup into $ARCHIVE; command: $COMMAND"  >>  "$LOG_FILE"
echo "Making backup into $ARCHIVE; command: $COMMAND"

### ARCHIVACE -----------------------------------------------------------------

$COMMAND		# Perform the archive command itself
RESULT=$?		# Get its return value ( 0 == ok)

### TEST THE OUTCOME
if [ $RESULT -eq 0 ];   
then 
   ## Check the archive ........................................................
   echo "Backup done at: `date`. Going to test the archive." >>  "$LOG_FILE"
   echo "Backup done at: `date`. Going to test the archive."
   if dar -t $ARCHIVE # > /dev/null # to ignore stdout in cron uncomment this
   then MSG="Archive created & successfully tessted."; 
   else MSG="Archive created but the test FAILED";
   fi
   echo "Test of the archive done at: `date`." >>  "$LOG_FILE"
   echo "Test of the archive done at: `date`."
else
   MSG="The backup FAILED (error code $RESULT)"
   echo -e "$MSG" "\nEnded at: `date` \n">>  "$LOG_FILE"
   echo "$MSG"
   exit 1
fi

### KATALOG - import into the manager ............................................
echo "Going to create a catalogue of the archive..." >>  "$LOG_FILE"
echo "Going to create a catalogue of the archive..."

dar -C "$CATALOGUE" -A "$ARCHIVE"
dar_manager -v -B "$DAR_MANAGER_DB" -A "$ARCHIVE"

echo "The catalogue created in $CATALOGUE and imported into the base $DAR_MANAGER_DB" >>  "$LOG_FILE"
echo "The catalogue created in $CATALOGUE and imported into the base $DAR_MANAGER_DB"

echo -e "$MSG" "\nEnded at: `date` \n">>  "$LOG_FILE"
echo "$MSG"

### Incremental backup
# -A dar_archive  - specifies a former backup as a base for this incremental backup
# 	Ex: dar ... -A a_full_backup  # there's no '<slice's number>.dar', only the archive's basename 
#	Note: instead of the origina dar_archive we can use its calatogue

### Extract the catalogue from a backup
# Ex: dar -A existing_dar_archive -C create_catalog_file_basename
