# THIS IS A GENERATED FILE DO NOT EDIT !
summary: DAR - Disk ARchive
Name: dar
Version: 2.0.1
Release: 8
Copyright: GPL
Icon: dar.gif
Group: Applications/Archiving
Source: http://dar.linux.free.fr/dar-2.0.1.tar.gz
URL: http://dar.linux.free.fr/
BuildRoot: %{_tmppath}/%{name}64-%{version}-%{release}-root
BuildRequires: zlib-devel >= 1.1.3, gcc-c++, bzip2-devel >= 1.0.2

%description
DAR is a command line tool to backup a directory tree and files. DAR is
able to make differential backups, split them over a set of disks or files
of a given size, use compression, filter files or subtrees to be saved or
not saved, directly access and restore given files. DAR is also able
to handle extented attributes, and can make remote backups through an
ssh session for example. Finally, DAR handles save and restore of hard
and symbolic links.

%prep
%setup

%clean
make clean
rm -rf %{buildroot}

%build
./configure --enable-os-bits=32 --enable-mode=64 --prefix=/usr --mandir=/usr/share/man
make

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr
make DESTDIR=%{buildroot} install-strip

%post

%files
%defattr(-,root,root,-)
/usr/bin/dar
/usr/bin/dar_slave
/usr/bin/dar_xform
/usr/bin/dar_manager
/usr/bin/dar_static
/usr/share/man/man1/dar.1
/usr/share/man/man1/dar_slave.1
/usr/share/man/man1/dar_xform.1
/usr/share/man/man1/dar_manager.1
/usr/include/dar/deci.hpp
/usr/include/dar/erreurs.hpp
/usr/include/dar/infinint.hpp
/usr/include/dar/integers.hpp
/usr/include/dar/libdar.hpp
/usr/include/dar/limitint.hpp
/usr/include/dar/real_infinint.hpp
/usr/include/dar/mask.hpp
/usr/include/dar/path.hpp
/usr/include/dar/statistics.hpp
/usr/include/dar/user_interaction.hpp
/usr/lib/libdar64.a
/usr/lib/libdar64.la
/usr/lib/libdar64.so
/usr/lib/libdar64.so.1
/usr/lib/libdar64.so.1.0.0
/usr/bin/dar_cp
/usr/share/dar/dar_par.dcf
/usr/share/dar/dar_par_create.duc
/usr/share/dar/dar_par_test.duc
%doc AUTHORS BUGS COPYING ChangeLog INSTALL NEWS README THANKS TODO doc/DOC_API doc/FEATURES doc/GOOD_BACKUP_PRACTICE doc/LIMITATIONS doc/NOTES doc/TUTORIAL doc/configure_options doc/samples/README doc/samples/cdbackup.sh doc/samples/darrc_sample doc/samples/sample1.txt doc/dar-differential-backup-mini-howto.en.html doc/LINKS

%changelog
* Tue Oct 21 2003 Denis Corbin <dar.linux@free.fr>
- added mini-howto and LINKS documentation

* Thu Oct  9 2003 Denis Corbin <dar.linux@free.fr>
- added sample scripts and dar_cp

* Mon Sep 15 2003 Denis Corbin <dar.linux@free.fr
- adapted spec file to dar version 2 (configure script)

* Wed May 14 2003 Denis Corbin <dar.linux@free.fr>
- added dependency libbz2
- see CHANGES file for more

* Thu Jan  9 2003 Denis Corbin <dar.linux@free.fr>
- removed the OS_BITS flag, which is no more necessary
- added dar_static in %files

* Thu Nov  7 2002 Axel Kohlmeyer <axel.kohlmeyer@theochem.ruhr-uni-bochum.de>
- modified the spec file to comply with standard redhat rpms
- allow building of rpm as non-root user
- add build dependency on zlib and c++
- handle x86/alpha arch from specfile.

* Thu Jun 27 2002 Denis Corbin

  - see file named "CHANGES"
