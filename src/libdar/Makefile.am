if PROFILING
  LD_PROF = -pg
  CPP_PROF = -pg
clean-local:
	rm -f gmon.out
	rm -f libdar.pc
else
  LD_PROF =
  CPP_PROF =
clean-local:
	rm -f libdar.pc
endif

localedir = @localedir@

if BUILD_MODE32
  MYLIB=libdar32.la
  AM_CPPFLAGS=-DLIBDAR_MODE=32 -DDAR_LOCALEDIR=\"$(localedir)\" $(CPP_PROF)
  VER_SRC=limitint.o
  LIBDAR_SUFFIX=32
  LIBDAR_MODE=-DMODE=32
else
if BUILD_MODE64
    MYLIB=libdar64.la
    AM_CPPFLAGS=-DLIBDAR_MODE=64 -DDAR_LOCALEDIR=\"$(localedir)\" $(CPP_PROF)
    VER_SRC=limitint.o
    LIBDAR_SUFFIX=64
    LIBDAR_MODE=-DMODE=64
else
    MYLIB=libdar.la
    AM_CPPFLAGS= -DDAR_LOCALEDIR=\"$(localedir)\" $(CPP_PROF)
    VER_SRC=real_infinint.o
    LIBDAR_SUFFIX=
    LIBDAR_MODE=
endif
endif

AM_LDFLAGS = @LTLIBINTL@  $(LD_PROF)
lib_LTLIBRARIES = $(MYLIB)

LIBDAR_MAJOR=`grep LIBDAR_COMPILE_TIME_MAJOR libdar.hpp | cut -f 2 -d "=" | cut -f 1 -d ';' | sed -e 's% %%g'`
LIBDAR_MEDIUM=`grep LIBDAR_COMPILE_TIME_MEDIUM libdar.hpp | cut -f 2 -d "=" | cut -f 1 -d ';' | sed -e 's% %%g'`
LIBDAR_MINOR=`grep LIBDAR_COMPILE_TIME_MINOR libdar.hpp | cut -f 2 -d "=" | cut -f 1 -d ';' | sed -e 's% %%g'`
LIBDAR_VERSION_IN=$(LIBDAR_MAJOR):$(LIBDAR_MINOR)
LIBDAR_VERSION_OUT=$(LIBDAR_MAJOR).$(LIBDAR_MEDIUM).$(LIBDAR_MINOR)


dist_noinst_DATA = libdar.hpp path.hpp mask.hpp integers.hpp real_infinint.hpp statistics.hpp user_interaction.hpp erreurs.hpp deci.hpp limitint.hpp infinint.hpp compressor.hpp special_alloc.hpp generic_file.hpp wrapperlib.hpp storage.hpp tuyau.hpp tools.hpp catalogue.hpp scrambler.hpp archive.hpp header_version.hpp ea.hpp crypto.hpp int_tools.hpp thread_cancellation.hpp tronconneuse.hpp mask_list.hpp database.hpp data_tree.hpp string_file.hpp user_group_bases.hpp
install-data-local:
	mkdir -p $(DESTDIR)$(pkgincludedir)
	cp ../../config.h $(DESTDIR)$(pkgincludedir)
	sed -e 's%../config.h%config.h%g' ../my_config.h | sed -e 's%HAVE_CONFIG_H%1%g' > ./my_config.h
	cp ../gettext.h .
	for file in $(dist_noinst_DATA) gettext.h my_config.h ; do sed -e 's%#include \"../my_config.h\"%INC_MY_CONFIG_FILE_H%g' "$$file" | sed -e "s%#include \"%#include \"$(pkgincludedir)/%g" | sed -e "s%INC_MY_CONFIG_FILE_H%#include \"$(pkgincludedir)/my_config.h\"%g" > $(DESTDIR)$(pkgincludedir)/"$$file"  ; done
	rm my_config.h gettext.h
	for file in $(DESTDIR)$(pkgincludedir)/* ; do sed -e 's%HAVE_%LIBDAR_HAS_%g' "$$file" | sed -e 's%PACKAGE%DAR_PACKAGE%g' | sed -e 's%SIZEOF_%LIBDAR_SIZEOF_%g' | sed -e 's%LSTAT_FOLLOWS_SLASHED_SYMLINK%LIBDAR_LSTAT_FOLLOWS_SLASHED_SYMLINK%g' | sed -e 's%VERSION%LIBDAR_VERSION%g' | sed -e 's%MUTEX_WORKS%LIBDAR_MUTEX_WORKS%g' | sed -e 's%OS_BITS%LIBDAR_OS_BITS%g' | sed -e 's%_AVAILABLE%_AVAILABLE_FOR_LIBDAR%g' | sed -e 's%STDC_HEADERS%LIBDAR_STDC_HEADERS%g' | sed -e 's%ENABLE_NLS%DAR_ENABLE_NLS%g' | sed -e 's%HAVE_GETTEXT%DAR_HAS_GETTEXT%g' > "$$file.tmp" && mv "$$file.tmp" "$$file" ; done
	grep DAR $(DESTDIR)$(pkgincludedir)/config.h | grep -v "#undef" >  $(DESTDIR)$(pkgincludedir)/config.h.tmp
	mv  $(DESTDIR)$(pkgincludedir)/config.h.tmp  $(DESTDIR)$(pkgincludedir)/config.h
	chmod 0644 $(DESTDIR)$(pkgincludedir)/*
	$(INSTALL) -d $(DESTDIR)$(libdir)/pkgconfig
	$(INSTALL) -m 0644 libdar.pc $(DESTDIR)$(libdir)/pkgconfig/libdar$(LIBDAR_SUFFIX).pc

uninstall-local:
	rm -rf $(DESTDIR)$(pkgincludedir) $(DESTDIR)$(libdir)/pkgconfig/libdar$(LIBDAR_SUFFIX).pc

all-local : libdar.pc

libdar.pc : libdar.pc.tmpl
	sed -e "s%#LIBDAR_VERSION#%$(LIBDAR_VERSION_OUT)%g" libdar.pc.tmpl | sed -e "s%#LIBDAR_SUFFIX#%$(LIBDAR_SUFFIX)%g" | sed -e "s%#LIBDAR_MODE#%$(LIBDAR_MODE)%g" > libdar.pc


noinst_HEADERS = defile.hpp ea_filesystem.hpp etage.hpp filesystem.hpp filtre.hpp header.hpp macro_tools.hpp null_file.hpp sar.hpp sar_tools.hpp terminateur.hpp tronc.hpp zapette.hpp test_memory.hpp cygwin_adapt.hpp elastic.hpp nls_swap.hpp database_header.hpp

libdar_la_LDFLAGS = -version-info $(LIBDAR_VERSION_IN)
libdar_la_SOURCES = catalogue.cpp catalogue.hpp compressor.cpp compressor.hpp deci.cpp deci.hpp defile.cpp defile.hpp ea.cpp ea.hpp ea_filesystem.cpp ea_filesystem.hpp erreurs.cpp erreurs.hpp etage.cpp etage.hpp filesystem.cpp filesystem.hpp filtre.cpp filtre.hpp generic_file.cpp generic_file.hpp header.cpp header_version.cpp real_infinint.cpp libdar.cpp macro_tools.cpp mask.cpp path.cpp sar.cpp sar_tools.cpp scrambler.cpp storage.cpp terminateur.cpp test_memory.cpp tools.cpp tronc.cpp tuyau.cpp user_interaction.cpp wrapperlib.cpp zapette.cpp special_alloc.cpp archive.cpp crypto.cpp cache.cpp cache.hpp int_tools.cpp tronconneuse.cpp elastic.cpp thread_cancellation.cpp mask_list.cpp database.cpp database_header.cpp data_tree.cpp string_file.cpp statistics.cpp user_group_bases.cpp

libdar32_la_LDFLAGS = -version-info $(LIBDAR_VERSION_IN)
libdar32_la_SOURCES = catalogue.cpp catalogue.hpp compressor.cpp compressor.hpp deci.cpp deci.hpp defile.cpp defile.hpp ea.cpp ea.hpp ea_filesystem.cpp ea_filesystem.hpp erreurs.cpp erreurs.hpp etage.cpp etage.hpp filesystem.cpp filesystem.hpp filtre.cpp filtre.hpp generic_file.cpp generic_file.hpp header.cpp header_version.cpp libdar.cpp macro_tools.cpp mask.cpp path.cpp sar.cpp sar_tools.cpp scrambler.cpp terminateur.cpp test_memory.cpp tools.cpp tronc.cpp tuyau.cpp user_interaction.cpp wrapperlib.cpp zapette.cpp special_alloc.cpp limitint.cpp storage.cpp archive.cpp crypto.cpp cache.cpp cache.hpp int_tools.cpp tronconneuse.cpp elastic.cpp thread_cancellation.cpp mask_list.cpp database.cpp database_header.cpp data_tree.cpp string_file.cpp statistics.cpp user_group_bases.cpp

libdar64_la_LDFLAGS = -version-info $(LIBDAR_VERSION_IN)
libdar64_la_SOURCES = catalogue.cpp catalogue.hpp compressor.cpp compressor.hpp deci.cpp deci.hpp defile.cpp defile.hpp ea.cpp ea.hpp ea_filesystem.cpp ea_filesystem.hpp erreurs.cpp erreurs.hpp etage.cpp etage.hpp filesystem.cpp filesystem.hpp filtre.cpp filtre.hpp generic_file.cpp generic_file.hpp header.cpp header_version.cpp libdar.cpp macro_tools.cpp mask.cpp path.cpp sar.cpp sar_tools.cpp scrambler.cpp terminateur.cpp test_memory.cpp tools.cpp tronc.cpp tuyau.cpp user_interaction.cpp wrapperlib.cpp zapette.cpp special_alloc.cpp limitint.cpp storage.cpp archive.cpp crypto.cpp cache.cpp cache.hpp int_tools.cpp tronconneuse.cpp elastic.cpp thread_cancellation.cpp mask_list.cpp database.cpp database_header.cpp data_tree.cpp string_file.cpp statistics.cpp user_group_bases.cpp
