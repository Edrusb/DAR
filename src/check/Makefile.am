
#
# conditionnal part: --enable-mode=...
#

localedir = @localedir@
system_path = @sysconfdir@

if BUILD_MODE32
    MYLIB=libdar32
    AM_CPPFLAGS=-DLIBDAR_MODE=32 -I../libdar -DDAR_LOCALEDIR=\"$(localedir)\" -DDAR_SYS_DIR=\"$(system_path)\" $(CPP_PROF) $(GPGME_CFLAGS)
else
if BUILD_MODE64
    MYLIB=libdar64
    AM_CPPFLAGS=-DLIBDAR_MODE=64 -I../libdar -DDAR_LOCALEDIR=\"$(localedir)\" -DDAR_SYS_DIR=\"$(system_path)\" $(CPP_PROF) $(GPGME_CFLAGS)
else
    MYLIB=libdar
    AM_CPPFLAGS=-I../libdar -DDAR_LOCALEDIR=\"$(localedir)\"  -DDAR_SYS_DIR=\"$(system_path)\" $(CPP_PROF) $(GPGME_CFLAGS)
endif
endif

helpers = build_tree.sh main.sh modif_tree.sh routine.sh loop.sh

dist_noinst_DATA = $(helpers) Old_format/archive_03.1.dar Old_format/archive_04.1.dar Old_format/archive_05.1.dar Old_format/archive_06.1.dar Old_format/archive_07.1.dar Old_format/archive_08.1.dar Old_format/archive_08-1.1.dar Old_format/archive_09.1.dar
noinst_PROGRAMS = all_features padder
LDADD = -L../libdar ../libdar/$(MYLIB).la $(GPGME_LIBS)
AM_LDFLAGS = @LTLIBINTL@

check-local:
	chmod a+x $(helpers)
	./loop.sh

all_features_SOURCES = all_features.cpp
all_features_LDADD = $(LDADD)
all_features_DEPENDENCIES = ../libdar/$(MYLIB).la

padder_SOURCES = padder.cpp

clean-local:
	chmod u+x $(helpers)
	if [ -x /bin/bash ] ; then OPT=tmp.file DAR=none ./routine.sh NONE ; fi
	rm -f tmp.file
