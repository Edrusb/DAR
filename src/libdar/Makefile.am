#######################################################################
# dar - disk archive - a backup/restoration program
# Copyright (C) 2002-2020 Denis Corbin
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

if BSD_SED
  SED_REGEX = -E
else
  SED_REGEX = -r
endif

if PROFILING
  LD_PROF = -pg
  CPP_PROF = -pg
clean-local:
	rm -f gmon.out
	rm -f libdar.pc
else
  LD_PROF =
  CPP_PROF =
clean-local:
	rm -f libdar.pc
endif

if WITH_LIBTHREADAR
    LIBTHREADAR_DEP_MODULES=parallel_tronconneuse.cpp parallel_block_compressor.cpp
else
    LIBTHREADAR_DEP_MODULES=
endif

if BUILD_MODE32
  MYLIB=libdar32.la
  AM_CPPFLAGS=-DLIBDAR_MODE=32 -DDAR_LOCALEDIR=\"$(localedir)\" $(CPP_PROF) $(GPGME_CFLAGS) $(LIBCURL_CFLAGS) $(LIBTHREADAR_CFLAGS) $(CXXSTDFLAGS)
  VER_SRC=limitint.o
  LIBDAR_SUFFIX=32
  LIBDAR_MODE=-DLIBDAR_MODE=32
else
if BUILD_MODE64
    MYLIB=libdar64.la
    AM_CPPFLAGS=-DLIBDAR_MODE=64 -DDAR_LOCALEDIR=\"$(localedir)\" $(CPP_PROF) $(GPGME_CFLAGS) $(LIBCURL_CFLAGS) $(LIBTHREADAR_CFLAGS) $(CXXSTDFLAGS)
    VER_SRC=limitint.o
    LIBDAR_SUFFIX=64
    LIBDAR_MODE=-DLIBDAR_MODE=64
else
    MYLIB=libdar.la
    AM_CPPFLAGS= -DDAR_LOCALEDIR=\"$(localedir)\" $(CPP_PROF) $(GPGME_CFLAGS) $(LIBCURL_CFLAGS) $(LIBTHREADAR_CFLAGS) $(CXXSTDFLAGS)
    VER_SRC=real_infinint.o
    LIBDAR_SUFFIX=
    LIBDAR_MODE=
endif
endif

AM_LDFLAGS = $(LTLIBINTL) $(LD_PROF) $(GPGME_LIBS) $(LIBCURL_LIBS) $(LIBTHREADAR_LIBS) $(CXXSTDFLAGS)
lib_LTLIBRARIES = $(MYLIB)

LIBDAR_MAJOR=`grep LIBDAR_COMPILE_TIME_MAJOR '$(srcdir)/get_version.hpp' | cut -f 2 -d "=" | cut -f 1 -d ';' | sed -e 's% %%g'`
LIBDAR_MEDIUM=`grep LIBDAR_COMPILE_TIME_MEDIUM '$(srcdir)/get_version.hpp' | cut -f 2 -d "=" | cut -f 1 -d ';' | sed -e 's% %%g'`
LIBDAR_MEDIUM_000=`grep LIBDAR_COMPILE_TIME_MEDIUM '$(srcdir)/get_version.hpp' | cut -f 2 -d "=" | cut -f 1 -d ';' | sed $(SED_REGEX) -e 's% %%g' -e 's%([^0-9]|^)([0-9]{1})([^0-9]|$$)%0\2%' -e 's%([^0-9]|^)([0-9]{2})([^0-9]|$$)%0\2%'`
LIBDAR_MINOR=`grep LIBDAR_COMPILE_TIME_MINOR '$(srcdir)/get_version.hpp' | cut -f 2 -d "=" | cut -f 1 -d ';' | sed -e 's% %%g'`
LIBDAR_LIBTOOL_CURRENT=$(LIBDAR_MAJOR)$(LIBDAR_MEDIUM_000)
LIBDAR_LIBTOOL_REVISION=$(LIBDAR_MINOR)
LIBDAR_LIBTOOL_AGE=$(LIBDAR_MEDIUM)
LIBDAR_VERSION_IN=$(LIBDAR_LIBTOOL_CURRENT):$(LIBDAR_LIBTOOL_REVISION):$(LIBDAR_LIBTOOL_AGE)
LIBDAR_VERSION_OUT=$(LIBDAR_MAJOR).$(LIBDAR_MEDIUM).$(LIBDAR_MINOR)

# header files required by external applications and that must be installed (make install)

dist_noinst_DATA = libdar.hpp archive.hpp database.hpp libdar_xform.hpp libdar_slave.hpp erreurs.hpp compile_time_features.hpp entrepot_libcurl.hpp get_version.hpp archive_options_listing_shell.hpp shell_interaction.hpp user_interaction_callback.hpp user_interaction_blind.hpp path.hpp statistics.hpp archive_options.hpp list_entry.hpp crypto.hpp archive_summary.hpp archive_listing_callback.hpp user_interaction.hpp database_options.hpp database_archives.hpp archive_num.hpp database_listing_callback.hpp infinint.hpp archive_aux.hpp integers.hpp entrepot.hpp secu_string.hpp mycurl_protocol.hpp deci.hpp mask.hpp mask_list.hpp crit_action.hpp fsa_family.hpp compression.hpp real_infinint.hpp datetime.hpp range.hpp cat_status.hpp ea.hpp entree_stats.hpp database_aux.hpp limitint.hpp gf_mode.hpp criterium.hpp int_tools.hpp proto_generic_file.hpp storage.hpp archive5.hpp archive_options5.hpp database5.hpp entrepot_libcurl5.hpp libdar5.hpp user_interaction5.hpp user_interaction_callback5.hpp shell_interaction_emulator.hpp memory_file.hpp tlv.hpp tlv_list.hpp fichier_global.hpp mem_ui.hpp entrepot_local.hpp etage.hpp data_tree.hpp tuyau.hpp tools.hpp compressor.hpp generic_file.hpp crc.hpp wrapperlib.hpp thread_cancellation.hpp capabilities.hpp fichier_local.hpp delta_sig_block_size.hpp zstd.hpp proto_compressor.hpp parallel_block_compressor.hpp block_compressor.hpp

install-data-local:
	mkdir -p $(DESTDIR)$(pkgincludedir)
	cp ../../config.h $(DESTDIR)$(pkgincludedir)/libdar_config.h
	sed -e 's%../config.h%libdar_config.h%g' -e 's%HAVE_CONFIG_H%1%g' -e 's%MY_CONFIG_H%LIBDAR_MY_CONFIG_H%g' '$(srcdir)/../my_config.h'  > ./libdar_my_config.h
	cp '$(srcdir)/../gettext.h' .
	for file in $(dist_noinst_DATA) gettext.h libdar_my_config.h ; do if [ -e $(srcdir)/$$file ] ; then src=$(srcdir)/$$file ; else src=$$file ; fi	; sed -e 's%#include \"../my_config.h\"%INC_MY_CONFIG_FILE_H%g' -e "s%#include \"%#include \"$(pkgincludedir)/%g" -e "s%INC_MY_CONFIG_FILE_H%#include \"$(pkgincludedir)/libdar_my_config.h\"%g" "$$src" > $(DESTDIR)$(pkgincludedir)/"$$file" ; done
	rm libdar_my_config.h gettext.h
	for file in $(DESTDIR)$(pkgincludedir)/* ; do sed -e 's%HAVE_%LIBDAR_HAS_%g' -e 's%PACKAGE%DAR_PACKAGE%g' -e 's%SIZEOF_%LIBDAR_SIZEOF_%g' -e 's%LSTAT_FOLLOWS_SLASHED_SYMLINK%LIBDAR_LSTAT_FOLLOWS_SLASHED_SYMLINK%g' -e 's%VERSION%LIBDAR_VERSION%g' -e 's%MUTEX_WORKS%LIBDAR_MUTEX_WORKS%g' -e 's%OS_BITS%LIBDAR_OS_BITS%g' -e 's%_AVAILABLE%_AVAILABLE_FOR_LIBDAR%g' -e 's%STDC_HEADERS%LIBDAR_STDC_HEADERS%g' -e 's%ENABLE_NLS%DAR_ENABLE_NLS%g' -e 's%HAVE_GETTEXT%DAR_HAS_GETTEXT%g' "$$file" > "$$file.tmp" && mv "$$file.tmp" "$$file" ; done
	egrep 'LIBDAR|DAR_' $(DESTDIR)$(pkgincludedir)/libdar_config.h | grep -v "#undef" >  $(DESTDIR)$(pkgincludedir)/config.h.tmp
	mv  $(DESTDIR)$(pkgincludedir)/config.h.tmp  $(DESTDIR)$(pkgincludedir)/libdar_config.h
	chmod 0644 $(DESTDIR)$(pkgincludedir)/*
	$(INSTALL) -d $(DESTDIR)$(pkgconfigdir)
	$(INSTALL) -m 0644 libdar.pc $(DESTDIR)$(pkgconfigdir)/libdar$(LIBDAR_SUFFIX).pc

uninstall-local:
	rm -rf $(DESTDIR)$(pkgincludedir) $(DESTDIR)$(pkgconfigdir)/libdar$(LIBDAR_SUFFIX).pc

all-local : libdar.pc

libdar.pc : libdar.pc.tmpl
	sed -e "s%#LIBDAR_VERSION#%$(LIBDAR_VERSION_OUT)%g" -e "s%#LIBDAR_SUFFIX#%$(LIBDAR_SUFFIX)%g" -e "s%#LIBDAR_MODE#%$(LIBDAR_MODE)%g" -e "s%#CXXFLAGS#%$(CXXFLAGS)%g" -e "s%#CXXSTDFLAGS#%$(CXXSTDFLAGS)%g" libdar.pc.tmpl > libdar.pc

# header files that are internal to libdar and that must not be installed (make install)
noinst_HEADERS = archive_version.hpp cache_global.hpp cache.hpp candidates.hpp cat_all_entrees.hpp catalogue.hpp cat_blockdev.hpp cat_chardev.hpp cat_delta_signature.hpp cat_detruit.hpp cat_device.hpp cat_directory.hpp cat_door.hpp cat_entree.hpp cat_eod.hpp cat_etoile.hpp cat_file.hpp cat_ignored_dir.hpp cat_ignored.hpp cat_inode.hpp cat_lien.hpp cat_mirage.hpp cat_nomme.hpp cat_prise.hpp cat_signature.hpp cat_tube.hpp contextual.hpp crypto_asym.hpp crypto_sym.hpp cygwin_adapt.hpp cygwin_adapt.h database_header.hpp data_dir.hpp defile.hpp ea_filesystem.hpp elastic.hpp entrepot_libcurl.hpp erreurs_ext.hpp escape_catalogue.hpp escape.hpp fichier_libcurl.hpp filesystem_backup.hpp filesystem_diff.hpp filesystem_hard_link_read.hpp filesystem_hard_link_write.hpp filesystem_restore.hpp filesystem_specific_attribute.hpp filesystem_tools.hpp filtre.hpp generic_file_overlay_for_gpgme.hpp generic_rsync.hpp generic_to_global_file.hpp hash_fichier.hpp header.hpp header_version.hpp i_archive.hpp i_database.hpp i_entrepot_libcurl.hpp i_libdar_xform.hpp label.hpp macro_tools.hpp mycurl_easyhandle_node.hpp mycurl_easyhandle_sharing.hpp mycurl_shared_handle.hpp nls_swap.hpp null_file.hpp op_tools.hpp pile_descriptor.hpp pile.hpp sar.hpp sar_tools.hpp scrambler.hpp secu_memory_file.hpp semaphore.hpp shell_interaction_emulator.hpp slave_zapette.hpp slice_layout.hpp smart_pointer.hpp sparse_file.hpp terminateur.hpp trivial_sar.hpp tronc.hpp tronconneuse.hpp trontextual.hpp user_group_bases.hpp zapette.hpp zapette_protocol.hpp mem_block.hpp parallel_tronconneuse.hpp crypto_segment.hpp crypto_module.hpp proto_tronco.hpp compress_module.hpp lz4_module.hpp gzip_module.hpp


ALL_SOURCES = archive5.cpp archive5.hpp archive_aux.cpp archive_aux.hpp archive.cpp archive.hpp archive_listing_callback.hpp archive_num.cpp archive_num.hpp archive_options5.hpp archive_options.cpp archive_options.hpp archive_options_listing_shell.cpp archive_options_listing_shell.hpp archive_summary.cpp archive_summary.hpp archive_version.cpp archive_version.hpp cache.cpp cache_global.cpp cache_global.hpp cache.hpp candidates.cpp candidates.hpp capabilities.cpp capabilities.hpp cat_all_entrees.hpp catalogue.cpp catalogue.hpp cat_blockdev.cpp cat_blockdev.hpp cat_chardev.cpp cat_chardev.hpp cat_delta_signature.cpp cat_delta_signature.hpp cat_detruit.cpp cat_detruit.hpp cat_device.cpp cat_device.hpp cat_directory.cpp cat_directory.hpp cat_door.cpp cat_door.hpp cat_entree.cpp cat_entree.hpp cat_eod.hpp cat_etoile.cpp cat_etoile.hpp cat_file.cpp cat_file.hpp cat_ignored.cpp cat_ignored_dir.cpp cat_ignored_dir.hpp cat_ignored.hpp cat_inode.cpp cat_inode.hpp cat_lien.cpp cat_lien.hpp cat_mirage.cpp cat_mirage.hpp cat_nomme.cpp cat_nomme.hpp cat_prise.cpp cat_prise.hpp cat_signature.cpp cat_signature.hpp cat_status.hpp cat_tube.cpp cat_tube.hpp compile_time_features.cpp compile_time_features.hpp compression.cpp compression.hpp compressor.cpp compressor.hpp contextual.cpp contextual.hpp crc.cpp crc.hpp crit_action.cpp crit_action.hpp criterium.cpp criterium.hpp crypto_asym.cpp crypto_asym.hpp crypto.cpp crypto.hpp crypto_sym.cpp crypto_sym.hpp cygwin_adapt.hpp cygwin_adapt.h database5.cpp database5.hpp database_archives.hpp database_aux.hpp database.cpp database_header.cpp database_header.hpp database.hpp database_listing_callback.hpp database_options.hpp data_dir.cpp data_dir.hpp data_tree.cpp data_tree.hpp datetime.cpp datetime.hpp deci.cpp deci.hpp defile.cpp defile.hpp ea.cpp ea_filesystem.cpp ea_filesystem.hpp ea.hpp elastic.cpp elastic.hpp entree_stats.cpp entree_stats.hpp entrepot.cpp entrepot.hpp entrepot_libcurl5.hpp entrepot_libcurl.hpp entrepot_local.cpp entrepot_local.hpp erreurs.cpp erreurs_ext.cpp erreurs_ext.hpp erreurs.hpp escape_catalogue.cpp escape_catalogue.hpp escape.cpp escape.hpp etage.cpp etage.hpp fichier_global.cpp fichier_global.hpp fichier_local.cpp fichier_local.hpp filesystem_backup.cpp filesystem_backup.hpp filesystem_diff.cpp filesystem_diff.hpp filesystem_hard_link_read.cpp filesystem_hard_link_read.hpp filesystem_hard_link_write.cpp filesystem_hard_link_write.hpp filesystem_restore.cpp filesystem_restore.hpp filesystem_specific_attribute.cpp filesystem_specific_attribute.hpp filesystem_tools.cpp filesystem_tools.hpp filtre.cpp filtre.hpp fsa_family.cpp fsa_family.hpp generic_file.cpp generic_file.hpp generic_file_overlay_for_gpgme.cpp generic_file_overlay_for_gpgme.hpp generic_rsync.cpp generic_rsync.hpp generic_to_global_file.hpp get_version.cpp get_version.hpp gf_mode.cpp gf_mode.hpp hash_fichier.cpp hash_fichier.hpp header.cpp header.hpp header_version.cpp header_version.hpp i_archive.cpp i_archive.hpp i_database.cpp i_database.hpp i_entrepot_libcurl.hpp i_libdar_xform.cpp i_libdar_xform.hpp infinint.hpp integers.cpp integers.hpp int_tools.cpp int_tools.hpp label.cpp label.hpp libdar5.cpp libdar5.hpp libdar.hpp libdar_slave.cpp libdar_slave.hpp libdar_xform.cpp libdar_xform.hpp limitint.hpp list_entry.cpp list_entry.hpp macro_tools.cpp macro_tools.hpp mask.cpp mask.hpp mask_list.cpp mask_list.hpp memory_file.cpp memory_file.hpp mem_ui.cpp mem_ui.hpp mycurl_easyhandle_node.cpp mycurl_easyhandle_node.hpp mycurl_easyhandle_sharing.cpp mycurl_easyhandle_sharing.hpp mycurl_protocol.cpp mycurl_protocol.hpp mycurl_shared_handle.cpp mycurl_shared_handle.hpp nls_swap.hpp null_file.hpp op_tools.cpp op_tools.hpp path.cpp path.hpp pile.cpp pile_descriptor.cpp pile_descriptor.hpp pile.hpp proto_generic_file.hpp range.cpp range.hpp real_infinint.hpp sar.cpp sar.hpp sar_tools.cpp sar_tools.hpp scrambler.cpp scrambler.hpp secu_memory_file.cpp secu_memory_file.hpp secu_string.cpp secu_string.hpp semaphore.cpp semaphore.hpp shell_interaction.cpp shell_interaction_emulator.cpp shell_interaction_emulator.hpp shell_interaction.hpp slave_zapette.cpp slave_zapette.hpp slice_layout.cpp slice_layout.hpp smart_pointer.hpp sparse_file.cpp sparse_file.hpp statistics.cpp statistics.hpp storage.cpp storage.hpp terminateur.cpp terminateur.hpp thread_cancellation.cpp thread_cancellation.hpp tlv.cpp tlv.hpp tlv_list.cpp tlv_list.hpp tools.cpp tools.hpp trivial_sar.cpp trivial_sar.hpp tronc.cpp tronc.hpp tronconneuse.cpp tronconneuse.hpp trontextual.cpp trontextual.hpp tuyau.cpp tuyau.hpp user_group_bases.cpp user_group_bases.hpp user_interaction5.cpp user_interaction5.hpp user_interaction_blind.cpp user_interaction_blind.hpp user_interaction_callback5.cpp user_interaction_callback5.hpp user_interaction_callback.cpp user_interaction_callback.hpp user_interaction.cpp user_interaction.hpp wrapperlib.cpp wrapperlib.hpp zapette.cpp zapette.hpp zapette_protocol.cpp zapette_protocol.hpp entrepot_libcurl.cpp fichier_libcurl.cpp i_entrepot_libcurl.cpp delta_sig_block_size.cpp zstd.cpp mem_block.hpp mem_block.cpp heap.hpp parallel_tronconneuse.hpp crypto_module.hpp proto_compressor.hpp parallel_block_compressor.hpp compress_module.hpp lz4_module.hpp lz4_module.cpp block_compressor.cpp block_compressor.hpp gzip_module.hpp gzip_module.cpp

libdar_la_LDFLAGS = -version-info $(LIBDAR_VERSION_IN)
libdar_la_SOURCES = $(ALL_SOURCES) real_infinint.cpp $(LIBTHREADAR_DEP_MODULES)

libdar32_la_LDFLAGS = -version-info $(LIBDAR_VERSION_IN)
libdar32_la_SOURCES = $(ALL_SOURCES) limitint.cpp $(LIBTHREADAR_DEP_MODULES)

libdar64_la_LDFLAGS = -version-info $(LIBDAR_VERSION_IN)
libdar64_la_SOURCES = $(ALL_SOURCES) limitint.cpp $(LIBTHREADAR_DEP_MODULES)

