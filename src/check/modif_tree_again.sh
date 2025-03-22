#!/bin/sh

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

if [ $# -ne 2 ] ; then
    echo "usage: $0 <dir> <discriminant>"
    exit 1
fi

SUB1=S"$2"B1
SUB2=S"$2"B2
SUB3=S"$2"B3

cd "$1"

cd "$SUB1"
echo "trigger binary patch again" >> for_binary_delta
