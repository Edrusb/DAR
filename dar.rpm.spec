summary: DAR - Disk ARchive
Name: dar
Version: 1.2.0
Release: 2
Copyright: GPL
Icon: dar.gif
Group: Applications/Archiving
Source: http://dar.linux.free.fr/dar-1.2.0.tar.gz
URL: http://dar.linux.free.fr/
%description
  DAR is a shell command, that makes backup of a directory tree and files.
  DAR is able to make differential backup, split it over a set of disks or 
  files of given size, use compression, filters files or subtrees not to save
  or to save, directly access and restore a given file. Dar also takes
  care of Extented Attributes, and is able to be make remote backup through a
  ssh session for example. Dar is also able to save and restore hard links not
  only symbolic links.

%prep
%setup

%clean 
make clean

%build
make

%install
make INSTALL_ROOT_DIR=/usr MAN_DIR=share/man install

%post

%files
/usr/bin/dar
/usr/bin/dar_slave
/usr/bin/dar_xform
/usr/bin/dar_manager
/usr/share/man/man1/dar.1
/usr/share/man/man1/dar_slave.1
/usr/share/man/man1/dar_xform.1
/usr/share/man/man1/dar_manager.1
%doc README TUTORIAL CHANGES LICENSE BUGS INSTALL TODO NOTES RESUME THANKS

%changelog

* Thu Jun 27 2002 Denis Corbin

  - see file named "CHANGES"
