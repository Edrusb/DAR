#######################################################################
# dar - disk archive - a backup/restoration program
# Copyright (C) 2002-2021 Denis Corbin
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
# to contact the author : http://dar.linux.free.fr/email.html
#######################################################################

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
    MYLIB=libdar32
    AM_CPPFLAGS=-DLIBDAR_MODE=32 -I'$(srcdir)/../libdar' -DDAR_LOCALEDIR=\"$(localedir)\" -DDAR_SYS_DIR=\"$(sysconfdir)\" $(CPP_PROF) $(GPGME_CFLAGS) $(LIBCURL_CFLAGS) $(LIBTHREADAR_CFLAGS) $(CXXSTDFLAGS)
else
if BUILD_MODE64
    MYLIB=libdar64
    AM_CPPFLAGS=-DLIBDAR_MODE=64 -I'$(srcdir)/../libdar' -DDAR_LOCALEDIR=\"$(localedir)\" -DDAR_SYS_DIR=\"$(sysconfdir)\" $(CPP_PROF) $(GPGME_CFLAGS) $(LIBCURL_CFLAGS) $(LIBTHREADAR_CFLAGS) $(CXXSTDFLAGS)
else
    MYLIB=libdar
    AM_CPPFLAGS=-I'$(srcdir)/../libdar' -DDAR_LOCALEDIR=\"$(localedir)\" -DDAR_SYS_DIR=\"$(sysconfdir)\" $(CPP_PROF) $(GPGME_CFLAGS) $(LIBCURL_CFLAGS) $(LIBTHREADAR_CFLAGS) $(CXXSTDFLAGS)
endif
endif

#
# conditionnal part UPX
#

if USE_UPX

if BUILD_DAR_STATIC
    ALSO_BUILD_INST = $(ALSO_BUILD)$(EXEEXT)
else
    ALSO_BUILD_INST =
endif

install-exec-hook :
	cd $(DESTDIR)$(bindir) ; $(UPX_PROG) -9 dar$(EXEEXT) dar_xform$(EXEEXT) dar_slave$(EXEEXT) dar_manager$(EXEEXT) dar_cp$(EXEEXT) dar_split$(EXEEXT) $(ALSO_BUILD_INST) || exit 0

# exit 0 because dar_split for example is today too small to be upx-compressed

else
install-exec-hook:
	echo "no executable packing, UPX not found or disabled"
endif



#
# unconditionnal part
#

bin_PROGRAMS = dar dar_xform dar_slave dar_manager dar_cp dar_split $(ALSO_BUILD)
LDADD = ../libdar/$(MYLIB).la $(LIBCURL_LIBS) $(GPGME_LIBS) $(LIBTHREADAR_LIBS) $(LTLIBINTL)
if DEBUG_STATIC
AM_LDFLAGS = -all-static $(LD_PROF) $(CXXSTDFLAGS)
else
AM_LDFLAGS = $(LD_PROF) $(CXXSTDFLAGS)
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
dar_split_CPPFLAGS = -I'$(srcdir)/../libdar' -DDAR_LOCALEDIR=\"$(localedir)\" -DDAR_SYS_DIR=\"$(sysconfdir)\" $(CPP_PROF)

#
# dar_static ( --enable-dar-static )
#

dar_static_SOURCES =  command_line.cpp command_line.hpp config_file.cpp config_file.hpp dar.cpp dar.hpp dar_suite.cpp dar_suite.hpp hide_file.cpp hide_file.hpp no_comment.cpp no_comment.hpp getopt_decision.h my_getopt_long.h line_tools.hpp line_tools.cpp crit_action_cmd_line.cpp crit_action_cmd_line.hpp
dar_static_DEPENDENCIES = ../libdar/$(MYLIB).la
dar_static_LDFLAGS = -all-static $(AM_LDFLAGS)
