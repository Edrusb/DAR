localedir = @localedir@

if BUILD_MODE32
  MYLIB=libdar32
  AM_CPPFLAGS=-DLIBDAR_MODE=32 -I../libdar -DDAR_LOCALEDIR=\"$(localedir)\" $(GPGME_CFLAGS)
else
if BUILD_MODE64
    MYLIB=libdar64
    AM_CPPFLAGS=-DLIBDAR_MODE=64 -I../libdar -DDAR_LOCALEDIR=\"$(localedir)\" $(GPGME_CFLAGS)
else
    MYLIB=libdar
    AM_CPPFLAGS=-I../libdar -DDAR_LOCALEDIR=\"$(localedir)\" $(GPGME_CFLAGS)
endif
endif

noinst_PROGRAMS = factoriel prime formula
dist_noinst_DATA = README

LDADD = ../libdar/$(MYLIB).la $(GPGME_LIBS)
AM_LDFLAGS = @LTLIBINTL@

factoriel_SOURCES = factoriel.cpp
factoriel_LDADD =  ../dar_suite/dar_suite.o ../dar_suite/line_tools.o $(LDADD)
factoriel_DEPENDENCIES = ../libdar/$(MYLIB).la ../dar_suite/dar_suite.cpp

prime_SOURCES = prime.cpp
prime_LDADD = ../dar_suite/dar_suite.o ../dar_suite/line_tools.o  $(LDADD)
prime_DEPENDENCIES = ../libdar/$(MYLIB).la ../dar_suite/dar_suite.cpp

formula_SOURCES = formula.cpp
formula_LDADD = ../dar_suite/dar_suite.o ../dar_suite/crit_action_cmd_line.o ../dar_suite/line_tools.o  $(LDADD)
formula_DEPENDENCIES = ../libdar/$(MYLIB).la ../dar_suite/line_tools.cpp ../dar_suite/crit_action_cmd_line.cpp ../dar_suite/dar_suite.cpp
