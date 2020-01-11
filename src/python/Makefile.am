#######################################################################
# dar - disk archive - a backup/restoration program
# Copyright (C) 2002-2019 Denis Corbin
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

if BUILD_MODE32
    MYLIB=libdar32
    AM_CPPFLAGS=-DLIBDAR_MODE=32 -I../libdar -DDAR_LOCALEDIR=\"$(localedir)\" -DDAR_SYS_DIR=\"$(system_path)\" $(CPP_PROF) $(GPGME_CFLAGS) $(LIBCURL_CFLAGS) $(LIBTHREADAR_CFLAGS) $(CXXSTDFLAGS) $(PYFLAGS)
else
if BUILD_MODE64
    MYLIB=libdar64
    AM_CPPFLAGS=-DLIBDAR_MODE=64 -I../libdar -DDAR_LOCALEDIR=\"$(localedir)\" -DDAR_SYS_DIR=\"$(system_path)\" $(CPP_PROF) $(GPGME_CFLAGS) $(LIBCURL_CFLAGS) $(LIBTHREADAR_CFLAGS) $(CXXSTDFLAGS) $(PYFLAGS)
else
    MYLIB=libdar
    AM_CPPFLAGS=-I../libdar -DDAR_LOCALEDIR=\"$(localedir)\"  -DDAR_SYS_DIR=\"$(system_path)\" $(CPP_PROF) $(GPGME_CFLAGS) $(LIBCURL_CFLAGS) $(LIBTHREADAR_CFLAGS) $(CXXSTDFLAGS) $(PYFLAGS)
endif
endif

if PYTHON_BINDING
  TARGET=libdar_python.la
else
  TARGET=
endif

AM_LDFLAGS = $(LTLIBINTL) $(LD_PROF) $(GPGME_LIBS) $(LIBCURL_LIBS) $(LIBTHREADAR_LIBS) $(CXXSTDFLAGS) ../libdar/$(MYLIB).la
lib_LTLIBRARIES = $(TARGET)

libdar_python_la_SOURCES = pybind11_libdar.cpp
libdar_python_la_DEPENDENCIES = ../libdar/$(MYLIB).la

all-local: libdar$(PYEXT)

libdar$(PYEXT):
	ln -sf libdar_python.la libdar$(PYEXT)

clean-local:
	rm -f libdar$(PYEXT)
