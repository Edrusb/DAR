if BUILD_MODE32
  MYLIB=dar32
  AM_CPPFLAGS=-DLIBDAR_MODE=32 -I../libdar
else
if BUILD_MODE64
    MYLIB=dar64
    AM_CPPFLAGS=-DLIBDAR_MODE=64 -I../libdar
else
    MYLIB=dar
    AM_CPPFLAGS=-I../libdar
endif
endif

noinst_PROGRAMS = factoriel prime
dist_noinst_DATA = README
LDADD = -L../libdar -l$(MYLIB)

factoriel_SOURCES = factoriel.cpp ../dar_suite/shell_interaction.cpp ../dar_suite/dar_suite.cpp
factoriel_DEPENDENCIES = ../libdar/lib$(MYLIB).la ../dar_suite/shell_interaction.cpp ../dar_suite/dar_suite.cpp

prime_SOURCES = prime.cpp ../dar_suite/shell_interaction.cpp ../dar_suite/dar_suite.cpp
prime_DEPENDENCIES = ../libdar/lib$(MYLIB).la ../dar_suite/shell_interaction.cpp ../dar_suite/dar_suite.cpp
