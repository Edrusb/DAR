#!/bin/bash

# Script Name: dar_backups.sh
# Author: Roi Rodriguez Mendez & Mauro Silvosa Rivera (Cluster Digital S.L.)
# Fixes by: Jason Lewis - jason at NO dickson SPAM dot st
# Description: dar_backups.sh is a script to be runned from cron which
#       backups data and stores it locally and optionally remote using scp.
#       It decides between doing a master or an incremental backup based
#       on the existance or not of a master one for the actual month.
# Revision History:
#       23.06.2008 - modified to work with latest version of dar which requires -g before each path to backup - Jason Lewis
#       24.10.2006 - changed script to do differential backups based on the last diff
#       18.10.2006 - added BACKUP_PATHS variable to simplify adding new paths
#                    Jason Lewis jason@NOdicksonSPAM.st
#       22.08.2005 - Creation


# Base directory where backups are to be stored
BASE_BAK_DIR=/backup


# base directory for files to backup. all paths for backing up are listed relative to this path
ROOT_DIR=/
# Paths to backup
# add paths here, in a space seperated list between round brackets.
# you can escape out spaces with \ or ''
# Paths should be relative to ROOT_DIR
#BACKUP_PATH=(my/first/path another\ path/with\ spaces 'yet another/path/with/spaces')
BACKUP_PATHS=( home user/lib/cgi-bin var/www/cgi-bin var/lib/cvs var/lib/svn var/lib/accounting mysql_backup usr/local/bin etc )

# Directory where backups for the actual month are stored (path relative to
# $BASE_BAK_DIR)
MONTHLY_BAK_DIR=`date -I | awk -F "-" '{ print $1"-"$2 }'`

# Variable de comprobacion de fecha
CURRENT_MONTH=$MONTHLY_BAK_DIR

# Name and path for the backup file.
SLICE_NAME=${BASE_BAK_DIR}/${MONTHLY_BAK_DIR}/backup_`date -I`

# Max backup file size
SLICE_SIZE=200M

# Remote backup settings
REMOTE_BAK="false"
REMOTE_HOST="example.com"
REMOTE_USR="bakusr"
REMOTE_BASE_DIR="/var/BACKUP/example.com/data"
REMOTE_MONTHLY_DIR=$MONTHLY_BAK_DIR
REMOTE_DIR=${REMOTE_BASE_DIR}/${REMOTE_MONTHLY_DIR}


########################################################
# you shouldn't need to edit anything below this line
#
#  STR='a,b,c'; paths=(${STR//,/ }); TEST=`echo ${paths[@]//#/-g }`;echo $TEST
# args=(); for x in "${paths[@]}"; do args+=(-g "$x"); done; program "${args[@]}"
#BACKUP_PATHS_STRING=`echo ${BACKUP_PATHS[@]//#/-g }`

args=()
for x in "${BACKUP_PATHS[@]}"; do args+=(-g "$x"); done;
BACKUP_PATHS_STRING="${args[@]}"
echo backup path string is "$BACKUP_PATHS_STRING"



## FUNCTIONS' DEFINITION
# Function which creates a master backup. It gets "true" as a parameter
# if the monthly directory has to be created.
function master_bak () {
    if [ "$1" == "true" ]
    then
	mkdir -p ${BASE_BAK_DIR}/${MONTHLY_BAK_DIR}
    fi

    /usr/bin/dar -m 256 -s $SLICE_SIZE -y -R $ROOT_DIR \
	$BACKUP_PATHS_STRING -c ${SLICE_NAME}_master #> /dev/null

    if [ "$REMOTE_BAK" == "true" ]
    then
	/usr/bin/ssh ${REMOTE_USR}@${REMOTE_HOST} "if [ ! -d ${REMOTE_DIR} ]; then mkdir -p $REMOTE_DIR; fi"
	for i in `ls ${SLICE_NAME}_master*.dar`
	do
	  /usr/bin/scp -C -p $i ${REMOTE_USR}@${REMOTE_HOST}:${REMOTE_DIR}/`basename $i` > /dev/null
	done
    fi
}

# Makes the incremental backups
function diff_bak () {
    MASTER=$1
    /usr/bin/dar -m 256 -s $SLICE_SIZE -y -R $ROOT_DIR \
        $BACKUP_PATHS_STRING -c ${SLICE_NAME}_diff \
	-A $MASTER #> /dev/null

    if [ "$REMOTE_BAK" == "true" ]
    then
	for i in `ls ${SLICE_NAME}_diff*.dar`
	do
	  /usr/bin/scp -C -p $i ${REMOTE_USR}@${REMOTE_HOST}:${REMOTE_DIR}/`basename $i` > /dev/null
	done
    fi
}

## MAIN FLUX
# Set appropriate umask value
umask 027

# Check for existing monthly backups directory
if [ ! -d ${BASE_BAK_DIR}/${MONTHLY_BAK_DIR} ]
then
    # If not, tell master_bak() to mkdir it
    master_bak "true"
else
    # Else:
    # MASTER not void if a master backup exists
    # original line to get the master backup does not take into account the diffs
    # MASTER=`ls ${BASE_BAK_DIR}/${MONTHLY_BAK_DIR}/*_master*.dar | tail -n 1 | awk -F "." '{ print $1 }'`
    # new master line gets the latest dar backup and uses that to make the diff
    MASTER=`ls -t ${BASE_BAK_DIR}/${MONTHLY_BAK_DIR}/*.dar | head -n 1 | awk -F "." '{ print $1 }'`
    # Check if a master backup already exists.
    if [ "${MASTER}" != "" ]
    then
	# If it exists, it's needed to make a differential one
	diff_bak $MASTER
    else
	# Else, do the master backup
	master_bak "false"
    fi
fi

