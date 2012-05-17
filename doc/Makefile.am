SUBDIRS = samples mini-howto man
dist_noinst_DATA = COMMAND_LINE Doxyfile
dist_pkgdata_DATA = README Features.html Limitations.html Notes.html Tutorial.html Good_Backup_Practice.html FAQ.html api_tutorial.html dar_doc.jpg dar_s_doc.jpg index.html dar-catalog-1.0.dtd Known_Bugs.html authentification.html dar_key.txt

INSTALL_HTML_MAN = $(INSTALL) -d $(DESTDIR)$(pkgdatadir)/man ; $(INSTALL) -m 0644 man/*.html $(DESTDIR)$(pkgdatadir)/man
UNINSTALL_HTML_MAN = rm -rf $(DESTDIR)$(pkgdatadir)/man

if USE_DOXYGEN
DOXYGEN = @DOXYGEN_PROG@

all-local: Doxyfile.tmp

Doxyfile.tmp:
	sed -e "s%##VERSION##%@PACKAGE_VERSION@%g" Doxyfile > Doxyfile.tmp
	cd .. ; $(DOXYGEN) doc/Doxyfile.tmp

clean-local:
	rm -rf html Doxyfile.tmp

install-data-hook:
	$(INSTALL)  -d $(DESTDIR)$(pkgdatadir)/html
	$(INSTALL) -m 0644 html/* $(DESTDIR)$(pkgdatadir)/html
	$(INSTALL_HTML_MAN)

uninstall-hook:
	rm -rf $(DESTDIR)$(pkgdatadir)/html
	$(UNINSTALL_HTML_MAN)
	rmdir $(DESTDIR)$(pkgdatadir) || true

else

all-local:

clean-local:

install-data-hook:
	$(INSTALL_HTML_MAN)

uninstall-hook:
	$(UNINSTALL_HTML_MAN)

endif




