summary: DAR - Disk ARchive
Name: dar
Version: 1.3.0
Release: 4
Copyright: GPL
Icon: dar.gif
Group: Applications/Archiving
Source: http://dar.linux.free.fr/dar-1.3.0.tar.gz
URL: http://dar.linux.free.fr/
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
BuildRequires: zlib-devel >= 1.1.3, gcc-c++, libbz2-devel >= 1.0.4

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
%ifarch alpha
make OPTIMIZATION="%{optflags} -O0" FILEOFFSET=yes
%else
make OPTIMIZATION="%{optflags} -O"
%endif

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr
make INSTALL_ROOT_DIR=%{buildroot}/usr MAN_DIR=share/man INSTALL=install install

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
%doc README TUTORIAL CHANGES LICENSE BUGS INSTALL TODO NOTES THANKS

%changelog

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


