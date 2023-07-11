#######################################################################
# dar - disk archive - a backup/restoration program
# Copyright (C) 2002-2023 Denis Corbin
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
# to contact the author, see the AUTHOR file
#######################################################################


if BUILD_MODE32
  MYLIB=libdar32
  AM_CPPFLAGS=-DLIBDAR_MODE=32 -I'$(srcdir)/../libdar' -DDAR_LOCALEDIR=\"$(localedir)\"
else
if BUILD_MODE64
    MYLIB=libdar64
    AM_CPPFLAGS=-DLIBDAR_MODE=64 -I'$(srcdir)/../libdar' -DDAR_LOCALEDIR=\"$(localedir)\"
else
    MYLIB=libdar
    AM_CPPFLAGS=-I'$(srcdir)/../libdar' -DDAR_LOCALEDIR=\"$(localedir)\"
endif
endif

noinst_PROGRAMS = factoriel prime formula
dist_noinst_DATA = README

LDADD = ../libdar/$(MYLIB).la $(LTLIBINTL)
AM_LDFLAGS =

factoriel_SOURCES = factoriel.cpp
factoriel_LDADD =  ../dar_suite/dar_suite.o ../dar_suite/line_tools.o $(LDADD)
factoriel_DEPENDENCIES = ../libdar/$(MYLIB).la ../dar_suite/dar_suite.cpp

prime_SOURCES = prime.cpp
prime_LDADD = ../dar_suite/dar_suite.o ../dar_suite/line_tools.o  $(LDADD)
prime_DEPENDENCIES = ../libdar/$(MYLIB).la ../dar_suite/dar_suite.cpp

formula_SOURCES = formula.cpp
formula_LDADD = ../dar_suite/dar_suite.o ../dar_suite/crit_action_cmd_line.o ../dar_suite/line_tools.o  $(LDADD)
formula_DEPENDENCIES = ../libdar/$(MYLIB).la ../dar_suite/line_tools.cpp ../dar_suite/crit_action_cmd_line.cpp ../dar_suite/dar_suite.cpp
