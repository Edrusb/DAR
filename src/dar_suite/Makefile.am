#
# conditional part: --enable-build-usage
#

if MAKE_USAGE

%.usage: %.xml dar-help$(EXEEXT)
	./dar-help$(EXEEXT) $< > $@

noinst_PROGRAMS = dar-help
dar_help_SOURCES = dar-help.c

command_line.cpp : dar.usage
dar_xform.cpp : dar_xform.usage
dar_slave.cpp : dar_slave.usage
dar_manager.cpp : dar_manager.usage

endif

dist_noinst_DATA = dar.xml dar_xform.xml dar_slave.xml dar_manager.xml

#
# conditionnal part: --enable-build-static
#

if BUILD_DAR_STATIC
    ALSO_BUILD = dar_static
else
    ALSO_BUILD =
endif

#
# Conditionnal part : Profiling
#

if PROFILING
    LD_PROF = -pg
    CPP_PROF = -pg
clean-local:
	rm -f gmon.out
else
    LD_PROF =
    CPP_PROF =
clean-local:

endif


#
# conditionnal part: --enable-mode=...
#

if BUILD_MODE32
    MYLIB=dar32
    AM_CPPFLAGS=-DLIBDAR_MODE=32 -I../libdar $(CPP_PROF)
else
if BUILD_MODE64
    MYLIB=dar64
    AM_CPPFLAGS=-DLIBDAR_MODE=64 -I../libdar  $(CPP_PROF)
else
    MYLIB=dar
    AM_CPPFLAGS=-I../libdar $(CPP_PROF)
endif
endif

#
# conditionnal part UPX
#

if USE_UPX
UPX = @UPX_PROG@

if BUILD_DAR_STATIC
    ALSO_BUILD_INST = $(ALSO_BUILD)$(EXEEXT)
else
    ALSO_BUILD_INST =
endif

install-exec-hook :
	cd $(DESTDIR)/$(bindir) ; $(UPX) -9 dar$(EXEEXT) dar_xform$(EXEEXT) dar_slave$(EXEEXT) dar_manager$(EXEEXT) dar_cp$(EXEEXT) $(ALSO_BUILD_INST)

else
install-exec-hook:
	echo "no executable packing, UPX not found or disabled"
endif



#
# unconditionnal part
#

bin_PROGRAMS = dar dar_xform dar_slave dar_manager dar_cp $(ALSO_BUILD)
LDADD = -L../libdar -l$(MYLIB)
if DEBUG_STATIC
AM_LDFLAGS = -all-static @LTLIBINTL@ $(LD_PROF)
else
AM_LDFLAGS = @LTLIBINTL@ $(LD_PROF)
endif

dar_SOURCES =  command_line.cpp command_line.hpp config_file.cpp config_file.hpp dar.cpp dar.hpp dar_suite.cpp dar_suite.hpp hide_file.cpp hide_file.hpp no_comment.cpp no_comment.hpp shell_interaction.cpp shell_interaction.hpp dar.usage getopt_decision.h my_getopt_long.h
dar_DEPENDENCIES = ../libdar/lib$(MYLIB).la

dar_xform_SOURCES = dar_suite.cpp dar_suite.hpp dar_xform.cpp shell_interaction.cpp shell_interaction.hpp dar_xform.usage
dar_xform_DEPENDENCIES = ../libdar/lib$(MYLIB).la

dar_slave_SOURCES =  dar_suite.cpp dar_suite.hpp dar_slave.cpp shell_interaction.cpp shell_interaction.hpp dar_slave.usage
dar_slave_DEPENDENCIES = ../libdar/lib$(MYLIB).la

dar_manager_SOURCES = dar_suite.cpp dar_suite.hpp dar_manager.cpp shell_interaction.cpp shell_interaction.hpp dar_manager.usage getopt_decision.h my_getopt_long.h
dar_manager_DEPENDENCIES = ../libdar/lib$(MYLIB).la

dar_cp_SOURCES = dar_cp.cpp dar_suite.cpp shell_interaction.cpp
dar_cp_DEPENDENCIES = ../libdar/lib$(MYLIB).la

#
# dar_static ( --enable-dar-static )
#

dar_static_SOURCES =  command_line.cpp command_line.hpp config_file.cpp config_file.hpp dar.cpp dar.hpp dar_suite.cpp dar_suite.hpp hide_file.cpp hide_file.hpp no_comment.cpp no_comment.hpp shell_interaction.cpp shell_interaction.hpp dar.usage
dar_static_DEPENDENCIES = ../libdar/lib$(MYLIB).la
dar_static_LDFLAGS = -all-static @LTLIBINTL@

