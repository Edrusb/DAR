#######################################################################
# dar - disk archive - a backup/restoration program
# Copyright (C) 2002-2020 Denis Corbin
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
# to contact the author : http://dar.linux.free.fr/email.html
#######################################################################

if BUILD_MODE32
    MYLIB=libdar32
    AM_CPPFLAGS=-DLIBDAR_MODE=32 -I'$(srcdir)/../dar_suite' -I'$(srcdir)/../libdar' -I../dar_suite -DDAR_LOCALEDIR=\"$(localedir)\" -DDAR_SYS_DIR=\"$(system_path)\" $(CPP_PROF) $(GPGME_CFLAGS) $(LIBCURL_CFLAGS) $(LIBTHREADAR_CFLAGS) $(CXXSTDFLAGS)
else
if BUILD_MODE64
    MYLIB=libdar64
    AM_CPPFLAGS=-DLIBDAR_MODE=64 -I'$(srcdir)/../dar_suite' -I'$(srcdir)/../libdar' -I../dar_suite -DDAR_LOCALEDIR=\"$(localedir)\" -DDAR_SYS_DIR=\"$(system_path)\" $(CPP_PROF) $(GPGME_CFLAGS) $(LIBCURL_CFLAGS) $(LIBTHREADAR_CFLAGS) $(CXXSTDFLAGS)
else
    MYLIB=libdar
    AM_CPPFLAGS=-I'$(srcdir)/../dar_suite' -I'$(srcdir)/../libdar' -I../dar_suite -DDAR_LOCALEDIR=\"$(localedir)\"  -DDAR_SYS_DIR=\"$(system_path)\" $(CPP_PROF) $(GPGME_CFLAGS) $(LIBCURL_CFLAGS) $(LIBTHREADAR_CFLAGS) $(CXXSTDFLAGS)
endif
endif


if WITH_LIBTHREADAR
    LIBTHREADAR_TEST_MODULES=test_heap test_parallel_tronconneuse test_block_compressor
else
    LIBTHREADAR_TEST_MODULES=
endif



noinst_PROGRAMS = test_hide_file test_terminateur test_catalogue test_infinint test_tronc test_compressor test_mask test_tuyau test_deci test_path test_erreurs test_sar test_filesystem test_scrambler test_generic_file test_storage test_limitint test_libdar test_cache test_tronconneuse test_elastic test_blowfish test_mask_list test_escape test_hash_fichier moving_file make_sparse_file hashsum test_crypto_asym test_range $(LIBTHREADAR_TEST_MODULES) test_rsync test_smart_pointer test_datetime test_entrepot_libcurl test_truncate

LDADD = ../libdar/$(MYLIB).la $(LIBCURL_LIBS) $(GPGME_LIBS) $(LIBTHREADAR_LIBS) $(LTLIBINTL)

if DEBUG_STATIC
AM_LDFLAGS = -all-static $(CXXSTDFLAGS)
else
AM_LDFLAGS = $(CXXSTDFLAGS)
endif

noinst_HEADERS = testtools.hpp

test_hide_file_SOURCES = test_hide_file.cpp
test_hide_file_LDADD = ../dar_suite/no_comment.o ../dar_suite/hide_file.o ../dar_suite/config_file.o $(LDADD)
test_hide_file_DEPENDENCIES = ../libdar/$(MYLIB).la

test_terminateur_SOURCES = test_terminateur.cpp
test_terminateur_DEPENDENCIES = ../libdar/$(MYLIB).la

test_catalogue_SOURCES = test_catalogue.cpp
test_catalogue_DEPENDENCIES = ../libdar/$(MYLIB).la

test_infinint_SOURCES = test_infinint.cpp
test_infinint_DEPENDENCIES = ../libdar/$(MYLIB).la

test_tronc_SOURCES = test_tronc.cpp testtools.cpp
test_tronc_DEPENDENCIES = ../libdar/$(MYLIB).la

test_compressor_SOURCES = test_compressor.cpp
test_compressor_DEPENDENCIES = ../libdar/$(MYLIB).la

test_mask_SOURCES = test_mask.cpp
test_mask_DEPENDENCIES = ../libdar/$(MYLIB).la

test_tuyau_SOURCES = test_tuyau.cpp
test_tuyau_LDADD =  ../dar_suite/dar_suite.o ../dar_suite/line_tools.o $(LDADD)
test_tuyau_DEPENDENCIES = ../libdar/$(MYLIB).la

test_deci_SOURCES = test_deci.cpp
test_deci_DEPENDENCIES = ../libdar/$(MYLIB).la

test_path_SOURCES = test_path.cpp
test_path_DEPENDENCIES = ../libdar/$(MYLIB).la

test_erreurs_SOURCES = test_erreurs.cpp testtools.cpp
test_erreurs_DEPENDENCIES = ../libdar/$(MYLIB).la

test_sar_SOURCES = test_sar.cpp testtools.cpp
test_sar_DEPENDENCIES = ../libdar/$(MYLIB).la

test_filesystem_SOURCES = test_filesystem.cpp
test_filesystem_DEPENDENCIES = ../libdar/$(MYLIB).la

test_scrambler_SOURCES = test_scrambler.cpp
test_scrambler_LDADD =   ../dar_suite/dar_suite.o ../dar_suite/line_tools.o $(LDADD)
test_scrambler_DEPENDENCIES = ../libdar/$(MYLIB).la

test_generic_file_SOURCES = test_generic_file.cpp
test_generic_file_DEPENDENCIES = ../libdar/$(MYLIB).la

test_storage_SOURCES = test_storage.cpp
test_storage_DEPENDENCIES = ../libdar/$(MYLIB).la

test_limitint_SOURCES = test_limitint.cpp
test_limitint_DEPENDENCIES = ../libdar/$(MYLIB).la
test_limitint_CXXFLAGS = -Wno-long-long

test_libdar_SOURCES = test_libdar.cpp
test_libdar_DEPENDENCIES = ../libdar/$(MYLIB).la $(srcdir)/../libdar/libdar.hpp $(srcdir)/../libdar/tools.hpp

test_cache_SOURCES = test_cache.cpp
test_cache_DEPENDENCIES = ../libdar/$(MYLIB).la $(srcdir)/../libdar/libdar.hpp $(srcdir)/../libdar/tools.hpp

test_tronconneuse_SOURCES = test_tronconneuse.cpp
test_tronconneuse_DEPENDENCIES = ../libdar/$(MYLIB).la

test_elastic_SOURCES = test_elastic.cpp
test_elastic_DEPENDENCIES = ../libdar/$(MYLIB).la

test_blowfish_SOURCES = test_blowfish.cpp
test_blowfish_DEPENDENCIES = ../libdar/$(MYLIB).la

test_mask_list_SOURCES = test_mask_list.cpp
test_mask_list_DEPENDENCIES = ../libdar/$(MYLIB).la

test_escape_SOURCES = test_escape.cpp
test_escape_DEPENDENCIES = ../libdar/$(MYLIB).la

test_hash_fichier_SOURCES = test_hash_fichier.cpp
test_hash_fichier_DEPENDENCIES = ../libdar/$(MYLIB).la

moving_file_SOURCES = moving_file.cpp
make_sparse_file_SOURCES = make_sparse_file.c
make_sparse_file_CPPFLAGS = -I'$(srcdir)/../libdar' -DDAR_LOCALEDIR=\"$(localedir)\" -DDAR_SYS_DIR=\"$(system_path)\" $(CPP_PROF)

hashsum_SOURCES = hashsum.c
hashsum_CPPFLAGS = -I'$(srcdir)/../libdar' -DDAR_LOCALEDIR=\"$(localedir)\" -DDAR_SYS_DIR=\"$(system_path)\" $(CPP_PROF)

test_crypto_asym_SOURCES = test_crypto_asym.cpp
test_crypto_asym_DEPENDENCIES = ../libdar/$(MYLIB).la

test_range_SOURCES = test_range.cpp
test_range_DEPENDENCIES = ../libdar/$(MYLIB).la

test_rsync_SOURCES = test_rsync.cpp
test_rsync_DEPENDENCIES = ../libdar/$(MYLIB).la

test_smart_pointer_SOURCES = test_smart_pointer.cpp
test_smart_pointer_DEPENDENCIES = ../libdar/$(MYLIB).la

test_datetime_SOURCES = test_datetime.cpp
test_datetime_DEPENDENCIES = ../libdar/$(MYLIB).la

test_entrepot_libcurl_SOURCES = test_entrepot_libcurl.cpp
test_entrepot_libcurl_DEPENDENCIES = ../libdar/$(MYLIB).la

test_truncate_SOURCES = test_truncate.cpp
test_truncate_DEPENDENCIES = ../libdar/$(MYLIB).la

test_heap_SOURCES = test_heap.cpp
test_heap_DEPENDENCIES = ../libdar/$(MYLIB).la

test_parallel_tronconneuse_SOURCES = test_parallel_tronconneuse.cpp
test_parallel_tronconneuse_DEPENDENCIES = ../libdar/$(MYLIB).la

test_block_compressor_SOURCES = test_block_compressor.cpp
test_block_compressor_DEPENDENCIES = ../libdar/$(MYLIB).la
