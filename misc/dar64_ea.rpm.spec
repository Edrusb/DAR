# THIS IS A GENERATED FILE DO NOT EDIT !
summary: DAR - Disk ARchive
Name: dar
Version: 2.1.5
Release: 1
Copyright: GPL
Icon: dar.gif
Group: Applications/Archiving
Source: http://dar.linux.free.fr/dar-2.1.5.tar.gz
URL: http://dar.linux.free.fr/
BuildRoot: %{_tmppath}/%{name}64_ea-%{version}-%{release}-root
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
./configure CXXFLAGS=-O --enable-mode=64 --enable-ea-support --prefix=/usr --mandir=/usr/share/man
make

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr
make DESTDIR=%{buildroot} install-strip

%post

%files
%defattr(-,root,root,-)
/usr/share/man/man1/dar.1
/usr/share/man/man1/dar_manager.1
/usr/share/man/man1/dar_slave.1
/usr/share/man/man1/dar_xform.1
/usr/share/man/man1/dar_cp.1
/usr/lib/libdar64.so.2.0.5
/usr/lib/libdar64.so.2
/usr/lib/libdar64.so
/usr/lib/libdar64.la
/usr/lib/libdar64.a
/usr/include/dar/config.h
/usr/include/dar/libdar.hpp
/usr/include/dar/path.hpp
/usr/include/dar/mask.hpp
/usr/include/dar/integers.hpp
/usr/include/dar/real_infinint.hpp
/usr/include/dar/statistics.hpp
/usr/include/dar/user_interaction.hpp
/usr/include/dar/erreurs.hpp
/usr/include/dar/deci.hpp
/usr/include/dar/limitint.hpp
/usr/include/dar/infinint.hpp
/usr/include/dar/compressor.hpp
/usr/include/dar/special_alloc.hpp
/usr/include/dar/generic_file.hpp
/usr/include/dar/wrapperlib.hpp
/usr/include/dar/storage.hpp
/usr/include/dar/tuyau.hpp
/usr/include/dar/tools.hpp
/usr/include/dar/catalogue.hpp
/usr/include/dar/scrambler.hpp
/usr/include/dar/archive.hpp
/usr/include/dar/header_version.hpp
/usr/include/dar/ea.hpp
/usr/include/dar/crypto.hpp
/usr/bin/dar
/usr/bin/dar_xform
/usr/bin/dar_slave
/usr/bin/dar_manager
/usr/bin/dar_cp
/usr/bin/dar_static
/usr/share/dar/dar_par.dcf
/usr/share/dar/dar_par_create.duc
/usr/share/dar/dar_par_test.duc
/usr/share/dar/FEATURES
/usr/share/dar/LIMITATIONS
/usr/share/dar/NOTES
/usr/share/dar/TUTORIAL
/usr/share/dar/DOC_API_V1
/usr/share/dar/DOC_API_V2
/usr/share/dar/GOOD_BACKUP_PRACTICE
/usr/share/dar/README
/usr/share/dar/dar-differential-backup-mini-howto.en.html
/usr/share/dar/LINKS

%changelog
* Sat Nov 22 2003 Denis Corbin <dar.linux@free.fr>
- removed the %doc in spec file, as documentation
  is now installed by "make"

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
