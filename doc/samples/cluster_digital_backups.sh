#!/bin/bash

# Script Name: dar_backups.sh
# Author: Roi Rodriguez Mendez & Mauro Silvosa Rivera (Cluster Digital S.L.)
# Description: dar_backups.sh is a script to be runned from cron which 
#       backups data and stores it locally and optionally remote using scp.
#       It decides between doing a master or an incremental backup based
#       on the existance or not of a master one for the actual month. The
#       remote copy feature needs a ssh authentication method which 
#       doesn't prompt for a password, in order to make it non-interactive
#       (useful for cron, if you plan to run it by hand, this is not
#       necessary).
# Version: 1.0
# Revision History: 
#       22.08.2005 - Creation


# Base directory where backups are stored
BASE_BAK_DIR=/var/BACKUP/data

# Directory where backups for the actual month are stored (path relative to
# $BASE_BAK_DIR)
MONTHLY_BAK_DIR=`date -I | awk -F "-" '{ print $1"-"$2 }'`

# Variable de comprobacion de fecha
CURRENT_MONTH=$MONTHLY_BAK_DIR

# Name and path for the backup file.
SLICE_NAME=${BASE_BAK_DIR}/${MONTHLY_BAK_DIR}/backup_`date -I`

# Max backup file size
SLICE_SIZE=100M

# Remote backup settings
REMOTE_BAK="true"
REMOTE_HOST="example.com"
REMOTE_USR="bakusr"
REMOTE_BASE_DIR="/var/BACKUP/example.com/data"
REMOTE_MONTHLY_DIR=$MONTHLY_BAK_DIR
REMOTE_DIR=${REMOTE_BASE_DIR}/${REMOTE_MONTHLY_DIR}


## FUNCTIONS' DEFINITION
# Function which creates a master backup. It gets "true" as a parameter
# if the monthly directory has to be created.
function master_bak () {
    if [ "$1" == "true" ]
    then
	mkdir -p ${BASE_BAK_DIR}/${MONTHLY_BAK_DIR}
    fi

    /usr/bin/dar -m 256 -s $SLICE_SIZE -y -R / \
	-g ./DATA -g ./home -g ./root -c ${SLICE_NAME}_master > /dev/null

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
    /usr/bin/dar -m 256 -s $SLICE_SIZE -y -R / \
	-g ./DATA -g ./home -g ./root -c ${SLICE_NAME}_diff \
	-A $MASTER > /dev/null

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
    MASTER=`ls ${BASE_BAK_DIR}/${MONTHLY_BAK_DIR}/*_master*.dar | tail -n 1 | awk -F "." '{ print $1 }'`
    
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
