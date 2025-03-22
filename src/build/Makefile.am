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

SUBDIRS = man src doc misc po
dist_noinst_DATA = INSTALL README THANKS TODO AUTHORS COPYING ChangeLog NEWS ABOUT-NLS

CPPCHECKDIR=./cppcheckbuilddir

cppcheck:
	@if cppcheck --help > /dev/null ; then \
	( mkdir -p $(CPPCHECKDIR) && cppcheck --force --file-filter="*.?pp" -isrc/testing -isrc/python --cppcheck-build-dir=$(CPPCHECKDIR) `pwd` ) || exit 1 ; \
	else \
	echo "cppcheck not present, aborting" || exit 1 ; \
	fi

clean-local:
	rm -rf $(CPPCHECKDIR)

