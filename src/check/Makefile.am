#######################################################################
# dar - disk archive - a backup/restoration program
# Copyright (C) 2002-2023 Denis Corbin
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

#
# conditionnal part: --enable-mode=...
#

if BUILD_MODE32
    MYLIB=libdar32
    AM_CPPFLAGS=-DLIBDAR_MODE=32 -I'$(srcdir)/../libdar' -DDAR_LOCALEDIR=\"$(localedir)\" -DDAR_SYS_DIR=\"$(system_path)\" $(CPP_PROF)
else
if BUILD_MODE64
    MYLIB=libdar64
    AM_CPPFLAGS=-DLIBDAR_MODE=64 -I'$(srcdir)/../libdar' -DDAR_LOCALEDIR=\"$(localedir)\" -DDAR_SYS_DIR=\"$(system_path)\" $(CPP_PROF)
else
    MYLIB=libdar
    AM_CPPFLAGS=-I'$(srcdir)/../libdar' -DDAR_LOCALEDIR=\"$(localedir)\"  -DDAR_SYS_DIR=\"$(system_path)\" $(CPP_PROF)
endif
endif

helpers = build_tree.sh final.sh main.sh modif_tree.sh routine.sh loop.sh make_env.sh

dist_noinst_DATA = $(helpers) Old_format/archive_01.1.dar Old_format/archive_02.1.dar Old_format/archive_03.1.dar Old_format/archive_03_nozip.1.dar Old_format/archive_04.1.dar Old_format/archive_05.1.dar Old_format/archive_06.1.dar Old_format/archive_07.1.dar Old_format/archive_08.1.dar Old_format/archive_08-1.1.dar  Old_format/archive_08-1_crypto_bf_test.1.dar Old_format/archive_08-1_nozip.1.dar Old_format/archive_09.1.dar Old_format/archive_09_crypto_bf_test.1.dar Old_format/archive_09_nozip.1.dar Old_format/archive_10.1.dar Old_format/archive_10-1.1.dar Old_format/archive_10-1_nozip.1.dar Old_format/archive_10-1_crypto_bf_test.1.dar sftp_mdelete ftp_mdelete
noinst_PROGRAMS = all_features padder bnonzero
LDADD = ../libdar/$(MYLIB).la $(LTLIBINTL)
AM_LDFLAGS =

MY_MAKEFILE=my_makefile
MY_ENV=my_env

$(MY_ENV): ./make_env.sh
	@ chmod a+x $(helpers)
	@ ./make_env.sh $(MY_ENV)

$(MY_MAKEFILE): $(MY_ENV) ./loop.sh
	@ ./loop.sh

check-local: $(MY_MAKEFILE)
	$(MAKE) -f $(MY_MAKEFILE) all
	@ ./final.sh

all_features_SOURCES = all_features.cpp
all_features_DEPENDENCIES = ../libdar/$(MYLIB).la

padder_SOURCES = padder.cpp
bnonzero_SOURCES = bnonzero.c

clean-local:
	rm -rf target_*
	rm -f $(MY_MAKEFILE) $(MY_ENV)
	rm -f compressible_file uncompressible_file

all-local: compressible_file uncompressible_file

compressible_file: bnonzero
	rm -f compressible_file
	./bnonzero compressible_file 1 2048

uncompressible_file:
	bzip2 -c9 < `which sh` > uncompressible_file
