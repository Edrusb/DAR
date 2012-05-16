SUBDIRS = samples mini-howto
dist_noinst_DATA = COMMAND_LINE configure_options Doxyfile
dist_pkgdata_DATA = FEATURES LIMITATIONS NOTES TUTORIAL GOOD_BACKUP_PRACTICE README LINKS FAQ api_tutorial.html

if USE_DOXYGEN
DOXYGEN = @DOXYGEN_PROG@

all-local: Doxyfile.tmp

Doxyfile.tmp:
	sed -e "s%##VERSION##%@PACKAGE_VERSION@%g" Doxyfile > Doxyfile.tmp
	cd .. ; $(DOXYGEN) doc/Doxyfile.tmp

clean-local:
	rm -rf html Doxyfile.tmp

else

all-local:

clean-local:

endif
