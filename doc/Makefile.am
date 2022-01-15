SUBDIRS = samples mini-howto man
dist_noinst_DATA = COMMAND_LINE Doxyfile portable_cp Benchmark_tools/README Benchmark_tools/always_change Benchmark_tools/bitflip Benchmark_tools/build_test_tree.bash Benchmark_tools/hide_change Benchmark_tools/historization_feature restoration_dependencies.txt
dist_pkgdata_DATA = README Features.html Limitations.html Notes.html Tutorial.html Good_Backup_Practice.html FAQ.html api_tutorial.html dar_doc.jpg dar_s_doc.jpg index.html dar-catalog.dtd authentification.html dar_key.txt old_dar_key1.txt old_dar_key2.txt from_sources.html presentation.html usage_notes.html python/libdar_test.py style.css restoration-with-dar.html benchmark.html benchmark_logs.html index_dar.html index_internal.html index_libdar.html

if USE_DOXYGEN
DOXYGEN = @DOXYGEN_PROG@

all-local: Doxyfile.tmp

Doxyfile.tmp:
	sed -e "s%##VERSION##%@PACKAGE_VERSION@%g" -e "s%##HAS_DOT##%@HAS_DOT@%g" -e 's%##SRCDIR##%$(abs_top_srcdir)%g' -e 's%##BUILDDIR##%$(abs_top_builddir)%g' '$(srcdir)/Doxyfile' > Doxyfile.tmp
	cd '$(top_srcdir)' ; $(DOXYGEN) '$(abs_top_builddir)/doc/Doxyfile.tmp'
	if [ -d html/search ]; then chmod u+x html/search ; fi

clean-local:
	rm -rf html Doxyfile.tmp doxygen_sqlite3.db

install-data-hook:
	'$(srcdir)/portable_cp' html $(DESTDIR)$(pkgdatadir)
	$(INSTALL) -d $(DESTDIR)$(pkgdatadir)/python
	$(INSTALL) -m 0644 '$(srcdir)/python/libdar_test.py' $(DESTDIR)$(pkgdatadir)/python

uninstall-hook:
	rm -rf $(DESTDIR)$(pkgdatadir)/html
	rm -rf $(DESTDIR)$(pkgdatadir)/python
	rmdir $(DESTDIR)$(pkgdatadir) || true

else

all-local:

clean-local:

install-data-hook:

uninstall-hook:

endif




