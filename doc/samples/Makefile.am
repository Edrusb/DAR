dist_noinst_DATA = README cdbackup.sh darrc_sample sample1.txt dar_par.dcf README
dist_pkgdata_SCRIPTS = dar_par_create.duc dar_par_test.duc
install-data-local:
	mkdir -p $(DESTDIR)$(pkgdatadir)
	sed -e "s%SOMEPATH%$(pkgdatadir)%g" dar_par.dcf > $(DESTDIR)$(pkgdatadir)/dar_par.dcf
	chmod 0644 $(DESTDIR)$(pkgdatadir)/dar_par.dcf

