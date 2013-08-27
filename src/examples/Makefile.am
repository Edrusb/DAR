localedir = @localedir@

if BUILD_MODE32
  MYLIB=dar32
  AM_CPPFLAGS=-DLIBDAR_MODE=32 -I../libdar -DDAR_LOCALEDIR=\"$(localedir)\"
else
if BUILD_MODE64
    MYLIB=dar64
    AM_CPPFLAGS=-DLIBDAR_MODE=64 -I../libdar -DDAR_LOCALEDIR=\"$(localedir)\"
else
    MYLIB=dar
    AM_CPPFLAGS=-I../libdar -DDAR_LOCALEDIR=\"$(localedir)\"
endif
endif

noinst_PROGRAMS = factoriel prime formula
dist_noinst_DATA = README
LDADD = -L../libdar -l$(MYLIB)

factoriel_SOURCES = factoriel.cpp ../dar_suite/shell_interaction.cpp ../dar_suite/dar_suite.cpp ../dar_suite/line_tools.cpp
factoriel_DEPENDENCIES = ../libdar/lib$(MYLIB).la ../dar_suite/shell_interaction.cpp ../dar_suite/dar_suite.cpp

prime_SOURCES = prime.cpp ../dar_suite/shell_interaction.cpp ../dar_suite/dar_suite.cpp ../dar_suite/line_tools.cpp
prime_DEPENDENCIES = ../libdar/lib$(MYLIB).la ../dar_suite/shell_interaction.cpp ../dar_suite/dar_suite.cpp

formula_SOURCES = formula.cpp ../dar_suite/shell_interaction.cpp ../dar_suite/dar_suite.cpp ../dar_suite/crit_action_cmd_line.cpp ../dar_suite/line_tools.cpp
formula_DEPENDENCIES = ../libdar/lib$(MYLIB).la ../dar_suite/line_tools.cpp ../dar_suite/shell_interaction.cpp ../dar_suite/crit_action_cmd_line.cpp ../dar_suite/dar_suite.cpp



