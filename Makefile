#######################################################################
## dar - disk archive - a backup#restoration program
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
## $Id: Makefile,v 1.44 2002/03/27 22:46:27 denis Rel $
##
#######################################################################

### where do you want to install dar :
INSTALL_ROOT_DIR = /usr/local

### where do you want the binary to go under INSTALL_ROOT_DIR
BIN_DIR = bin

### where do you want the man page to go under INSTALL_ROOT_DIR
MAN_DIR = man


##############
# NOTHING TO CHANGE BELOW, EXCEPT FOR DEVELOPMENT or DEBUGGING
#######################################################################
CC = g++

# OPTIMIZATION = -g # debugging options
OPTIMIZATION = -O  

CPPFLAGS = $(OPTIMIZATION) -Wall # -DTEST_MEMORY 
# the -DTEST_MEMORY option is for memory leakage detection. 
# It makes the execution very slow, so don't add it for normal use

LDFLAGS = $(OPTIMIZATION) -Wall -static 
# note : static link has been chosen to be abe to restore an achive
# in a very simple environment, without having to look for 
# a particular dynamic library and version. This makes of course the
#  binary a bit bigger but not much than that.

LIBS =  -lstdc++ -lz
OBJ = storage.o infinint.o deci.o erreurs.o generic_file.o compressor.o user_interaction.o sar.o header.o tools.o path.o mask.o tronc.o catalogue.o filesystem.o filtre.o dar.o command_line.o terminateur.o defile.o test_memory.o
CIBLE = dar

$(CIBLE) : $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) $(LIBS) -o $@

all : $(CIBLE) test

install : $(CIBLE)
	install -g root -o root -d $(INSTALL_ROOT_DIR)/$(BIN_DIR)
	install -g root -o root -m 0555 -s $(CIBLE) $(INSTALL_ROOT_DIR)/$(BIN_DIR)
	install -g root -o root -d $(INSTALL_ROOT_DIR)/$(MAN_DIR)/man1
	install -g root -o root -m 0444 $(CIBLE).1 $(INSTALL_ROOT_DIR)/$(MAN_DIR)/man1

uninstall : 
	rm -f $(INSTALL_ROOT_DIR)/$(BIN_DIR)/$(CIBLE)
	rm -f $(INSTALL_ROOT_DIR)/$(MAN_DIR)/$(CIBLE).1

test : test_storage test_deci test_infinint factoriel test_erreurs test_compressor test_sar test_path test_mask test_tronc test_catalogue test_filesystem test_terminateur

clean :
	rm -f $(OBJ) $(CIBLE) test_storage.o test_storage test_deci.o test_deci test_infinint.o test_infinint factoriel.o factoriel test_erreurs.o test_erreurs test_compressor.o test_compressor test_sar.o test_sar test_path.o test_path test_mask test_mask.o test_tronc.o test_tronc test_catalogue test_catalogue.o test_filesystem.o test_filesystem testtools.o test_terminateur test_terminateur.o

test_storage : storage.o test_storage.o erreurs.o infinint.o generic_file.o tools.o path.o user_interaction.o test_memory.o
	$(CC) $(LDFLAGS) path.o tools.o generic_file.o infinint.o erreurs.o storage.o user_interaction.o test_storage.o test_memory.o $(LIBS) -o $@

test_deci : deci.o test_deci.o erreurs.o storage.o infinint.o generic_file.o path.o tools.o user_interaction.o test_memory.o
	$(CC) $(LDFLAGS) deci.o erreurs.o storage.o infinint.o generic_file.o path.o tools.o user_interaction.o test_deci.o test_memory.o $(LIBS) -o $@

test_infinint : erreurs.o storage.o infinint.o generic_file.o path.o tools.o test_infinint.o deci.o user_interaction.o test_memory.o
	$(CC) $(LDFLAGS) erreurs.o storage.o infinint.o generic_file.o path.o tools.o deci.o user_interaction.o test_infinint.o test_memory.o $(LIBS) -o $@

factoriel : factoriel.o erreurs.o storage.o infinint.o generic_file.o path.o tools.o deci.o user_interaction.o test_memory.o
	$(CC) $(LDFLAGS) erreurs.o storage.o infinint.o generic_file.o path.o tools.o deci.o factoriel.o test_memory.o user_interaction.o $(LIBS) -o $@

test_erreurs : test_erreurs.o
	$(CC) $(LDFLAGS) erreurs.o test_erreurs.o $(LIBS) -o $@

test_compressor : compressor.o erreurs.o storage.o infinint.o generic_file.o path.o tools.o test_compressor.o user_interaction.o test_memory.o
	$(CC) $(LDFLAGS) compressor.o erreurs.o storage.o infinint.o generic_file.o path.o tools.o test_compressor.o user_interaction.o test_memory.o $(LIBS) -o $@

test_sar : sar.o test_sar.o erreurs.o storage.o infinint.o generic_file.o path.o tools.o user_interaction.o header.o deci.o testtools.o test_memory.o
	$(CC) $(LDFLAGS) sar.o erreurs.o storage.o infinint.o generic_file.o path.o tools.o user_interaction.o header.o deci.o testtools.o test_sar.o test_memory.o  $(LIBS) -o $@

test_path : test_path.o erreurs.o storage.o infinint.o generic_file.o path.o tools.o user_interaction.o test_memory.o
	$(CC) $(LDFLAGS) erreurs.o storage.o infinint.o generic_file.o path.o tools.o test_path.o user_interaction.o test_memory.o $(LIBS) -o $@

test_mask : mask.o test_mask.o erreurs.o storage.o infinint.o generic_file.o path.o tools.o user_interaction.o 
	$(CC) $(LDFLAGS) mask.o erreurs.o storage.o infinint.o generic_file.o path.o tools.o test_mask.o user_interaction.o $(LIBS) -o $@

test_tronc : tronc.o erreurs.o storage.o infinint.o generic_file.o path.o tools.o test_tronc.o deci.o user_interaction.o
	$(CC) $(LDFLAGS) tronc.o erreurs.o storage.o infinint.o generic_file.o path.o tools.o deci.o testtools.o test_tronc.o user_interaction.o $(LIBS) -o $@

test_catalogue : catalogue.o test_catalogue.o erreurs.o storage.o infinint.o generic_file.o path.o tools.o tronc.o user_interaction.o deci.o test_memory.o
	$(CC) $(LDFLAGS) catalogue.o erreurs.o storage.o infinint.o generic_file.o path.o tools.o tronc.o user_interaction.o deci.o test_catalogue.o test_memory.o $(LIBS) -o $@

test_filesystem : filesystem.o test_filesystem.o erreurs.o storage.o infinint.o generic_file.o path.o tools.o catalogue.o user_interaction.o tronc.o deci.o test_memory.o
	$(CC) $(LDFLAGS) filesystem.o erreurs.o storage.o infinint.o generic_file.o path.o tools.o catalogue.o user_interaction.o tronc.o deci.o test_filesystem.o test_memory.o $(LIBS) -o $@

test_terminateur : terminateur.o erreurs.o infinint.o storage.o path.o generic_file.o deci.o test_terminateur.o tools.o user_interaction.o test_memory.o
	$(CC) $(LDFLAGS) terminateur.o erreurs.o infinint.o storage.o path.o generic_file.o deci.o tools.o test_terminateur.o user_interaction.o test_memory.o $(LIBS) -o $@

test_storage.o : test_storage.cpp erreurs.hpp
test_deci.o : test_deci.cpp deci.hpp
test_infinint.o : test_infinint.cpp infinint.hpp deci.hpp
test_erreurs.o : test_erreurs.cpp erreurs.hpp
test_sar.o : test_sar.cpp testtools.hpp sar.hpp
factoriel : factoriel.cpp infinint.hpp deci.hpp
erreurs.o : erreurs.cpp erreurs.hpp
infinint.o : infinint.cpp infinint.hpp storage.hpp erreurs.hpp generic_file.hpp path.hpp
storage.o : storage.cpp storage.hpp erreurs.hpp infinint.hpp
deci.o : deci.cpp deci.hpp erreurs.hpp storage.hpp infinint.hpp
generic_file.o : generic_file.cpp generic_file.hpp erreurs.hpp tools.hpp catalogue.hpp user_interaction.hpp
compressor.o : compressor.cpp compressor.hpp erreurs.hpp
user_interaction.o : user_interaction.cpp user_interaction.hpp erreurs.hpp
header.o : header.cpp header.hpp
tools.o : tools.cpp tools.hpp erreurs.hpp
path.o : path.cpp path.hpp 
mask.o : mask.cpp mask.hpp tools.hpp erreurs.hpp path.hpp
tronc.o : tronc.cpp tronc.hpp
catalogue.o : catalogue.cpp catalogue.hpp tools.hpp tronc.hpp user_interaction.hpp
filesystem.o : filesystem.cpp tools.hpp erreurs.hpp filesystem.hpp user_interaction.hpp catalogue.hpp
filtre.o : filtre.cpp user_interaction.hpp erreurs.hpp filtre.hpp filesystem.hpp defile.hpp test_memory.hpp
dar.o : dar.cpp dar.hpp erreurs.hpp generic_file.hpp infinint.hpp path.hpp mask.hpp user_interaction.hpp terminateur.hpp compressor.hpp command_line.hpp catalogue.hpp sar.hpp filtre.hpp tools.hpp header.hpp test_memory.hpp
command_line.o : command_line.cpp deci.hpp command_line.hpp user_interaction.hpp dar.hpp
terminateur.o : terminateur.cpp terminateur.hpp
sar.o : sar.cpp sar.hpp deci.hpp user_interaction.hpp tools.hpp header.hpp
defile.o : defile.cpp defile.hpp path.hpp catalogue.hpp
test_memory.o : test_memory.cpp test_memory.hpp erreurs.hpp
