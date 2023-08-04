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
    MYLIB=libdar32.so
    AM_CPPFLAGS=-fPIC -DLIBDAR_MODE=32 -I'../..' -I'$(srcdir)/..' -I'$(srcdir)/../libdar' -DDAR_LOCALEDIR=\"$(localedir)\" -DDAR_SYS_DIR=\"$(system_path)\" $(CPP_PROF) $(PYFLAGS)
else
if BUILD_MODE64
    MYLIB=libdar64.so
    AM_CPPFLAGS=-fPIC -DLIBDAR_MODE=64 -I'../..' -I'$(srcdir)/..' -I'$(srcdir)/../libdar' -DDAR_LOCALEDIR=\"$(localedir)\" -DDAR_SYS_DIR=\"$(system_path)\" $(CPP_PROF) $(PYFLAGS)
else
    MYLIB=libdar.so
    AM_CPPFLAGS=-fPIC -I'../..' -I'$(srcdir)/..' -I'$(srcdir)/../libdar' -DDAR_LOCALEDIR=\"$(localedir)\"  -DDAR_SYS_DIR=\"$(system_path)\" $(CPP_PROF) $(PYFLAGS) -fPIC
endif
endif

dist_pkgdata_DATA = pybind11_libdar.cpp

if PYTHON_BINDING
  TARGET=libdar$(PYEXT)

install-data-hook: $(TARGET)
	$(INSTALL) -d $(DESTDIR)$(libdir)/python3/dist-packages
	$(INSTALL) -m 0644 $(TARGET) $(DESTDIR)$(libdir)/python3/dist-packages

uninstall-hook:
	rm -rf $(DESTDIR)$(libdir)/python3/dist-packages/$(TARGET)
	rmdir -p $(DESTDIR)$(libdir)/python3/dist-packages 2> /dev/null || echo ""

else
  TARGET=
endif

AM_LDFLAGS = -shared -Wl,--no-as-needed ../libdar/.libs/$(MYLIB)

all-local: $(TARGET)

.cpp.o:
	$(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(CPPFLAGS) $(AM_CPPFLAGS) $(AM_CFLAGS) $(CFLAGS) -c $< -o $@

libdar$(PYEXT): pybind11_libdar.o
	$(CXX) $(AM_LDFLAGS) $<  -o $@

clean-local:
	rm -f libdar$(PYEXT) *.o
