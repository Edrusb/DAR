localedir = @localedir@

if BUILD_MODE32
MYLIB=dar32
AM_CPPFLAGS=-DLIBDAR_MODE=32 -I../libdar -I../dar_suite -DDAR_LOCALEDIR=\"$(localedir)\"
else
if BUILD_MODE64
MYLIB=dar64
AM_CPPFLAGS=-DLIBDAR_MODE=64 -I../libdar -I../dar_suite -DDAR_LOCALEDIR=\"$(localedir)\"
else
MYLIB=dar
AM_CPPFLAGS=-I../libdar -I../dar_suite -DDAR_LOCALEDIR=\"$(localedir)\"
endif
endif

noinst_PROGRAMS = test_hide_file test_terminateur test_catalogue test_infinint test_tronc test_compressor test_mask test_tuyau test_deci test_path test_erreurs test_sar test_filesystem test_scrambler test_generic_file test_storage test_special_alloc test_limitint test_libdar test_cache test_tronconneuse test_elastic test_blowfish test_mask_list test_string_file test_escape test_hash_fichier test_old_api make_sparse_file
AM_LDFLAGS = -all-static @LTLIBINTL@

LDADD = -L../libdar -l$(MYLIB)
noinst_HEADERS = testtools.hpp

test_hide_file_SOURCES = test_hide_file.cpp ../dar_suite/no_comment.cpp ../dar_suite/hide_file.cpp ../dar_suite/config_file.cpp ../dar_suite/shell_interaction.cpp
test_hide_file_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_terminateur_SOURCES = test_terminateur.cpp ../dar_suite/shell_interaction.cpp
test_terminateur_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_catalogue_SOURCES = test_catalogue.cpp ../dar_suite/shell_interaction.cpp
test_catalogue_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_infinint_SOURCES = test_infinint.cpp ../dar_suite/shell_interaction.cpp
test_infinint_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_tronc_SOURCES = test_tronc.cpp  ../dar_suite/shell_interaction.cpp testtools.cpp
test_tronc_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_compressor_SOURCES = test_compressor.cpp ../dar_suite/shell_interaction.cpp
test_compressor_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_mask_SOURCES = test_mask.cpp
test_mask_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_tuyau_SOURCES = test_tuyau.cpp ../dar_suite/shell_interaction.cpp ../dar_suite/dar_suite.cpp ../dar_suite/line_tools.cpp
test_tuyau_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_deci_SOURCES = test_deci.cpp
test_deci_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_path_SOURCES = test_path.cpp
test_path_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_erreurs_SOURCES = test_erreurs.cpp
test_erreurs_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_sar_SOURCES = test_sar.cpp  ../dar_suite/shell_interaction.cpp testtools.cpp
test_sar_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_filesystem_SOURCES = test_filesystem.cpp ../dar_suite/shell_interaction.cpp
test_filesystem_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_scrambler_SOURCES = test_scrambler.cpp  ../dar_suite/dar_suite.cpp ../dar_suite/shell_interaction.cpp ../dar_suite/line_tools.cpp
test_scrambler_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_generic_file_SOURCES = test_generic_file.cpp ../dar_suite/shell_interaction.cpp
test_generic_file_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_storage_SOURCES = test_storage.cpp
test_storage_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_special_alloc_SOURCES = test_special_alloc.cpp
test_special_alloc_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_limitint_SOURCES = test_limitint.cpp ../dar_suite/shell_interaction.cpp
test_limitint_DEPENDENCIES = ../libdar/lib$(MYLIB).la
test_limitint_CXXFLAGS = -Wno-long-long

test_libdar_SOURCES = test_libdar.cpp ../dar_suite/shell_interaction.cpp
test_libdar_DEPENDENCIES = ../libdar/lib$(MYLIB).la ../libdar/libdar.hpp ../libdar/tools.hpp

test_cache_SOURCES = test_cache.cpp ../dar_suite/shell_interaction.cpp
test_cache_DEPENDENCIES = ../libdar/lib$(MYLIB).la ../libdar/libdar.hpp ../libdar/tools.hpp

test_tronconneuse_SOURCES = test_tronconneuse.cpp ../dar_suite/shell_interaction.cpp
test_tronconneuse_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_elastic_SOURCES = test_elastic.cpp ../dar_suite/shell_interaction.cpp
test_elastic_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_blowfish_SOURCES = test_blowfish.cpp ../dar_suite/shell_interaction.cpp
test_blowfish_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_mask_list_SOURCES = test_mask_list.cpp ../dar_suite/shell_interaction.cpp
test_mask_list_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_string_file_SOURCES = test_string_file.cpp ../dar_suite/shell_interaction.cpp testtools.cpp
test_string_file_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_escape_SOURCES = test_escape.cpp ../dar_suite/shell_interaction.cpp
test_escape_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_hash_fichier_SOURCES = test_hash_fichier.cpp ../dar_suite/shell_interaction.cpp
test_hash_fichier_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_old_api_SOURCES = test_old_api.cpp
test_old_api_DEPENDENCIES = ../libdar/lib$(MYLIB).la

make_sparse_file_SOURCES = make_sparse_file.c
