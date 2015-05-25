localedir = @localedir@

if BUILD_MODE32
MYLIB=dar32
AM_CPPFLAGS=-DLIBDAR_MODE=32 -I../libdar -I../dar_suite -DDAR_LOCALEDIR=\"$(localedir)\" $(GPGME_CFLAGS)
else
if BUILD_MODE64
MYLIB=dar64
AM_CPPFLAGS=-DLIBDAR_MODE=64 -I../libdar -I../dar_suite -DDAR_LOCALEDIR=\"$(localedir)\" $(GPGME_CFLAGS)
else
MYLIB=dar
AM_CPPFLAGS=-I../libdar -I../dar_suite -DDAR_LOCALEDIR=\"$(localedir)\" $(GPGME_CFLAGS)
endif
endif

if WITH_LIBTHREADAR
    LIBTHREADAR_TEST_MODULES=test_generic_thread
else
    LIBTHREADAR_TEST_MODULES=
endif


noinst_PROGRAMS = test_hide_file test_terminateur test_catalogue test_infinint test_tronc test_compressor test_mask test_tuyau test_deci test_path test_erreurs test_sar test_filesystem test_scrambler test_generic_file test_storage test_special_alloc test_limitint test_libdar test_cache test_tronconneuse test_elastic test_blowfish test_mask_list test_escape test_hash_fichier moving_file make_sparse_file hashsum test_crypto_asym test_range $(LIBTHREADAR_TEST_MODULES)
AM_LDFLAGS = -all-static @LTLIBINTL@
LDADD = -L../libdar -l$(MYLIB) $(GPGME_LIBS)

noinst_HEADERS = testtools.hpp

test_hide_file_SOURCES = test_hide_file.cpp
test_hide_file_LDADD = ../dar_suite/no_comment.o ../dar_suite/hide_file.o ../dar_suite/config_file.o ../dar_suite/shell_interaction.o $(LDADD)
test_hide_file_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_terminateur_SOURCES = test_terminateur.cpp
test_terminateur_LDADD = ../dar_suite/shell_interaction.o $(LDADD)
test_terminateur_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_catalogue_SOURCES = test_catalogue.cpp
test_catalogue_LDADD = ../dar_suite/shell_interaction.o $(LDADD)
test_catalogue_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_infinint_SOURCES = test_infinint.cpp
test_infinint_LDADD =  ../dar_suite/shell_interaction.o $(LDADD)
test_infinint_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_tronc_SOURCES = test_tronc.cpp testtools.cpp
test_tronc_LDADD =  ../dar_suite/shell_interaction.o $(LDADD)
test_tronc_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_compressor_SOURCES = test_compressor.cpp
test_compressor_LDADD = ../dar_suite/shell_interaction.o $(LDADD)
test_compressor_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_mask_SOURCES = test_mask.cpp
test_mask_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_tuyau_SOURCES = test_tuyau.cpp
test_tuyau_LDADD =  ../dar_suite/shell_interaction.o ../dar_suite/dar_suite.o ../dar_suite/line_tools.o $(LDADD)
test_tuyau_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_deci_SOURCES = test_deci.cpp
test_deci_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_path_SOURCES = test_path.cpp
test_path_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_erreurs_SOURCES = test_erreurs.cpp testtools.cpp
test_erreurs_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_sar_SOURCES = test_sar.cpp testtools.cpp
test_sar_LDADD = ../dar_suite/shell_interaction.o $(LDADD)
test_sar_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_filesystem_SOURCES = test_filesystem.cpp
test_filesystem_LDADD = ../dar_suite/shell_interaction.o $(LDADD)
test_filesystem_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_scrambler_SOURCES = test_scrambler.cpp
test_scrambler_LDADD =   ../dar_suite/dar_suite.o ../dar_suite/shell_interaction.o ../dar_suite/line_tools.o $(LDADD)
test_scrambler_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_generic_file_SOURCES = test_generic_file.cpp
test_generic_file_LDADD =  ../dar_suite/shell_interaction.o $(LDADD)
test_generic_file_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_storage_SOURCES = test_storage.cpp
test_storage_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_special_alloc_SOURCES = test_special_alloc.cpp
test_special_alloc_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_limitint_SOURCES = test_limitint.cpp
test_limitint_LDADD =  ../dar_suite/shell_interaction.o $(LDADD)
test_limitint_DEPENDENCIES = ../libdar/lib$(MYLIB).la
test_limitint_CXXFLAGS = -Wno-long-long

test_libdar_SOURCES = test_libdar.cpp
test_libdar_LDADD = ../dar_suite/shell_interaction.o $(LDADD)
test_libdar_DEPENDENCIES = ../libdar/lib$(MYLIB).la ../libdar/libdar.hpp ../libdar/tools.hpp

test_cache_SOURCES = test_cache.cpp
test_cache_LDADD =  ../dar_suite/shell_interaction.o $(LDADD)
test_cache_DEPENDENCIES = ../libdar/lib$(MYLIB).la ../libdar/libdar.hpp ../libdar/tools.hpp

test_tronconneuse_SOURCES = test_tronconneuse.cpp
test_tronconneuse_LDADD = ../dar_suite/shell_interaction.o $(LDADD)
test_tronconneuse_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_elastic_SOURCES = test_elastic.cpp
test_elastic_LDADD = ../dar_suite/shell_interaction.o $(LDADD)
test_elastic_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_blowfish_SOURCES = test_blowfish.cpp
test_blowfish_LDADD =  ../dar_suite/shell_interaction.o $(LDADD)
test_blowfish_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_mask_list_SOURCES = test_mask_list.cpp
test_mask_list_LDADD =  ../dar_suite/shell_interaction.o $(LDADD)
test_mask_list_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_escape_SOURCES = test_escape.cpp
test_escape_LDADD =  ../dar_suite/shell_interaction.o $(LDADD)
test_escape_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_hash_fichier_SOURCES = test_hash_fichier.cpp
test_hash_fichier_LDADD =  ../dar_suite/shell_interaction.o $(LDADD)
test_hash_fichier_DEPENDENCIES = ../libdar/lib$(MYLIB).la

moving_file_SOURCES = moving_file.cpp
make_sparse_file_SOURCES = make_sparse_file.c

hashsum_SOURCES = hashsum.c

test_crypto_asym_SOURCES = test_crypto_asym.cpp
test_crypto_asym_LDADD = ../dar_suite/shell_interaction.o $(LDADD)
test_crypto_asym_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_range_SOURCES = test_range.cpp
test_range_LDADD =  ../dar_suite/shell_interaction.o $(LDADD)
test_range_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_generic_thread_SOURCES = test_generic_thread.cpp
test_generic_thread_LDADD =  ../dar_suite/shell_interaction.o $(LDADD)
test_generic_thread_DEPENDENCIES = ../libdar/lib$(MYLIB).la
