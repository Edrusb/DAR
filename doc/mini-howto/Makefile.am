dist_pkgdata_DATA = dar-differential-backup-mini-howto.en.html dar-differential-backup-mini-howto.it.html dar-differential-backup-mini-howto.es.html index.html

install-data-hook:
	$(INSTALL) -d $(DESTDIR)$(pkgdatadir)/mini-howto
	$(INSTALL) -m 0644 $(dist_pkgdata_DATA) $(DESTDIR)$(pkgdatadir)/mini-howto

uninstall-hook:
	rm -rf $(DESTDIR)$(pkgdatadir)/mini-howto
