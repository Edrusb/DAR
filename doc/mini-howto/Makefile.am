dist_pkgdata_DATA = dar-differential-backup-mini-howto.en.html dar-differential-backup-mini-howto.it.html dar-differential-backup-mini-howto.es.html index.html

install-data-hook:
	$(INSTALL) -d $(DESTDIR)$(pkgdatadir)/mini-howto
	for f in $(dist_pkgdata_DATA); do $(INSTALL) -m 0644 '$(srcdir)/'"$${f}" $(DESTDIR)$(pkgdatadir)/mini-howto; done

uninstall-hook:
	rm -rf $(DESTDIR)$(pkgdatadir)/mini-howto
