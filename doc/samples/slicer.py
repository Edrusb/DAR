#!/usr/bin/python3

#######################################################################
# dar - disk archive - a backup/restoration program
# Copyright (C) 2002-2024 Denis Corbin
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

import shutil
import sys

def usage(cmd_name):
    print("usage: {} <path> <slice_size>".format(cmd_name))
    print("")
    print("       {} will wait and ask user to press enter if the remaing space on <path> is smaller than <slice_size>".format(cmd_name))
    print("       It is expected to be provided to dar as argument to its -E option:".format(cmd_name))
    print("")
    print("       dar ... -s 10G -E \"{} %p 10G\" ... other options".format(cmd_name))
    print("")
    exit(1)

def get_free_space(path):
    """
    return the available size in byte of the storage
    where the given path is located
    """

    total, used, free = shutil.disk_usage(path)
    return free


def slice_size_to_bytes(slice_size):
    """"
    converts a string describing a slice size
    possibily using k, K, M, G... suffix following
    the syntax supported by Dar, into the
    equivalent size in bytes.
    """
    if len(slice_size) == 0:
        raise Exception("Empty string is not a valid slice size")
    val = 0
    unit_val = 1
    try:
        val = int(slice_size)
    except:
        # the provided slice is not an integer assuming the last byte is a unit char
        unit_char = slice_size[len(slice_size)-1:len(slice_size)]
        slice_size = slice_size[0:len(slice_size)-1]
        try:
            val = int(slice_size)
        except:
            raise Exception("Not a valid format for a slice size information")
        if unit_char == "k" or unit_char == "K":
            unit_val = 1024
        elif unit_char == "M":
            unit_val = 1024*1024
        elif unit_char == "G":
            unit_val = 1024**3
        elif unit_char == "T":
            unit_val = 1024**4
        elif unit_char == "P":
            unit_val = 1024**5
        elif unit_char == "E":
            unit_val = 1024**6
        elif unit_char == "Z":
            unit_val = 1024**7
        elif unit_char == "Y":
            unit_val = 1024**8
        elif unit_char == "R":
            unit_val = 1024**9
        elif unit_char == "Q":
            unit_val = 1024**10
        else:
            raise Exception("Not a valid format for a slice size information")
    return val * unit_val


if len(sys.argv) != 3:
    usage(sys.argv[0])
else:
    slice_size = slice_size_to_bytes(sys.argv[2])
    space = get_free_space(sys.argv[1])
    if space < slice_size:
        waiting=input("Not enought space on disk to store a slice entirely, pleae change the disk, then press enter when ready")
    else:
        print("Enough space on disk, continuing with the next slice")

