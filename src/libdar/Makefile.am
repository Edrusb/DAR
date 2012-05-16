if BUILD_MODE32
    MYLIB=libdar32.la
    AM_CPPFLAGS=-DLIBDAR_MODE=32
    VER_SRC=limitint.o
else
if BUILD_MODE64
    MYLIB=libdar64.la
    AM_CPPFLAGS=-DLIBDAR_MODE=64
    VER_SRC=limitint.o
else
    MYLIB=libdar.la
    AM_CPPFLAGS=
    VER_SRC=real_infinint.o
endif
endif

lib_LTLIBRARIES = $(MYLIB)
noinst_PROGRAMS = get_version

dist_noinst_DATA = libdar.hpp path.hpp mask.hpp integers.hpp real_infinint.hpp statistics.hpp user_interaction.hpp erreurs.hpp deci.hpp limitint.hpp infinint.hpp compressor.hpp special_alloc.hpp generic_file.hpp wrapperlib.hpp storage.hpp tuyau.hpp tools.hpp catalogue.hpp scrambler.hpp archive.hpp header_version.hpp ea.hpp crypto.hpp int_tools.hpp
install-data-local:
	mkdir -p $(DESTDIR)$(pkgincludedir)
	cp ../../config.h $(DESTDIR)$(pkgincludedir)
	for file in $(dist_noinst_DATA) ; do sed -e "s%#include \"../my_config.h\"%INC_CONFIG_FILE_H%g" "$$file" | sed -e "s%#include \"%#include \"$(pkgincludedir)/%g" | sed -e "s%INC_CONFIG_FILE_H%#include \"$(pkgincludedir)/config.h\"%g" > $(DESTDIR)$(pkgincludedir)/"$$file"  ; done
	for file in $(DESTDIR)$(pkgincludedir)/* ; do sed -e "s%HAVE_%LIBDAR_HAS_%g" "$$file" | sed -e "s%PACKAGE%DAR_PACKAGE%g" | sed -e "s%SIZEOF_%LIBDAR_SIZEOF_%g" | sed -e "s%LSTAT_FOLLOWS_SLASHED_SYMLINK%LIBDAR_LSTAT_FOLLOWS_SLASHED_SYMLINK%g" | sed -e "s%VERSION%LIBDAR_VERSION%g" | sed -e "s%OS_BITS%LIBDAR_OS_BITS%g" > "$$file.tmp" && mv "$$file.tmp" "$$file" ; done
	grep DAR $(DESTDIR)$(pkgincludedir)/config.h | grep -v "#undef" >  $(DESTDIR)$(pkgincludedir)/config.h.tmp
	mv  $(DESTDIR)$(pkgincludedir)/config.h.tmp  $(DESTDIR)$(pkgincludedir)/config.h
	chmod 0644 $(DESTDIR)$(pkgincludedir)/*

noinst_HEADERS = defile.hpp ea_filesystem.hpp etage.hpp filesystem.hpp filtre.hpp header.hpp macro_tools.hpp null_file.hpp sar.hpp sar_tools.hpp terminateur.hpp tronc.hpp zapette.hpp test_memory.hpp cygwin_adapt.hpp

libdar_la_LDFLAGS = -version-info `./get_version$(EXEEXT)`
libdar_la_SOURCES = catalogue.cpp catalogue.hpp compressor.cpp compressor.hpp deci.cpp deci.hpp defile.cpp defile.hpp ea.cpp ea.hpp ea_filesystem.cpp ea_filesystem.hpp erreurs.cpp erreurs.hpp etage.cpp etage.hpp filesystem.cpp filesystem.hpp filtre.cpp filtre.hpp generic_file.cpp generic_file.hpp header.cpp header_version.cpp real_infinint.cpp libdar.cpp macro_tools.cpp mask.cpp path.cpp sar.cpp sar_tools.cpp scrambler.cpp storage.cpp terminateur.cpp test_memory.cpp tools.cpp tronc.cpp tuyau.cpp user_interaction.cpp wrapperlib.cpp zapette.cpp special_alloc.cpp archive.cpp crypto.cpp int_tools.cpp
libdar_la_DEPENDENCIES = get_version$(EXEEXT)

libdar32_la_LDFLAGS = -version-info `./get_version$(EXEEXT)`
libdar32_la_SOURCES = catalogue.cpp catalogue.hpp compressor.cpp compressor.hpp deci.cpp deci.hpp defile.cpp defile.hpp ea.cpp ea.hpp ea_filesystem.cpp ea_filesystem.hpp erreurs.cpp erreurs.hpp etage.cpp etage.hpp filesystem.cpp filesystem.hpp filtre.cpp filtre.hpp generic_file.cpp generic_file.hpp header.cpp header_version.cpp libdar.cpp macro_tools.cpp mask.cpp path.cpp sar.cpp sar_tools.cpp scrambler.cpp terminateur.cpp test_memory.cpp tools.cpp tronc.cpp tuyau.cpp user_interaction.cpp wrapperlib.cpp zapette.cpp special_alloc.cpp limitint.cpp storage.cpp archive.cpp crypto.cpp int_tools.cpp
libdar32_la_DEPENDENCIES = get_version$(EXEEXT)

libdar64_la_LDFLAGS = -version-info `./get_version$(EXEEXT)`
libdar64_la_SOURCES = catalogue.cpp catalogue.hpp compressor.cpp compressor.hpp deci.cpp deci.hpp defile.cpp defile.hpp ea.cpp ea.hpp ea_filesystem.cpp ea_filesystem.hpp erreurs.cpp erreurs.hpp etage.cpp etage.hpp filesystem.cpp filesystem.hpp filtre.cpp filtre.hpp generic_file.cpp generic_file.hpp header.cpp header_version.cpp libdar.cpp macro_tools.cpp mask.cpp path.cpp sar.cpp sar_tools.cpp scrambler.cpp terminateur.cpp test_memory.cpp tools.cpp tronc.cpp tuyau.cpp user_interaction.cpp wrapperlib.cpp zapette.cpp special_alloc.cpp limitint.cpp storage.cpp archive.cpp crypto.cpp int_tools.cpp
libdar64_la_DEPENDENCIES = get_version$(EXEEXT)

get_version_SOURCES = get_version.cpp
get_version_DEPENDENCIES = catalogue.o compressor.o deci.o defile.o ea.o ea_filesystem.o erreurs.o etage.o filesystem.o filtre.o generic_file.o header.o header_version.o libdar.o macro_tools.o mask.o path.o sar.o sar_tools.o scrambler.o storage.o terminateur.o test_memory.o tools.o tronc.o tuyau.o user_interaction.o wrapperlib.o zapette.o special_alloc.o archive.o crypto.o int_tools.o $(VER_SRC)
get_version_LDADD = catalogue.o compressor.o deci.o defile.o ea.o ea_filesystem.o erreurs.o etage.o filesystem.o filtre.o generic_file.o header.o header_version.o libdar.o macro_tools.o mask.o path.o sar.o sar_tools.o scrambler.o storage.o terminateur.o test_memory.o tools.o tronc.o tuyau.o user_interaction.o wrapperlib.o zapette.o special_alloc.o archive.o crypto.o int_tools.o $(VER_SRC)
