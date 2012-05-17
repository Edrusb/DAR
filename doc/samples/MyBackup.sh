#! /bin/bash

# (best viewed with tabs every 4 characters)

#   Copyright (C) 2009 Charles
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, US

# Filename: MyBackup.sh
	prg_ver='0.2'

# Purpose: 
#	Runs backups using dar and, if possible, LVM snapshots

# Usage: 
#	See usage function below (search for "function usage").
	
# Environment:
#	Developed and tested on ubuntu 8.04.2 desktop 32-bit with
#	* bash 3.2-0ubuntu16
#	* dar and dar-static 2.3.5-1
# 	* dvd+rw-tools 7.0-9ubuntu1
#	* gnome-volume-manager 2.22.1-1ubuntu6

# History:
#	28jun08	Charles
#			* Begun!
#	2aug8	Charles
#			* Added ability to back up directory trees on non-LVM volumes.
#	9feb9	Charles
#			* Moved snapshot unmount and lvremove into function and call as soon as snapshot no longer needed.
#			* Introduced cumulative error message in finalise function so it does not give up on first error.
#	26mar9	Charles
#			* Introduced use of script command to both allow output from several commands to go to screen as usual and capture it for analysis and logging..
#			* Modified call_msg() and added -n option to msg() to allow suppressing on-screen message.
#			* Added command line option to override config file's DVD device.
#	1apr9	Charles
#			* Cleaned  characters from command screen output captured and written to log.
#			* Added copyright and licence.
#			* Added removal of temporary DVD mount directory.
#			* Added log file name to final screen message.
#			* Changed dar --create scren output analysis to cope with more data than bash can handle.
#			* Added options to msg() to suppress writing log, screen and timestamp in log.
#			* Added progress indicator when awk analysing dar --create output.

# Wishlist (in approx descending order of importance):
#	* Single tmp directory (using mktemp -d) for all tmp files (default under /var/tmp, optionally changeable in config file) then single, unconditional, rm -fr in finalise)
#	* Add dar_cp?
#	* Add Partridge?
#	* Add differential backups.
#	* Use awk to build prune list (too slow in bash when list is long) or show progress indicator.
#	* Introduce Warning (extra to Information and Error. Backups should keep going if possible).
#	* Add log file age-out
#	* Remove (incomplete) provisions for non-interactive running.
#	* When waiting for DVD load, sit in a delayed loop looking for it so user does not have to press Enter.
#	* Do not format if not DVD[+-]RW.  This info can be got in ck_DVD_loaded().  Not urgent because the format command fails gracefully.
#	* For "interactive" test, change to standard test of checking PS1
#	* Allow more than one "Direct directory tree" and default to adding /etc/lvm if it is present (crudely hardcoded for now)
#	* If "Direct directory tree"s (inc /etc/lvm if it is hardcoded) are on LVM then use LVM snapshot when writing them to DVD
#	* Add support for messages to user, e.g. to remind to quiesce something before backing up.
#	* Add verbosity option including dar --verbose.
#	* Allow for no writing to DVD (suit running unattended and for differentials when they come. Use -s no_DVD?).
#	* Handle screen output from growisofs nicely -- show the user a single countdown line.
#	* Default directories when not root under ~/.$prgnam to suit personal use on multi-user system. No snapshots!
#	* Config file additions: size of LVM snapshot.
#	* More error trapping in parse_cfg() inc. check DVD device is OK (function so can call when parsing comand line too).
# 	* Allow tabs and spaces in prune and exclude lists and in name of "Directory tree (fs_root)".

# Programmers' notes: error and trap handling: 
#   * All errors are fatal and finalise() is called.
#   * At any time, a trapped event may transfer control to finalise().
#   * To allow finalise() to tidy up before exiting, changes that need to be undone are
#     noted with global variables named <change name>_flag and the data required
#     to undo those changes is kept in global variables.
#	* finalise() uses the same functions to undo the changes as are used when they are
#	  undone routinely.
#	* $finalising_flag is used to prevent recursive calls when errors are encountered while finalise() is running,

# Programmers' notes: logical volume names
#   * Block device names are /dev/mapper/<volume group>-<logical volume>. 
#   * LVM commands require /dev/<volume group>/<logical volume> names and these are provided as symlinks to the block device names.
#   * For consistency this script uses the names required by LVM commands except when an actual block device is essential.

# Programmers' notes: variable naming and value usage
#	* Directory names called *_dir and given value endig in /
#	* Logic flags called *_flag and given values YES or NO

# Programmers' notes: function call tree
#	+
#	|
#	+-- initialise
#	|   |
#	|   +-- usage
#	|   |
#	|   +-- parse_cfg
#	|
#	+-- back_up_dirtrees
#	|   |
#	|   +-- get_dev_afn
#	|   |
#	|   +-- set_up_snapshot
#	|   |
#	|   +-- do_dar_create
#	|   |
#	|   +-- do_dar_diff
#	|   |
#	|   +-- drop_snapshot
#	|
#	+-- copy_to_DVD
#	|   |
#	|   +-- ck_DVD_loaded
#	|   |
#	|   +-- format_DVD
#	|
#	+-- finalise
#	    |
#	    +-- drop_snapshot
#
# Utility functions called from various places:
# 	call_msg ck_dir ck_file msg report_dar_retval

# Function definitions in alphabetical order.  Execution begins after the last function definition

#--------------------------
# Name: back_up_dirtrees
# Purpose: Does the dar jobs
# Usage: back_up_dirtrees 
#--------------------------
function back_up_dirtrees {

	fct "${FUNCNAME[ 0 ]}" 'started'

    local idx

	idx=1
	while [[ $idx -le "$n_dar_jobs" ]]
	do
		get_dev_afn $idx
		if [[ ${dev_afn#/dev/mapper/} != $dev_afn ]]; then
	   		call_msg 'I' "Directory tree to be backed up, ${cfg_fs_roots[ $idx ]}, is on logical volume '$dev_afn'.  Using LVM snapshot"
			set_up_snapshot $idx
		else
			snapshot_mnt_points[ $idx ]=""
		fi
		do_dar_create $idx
		do_dar_diff $idx
		if [[ ${snapshot_vol_mounted_flag[ $idx ]:-} = 'YES' ]]; then
			drop_snapshot $idx
		fi
		: $(( idx++ ))
	done

	fct "${FUNCNAME[ 0 ]}" 'returning'

}  # end of back_up_dirtrees

#--------------------------
# Name: call_msg
# Purpose: calls msg() and, for error messges, if finalise() has not been called, calls it
# $1 - message class
# $2 - message
#--------------------------
function call_msg {

   	msg -l $log_afn "$@"

	if [[ $1 = 'E' ]]; then
    	if [[ $interactive_flag = 'YES' ]]; then
			press_enter_to_continue 				# Ensure user has chance to see error message
		fi
		if [[ ${finalising_flag:-} != 'YES' ]]; then
			finalise 1
		fi
	fi

	return 0

}  # end of function call_msg

#--------------------------
# Name: ck_DVD_loaded
# Purpose: Checks whether a recordable or rewriteable DVD is loaded
# Usage: ck_DVD_loaded 
# Return value:  0 if recordable or rewriteable DVD is loaded, 1 otherwise
#--------------------------
function ck_DVD_loaded {

	fct "${FUNCNAME[ 0 ]}" 'started'

    local buf test

	buf="$( $dvd_rw_mediainfo "$DVD_dev" 2>&1 )"
	test="${buf##*Mounted Media:*( )}"			# strip up to mounted media data 
    test="${test%%$lf*}"							# strip remaining lines
	case "$test" in
		*'DVD'[+-]'R'* )
			call_msg 'I' "Media info:$lf$( echo "$buf" | $grep 'Media' )"
			fct "${FUNCNAME[ 0 ]}" 'returning 0'
			return 0	
			;;
	esac

	fct "${FUNCNAME[ 0 ]}" 'returning 1'
	return 1

}  # end of ck_DVD_loaded

#--------------------------
# Name: ck_file
# Purpose: for each file listed in the argument list: checks that it is 
#   reachable, exists and that the user has the listed permissions
# Usage: ck_file [ file_name:<filetype><permission>... ]
#	where 
#		file_name is a file name
#		type is f (file) or d (directoy)
#		permissions are one or more of r, w and x.  For example rx
#--------------------------
function ck_file {

    local buf perm perms retval filetype

    # For each file ...
    # ~~~~~~~~~~~~~~~~~
    retval=0
    while [[ ${#} -gt 0 ]]
    do
	    file=${1%:*}
	    buf=${1#*:}
		filetype="${buf:0:1}"
	    perms="${buf:1:1}"
	    shift

	    if [[ $file = $perms ]]; then
		    echo "$prgnam: ck_file: no permisssions requested for file '$file'" >&2 
		    return 1
	    fi

	    # Is the file reachable and does it exist?
	    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		case $filetype in
			f )
	    		if [ ! -f $file ]; then			# using [ rather than [[ because any embedded spaces in the filename are \ escaped
					echo "file '$file' is unreachable, does not exist or is not an ordinary file" >&2
					: $(( retval=retval+1 ))
					continue
	   		 	fi
				;;
			d )
	    		if [ ! -d $file ]; then			# using [ rather than [[ because any embedded spaces in the directory name are \ escaped
					echo "directory '$file' is unreachable, does not exist or is not a directory" >&2
					: $(( retval=retval+1 ))
					continue
	   		 	fi
				;;
			* )
		    	echo "$prgnam: ck_file: invalid filetype '$filetype' specified for file '$file'" >&2 
		    	return 1
		esac

	    # Does the file have the requested permissions?
	    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	    buf="$perms"
	    while [[ $buf != '' ]]
	    do
		    perm="${buf:0:1}"
		    buf="${buf:1}"
            case $perm in
	    		r )	
	    		    if [[ ! -r $file ]]; then
	    		        echo "file '$file' does not have requested read permission" >&2
	    		        let retval=retval+11G
	    		        continue
	    		    fi
                    ;;
		    	w )
		    		if [[ ! -w $file ]]; then
		    			echo "file '$file' does not have requested write permission" >&2
		    			let retval=retval+1
		    			continue
		    		fi
		    		;;
		    	x )
		    		if [[ ! -x $file ]]; then
		    			echo "file '$file' does not have requested execute permission" >&2
		    			let retval=retval+1
		    			continue
		    		fi
		    		;;
		    	* )
		    		echo "$prgnam: ck_file: unexpected permisssion '$perm' requested for file '$file'" >&2
		    		return 1
		    esac
	    done

    done

return $retval

}  #  end of function ck_file

#--------------------------
# Name: ck_dir
# Purpose: for each directory listed in the argument list: checks that it is 
#   reachable, exists and that the user has the listed permissions
# Usage: ck_dir [ dir_name:permissions ... ]
#	where 
#		dir_name is a directory name
#		permissions are none or more of r, w and x.  For example rx
#--------------------------
function ck_dir {

	local buf dir perm perms retval
	
	# For each directory...
	# ~~~~~~~~~~~~~~~~~~~~~
	retval=0
	while [[ $# -gt 0 ]]
	do
		dir=${1%:*}
		perms=${1#*:}
		shift
	
		if [[ $dir = $perms ]]
		then
			echo "$prgnam: ck_dir: no permisssions requested for directory '$dir'" >&2
			\exit 1
		fi
	
		# Does the directory exist and is it reachable?
		# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		if [[ ! -d $dir ]]
		then
		 	echo "directory '$dir' does not exist or is unreachable" >&2
			let retval=retval+1
			continue
		fi
	
		# Does the directory have the requested permissions?
		# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		while [[ $perms != '' ]]
		do
			perm="${perms:0:1}"
			perms="${perms:1}"
			case $perm in 
				r )
					if [[ ! -r $dir ]]
					then
		 				echo "directory '$dir' does not have requested read permission" >&2
						let retval=retval+1
						continue
					fi
					;;
				w )
					if [[ ! -w $dir ]]
					then
		 				echo "directory '$dir' does not have requested write permission" >&2
						let retval=retval+1
						continue
					fi
					;;
				x )
					if [[ ! -x $dir ]]
					then
		 				echo "directory '$dir' does not have requested search (x) permission" >&2
						let retval=retval+1
						continue
					fi
					;;
				* )
					echo "$prgnam: ck_dir: unexpected permisssion '$perm' requested for directory '$dir'" >&2
					\exit 1
			esac
		done
	
	done
	
	return $retval

}  #  end of function ck_dir

#--------------------------
# Name: copy_to_DVD
# Purpose: copies on-disk backups to DVD
# Usage: copy_to_DVD 
#--------------------------
function copy_to_DVD {

	fct "${FUNCNAME[ 0 ]}" 'started'

    local buf cksum_out error_flag file_list fn i log_msg msg retval

	# Get DVD loaded
	# ~~~~~~~~~~~~~~
	until ck_DVD_loaded
	do
		if [[ $interactive_flag = 'NO' ]]; then
			call_msg 'E' 'Not running interactively and no DVD already loaded'
		fi
		case "$skip_to" in
			'' | 'DVD_write' )
				msg='Please load a recordable or rewritable DVD and press enter to continue'
				;;
			'DVD_verify' )
				msg='Please load the DVD already recorded for this backup job and press enter to continue'
				;;
			* )
				call_msg 'E' "Programming error: unexpected skip_to value, '$skip_to'"
				;;
		esac
		echo "$msg (or Q to quit)" > /dev/tty
		read
		case "$REPLY" in
			'q' | 'Q' )
				call_msg 'I' 'Program terminated by user quitting when asked to load a DVD'
				finalise 0
		esac
		echo 'Continuing ...'
	done

	# Remove it from HAL's/gnome-volume-manager's automount
	# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	$sleep 5										# Intended to be long enough for any automounting system such as gnome-volume-manager to do its thing.
	buf="$( "$umount" "$DVD_dev" 2>&1 )"			# umount output is ignored because DVD may not be automounted and, if it is, the best action is to continue anyway

	call_msg 'I' "DEBUG for https://bugs.launchpad.net/ubuntu/+source/linux/+bug/228624: When DVD loaded (and unmounted), /sys/block/sr0/size contains: $(/bin/cat /sys/block/sr0/size)"

	# Copy files to DVD
	# ~~~~~~~~~~~~~~~~~
	if [[ $skip_to != 'DVD_verify' ]]; then

		# Format DVD
		# ~~~~~~~~~~
		format_DVD

		# Build list of files to copy to DVD
		# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		file_list="$dar_static LVM=/etc/lvm"
		if [[ ${drct_dir_tree:-} != '' ]]; then
			buf="${drct_dir_tree##*/}"
			file_list="$file_list $buf=$drct_dir_tree"
		fi
		i=1
		while [[ $i -le "$n_dar_jobs" ]]
		do
			file_list="$file_list ${bk_dir}${basenames[ $i ]}.1.dar "
			: $(( i++ ))
		done

		# Write shellscript to copy files to DVD
		# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		buf="#! /bin/bash$lf $growisofs -Z $DVD_dev -speed=1 -iso-level 4 -r -graft-points $file_list$lf# End of script"
		echo "$buf" > $tmp_sh_afn 	
		call_msg 'I' "Running on-the-fly script to copy files to DVD.  Script is:$lf$buf"
	
		# Run the shellscript, capturing its screen output in a temporary file
		# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		"$script" '-c' "$tmp_sh_afn" '-q' '-f' "$tmp_afn"
	
		# Analyse captured screen output
		# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		error_flag='NO'
		log_msg=''
		exec 3< $tmp_afn 												# Set up the temp file for reading on file descriptor 3
		while read -u 3 buf												# For each line of the script output to screen
		do
			buf="${buf%}"
			case "$buf" in
				':-('* \
				| 'File'*'is larger than'* \
				| '-allow-limited-size was not specified'* )			# Error
					error_flag='YES'
					log_msg="$log_msg$lf$buf"
					;;
				'Executing'* \
				| 'Warning: Creating ISO-9660:'* \
				| 'Warning: ISO-9660 filenames'* \
				| *'Current Write Speed'* \
				| 'Total'* \
				| 'Path table'* \
				| 'Max brk space'* \
				| *'extents written'* \
				| 'builtin_dd:'* \
				| *'flushing cache'* \
				| *'stopping de-icing'* \
				| *'writing lead-out'* \
				) 														# Normal output
					log_msg="$log_msg$lf$buf"
					;;
				*'done, estimate finish'* )								# Progress reporting -- not required in log
					;;
				* )														# Unexpected output
					error_flag='YES'
					log_msg="$log_msg${lf}$prgnam identified unexpected output: $buf"
					;;
			esac
	
		done
		exec 3<&- 														# Free file descriptor 3
	
		if [[ $error_flag = 'NO' ]]; then 
			call_msg -n s 'I' "Files copied to DVD OK:$log_msg"
		else
			call_msg 'E' "Unexpected output when copying files to DVD:$log_msg"
		fi
		call_msg 'I' "DEBUG for https://bugs.launchpad.net/ubuntu/+source/linux/+bug/228624: After copying files to DVD, /sys/block/sr0/size contains: $(/bin/cat /sys/block/sr0/size)" 

	fi

	# Mount DVD
	# ~~~~~~~~~
	buf="$($mkdir -p "$DVD_mnt_dir" 2>&1)"
	if [[ $? -ne 0 ]]; then
  	 	call_msg 'E' "Unable to establish DVD mount point directory '$DVD_mnt_dir'.  $mkdir output was$lf$buf" 
	fi
	DVD_mnt_dir_created_flag='YES'
	$umount $DVD_dev >/dev/null 2>&1 	# crude and precautionary
	buf="$( $mount -o ro -t iso9660 $DVD_dev $DVD_mnt_dir 2>&1 )"		# Do not rely on file system type auto-deetection
	if [[ $buf != '' ]]; then
		call_msg 'E' "unable to mount DVD $DVD_dev at temporary mount point $DVD_mnt_dir: $buf"
	fi
	DVD_mounted_flag='YES'
	call_msg 'I' "DEBUG for https://bugs.launchpad.net/ubuntu/+source/linux/+bug/228624: After mounting DVD, /sys/block/sr0/size contains: $(/bin/cat /sys/block/sr0/size)" 

	# Compare dar backup file(s) on DVD against on-disk versions
	# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	err_flag='NO'
	i=1
	while [[ $i -le "$n_dar_jobs" ]]
	do
		fn="${basenames[ $i ]}.1.dar"

		# Checksum on-DVD backup
		# ~~~~~~~~~~~~~~~~~~~~~~
		# Do this first because it is the most error prone
		buf="$( $cksum $DVD_mnt_dir$fn 2>&1 )"
		retval=$?
		if [[ $retval -ne 0 ]]; then
			call_msg 'E' "unable to checksum $DVD_mnt_dir$fn.  Output was:$lf$buf" 
		else
			cksum_out="${buf%% /*}" # strip file name
			call_msg 'I' "checksum of ${DVD_mnt_dir}$fn is: $cksum_out"
		fi
		
		# Checksum on-disk backup
		# ~~~~~~~~~~~~~~~~~~~~~~~
		buf="$( $cksum $bk_dir$fn 2>&1 )"
		retval=$?
		if [[ $retval -ne 0 ]]; then
			call_msg 'E' "unable to checksum $bk_dir$fn.  Output was:$lf$buf" 
		else
			buf="${buf%% /*}" # strip file name
			call_msg 'I' "checksum of ${bk_dir}$fn is: $buf"
		fi

		# Compare check sums
		# ~~~~~~~~~~~~~~~~~~
		if [[ $buf = $cksum_out ]]; then
			call_msg 'I' "dar file on DVD is the same as dar file on disk"
		else
			call_msg 'W' "dar file on DVD not the same as dar file on disk"
			err_flag='YES'
		fi

		: $(( i++ ))
	done

	if [[ $err_flag != 'NO' ]]; then
		call_msg 'E' "one or more dar files on DVD not same as on disk (see above)"
	fi

	fct "${FUNCNAME[ 0 ]}" 'returning'

}  # end of copy_to_DVD

#--------------------------
# Name: do_dar_create
# Purpose: runs a single dar job
# Usage: do_dar_create idx
#	where 
#		idx is the index to a set of arrays of dar create job parameters
#--------------------------
function do_dar_create {

	fct "${FUNCNAME[ 0 ]}" "started with idx $1"

    local buf idx dar_cmd dir error_flag exclude_list log_msg log_msg_idx_max mask cfg_fs_root mnt prune_list retval dar_fs_root snapshot_mnt_point warning_flag

	idx="$1"

	# Build fs_root for use on dar command
	# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	cfg_fs_root="${cfg_fs_roots[ $idx ]}"				# fs_root as specified in the config file
	cfg_fs_root="${cfg_fs_root%%*(/)}/"					# ensure single trailing /
	if [[ ${snapshot_mnt_points[ $idx ]} = "" ]]; then
		# dirtree to back up is not on LV
		dar_fs_root="$cfg_fs_root"
	else
		# dirtree to back up is on LV
		buf="$( $df "$cfg_fs_root" 2>&1 )"				# No error trap because same command ran OK in get_dev_afn()
		mnt="${buf##* }"								# live file system mount point
		buf="${cfg_fs_root#$mnt/}"						# directory tree relative to mount point
		snapshot_mnt_point="${snapshot_mnt_points[ $idx ]}"
		dar_fs_root="${snapshot_mnt_point}${buf%/}"		# the %/ is necessary when fs_root is /
	fi
	dar_fs_root="${dar_fs_root// /\\ }"					# Escape any embedded spaces
	dar_fs_roots[ $idx ]="$dar_fs_root"

	# Build prune list
	# ~~~~~~~~~~~~~~~~
	buf="${prunes[ $idx ]##*( )}"
	prune_list=
	while [[ $buf != '' ]]
	do
		dir="${buf%%[ 	]*}"							# strip all after first space or tab
		buf="${buf#"$dir"}"								# remove directory just taken
		prune_list="$prune_list -P $dir "
		buf="${buf##*([ 	])}"						# strip leading spaces and tabs
	done

	# Build exclude list
	# ~~~~~~~~~~~~~~~~~~
	buf="${excludes[ $idx ]##*( )}"
	exclude_list=
	while [[ $buf != '' ]]
	do
		mask="${buf%%[ 	]*}"							# strip all after first space or tab
		buf="${buf#"$mask"}"							# remove mask just taken
		exclude_list="$exclude_list -X $mask "
		buf="${buf##*([ 	])}"						# strip leading spaces and tabs
	done

	# Build dar create command
	# ~~~~~~~~~~~~~~~~~~~~~~~~
	dar_cmd="$( echo \
	$dar --create "${bk_dir}${basenames[ $idx ]}" --fs-root $dar_fs_root \
    --alter=atime --empty-dir --mincompr 256 --noconf --no-warn --verbose \
    --prune lost+found \
    $prune_list \
    $exclude_list \
    --gzip=5 --alter=no-case \
	-Z '*.asf' \
	-Z '*.avi' \
	-Z '*.bzip' \
	-Z '*.bzip2' \
	-Z '*.divx' \
	-Z '*.gif' \
	-Z '*.gzip' \
	-Z '*.jpeg' \
	-Z '*.jpg' \
	-Z '*.mp3' \
	-Z '*.mpeg' \
	-Z '*.mpg' \
	-Z '*.png' \
	-Z '*.ra' \
	-Z '*.rar' \
	-Z '*.rm' \
	-Z '*.tgz' \
	-Z '*.wma' \
	-Z '*.wmv' \
	-Z '*.Z' \
	-Z '*.zip' \
	)"

	# Write a shellscript to run dar --create
	# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	buf="#! /bin/bash$lf$dar_cmd${lf}echo $dar_retval_preamble\$?$lf# End of script" > $tmp_sh_afn 	
	echo "$buf" > $tmp_sh_afn 	

	# Run the shellscript, capturing its screen output in a temporary file
	# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	call_msg 'I' "Running on-the-fly script to run dar backup.  Script is:$lf$buf"
	"$script" '-c' "$tmp_sh_afn" '-q' '-f' "$tmp_afn"

	# Analyse captured screen output
	# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	# When the dar command produces a lot of output, it takes too long to process in bash so awk is used instead.
	# The awk program below analyses the dar screen output and writes bash variable assignment statements which are actioned by eval
	# @@ more efficient for awk to write dar --create output driectly to log, only passing unexpected lines back to bash?
	eval "$( $awk '
		function write_log_msg()
	     	{
				log_msg_idx++
				log_msg = substr(log_msg, 2)						# Remove initial \n
				gsub(squote, squote dquote squote dquote squote, log_msg)
				gsub("\r", "", log_msg)
				print "log_msg[" log_msg_idx "]="squote log_msg squote
				log_msg = ""
			}

	    BEGIN {
				print "Analysing dar --create output for logging and error reporting" > "/dev/tty"
	            dquote = "\042"
				error_flag = "NO"
				log_msg_idx = -1
	            squote = "\047"
				warning_flag = "NO"
	    	}

		// \
			{
				# When there are too many lines in the log message this program is slow and has been seen to break (no log_msg output)
				# The solution is to write it 500 line chunks
				if ((NR % 500) > 498.1) 
				{
					printf "." > "/dev/tty"
					write_log_msg()
				}
			}

		/Return value from dar command is: / \
			{
				i = split($0, array)
				dar_retval = array[i]
				next
			}

		/^Removing file / \
		|| /^Adding file to archive: / \
		|| /^Writing archive contents\.\.\./ \
		|| /^\r$/ \
		|| /^Recording hard link to archive: / \
		|| / --------------------------------------------/ \
		|| /.* inode\(s\) saved/ \
		|| /.* with .* hard link\(s\) recorded/ \
		|| /.* inode\(s\) not saved \(no inode\/file change\)/ \
		|| /.* inode\(s\) ignored \(excluded by filters\)/ \
		|| /.* inode\(s\) recorded as deleted from reference backup/ \
		|| /^ Total number of inodes* considered: / \
		|| /^ EA saved for [0-9]* inode\(s\)/ \
			{														# Normal
				log_msg = log_msg "\n" $0
				next
			}
		
		/inode\(s\) changed at the moment of the backup/ \
			{														# Maybe normal, maybe unexpected
				split($0, array)
				if (array[1] == 0)				
				{
					log_msg = log_msg "\n" $0
				}
				else
				{
					log_msg = log_msg "\n" "'"$prgnam"' identified unexpected output: " $0
					log_bad_msg = log_bad_msg "\n" $0
					if ("'"${snapshot_mnt_points[ $idx ]}"'" == "") { error_flag = "YES" } else { warning_flag = "YES" }
				}
				next
			}

		/.* inode\(s\) failed to save \(filesystem error\)/ \
			{														# Maybe normal, maybe unexpected
				split($0, array)
				if (array[1] == 0)
				{													# Normal
					log_msg = log_msg "\n" $0
				}
				else
				{
					warning_flag = "YES" 							# Filesystem error but keep going with the backup		
					log_msg = log_msg "\n" "'"$prgnam"' identified unexpected output: " $0
					log_bad_msg = log_bad_msg "\n" $0
				}
				next
			}

		/.*/ \
			{
				warning_flag = "YES" 								# Unexpected output but keep going with the backup		
				log_msg = log_msg "\n" "'"$prgnam"' identified unexpected output: " $0
				log_bad_msg = log_bad_msg "\n" $0
				next
			}

		END {
				printf "\n" > "/dev/tty"
				log_msg = log_msg "\n" $0 							# Last line is " --------------------------------------------"
				write_log_msg()
				print "log_msg_idx_max=" log_msg_idx
				print "dar_retval=" dar_retval
				print "error_flag=" squote error_flag squote
				print "log_bad_msg=" log_bad_msg
				print "warning_flag=" squote warning_flag squote
			}' \
	$tmp_afn )"

	# Message and log as required
	# ~~~~~~~~~~~~~~~~~~~~~~~~~~~
	# The following line only prints the first 500 lines. Too much data for bash?  Solution is to loop for each set of 500 lines
	# call_msg '-n' 'I' "dar --create output:${log_msg[@]}"
	call_msg -n s 'I' "dar --create output:"
	for (( idx = 0; idx <= log_msg_idx_max; idx++))
	do
		call_msg -n s -n t 'I' "${log_msg[$idx]}"
	done
	if [[ $error_flag = 'YES' ]]; then
		call_msg 'E' "Unexpected output from dar --create:$log_bad_msg"
	else 
		if [[ $warning_flag = 'YES' ]]; then
			call_msg 'W' "Unexpected output from dar --create:$log_bad_msg"
		fi
	fi
	report_dar_retval $dar_retval

	fct "${FUNCNAME[ 0 ]}" 'returning'

}  # end of do_dar_create

#--------------------------
# Name: do_dar_diff
# Purpose: checks dar backup file on disk against backed up files.
# Usage: do_dar_diff
# 		$1 - index number of dar job
#--------------------------
function do_dar_diff {

	fct "${FUNCNAME[ 0 ]}" 'started'

    local buf dar_retval error_flag idx log_msg warning_flag

	idx="$1"

	# Write a shellscript
	# ~~~~~~~~~~~~~~~~~~~
	buf="#! /bin/bash$lf$dar --diff ${bk_dir}${basenames[ $idx ]} --fs-root ${dar_fs_roots[ $idx ]}${lf}echo $dar_retval_preamble\$?$lf# End of script" > $tmp_sh_afn 	
	echo "$buf" > $tmp_sh_afn 	

	# Run the shellscript, capturing its screen output in a temporary file
	# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	call_msg 'I' "Running on-the-fly script to verify dar backup on disk against backed up files.  Script is:$lf$buf"
	"$script" '-c' "$tmp_sh_afn" '-q' '-f' "$tmp_afn"

	# Analyse captured screen output
	# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	error_flag='NO'
	log_msg=''
	warning_flag='NO'
	exec 3< $tmp_afn 												# set up the temp file for reading on file descriptor 3
	while read -u 3 buf												# for each line of the script output to screen
	do
		buf="${buf%}"
		case "$buf" in
			$dar_retval_preamble* )
				buf="${buf#$dar_retval_preamble}"
				dar_retval="$buf"
				;;
			'' \
			| *'--------------------------------------------'* \
			| *'inode(s) treated' \
			| *'inode(s) ignored (excluded by filters)' \
			| *'Total number of inode considered:'* ) 				# Normal output
				log_msg="$log_msg$lf$buf"
				;;
			*'inode(s) do not match those on filesystem' )			# Maybe normal, maybe unexpected
				log_msg="$log_msg$lf$buf"
				buf="${buf%% *}"
				if [[ "$buf" != '0' ]]; then
					if [[ "${snapshot_mnt_points[ $idx ]}" != '' ]]; then
						error_flag='YES'							# Unexpected with LVM snapshot
					else
						warning_flag='YES'							# Normal but iffy when not LVM snapshot
					fi
				fi
				;;
			* )														# Unexpected output
				error_flag='YES'
				log_msg="$log_msg${lf}$prgnam identified unexpected output: $buf"
				;;
		esac

	done
	exec 3<&- 														# free file descriptor 3

	# Message and log as required
	# ~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if [[ $error_flag = 'NO' ]]; then 
		call_msg -n s 'I' "Verifed dar backup on disk against backed up files OK:$log_msg"
	else
		call_msg 'E' "Unexpected output when verifying dar backup on disk against backed up files:$log_msg"
	fi
	report_dar_retval $dar_retval
	if [[ $warning_flag == 'YES' ]]; then
		call_msg 'I' 'WARNING: some files had changed on disk (see above)'
	fi

	fct "${FUNCNAME[ 0 ]}" 'returning'

}  # end of do_dar_diff

#--------------------------
# Name: drop_snapshot
# Purpose: umnmounts and removes a snapshot
# Usage: drop_snapshot
# 		$1 - index number of dar job
# 		$2 - name of variable to put any error message in (only used when finalising)
#--------------------------
function drop_snapshot {

	fct "${FUNCNAME[ 0 ]}" 'started'

    local buf idx my_msg

	idx="$1"

	# Unmount the snapshot
	# ~~~~~~~~~~~~~~~~~~~~
	buf="$( "$umount" "${snapshot_mnt_points[ $idx ]}" 2>&1 )"
	if [[ $? -eq 0 ]]; then
		snapshot_vol_mounted_flag[ $idx ]='NO'
		call_msg 'I' "Unmounted snapshot from ${snapshot_mnt_points[ $idx ]}."
	else
		my_msg="Unable to unmount snapshot from ${snapshot_mnt_points[ $idx ]}.  $umount output was$lf$buf"
		if [[ ${finalising_flag:-} = '' ]]; then
			call_msg 'E' "$my_msg"
		else
			eval "$2"="'$my_msg'"
			fct "${FUNCNAME[ 0 ]}" 'returning on umount error'
			return 1
		fi
	fi

	# Remove the snapshot
	# ~~~~~~~~~~~~~~~~~~~
	buf="$( $lvremove --force ${LVM_snapshot_devices[ $idx ]} 2>&1 )"
	if [[ $? -eq 0 ]]; then
		call_msg 'I' "Removed snapshot volume ${LVM_snapshot_devices[ $idx ]}."
	else
		my_msg="Unable to remove snapshot volume ${LVM_snapshot_devices[ $idx ]}.  $lvremove output was$lf$buf"
		if [[ ${finalising_flag:-} = '' ]]; then
			call_msg 'E' "$my_msg"
		else
			eval "$2"="'$my_msg'"
			fct "${FUNCNAME[ 0 ]}" 'returning on lvremove error'
			return 1
		fi
	fi

	fct "${FUNCNAME[ 0 ]}" 'returning'

}  # end of drop_snapshot

#--------------------------
# Name: fct
# Purpose: function call trace (for debugging)
# $1 - name of calling function 
# $2 - message.  If it starts with "started" or "returning" then the output is prettily indented
#--------------------------
function fct {

	if [[ $debug = 'NO' ]]; then
		return 0
	fi

	fct_ident="${fct_indent:=}"

	case $2 in
		'started'* )
			fct_indent="$fct_indent  "
			call_msg 'I' "DEBUG: $fct_indent$1: $2"
			;;
		'returning'* )
			call_msg 'I' "DEBUG: $fct_indent$1: $2"
			fct_indent="${fct_indent#  }"
			;;
		* )
			call_msg 'I' "DEBUG: $fct_indent$1: $2"
	esac

}  # end of function fct

#--------------------------
# Name: finalise
# Purpose: cleans up and gets out of here
#--------------------------
function finalise {
	fct "${FUNCNAME[ 0 ]}" 'started'

	local buf msg msgs my_retval retval

	finalising_flag='YES'
	
	# Set return value
	# ~~~~~~~~~~~~~~~~
	# Choose the highest and give message if finalising on a trapped signal
	my_retval="${prgnam_retval:-0}" 
	if [[ $1 -gt $my_retval ]]; then
		my_retval=$1
	fi
	case $my_retval in 
		129 | 130 | 131 | 143 )
			case $my_retval in
				129 )
					buf='SIGHUP'
					;;
				130 )
					buf='SIGINT'
					;;
				131 )
					buf='SIGQUIT'
					;;
				143 )
					buf='SIGTERM'
					;;
			esac
			call_msg 'I' "finalising on $buf"
			;;
	esac

	# Drop any snapshot(s)
	# ~~~~~~~~~~~~~~~~~~~~
	msgs=''
	i="$n_dar_jobs"
	while [[ $i -gt 0 ]]
	do
		if [[ ${snapshot_vol_mounted_flag[ $i ]:-} = 'YES' ]]; then
			drop_snapshot $i 'msg'
			retval=$?
			if [[ $retval -ne 0 ]]; then
				msgs="$msgs$lf$msg"
				if [[ $retval -gt $my_retval ]]; then
					my_retval=$retval
				fi
			fi
		fi
		: $(( i-- ))
	done

	# Unmount DVD
	# ~~~~~~~~~~~
	# At first eject was used to both unmount and eject but it was not reliable
	if [[ ${DVD_mounted_flag:-} = 'YES' ]]; then
		# Sometimes umount fails with "device is busy" so pause for a second before umounting
		$sleep 1
		buf="$( $umount $DVD_dev 2>&1 )"
		if [[ $? -eq 0 ]]; then
			buf="$( $eject $DVD_dev 2>&1 )"
			if [[ $? -eq 0 ]]; then
				call_msg 'I' 'DVD unmounted and ejected'
			else
				msgs="$msgs${lf}Unable to eject $DVD_dev.  $eject output was$lf$buf"
			fi
		else
			msgs="$msgs${lf}Unable to unmount $DVD_dev.  $umount output was$lf$buf"
		fi
	fi

	# Remove any temporary DVD mount directory
	# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if [[ ${DVD_mnt_dir_created_flag:-} = 'YES' ]]; then
		$rm -fr $DVD_mnt_dir 2> /dev/null
	fi

	# Remove any temporary data file
	# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if [[ ${tmp_file_created_flag:-} = 'YES' ]]; then
		$rm -f $tmp_afn 2> /dev/null
	fi

	# Remove any temporary script file
	# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if [[ ${tmp_sh_file_created_flag:-} = 'YES' ]]; then
		$rm -f $tmp_sh_afn 2> /dev/null
	fi

	# Final log messages
	# ~~~~~~~~~~~~~~~~~~
	if [[ "$msgs" != '' ]]; then
		msgs="${msgs#$lf}"		# strip leading linefeed
		call_msg 'E' "$msgs"
	fi
	call_msg 'I' "Exiting with return value $my_retval"
	call_msg -n l 'I' "Log file: $log_afn"

	# Exit, ensuring exit is used, not any alias
	# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	fct "${FUNCNAME[ 0 ]}" 'exiting'
	\exit $my_retval

}  # end of function finalise

#--------------------------
# Name: format_DVD
# Purpose: formats the DVD
# Usage: format_DVD
#--------------------------
function format_DVD {

	fct "${FUNCNAME[ 0 ]}" "started"

    local buf error_flag log_msg

	# Formatting the DVD is precautionary.  Some DVDs, written on other systems, caused genisofs (called by 
	# growisofs) to error.  Experiments showed the problem could be worked around by formatting.

	# Write shellscript to format the DVD
	# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	buf="#! /bin/bash$lf$dvd_rw_format -force $DVD_dev$lf# End of script" > $tmp_sh_afn 	
	echo "$buf" > $tmp_sh_afn 	

	# Run the shellscript, capturing its screen output in a temporary file
	# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	call_msg 'I' "Running on-the-fly script to format DVD.  Script is:$lf$buf"
	"$script" '-c' "$tmp_sh_afn" '-q' '-f' "$tmp_afn"

	# Analyse captured screen output
	# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	error_flag='NO'
	log_msg=''
	exec 3< $tmp_afn 												# set up the temp file for reading on file descriptor 3
	while read -u 3 buf												# for each line of the script output to screen
	do
		buf="${buf%}"
		case "$buf" in
			'* BD/DVD±RW/-RAM format utility'*  | '* 4.7GB DVD'* ) 	# Normal output
				log_msg="$log_msg$lf$buf"
				;;
			'* formatting'* )										# Progress reporting -- not required in log
				;;
			* )														# Unexpected output
				error_flag='YES'
				log_msg="$log_msg${lf}$prgnam identified unexpected output: $buf"
				;;
		esac

	done
	exec 3<&- 														# free file descriptor 3

	if [[ $error_flag = 'NO' ]]; then 
		call_msg -n s 'I' "Formatted DVD OK:$log_msg"
	else
		call_msg 'E' "Unexpected output when formatting DVD:$log_msg"
	fi

	call_msg 'I' "DEBUG for https://bugs.launchpad.net/ubuntu/+source/linux/+bug/228624: After formattng DVD, /sys/block/sr0/size contains: $(/bin/cat /sys/block/sr0/size)" 

	fct "${FUNCNAME[ 0 ]}" "returning"

}  # end of format_DVD

#--------------------------
# Name: get_dev_afn
# Purpose: gets the absolute filename of the device file of the filesystem the dirtree is
# Usage: get_dev_afn index
#	where 
#		index is the dar job index number
#--------------------------
function get_dev_afn {

	fct "${FUNCNAME[ 0 ]}" "started with idx=$1"

    local buf idx cfg_fs_root 

	# Get device name for file system containing the directory tree to back up
	# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	idx="$1"
	cfg_fs_root="${cfg_fs_roots[ $idx ]}"
	buf="$($df "$cfg_fs_root" 2>&1)"
	if [[ $? -ne 0 ]]; then
	    call_msg 'E' "Unable to get device name for file system with directory tree '$cfg_fs_root'.  $df output was$lf$buf" 
	fi
	# df wraps output if the device path is too long to fit in the "Filesystem" column
	buf="${buf#*$lf}"						# strip first line
	buf="${buf%$lf*}"						# strip possible third (now second) line
	dev_afn="${buf%% *}"					# strip possible space onward

	fct "${FUNCNAME[ 0 ]}" "returning. \$dev_afn is '$dev_afn'"

}  # end of get_dev_afn

#--------------------------
# Name: initialise
# Purpose: sets up environment and parses command line
#--------------------------
function initialise {

	fct "${FUNCNAME[ 0 ]}" 'started'

	local args cfg_fn

	# Set defaults that may be overriden by command line parsing
	# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	cfg_dir="/etc/opt/$prgnam/" 		# can only be overridden on the command line
	log_dir="/var/opt/$prgnam/log/"		# can only be overridden on the command line
	DVD_mnt_dir="/var/tmp/$prgnam/mnt/DVD/"
	skip_to=''
	snapshot_mnt_base_dir="/var/tmp/$prgnam/mnt/snapshots/"
	writer_opt_flag='NO'

	# Parse command line
	# ~~~~~~~~~~~~~~~~~~
	args="${*:-}"
	while getopts c:dhl:s:w: opt 2>/dev/null
	do
		case $opt in
			c)
				if [[ ${OPTARG:0:1} != '/' ]]; then
					cfg_afn="${cfg_dir}$OPTARG"
				else
					cfg_afn="$OPTARG"
				fi
				;;
			d )
				debug='YES'
				;;
			h )
				usage -v
				\exit 0
				;;
			l)
				if [[ ${OPTARG:0:1} != '/' ]]; then
					usage -v
					\exit 0
				fi
				log_dir="$OPTARG"
				if [[ ${log_dir:${#log_dir}} != '/' ]]; then
					log_dir="$log_dir/"
				fi
				;;
			s )
				case "$OPTARG" in
					'DVD_write' | 'DVD_verify' ) 
						skip_to="$OPTARG"
						;;
					* )
						echo "Option -s: invalid value '$OPTARG'" >&2
						usage
						\exit 1
						;;
				esac
				;;
			w )
				writer_opt_flag='YES'
				DVD_dev="$OPTARG"
				;;
			* )
				usage
				\exit 1
		esac
	done

	# Test for extra arguments
	# ~~~~~~~~~~~~~~~~~~~~~~~~
	shift $(( $OPTIND-1 ))
	if [[ $* != '' ]]; then
        usage
        \exit 1
	fi

	# Test for any mandatory options not set
	# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if [[ ${cfg_afn:-} = '' ]]; then
        usage
        \exit 1
	fi
	
	# Utility variables
	# ~~~~~~~~~~~~~~~~~
	dar_retval_preamble='Return value from dar command is: '
	lf=$'\n'							# ASCII linefeed, a.k.a newline
	squote="'"

	# Linux executables
	# ~~~~~~~~~~~~~~~~~
	export PATH=/usr/bin 				# $PATH is not used by this script but growisofs needs it to find genisofs
	awk='/usr/bin/awk'
	chmod='/bin/chmod'
	cksum='/usr/bin/cksum'
	dar_static='/usr/bin/dar_static'
	dar='/usr/bin/dar'
	date='/bin/date'
	df='/bin/df'
	dvd_rw_format='/usr/bin/dvd+rw-format'
	dvd_rw_mediainfo='/usr/bin/dvd+rw-mediainfo'
	eject='/usr/bin/eject'
	grep='/bin/grep'
	growisofs='/usr/bin/growisofs'
	lvcreate='/sbin/lvcreate'
	lvremove='/sbin/lvremove'
	lvs='/sbin/lvs'
	mkdir='/bin/mkdir'
	mktemp='/bin/mktemp'
	mount='/bin/mount'
	ps='/bin/ps'
	readlink='/bin/readlink'
	rm='/bin/rm'
	script='/usr/bin/script'
	sleep='/bin/sleep'
	stty='/bin/stty'
	umount='/bin/umount'

	buf="$( ck_file $awk:fx $chmod:fx $cksum:fx $dar_static:fx $dar:fx $date:fx $df:fx $dvd_rw_format:fx $dvd_rw_mediainfo:fx $eject:fx $grep:fx $growisofs:fx $lvcreate:fx $lvremove:fx $lvs:fx $mkdir:fx $mktemp:fx $mount:fx $ps:fx $readlink:fx $rm:fx $script:fx $sleep:fx $stty:fx $umount:fx 2>&1 )"
	if [[ $? -ne 0 ]] ; then
		echo "$prgnam: terminating on Linux executable problem(s):$lf$buf" >&2
		\exit 1
	fi

	# Note whether being run from a terminal
	# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	buf="$( $ps -p $$ -o tty 2>&1 )"
	case $buf in
		*TT*-* )
			interactive_flag='NO'
			;;
		*TT* )
			interactive_flag='YES'
			;;
		* )
			echo "$prgnam: Unable to determine if being run interactively.  ps output was: $buf" >&2
			\exit 1
	esac

	# Does the configuration file exist?
	# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	buf="$( ck_file $cfg_afn:fr 2>&1 )"
	if [[ $buf != '' ]]
	then
		echo "Terminating on configuration file problem:$lf$buf" >&2
		\exit 1
	fi

	# Initialise log file
	# ~~~~~~~~~~~~~~~~~~~
	# First part of the log file name is the config file name with any symlinks resolved
	cfg_fn="$( $readlink $cfg_afn 2>&1 )"
	if [[ $cfg_fn = '' ]]; then
		cfg_fn="${cfg_afn##*/}"
	fi
	buf="$( ck_dir $log_dir:wx 2>&1 )"
	if [[ $? -ne 0 ]] ; then
		echo "$prgnam: Terminating on log directory problem:$lf$buf" >&2
		\exit 1
	fi
	timestamp="$( $date +%y-%m-%d@%H:%M:%S)"
	log_afn="${log_dir}${cfg_fn}_$timestamp"

	# Up to this point any messages have been given using echo followed by \exit 1.  Now 
	# the essentials for call_msg() and finalise() have been established, all future messages 
	# will be sent using call_msg() and error mesages will then call finalise().

	call_msg 'I' "$prgnam version $prg_ver started with command line '$args'.  Logging to '$log_afn'"

	fct "${FUNCNAME[ 0 ]}" 'started (this message delayed until logging initialised)'

	# Create temporary file
	# ~~~~~~~~~~~~~~~~~~~~~
	tmp_afn="$( $mktemp "/tmp/$prgnam.tmp.XXXXXXXX" 2>&1 )"
	rc=$?
	if [[ $rc -ne 0 ]]; then
		call_msg 'E' "Unable to create temporary file:$lf$tmp_afn"
	fi
	tmp_file_created_flag='YES'

	# Create temporary script file
	# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	tmp_sh_afn="$( $mktemp "/tmp/$prgnam.tmp_sh.XXXXXXXX" 2>&1 )"
	rc=$?
	if [[ $rc -ne 0 ]]; then
		call_msg 'E' "Unable to create temporary script file:$lf$tmp_sh_afn"
	fi
	tmp_sh_file_created_flag='YES'
	$chmod '744' "$tmp_sh_afn"

	# Parse configuration file
	# ~~~~~~~~~~~~~~~~~~~~~~~~
	parse_cfg "$cfg_afn"

	fct "${FUNCNAME[ 0 ]}" 'returning'

}  # end of function initialise

#--------------------------
# Name: msg
# Purpose: generalised messaging interface
# Usage: msg [ -l logfile ] [ -n l|s|t ] class msg_text
#    -l logs any error messages to logfile unless -n l overrides
#    -n suppress:
#		l writing to log
#		s writing to stdout or stderr
#		t writing timestamp and I|W|E to log
#    class must be one of I, W or E indicating Information, Warning or Error
#    msg_text is the text of the message
# Return code:  always zero (exits on error)
#--------------------------
function msg {

    local args buf indent line logfile message_text no_log_flag no_screen_flag no_timestamp_flag preamble

    # Parse any options
    # ~~~~~~~~~~~~~~~~~
	args="${@:-}"
	OPTIND=0								# Don't inherit a value
	no_log_flag='NO'
	no_screen_flag='NO'
	no_timestamp_flag='NO'
	while getopts l:n: opt 2>/dev/null
	do
		case $opt in
			l)
				logfile="$OPTARG"
				;;
			n)
				case ${OPTARG:=} in
					'l' )
						no_log_flag='YES'
						;;
					's' )
						no_screen_flag='YES'
						;;
					't' )
						no_timestamp_flag='YES'
						;;
					* )
			    		echo "$prgnam: msg: invalid -n option argument, '$OPTARG'" >&2
			    		\exit 1
				esac
				;;
			* )
			    echo "$prgnam: msg: invalid option, '$opt'" >&2
			    \exit 1
			    ;;
		esac
	done
	shift $(( $OPTIND-1 ))

    # Parse the mandatory positional parameters (class and message)
    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    class="${1:-}"
    case "$class" in 
    	I | 'W' | 'E' ) 
	    	;;
	    * )
	    	echo "$prgnam: msg: invalid arguments: '$args'" >&2
	    	\exit 1
    esac

    message_text="$2"

    # Write to log if required
    # ~~~~~~~~~~~~~~~~~~~~~~~~
    if [[ $logfile != ''  && $no_log_flag == 'NO' ]]
    then
		if [[ $no_timestamp_flag == 'NO' ]]; then
			buf="$( $date +%H:%M:%S ) $class "
		else
			buf=''
		fi
		echo "$buf$message_text" >> $logfile
		if [[ $? -ne 0 ]]
		then
			echo "$prgnam: msg: unable to write to log file '$logfile'" >&2
			\exit 1
		fi
	fi
	
	# Write to stdout or stderr if running interactively and not suppressed
	# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if [[ $interactive_flag = 'YES' && $no_screen_flag = 'NO' ]]; then
	    case "$class" in 
	    	I  ) 
				echo "$message_text" >&1
		    	;;
		    W  ) 
				echo "WARNING: $message_text" >&1
		    	;;
		    E )
				echo "ERROR: $message_text" >&2
		    	;;
	    esac
	fi

    return 0

}  #  end of function msg

#--------------------------
# Name: parse_cfg
# Purpose: parses the configuration file
# $1 - pathname of file to parse
#--------------------------
function parse_cfg {

	fct "${FUNCNAME[ 0 ]}" 'started'

	local bk_dir_flag buf cfg_afn data drct_dir_tree_flag DVD_dev_flag DVD_mnt_dir_flag keyword snapshot_mnt_base_dir_flag 
	local -a array
	
	cfg_afn="$1"

	# Does the file exist?
	# ~~~~~~~~~~~~~~~~~~~~
	buf="$( ck_file $cfg_afn:fr 2>&1 )"
	if [[ $buf != '' ]]
	then
		call_msg 'E' "Terminating on configuration file problem:$lf$buf"
		\exit 1
	fi

	# For each line in the config file ...
	# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	emsg=''
	n_dar_jobs=0
	exec 3< $cfg_afn 								# set up the config file for reading on file descriptor 3
	while read -u 3 buf								# for each line of the config file
	do
		buf="${buf%%#*}"							# strip any comment
		buf="${buf%%*( 	)}"							# strip any trailing spaces and tabs
		buf="${buf##*( 	)}"							# strip any leading spaces and tabs
		if [[ $buf = '' ]]; then
			continue								# empty line
		fi
		keyword="${buf%%:*}"
		keyword="${keyword%%*( 	)}"					# strip any trailing spaces and tabs
		data="${buf#*:}"							# strip to first : (keyword +)
		data="${data##*([ 	])}"					# strip any leading spaces and tabs
		case "$keyword" in
			'Backup directory' )
				if [[ ${bk_dir_flag:-} = 'YES' ]]; then
					emsg="$emsg${lf}  Backup directory specified again"
				fi
				bk_dir_flag='YES'
				bk_dir="${data%%*(/)}/"  			# silently ensure a single trailing slash
				;;
			'Basename' )
				: $(( n_dar_jobs++ ))
				basenames[ $n_dar_jobs ]="$data"
				;;
			'Direct directory tree' )
				if [[ ${drct_dir_tree_flag:-} = 'YES' ]]; then
					emsg="$emsg${lf}  Direct directory tree specified again"
				fi
				drct_dir_tree_flag='YES'
				eval array=( "$data" )					# parses any quoted words
				if [[ ${#array[*]} -ne 1 ]]; then
					emsg="$emsg${lf}  Direct directory tree data is not one word"
				fi
				drct_dir_tree="${array[0]// /\\ }"		# escapes any spaces
				;;
			'DVD mount directory' )
				if [[ ${DVD_mnt_dir_flag:-} = 'YES' ]]; then
					emsg="$emsg${lf}  DVD mount directory specified again"
				fi
				DVD_mnt_dir_flag='YES'
				DVD_mnt_dir="${data%%*(/)}/"  			# silently ensure a single trailing slash
				;;
			'DVD writer device' )
				if [[ ${DVD_dev_flag:-} = 'YES' ]]; then
					emsg="$emsg${lf}  DVD writer device specified again"
				fi
				DVD_dev_flag='YES'
				if [[ $writer_opt_flag = 'NO' ]]; then
					DVD_dev="$data"
				else
					call_msg 'I' "DVD writer specified on command line with -w option; DVD writer in configuration file $cfg_afn ignored"
				fi
				;;
			'Directory tree (fs_root)' )
				eval array=( "$data" )					# parses any quoted words
				if [[ ${#array[*]} -ne 1 ]]; then
					emsg="$emsg${lf}  Directory tree data is not one word"
				fi
				cfg_fs_roots[ $n_dar_jobs ]="${array[0]}"
				;;
			'Excludes' )
				excludes[ $n_dar_jobs ]="$data"
				;;
			'Prunes' )
				prunes[ $n_dar_jobs ]="$data"
				;;
			'Snapshot mount base directory' )
				if [[ ${snapshot_mnt_base_dir_flag:-} = 'YES' ]]; then
					emsg="$emsg${lf}  Snapshot mount base directory specified again"
				fi
				snapshot_mnt_base_dir_flag='YES'
				snapshot_mnt_base_dir="${data%%*(/)}/"  # silently ensure a single trailing slash
				;;
			* )
				emsg="$emsg${lf}  Unrecognised keyword. '$keyword'"
				;;
		esac

	done
	exec 3<&- # free file descriptor 3

	# Normalisation and error traps
	# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if [[ ${bk_dir_flag:-} != 'YES' ]]; then
		emsg="$emsg${lf}  Backup directory not specified"
	fi
	if [[ ${drct_dir_tree_flag:-} = 'YES' ]]; then
		buf="$( ck_file "$drct_dir_tree":drx 2>&1 )"
		if [[ $? -ne 0 ]]; then
			emsg="$emsg${lf}  Direct directory tree root: $buf"
		fi
	else
		drct_dir_tree=''
	fi
	if [[ ${DVD_dev_flag:-} != 'YES' ]]; then
		emsg="$emsg${lf}  DVD writer device not specified"
	fi
	if [[ ${snapshot_mnt_base_dir_flag:-} = 'YES' ]]; then
		buf="$( $readlink --canonicalize-missing $snapshot_mnt_base_dir 2>&1 )"
		buf="${buf%%*(/)}/"															# ensure single trailing /
		if [[ "$buf" != "$snapshot_mnt_base_dir" ]]; then
			call_msg 'I' "Snapshot mount base directory '$snapshot_mnt_base_dir' normalised to '$buf' by resolving symlinks"
			snapshot_mnt_base_dir="$buf"
		fi
	fi

	if [[ $emsg != '' ]]
	then
		call_msg 'E' "In configuration file $cfg_afn:$emsg"
		finalise 1
	fi

	fct "${FUNCNAME[ 0 ]}" 'returning'

}  # end of function parse_cfg

#--------------------------
# Name: press_enter_to_continue
# Purpose: prints "Press enter to continue..." message and waits for user to do so
#--------------------------
function press_enter_to_continue {

	echo "Press enter to continue..."
	read

}  # end of function press_enter_to_continue

#--------------------------
# Name: report_dar_retval
# Purpose: reports on return value from dar
# Usage: report_dar_retval retval
#	where 
#		retval is the return value from dar
#--------------------------
function report_dar_retval {

	fct "${FUNCNAME[ 0 ]}" 'started'

    local retval 

	retval=$?

	case $retval in
		0 )
		    ;;
		1 )
		    call_msg 'E' "Programming error in $prgnam: dar syntax error"
		    ;;
        2 )
 		    call_msg 'E' 'dar: hardware problem or lack of memory'
 		    ;;
        3 | 7 | 8 | 9 | 10 )
  		    call_msg 'E' "dar: internal dar problem; consult dar man page for return code $retval"
 		    ;;
        4 )
            if [[ $interactive_flag = 'NO' ]]; then
    		    call_msg 'E' 'dar: user input required but not running interactively'
            fi
 		    ;;
        5 )
    		call_msg 'W' 'dar: file to be backed up could not be opened or read'
    		if [[ ${prgnam_retval:-} -lt 5 ]]; then
    		    prgnam_retval=5
    		fi
 		    ;;
        6 )
            # Should not be possible!  Code written here in case dar command altered to include -E or -F
    		call_msg 'W' 'dar: called program error'
    		if [[ ${prgnam_retval:-} -lt 6 ]]; then
    		    prgnam_retval=6
    		fi
 		    ;;
        11 )
    		call_msg 'W' 'dar: detected at least one file changed while being backed up'
    		if [[ ${prgnam_retval:-} -lt 11 ]]; then
    		    prgnam_retval=11
    		fi
 		    ;;
		* )
			# In case further return values introduced to dar
  		    call_msg 'E' "dar: unexpected return code $retval.  $prgnam needs updating to suit"
 		    ;;
	esac

	fct "${FUNCNAME[ 0 ]}" 'returning'

}  # end of report_dar_retval

#--------------------------
# Name: set_up_snapshot
# Purpose: creates and mounts LVM snapshot of the file system containing the directory tree 
# 	to back up (the fs_root on dar's -R option)
# Usage: set_up_snapshot index
#	where 
#		index is the dar job index number
#--------------------------
function set_up_snapshot {

	fct "${FUNCNAME[ 0 ]}" "started with idx=$1"

    local buf cmd idx LVM_device LVM_snapshot_block_device size vg

	idx="$1"

	# Build alternative device name and snapshot device names
	# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	buf="${dev_afn#/dev/mapper/}"
	buf="${buf%% *}"
	vg="${buf%%-*}"
	vn="${buf#*-}"
	LVM_device="/dev/$vg/$vn"
	LVM_snapshot_device="${LVM_device}_snap"
	LVM_snapshot_block_device="/dev/mapper/${vg}-${vn}_snap"
	LVM_snapshot_devices[ $idx ]="$LVM_snapshot_device"

	# Build snapshot mount point name
	# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	snapshot_mnt_point="${snapshot_mnt_base_dir}$vg/$vn/"
	snapshot_mnt_points[ $idx ]="$snapshot_mnt_point"

	# Ensure snapshot volume exists
	# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if [[ -b $LVM_snapshot_device ]]; then
		# The snapshot already exists.  This happens:
		#    * Commonly - when this script is backing up more than one directory tree on the same filesystem.
		#  	 * Unusually - when a previous instance of this script has not cleaned up properly.
		call_msg 'I' "LVM snapshot volume $LVM_snapshot_device already exists"
	else
		# Get size of mounted logical volume
		# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		# Units k (KiB) are chosen for precision and so they can be used on the lvcreate command
		buf="$($lvs --units k -o lv_size "$LVM_device" 2>&1)"
		if [[ $? -ne 0 ]]; then
		    call_msg 'E' "Unable to get logical volume information for '$LVM_device'.  $lvs output was$lf$buf" 
		fi
		buf="${buf##*LSize*([!0-9])}"
		size="${buf%%.*}"
	
		# Flush file system buffers
		# ~~~~~~~~~~~~~~~~~~~~~~~~~
		($sync; $sleep 1; $sync; $sleep 1) >/dev/null 2>&1
	
		# Create snapshot volume
		# ~~~~~~~~~~~~~~~~~~~~~~
		# This is made same size as original to be 100% sure it will be big enough.
		buf="$($lvcreate --size ${size}k --snapshot --name "$LVM_snapshot_device" "$LVM_device" 2>&1)"
		if [[ $? -ne 0 ]]; then
		    call_msg 'E' "Unable to create snapshot volume.  $lvcreate output was$lf$buf" 
		fi
		call_msg 'I' "LVM snapshot volume $LVM_snapshot_device created"
	fi

	#  Ensure snapshot mounted
	# ~~~~~~~~~~~~~~~~~~~~~~~~
	cmd="$df $LVM_snapshot_block_device"
	buf="$( $cmd 2>&1 )"
	case ${buf##* } in
		'/dev' )
			# Mount snapshot
			buf="$($mkdir -p "$snapshot_mnt_point" 2>&1)"
	 		if [[ $? -ne 0 ]]; then
	   		 	call_msg 'E' "Unable to establish snapshot mount point directory '$snapshot_mnt_point'.  $mkdir output was$lf$buf" 
			fi
			buf="$($mount -o ro "$LVM_snapshot_device" "$snapshot_mnt_point" 2>&1)"
			if [[ $? -ne 0 ]]; then
	   		 	call_msg 'E' "Unable to mount snapshot volume.  $mount output was$lf$buf" 
			fi
			snapshot_vol_mounted_flag[ $idx ]='YES'
			call_msg 'I' "LVM snapshot volume $LVM_snapshot_device mounted on $snapshot_mnt_point"
			;;
		"${snapshot_mnt_point%/}")
			call_msg 'I' "LVM snapshot volume $LVM_snapshot_device already mounted on $snapshot_mnt_point"
			;;
		* )
			call_msg 'E' "LVM snapshot volume $LVM_snapshot_device already mounted but not on $snapshot_mnt_point$lf$cmd output was$lf$buf"
			;;
	esac

	fct "${FUNCNAME[ 0 ]}" 'returning'

}  # end of set_up_snapshot

#--------------------------
# Name: usage
# Purpose: prints usage message
#--------------------------
function usage {
	fct "${FUNCNAME[ 0 ]}" 'started'

	echo "usage: $prgnam -c <cfg file> [-d] [-h] [-l <log dir>] [-s DVD_write | DVD_verify] [-w <writer>" >&2
	if [[ ${1:-} = '' ]]
	then
		echo "(use -h for help)" >&2
	else
		echo "where:
	-c names the configuration file.  If it does not begin with / then it is prefixed with $cfg_dir
	   See sample configuration file for more information.
	-d turns debugging trace on
	-h prints this help
	-l names the log directory, default $log_dir
	-s skips to DVD write (DVD_write) or verification (DVD_verify).  Intended for use after either has failed.
	-w names the DVD writer, overriding the writer specified in the configuration file. Example /dev/scd1.
" >&2
	fi

	fct "${FUNCNAME[ 0 ]}" 'returning'

}  # end of function usage

#--------------------------
# Name: main
# Purpose: where it all happens
#--------------------------

# Configure script environment
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
set -o posix
set -o nounset
shopt -s extglob # allow extended pattern matching operators 

# Configure traps
# ~~~~~~~~~~~~~~~
# Set up essentials for finalise() and what it may call before setting traps (because finalise() may be called at any time after then)
debug='NO'
n_dar_jobs=0
prgnam=${0##*/}			# program name w/o path
trap 'finalise 129' 'HUP'
trap 'finalise 130' 'INT'
trap 'finalise 131' 'QUIT'
trap 'finalise 143' 'TERM'

# Initialise
# ~~~~~~~~~~
initialise "${@:-}"

# Back up each directory tree
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~
if [[ $skip_to = '' ]]; then
	back_up_dirtrees
fi

# Copy backup files to DVD
# ~~~~~~~~~~~~~~~~~~~~~~~~
copy_to_DVD

# Finalise
# ~~~~~~~~
finalise 0

