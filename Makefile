#######################################################################
## dar - disk archive - a backup restoration program
## Copyright (C) 2002 Denis Corbin
##
## This program is free software; you can redistribute it and#or
## modify it under the terms of the GNU General Public License
## as published by the Free Software Foundation; either version 2
## of the License, or (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
##
## to contact the author : dar.linux@free.fr
#######################################################################
## $Id: Makefile,v 1.88 2002/06/27 18:58:49 denis Rel $
##
#######################################################################

### to form the name of the dar documentation directory
DAR_VERSION = `grep "*application_version" dar_suite.cpp | cut -d \" -f 2`

### where do you want to install dar:
INSTALL_ROOT_DIR = /usr/local

### where do you want the binary to go under INSTALL_ROOT_DIR
BIN_DIR = bin

### where do you want the man page to go under INSTALL_ROOT_DIR
MAN_DIR = man

### where do you want the documentation files to go under INSTALL_ROOT_DIR
### note that you have to call "make install-doc" to install doc there
DOC_DIR = doc/dar-$(DAR_VERSION)

### if you want Extended Attributes (EA) Support uncomment the following
# EA_SUPPORT = "yes"

### if you need large file support uncomment the following (files > 2 GB)
# FILEOFFSET = -D_FILE_OFFSET_BITS=64
### NOTE:
# This macro should only be selected if the system provides
# mechanisms for handling large files.  On 64 bits systems this macro
# has no effect since the `*64' functions are identical to the
# normal functions.

### choose your C++ compiler/linker
CXX = g++

##############
# NOTHING TO CHANGE BELOW, EXCEPT FOR DEVELOPMENT or DEBUGGING
#######################################################################

# OPTIMIZATION = -g # debugging options
# OPTIMIZATION = -ggdb3
OPTIMIZATION = -O # -pg

ifndef OPTIMIZATION
OPTIMIZATION =
endif

ifndef FILEOFFSET
FILEOFFSET=
endif

CPPFLAGS_COMMON = -std=c++98 $(OPTIMIZATION) $(FILEOFFSET) -Wall # -pedantic # -DTEST_MEMORY
OPTIMIZATION = -O

CPPFLAGS =  $(OPTIMIZATION) -Wall # -DTEST_MEMORY
# the -DTEST_MEMORY option is for memory leakage detection.
# It makes the execution very slow, so don't add it for normal use
CXXFLAGS=${CPPFLAGS}

LDFLAGS = $(OPTIMIZATION) $(FILEOFFSET) -Wall -static
# note : static link has been chosen to be able to restore an achive
# in a very simple environment, without having to look for
# a particular dynamic library and version. This makes of course the
# binary a bit bigger but not much than that.
LIBS_COMMON =  -lstdc++ -lz

ifdef EA_SUPPORT
CPPFLAGS = $(CPPFLAGS_COMMON) -DEA_SUPPORT
LIBS = $(LIBS_COMMON) -lattr
else
CPPFLAGS = $(CPPFLAGS_COMMON)
LIBS = $(LIBS_COMMON)
endif

OBJ = storage.o infinint.o deci.o erreurs.o generic_file.o compressor.o user_interaction.o sar.o header.o tools.o path.o mask.o tronc.o catalogue.o filesystem.o filtre.o dar.o command_line.o terminateur.o defile.o test_memory.o ea.o ea_filesystem.o header_version.o sar_tools.o tuyau.o zapette.o dar_suite.o
OBJ_XFER = dar_xform.o sar.o generic_file.o user_interaction.o erreurs.o path.o tools.o infinint.o storage.o header.o deci.o sar_tools.o tuyau.o dar_suite.o test_memory.o
LIBS_XFER = $(LIBS_COMMON)
OBJ_SLAVE = dar_slave.o generic_file.o user_interaction.o erreurs.o path.o tools.o infinint.o storage.o header.o header_version.o deci.o sar_tools.o sar.o tuyau.o dar_suite.o zapette.o test_memory.o
LIBS_SLAVE = $(LIBS_COMMON)
ALL_SRC = storage.cpp infinint.cpp deci.cpp erreurs.cpp generic_file.cpp compressor.cpp user_interaction.cpp sar.cpp header.cpp tools.cpp path.cpp mask.cpp tronc.cpp catalogue.cpp filesystem.cpp filtre.cpp dar.cpp command_line.cpp terminateur.cpp defile.cpp test_memory.cpp ea.cpp ea_filesystem.cpp header_version.cpp test_deci.cpp test_infinint.cpp factoriel.cpp test_erreurs.cpp test_compressor.cpp test_sar.cpp test_path.cpp test_mask.cpp test_tronc.cpp test_catalogue.cpp test_filesystem.cpp testtools.cpp test_terminateur.cpp dar_xform.cpp sar_tools.cpp dar_suite.cpp tuyau.cpp dar_slave.cpp zapette.cpp test_tuyau.cpp

CIBLE = dar dar_xform dar_slave

default : $(CIBLE)

dar : $(OBJ)
	$(CXX) $(LDFLAGS) $(OBJ) $(LIBS) -o $@

all : $(CIBLE) test

depend : $(ALL_SRC)
	makedepend -Y. $(ALL_SRC) -w200
	rm Makefile.bak

install : $(CIBLE)
	install -g root -o root -d $(INSTALL_ROOT_DIR)/$(BIN_DIR)
	install -g root -o root -m 0555 -s $(CIBLE) $(INSTALL_ROOT_DIR)/$(BIN_DIR)
	install -g root -o root -d $(INSTALL_ROOT_DIR)/$(MAN_DIR)/man1
	install -g root -o root -m 0444 dar.1 $(INSTALL_ROOT_DIR)/$(MAN_DIR)/man1
	install -g root -o root -m 0444 dar_xform.1 $(INSTALL_ROOT_DIR)/$(MAN_DIR)/man1
	install -g root -o root -m 0444 dar_slave.1 $(INSTALL_ROOT_DIR)/$(MAN_DIR)/man1

install-doc: BUGS CHANGES INSTALL LICENSE NOTES README TODO TUTORIAL
	install -g root -o root -d $(INSTALL_ROOT_DIR)/$(DOC_DIR)
	install -g root -o root -m 0444 BUGS CHANGES INSTALL LICENSE NOTES README TODO TUTORIAL $(INSTALL_ROOT_DIR)/$(DOC_DIR)

uninstall :
	cd $(INSTALL_ROOT_DIR)/$(BIN_DIR); rm -f $(CIBLE) dar_xfer
	cd $(INSTALL_ROOT_DIR)/$(MAN_DIR); rm -f dar.1 dar_xform.1 dar_slave.1 dar_fer.1
	if [ -d $(INSTALL_ROOT_DIR)/$(DOC_DIR) ] ; then cd $(INSTALL_ROOT_DIR)/$(DOC_DIR); rm -f BUGS CHANGES INSTALL LICENSE NOTES README TODO TUTORIAL; fi
	if [ -d $(INSTALL_ROOT_DIR)/$(DOC_DIR) ] ; then rmdir $(INSTALL_ROOT_DIR)/$(DOC_DIR); fi
	echo $(DOC_DIR)

test : test_storage test_deci test_infinint factoriel test_erreurs test_compressor test_sar test_path test_mask test_tronc test_catalogue test_filesystem test_terminateur test_generic_file test_tuyau

clean :
	rm -f $(OBJ) $(CIBLE) dar_xform.o test_storage.o test_storage test_deci.o test_deci test_infinint.o test_infinint factoriel.o factoriel test_erreurs.o test_erreurs test_compressor.o test_compressor test_sar.o test_sar test_path.o test_path test_mask test_mask.o test_tronc.o test_tronc test_catalogue test_catalogue.o test_filesystem.o test_filesystem testtools.o test_terminateur test_terminateur.o test_generic_file test_generic_file.o dar_slave dar_slave.o test_tuyau.o test_tuyau

test_storage : storage.o deci.o test_storage.o erreurs.o infinint.o generic_file.o tools.o path.o user_interaction.o test_memory.o tuyau.o
	$(CXX) $(LDFLAGS) path.o deci.o tools.o generic_file.o infinint.o erreurs.o storage.o user_interaction.o test_storage.o test_memory.o tuyau.o $(LIBS) -o $@

test_deci : deci.o test_deci.o erreurs.o storage.o infinint.o generic_file.o path.o tools.o user_interaction.o test_memory.o tuyau.o
	$(CXX) $(LDFLAGS) deci.o erreurs.o storage.o infinint.o generic_file.o path.o tools.o user_interaction.o test_deci.o test_memory.o tuyau.o $(LIBS) -o $@

test_infinint : erreurs.o storage.o infinint.o generic_file.o path.o tools.o test_infinint.o deci.o user_interaction.o test_memory.o tuyau.o
	$(CXX) $(LDFLAGS) erreurs.o storage.o infinint.o generic_file.o path.o tools.o deci.o user_interaction.o test_infinint.o test_memory.o tuyau.o $(LIBS) -o $@

factoriel : factoriel.o erreurs.o storage.o infinint.o generic_file.o path.o tools.o deci.o user_interaction.o test_memory.o tuyau.o
	$(CXX) $(LDFLAGS) erreurs.o storage.o infinint.o generic_file.o path.o tools.o deci.o factoriel.o test_memory.o user_interaction.o tuyau.o $(LIBS) -o $@

test_erreurs : test_erreurs.o
	$(CXX) $(LDFLAGS) erreurs.o test_erreurs.o $(LIBS) -o $@

test_compressor : compressor.o deci.o erreurs.o storage.o infinint.o generic_file.o path.o tools.o test_compressor.o user_interaction.o test_memory.o tuyau.o
	$(CXX) $(LDFLAGS) compressor.o deci.o erreurs.o storage.o infinint.o generic_file.o path.o tools.o test_compressor.o user_interaction.o test_memory.o tuyau.o $(LIBS) -o $@

test_sar : sar.o test_sar.o erreurs.o storage.o infinint.o generic_file.o path.o tools.o user_interaction.o header.o deci.o testtools.o test_memory.o tuyau.o
	$(CXX) $(LDFLAGS) sar.o erreurs.o storage.o infinint.o generic_file.o path.o tools.o user_interaction.o header.o deci.o testtools.o test_sar.o test_memory.o tuyau.o $(LIBS) -o $@

test_path : test_path.o deci.o erreurs.o storage.o infinint.o generic_file.o path.o tools.o user_interaction.o test_memory.o tuyau.o
	$(CXX) $(LDFLAGS) deci.o erreurs.o storage.o infinint.o generic_file.o path.o tools.o test_path.o user_interaction.o test_memory.o tuyau.o $(LIBS) -o $@

test_mask : mask.o test_mask.o deci.o erreurs.o storage.o infinint.o generic_file.o path.o tools.o user_interaction.o tuyau.o
	$(CXX) $(LDFLAGS) mask.o deci.o erreurs.o storage.o infinint.o generic_file.o path.o tools.o test_mask.o user_interaction.o tuyau.o $(LIBS) -o $@

test_tronc : tronc.o erreurs.o storage.o infinint.o generic_file.o path.o tools.o test_tronc.o deci.o user_interaction.o tuyau.o
	$(CXX) $(LDFLAGS) tronc.o erreurs.o storage.o infinint.o generic_file.o path.o tools.o deci.o testtools.o test_tronc.o user_interaction.o tuyau.o $(LIBS) -o $@

test_catalogue : catalogue.o header_version.o ea.o test_catalogue.o erreurs.o storage.o infinint.o generic_file.o path.o tools.o tronc.o user_interaction.o deci.o test_memory.o tuyau.o compressor.o
	$(CXX) $(LDFLAGS) catalogue.o header_version.o ea.o erreurs.o storage.o infinint.o generic_file.o path.o tools.o tronc.o user_interaction.o deci.o test_catalogue.o test_memory.o tuyau.o compressor.o $(LIBS) -o $@

test_filesystem : filesystem.o test_filesystem.o erreurs.o storage.o infinint.o generic_file.o path.o tools.o catalogue.o user_interaction.o tronc.o deci.o test_memory.o ea_filesystem.o ea.o header_version.o tuyau.o compressor.o
	$(CXX) $(LDFLAGS) filesystem.o erreurs.o storage.o infinint.o generic_file.o path.o tools.o catalogue.o user_interaction.o tronc.o deci.o test_filesystem.o test_memory.o ea_filesystem.o ea.o header_version.o tuyau.o compressor.o $(LIBS) -o $@

test_terminateur : terminateur.o erreurs.o infinint.o storage.o path.o generic_file.o deci.o test_terminateur.o tools.o user_interaction.o test_memory.o tuyau.o
	$(CXX) $(LDFLAGS) terminateur.o erreurs.o infinint.o storage.o path.o generic_file.o deci.o tools.o test_terminateur.o user_interaction.o test_memory.o tuyau.o $(LIBS) -o $@

test_generic_file : infinint.o storage.o path.o generic_file.o test_generic_file.o erreurs.o tools.o user_interaction.o deci.o tuyau.o
	$(CXX) $(LDFLAGS) infinint.o storage.o path.o generic_file.o test_generic_file.o erreurs.o tools.o user_interaction.o deci.o tuyau.o $(LIBS) -o $@

test_tuyau : test_tuyau.o infinint.o storage.o path.o generic_file.o erreurs.o tools.o user_interaction.o deci.o tuyau.o dar_suite.o test_memory.o
	$(CXX) $(LDFLAGS) infinint.o storage.o path.o generic_file.o test_tuyau.o erreurs.o tools.o user_interaction.o deci.o tuyau.o dar_suite.o test_memory.o $(LIBS) -o $@


dar_xform : $(OBJ_XFER)
	$(CXX) $(LDFLAGS) $(OBJ_XFER) $(LIBS_XFER) -o $@

dar_slave : $(OBJ_SLAVE)
	$(CXX) $(LDFLAGS) $(OBJ_SLAVE) $(LIBS_SLAVE) -o $@

# DO NOT DELETE

storage.o: storage.hpp erreurs.hpp infinint.hpp generic_file.hpp path.hpp
infinint.o: infinint.hpp storage.hpp erreurs.hpp generic_file.hpp path.hpp
deci.o: deci.hpp storage.hpp erreurs.hpp infinint.hpp
erreurs.o: erreurs.hpp
generic_file.o: generic_file.hpp infinint.hpp storage.hpp erreurs.hpp path.hpp tools.hpp tuyau.hpp user_interaction.hpp
compressor.o: compressor.hpp generic_file.hpp infinint.hpp storage.hpp erreurs.hpp path.hpp
user_interaction.o: user_interaction.hpp erreurs.hpp tools.hpp path.hpp generic_file.hpp infinint.hpp storage.hpp tuyau.hpp
sar.o: sar.hpp generic_file.hpp infinint.hpp storage.hpp erreurs.hpp path.hpp header.hpp deci.hpp user_interaction.hpp tools.hpp tuyau.hpp
header.o: header.hpp infinint.hpp storage.hpp erreurs.hpp generic_file.hpp path.hpp
tools.o: tools.hpp path.hpp erreurs.hpp generic_file.hpp infinint.hpp storage.hpp tuyau.hpp deci.hpp
path.o: path.hpp erreurs.hpp
mask.o: tools.hpp path.hpp erreurs.hpp generic_file.hpp infinint.hpp storage.hpp tuyau.hpp mask.hpp
tronc.o: tronc.hpp generic_file.hpp infinint.hpp storage.hpp erreurs.hpp path.hpp
catalogue.o: tools.hpp path.hpp erreurs.hpp generic_file.hpp infinint.hpp storage.hpp tuyau.hpp catalogue.hpp header_version.hpp ea.hpp compressor.hpp tronc.hpp user_interaction.hpp deci.hpp
catalogue.o: header.hpp
filesystem.o: tools.hpp path.hpp erreurs.hpp generic_file.hpp infinint.hpp storage.hpp tuyau.hpp filesystem.hpp catalogue.hpp header_version.hpp ea.hpp compressor.hpp user_interaction.hpp
filesystem.o: ea_filesystem.hpp
filtre.o: user_interaction.hpp erreurs.hpp filtre.hpp mask.hpp path.hpp compressor.hpp generic_file.hpp infinint.hpp storage.hpp catalogue.hpp header_version.hpp tools.hpp tuyau.hpp ea.hpp
filtre.o: filesystem.hpp defile.hpp test_memory.hpp null_file.hpp
dar.o: erreurs.hpp generic_file.hpp infinint.hpp storage.hpp path.hpp mask.hpp user_interaction.hpp terminateur.hpp compressor.hpp command_line.hpp catalogue.hpp header_version.hpp tools.hpp
dar.o: tuyau.hpp ea.hpp sar.hpp header.hpp filtre.hpp defile.hpp deci.hpp test_memory.hpp dar.hpp sar_tools.hpp dar_suite.hpp zapette.hpp filesystem.hpp
command_line.o: deci.hpp storage.hpp erreurs.hpp infinint.hpp command_line.hpp compressor.hpp generic_file.hpp path.hpp mask.hpp user_interaction.hpp tools.hpp tuyau.hpp dar.hpp dar_suite.hpp
terminateur.o: terminateur.hpp generic_file.hpp infinint.hpp storage.hpp erreurs.hpp path.hpp
defile.o: defile.hpp catalogue.hpp infinint.hpp storage.hpp erreurs.hpp path.hpp generic_file.hpp header_version.hpp tools.hpp tuyau.hpp ea.hpp compressor.hpp
test_memory.o: user_interaction.hpp test_memory.hpp erreurs.hpp
ea.o: ea.hpp generic_file.hpp infinint.hpp storage.hpp erreurs.hpp path.hpp tools.hpp tuyau.hpp
ea_filesystem.o: ea.hpp generic_file.hpp infinint.hpp storage.hpp erreurs.hpp path.hpp tools.hpp tuyau.hpp ea_filesystem.hpp user_interaction.hpp
header_version.o: header_version.hpp generic_file.hpp infinint.hpp storage.hpp erreurs.hpp path.hpp tools.hpp tuyau.hpp
test_deci.o: deci.hpp storage.hpp erreurs.hpp infinint.hpp test_memory.hpp
test_infinint.o: infinint.hpp storage.hpp erreurs.hpp deci.hpp test_memory.hpp
factoriel.o: infinint.hpp storage.hpp erreurs.hpp deci.hpp test_memory.hpp generic_file.hpp path.hpp
test_erreurs.o: erreurs.hpp
test_compressor.o: compressor.hpp generic_file.hpp infinint.hpp storage.hpp erreurs.hpp path.hpp test_memory.hpp
test_sar.o: sar.hpp generic_file.hpp infinint.hpp storage.hpp erreurs.hpp path.hpp header.hpp deci.hpp testtools.hpp user_interaction.hpp test_memory.hpp
test_path.o: path.hpp erreurs.hpp test_memory.hpp
test_mask.o: mask.hpp path.hpp erreurs.hpp
test_tronc.o: tronc.hpp generic_file.hpp infinint.hpp storage.hpp erreurs.hpp path.hpp deci.hpp testtools.hpp user_interaction.hpp
test_catalogue.o: testtools.hpp infinint.hpp storage.hpp erreurs.hpp generic_file.hpp path.hpp catalogue.hpp header_version.hpp tools.hpp tuyau.hpp ea.hpp compressor.hpp user_interaction.hpp
test_catalogue.o: test_memory.hpp
test_filesystem.o: filesystem.hpp catalogue.hpp infinint.hpp storage.hpp erreurs.hpp path.hpp generic_file.hpp header_version.hpp tools.hpp tuyau.hpp ea.hpp compressor.hpp user_interaction.hpp
test_filesystem.o: test_memory.hpp
testtools.o: deci.hpp storage.hpp erreurs.hpp infinint.hpp testtools.hpp generic_file.hpp path.hpp user_interaction.hpp
test_terminateur.o: terminateur.hpp generic_file.hpp infinint.hpp storage.hpp erreurs.hpp path.hpp deci.hpp test_memory.hpp
dar_xform.o: sar.hpp generic_file.hpp infinint.hpp storage.hpp erreurs.hpp path.hpp header.hpp sar_tools.hpp user_interaction.hpp tools.hpp tuyau.hpp dar_suite.hpp
sar_tools.o: erreurs.hpp user_interaction.hpp sar.hpp generic_file.hpp infinint.hpp storage.hpp path.hpp header.hpp tools.hpp tuyau.hpp
dar_suite.o: dar_suite.hpp user_interaction.hpp erreurs.hpp test_memory.hpp
tuyau.o: tuyau.hpp generic_file.hpp infinint.hpp storage.hpp erreurs.hpp path.hpp tools.hpp user_interaction.hpp
dar_slave.o: user_interaction.hpp zapette.hpp generic_file.hpp infinint.hpp storage.hpp erreurs.hpp path.hpp sar.hpp header.hpp tuyau.hpp tools.hpp dar_suite.hpp
zapette.o: zapette.hpp generic_file.hpp infinint.hpp storage.hpp erreurs.hpp path.hpp user_interaction.hpp
test_tuyau.o: tuyau.hpp generic_file.hpp infinint.hpp storage.hpp erreurs.hpp path.hpp tools.hpp user_interaction.hpp dar_suite.hpp
