#!/bin/bash

# ftpbackup.sh - Version 1.1 - 2006-01-09 - Patrick Nagel

# Carry out  backups automatically and put the resulting
# archive onto a backup FTP server. Mail the result to
# root.
#
# Dependencies: ncftp

# Change this to your needs ###########################

PASSWORDFILE="/root/ftpbackup.credentials"
#	$PASSWORDFILE must look like this (and should
#	of course only be readable for the user who
#	executes the script):
#       -----------------------------
#	|USER="username"            |
#	|PASS="password"            |
#	|SERVER="hostname.of.server"|
#	-----------------------------

LOGFILE="/root/ftpbackup.log"
#	The logfile will be gzipped and be available
#	as $LOGFILE.gz after the script exits.

NUMBEROFBACKUPS=2
#	How many different backups should this script
#	carry out?
BACKUPCOMMAND[1]="/root/backup-root.sh"
#	Backup command which carries out 1st backup.
#	Each backup command must create exactly ONE
#	archive file.
BACKUPCOMMAND[2]="/root/backup-storage.sh"
#	Backup command which carries out 2nd backup.
BACKUPCOMMAND[n]=""
#	Backup command which carries out nth backup.

LOCALBACKUPDIR="/mnt/storage/backup"
#	This is where the backup archive (must be ONE
#	FILE!) will be stored by the $BACKUPCOMMAND[x]
#	program.
MOUNTPOINT="/mnt/storage"
#	The mountpoint of the partition where the
#	backup archives will be stored on.
#	For free space statistics.

BACKUPFTPQUOTA=42949672960
#	Backup FTP server quota or total storage amount
#	(in bytes).

#######################################################


# Initial variables and checks

which ncftp &>/dev/null || { echo "Missing ncftp, which is a dependency of this script."; exit 1; }
STARTTIME="$(date +%T)"


# Functions

function backup_to_ftp_start() {
	ncftpbatch -D
	return
}

function backup_to_ftp_queue() {
	# Puts newest file in ${LOCALBACKUPDIR} to the backup FTP server.
	source ${PASSWORDFILE}
	BACKUPFILE="${LOCALBACKUPDIR}/$(ls -t -1 ${LOCALBACKUPDIR} | head -n 1)"
	ncftpput -bb -u ${USER} -p ${PASS} ${SERVER} / ${BACKUPFILE}
	return
}

function backup_local_used() {
	du -bs ${LOCALBACKUPDIR} | awk '{printf($1)}'
	return
}

function backup_local_free() {
	df -B 1 --sync ${MOUNTPOINT} | tail -n 1 | awk '{printf($4)}'
	return
}

function backup_ftp_used() {
	source ${PASSWORDFILE}
	ncftpls -l -u ${USER} -p ${PASS} ftp://${SERVER} | grep -- '^-' | echo -n $(($(awk '{printf("%i+", $5)}'; echo "0")))
	return
}

function backup_ftp_free() {
	echo -n $((${BACKUPFTPQUOTA} - $(backup_ftp_used)))
	return
}

function backup_success() {
	{ echo -en "Backup succeeded.\n\nBackup started at ${STARTTIME} and ended at $(date +%T).\n\n"
	  echo -en "Statistics after backup (all numbers in bytes):\n"
	  echo -en "Used on Backup-FTP:             $(backup_ftp_used)\n"
	  echo -en "Free on Backup-FTP:             $(backup_ftp_free)\n"
	  echo -en "Used on local backup directory: $(backup_local_used)\n"
	  echo -en "Free on local backup directory: $(backup_local_free)\n"
	} | mail -s "Backup succeeded" root
	return
}

function backup_failure_exit() {
	{ echo -en "Backup failed!\n\nBackup started at ${STARTTIME} and ended at $(date +%T).\n\n"
	  echo -en "Statistics after backup failure (all numbers in bytes):\n"
	  echo -en "Used on Backup-FTP:             $(backup_ftp_used)\n"
	  echo -en "Free on Backup-FTP:             $(backup_ftp_free)\n"
	  echo -en "Used on local backup directory: $(backup_local_used)\n"
	  echo -en "Free on local backup directory: $(backup_local_free)\n"
	} | mail -s "Backup FAILED" root
	gzip -f ${LOGFILE}
	exit 1
}


# Main

rm -f ${LOGFILE}
# In case the script has been aborted before

{ for ((i=1; i<=${NUMBEROFBACKUPS}; i+=1)); do
  	${BACKUPCOMMAND[$i]} >>${LOGFILE} 2>&1 && backup_to_ftp_queue >>${LOGFILE} 2>&1
  done && \
  backup_to_ftp_start >>${LOGFILE} 2>&1 && \
  backup_success
} || backup_failure_exit

gzip -f ${LOGFILE}
