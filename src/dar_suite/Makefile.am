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
	cd $(DESTDIR)$(bindir) ; $(UPX) -9 dar$(EXEEXT) dar_xform$(EXEEXT) dar_slave$(EXEEXT) dar_manager$(EXEEXT) dar_cp$(EXEEXT) $(ALSO_BUILD_INST)

else
install-exec-hook:
	echo "no executable packing, UPX not found or disabled"
endif



#
# unconditionnal part
#

bin_PROGRAMS = dar dar_xform dar_slave dar_manager dar_cp dar_split $(ALSO_BUILD)
LDADD = ../libdar/$(MYLIB).la $(GPGME_LIBS)
if DEBUG_STATIC
AM_LDFLAGS = -all-static @LTLIBINTL@ $(LD_PROF)
else
AM_LDFLAGS = @LTLIBINTL@ $(LD_PROF)
endif

dar_SOURCES =  command_line.cpp command_line.hpp config_file.cpp config_file.hpp dar.cpp dar.hpp dar_suite.cpp dar_suite.hpp hide_file.cpp hide_file.hpp no_comment.cpp no_comment.hpp getopt_decision.h my_getopt_long.h line_tools.hpp line_tools.cpp crit_action_cmd_line.cpp crit_action_cmd_line.hpp
dar_DEPENDENCIES = ../libdar/$(MYLIB).la

dar_xform_SOURCES = dar_suite.cpp dar_suite.hpp dar_xform.cpp line_tools.hpp line_tools.cpp
dar_xform_DEPENDENCIES = ../libdar/$(MYLIB).la

dar_slave_SOURCES =  dar_suite.cpp dar_suite.hpp dar_slave.cpp line_tools.hpp line_tools.cpp
dar_slave_DEPENDENCIES = ../libdar/$(MYLIB).la

dar_manager_SOURCES = dar_suite.cpp dar_suite.hpp dar_manager.cpp getopt_decision.h my_getopt_long.h line_tools.hpp line_tools.cpp no_comment.cpp no_comment.hpp hide_file.cpp hide_file.hpp
dar_manager_DEPENDENCIES = ../libdar/$(MYLIB).la

dar_cp_SOURCES = dar_suite.cpp dar_suite.hpp dar_cp.cpp line_tools.hpp line_tools.cpp
dar_cp_DEPENDENCIES = ../libdar/$(MYLIB).la

dar_split_SOURCES = dar_split.c

#
# dar_static ( --enable-dar-static )
#

dar_static_SOURCES =  command_line.cpp command_line.hpp config_file.cpp config_file.hpp dar.cpp dar.hpp dar_suite.cpp dar_suite.hpp hide_file.cpp hide_file.hpp no_comment.cpp no_comment.hpp getopt_decision.h my_getopt_long.h line_tools.hpp line_tools.cpp crit_action_cmd_line.cpp crit_action_cmd_line.hpp
dar_static_DEPENDENCIES = ../libdar/$(MYLIB).la
dar_static_LDFLAGS = -all-static @LTLIBINTL@
