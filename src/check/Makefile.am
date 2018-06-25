
#
# conditionnal part: --enable-mode=...
#

if BUILD_MODE32
    MYLIB=libdar32
    AM_CPPFLAGS=-DLIBDAR_MODE=32 -I../libdar -DDAR_LOCALEDIR=\"$(localedir)\" -DDAR_SYS_DIR=\"$(system_path)\" $(CPP_PROF) $(GPGME_CFLAGS) $(LIBCURL_CFLAGS) $(LIBTHREADAR_CFLAGS) $(CXXSTDFLAGS) $(DEBUG_CXXFLAGS)
else
if BUILD_MODE64
    MYLIB=libdar64
    AM_CPPFLAGS=-DLIBDAR_MODE=64 -I../libdar -DDAR_LOCALEDIR=\"$(localedir)\" -DDAR_SYS_DIR=\"$(system_path)\" $(CPP_PROF) $(GPGME_CFLAGS) $(LIBCURL_CFLAGS) $(LIBTHREADAR_CFLAGS) $(CXXSTDFLAGS) $(DEBUG_CXXFLAGS)
else
    MYLIB=libdar
    AM_CPPFLAGS=-I../libdar -DDAR_LOCALEDIR=\"$(localedir)\"  -DDAR_SYS_DIR=\"$(system_path)\" $(CPP_PROF) $(GPGME_CFLAGS) $(LIBCURL_CFLAGS) $(LIBTHREADAR_CFLAGS) $(CXXSTDFLAGS) $(DEBUG_CXXFLAGS)
endif
endif

helpers = build_tree.sh final.sh main.sh modif_tree.sh routine.sh loop.sh make_env.sh

dist_noinst_DATA = $(helpers) Old_format/archive_03.1.dar Old_format/archive_04.1.dar Old_format/archive_05.1.dar Old_format/archive_06.1.dar Old_format/archive_07.1.dar Old_format/archive_08.1.dar Old_format/archive_08-1.1.dar Old_format/archive_09.1.dar
noinst_PROGRAMS = all_features padder
LDADD = -L../libdar ../libdar/$(MYLIB).la $(GPGME_LIBS)
AM_LDFLAGS = $(LTLIBINTL) $(GPGME_LIBS) $(LIBCURL_LIBS) $(LIBTHREADAR_LIBS) $(CXXSTDFLAGS) $(DEBUG_LDFAGS)
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
all_features_LDADD = $(LDADD)
all_features_DEPENDENCIES = ../libdar/$(MYLIB).la

padder_SOURCES = padder.cpp

clean-local:
	rm -rf target_*
	rm -f $(MY_MAKEFILE) $(MY_ENV)

