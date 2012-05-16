if BUILD_MODE32
    MYLIB=dar32
    AM_CPPFLAGS=-DLIBDAR_MODE=32 -I../libdar -I../dar_suite
else
if BUILD_MODE64
    MYLIB=dar64
    AM_CPPFLAGS=-DLIBDAR_MODE=64 -I../libdar -I../dar_suite
else
    MYLIB=dar
    AM_CPPFLAGS=-I../libdar -I../dar_suite
endif
endif

noinst_PROGRAMS = test_hide_file test_terminateur test_catalogue test_infinint test_tronc test_compressor test_mask test_tuyau test_deci test_path test_erreurs test_sar test_filesystem test_scrambler test_generic_file test_storage test_special_alloc test_limitint test_libdar
LDADD = -L../libdar -l$(MYLIB)
AM_LDFLAGS = -all-static
noinst_HEADERS = testtools.hpp

test_hide_file_SOURCES = test_hide_file.cpp ../dar_suite/no_comment.cpp ../dar_suite/hide_file.cpp ../dar_suite/config_file.cpp
test_hide_file_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_terminateur_SOURCES = test_terminateur.cpp
test_terminateur_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_catalogue_SOURCES = test_catalogue.cpp ../dar_suite/shell_interaction.cpp
test_catalogue_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_infinint_SOURCES = test_infinint.cpp
test_infinint_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_tronc_SOURCES = test_tronc.cpp  ../dar_suite/shell_interaction.cpp testtools.cpp
test_tronc_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_compressor_SOURCES = test_compressor.cpp
test_compressor_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_mask_SOURCES = test_mask.cpp
test_mask_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_tuyau_SOURCES = test_tuyau.cpp ../dar_suite/shell_interaction.cpp ../dar_suite/dar_suite.cpp
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

test_scrambler_SOURCES = test_scrambler.cpp  ../dar_suite/dar_suite.cpp ../dar_suite/shell_interaction.cpp
test_scrambler_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_generic_file_SOURCES = test_generic_file.cpp
test_generic_file_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_storage_SOURCES = test_storage.cpp
test_storage_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_special_alloc_SOURCES = test_special_alloc.cpp ../dar_suite/shell_interaction.cpp
test_special_alloc_DEPENDENCIES = ../libdar/lib$(MYLIB).la

test_limitint_SOURCES = test_limitint.cpp ../dar_suite/shell_interaction.cpp
test_limitint_DEPENDENCIES = ../libdar/lib$(MYLIB).la
test_limitint_CXXFLAGS = -Wno-long-long

test_libdar_SOURCES = test_libdar.cpp
test_libdar_DEPENDENCIES = ../libdar/lib$(MYLIB).la ../libdar/libdar.hpp ../libdar/tools.hpp
