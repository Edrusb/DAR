#######################################################################
# dar - disk archive - a backup/restoration program
# Copyright (C) 2002-2022 Denis Corbin
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
# to contact the author, see the AUTHOR file
#######################################################################

# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.69)
AC_INIT([DAR], [2.7.6.RC3], [[http://sourceforge.net/tracker/?group_id=65612&atid=511612]])
AC_CONFIG_HEADERS([config.h])
AC_LANG([C++])
AC_CONFIG_SRCDIR([src/libdar/catalogue.cpp])
AC_DEFINE_UNQUOTED(DAR_VERSION, "AC_PACKAGE_VERSION", [dar and dar_suite version, definition in configure.ac])

AM_INIT_AUTOMAKE([subdir-objects])
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION
XGETTEXT_EXTRA_OPTIONS='--keyword=dar_gettext'
AM_ICONV

####
# configure checks what is available from the operational system:
# - it displays things on output for the user running the configure script has status information
# - it sets some shell variable that are not used outside the configuration script
# - it sets some shell variable that can be susbstitued in Makefile.in files (see AC_SUBST() and AC_CONFIG_FILES())
#   also known as "output variables"
# - it defines macros that get stored in config.h and used in source code (see AC_DEFINE())
#
# header files:
#   header files are #included in source code if they HAVE_... macro has been defined in config.h
#
# libraries:
#   necessary library flags are stored in the "output variables" LIBS substituable shell variable and passed to Makefile.in
#   as well as other like LIBTHREADAR_LIBS/CFLAGS and LIBCURL_LIBSS/CFLAGS due to the use of PKG_CHECK_MODULE macro
#   that get substitued in Makefiles.in when the @LIBS@ form is met.
#   In addition, the AM_PATH_GPGME defines he GPGME_CFLAGS and GPGME_LIBS output variable
#

# have a specific variable for pkgconfig, setting the default value:
AC_SUBST(pkgconfigdir, [${libdir}/pkgconfig])
AC_ARG_WITH([pkgconfigdir],
            AS_HELP_STRING(--with-pkgconfigdir=DIR, [defines an alternative directory to install pkconfig files, default is '${libdir}/pkgconfig']),
            [
                if [ ! -z "$withval" ] ; then
                 AC_SUBST(pkgconfigdir, $withval)
               fi
            ],
            []
           )

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_MAKE_SET
AC_PROG_RANLIB

AC_MSG_CHECKING([for C++ compiler usability])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],
                                  [ class test { public: int test; }; ])],
                  [AC_MSG_RESULT(ok)],
                  [AC_MSG_ERROR([No C++ compiler found])])

# Defining _XOPEN_SOURCE to get extra field in struct stat
AC_DEFINE(_XOPEN_SOURCE, 700, [activates POSIX.1-2008 symbols in order to allow microsecond time setting, as well as ctime_r() call])

# Define _BSD_SOURCE in order to be able to call makedev(), minor() and major() under OpenBSD when _XOPEN_SOURCE is set
AC_DEFINE(_BSD_SOURCE, 1, [activate makedev(), major() and minor() when _XOPEN_SOURCE is set])
AC_DEFINE(_DEFAULT_SOURCE, 1, [disabling warning about _BSD_SOURCE to be deprecated])

###########
## THE FOLLOWING "DEFINE" USED TO RE-ENABLE FULL LIBC FEATURES ON DIFFERENT OPERATING SYSTEMS
## HAVE BEEN BORROWED FROM PYTHON's configure.in
##
##

# The later defininition of _XOPEN_SOURCE disables certain features
# on Linux, so we need _GNU_SOURCE to re-enable them (makedev, tm_zone).
AC_DEFINE(_GNU_SOURCE, 1, [Define on Linux to activate all library features])

# The later defininition of _XOPEN_SOURCE and _POSIX_C_SOURCE disables
# certain features on NetBSD, so we need _NETBSD_SOURCE to re-enable
# them.
AC_DEFINE(_NETBSD_SOURCE, 1, [Define on NetBSD to activate all library features])

# The later defininition of _XOPEN_SOURCE and _POSIX_C_SOURCE disables
# certain features on FreeBSD, so we need __BSD_VISIBLE to re-enable
# them.
AC_DEFINE(__BSD_VISIBLE, 1, [Define on FreeBSD to activate all library features])

# The later defininition of _XOPEN_SOURCE and _POSIX_C_SOURCE disables
# certain features on Mac OS X, so we need _DARWIN_C_SOURCE to re-enable
# them.
AC_DEFINE(_DARWIN_C_SOURCE, 1, [Define on Darwin to activate all library features])

##
##
###########

# Checks for libraries.
AC_CHECK_LIB(socket, [socket], [], [])
AC_CHECK_LIB(nsl, [endnetconfig], [], [])
AC_CHECK_LIB(cap, [cap_get_proc], [], [])


AC_ARG_ENABLE( [libdl-linking],
               AS_HELP_STRING(--disable-libdl-linking, [ignore any libdl and avoid linking against it]),
               [
               AS_IF([test "x$enable_libdl_linking" != "xno"],
                     [AC_MSG_ERROR([invalid argument given to --disable-libdl-linking])]
                    )
               ],
               [
                 AC_CHECK_LIB(dl, [dlsym], [], [])
               ])


# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT

AC_CHECK_HEADERS([fcntl.h netinet/in.h arpa/inet.h stdint.h stdlib.h string.h sys/ioctl.h sys/socket.h termios.h unistd.h utime.h sys/types.h signal.h errno.h sys/un.h sys/stat.h time.h fnmatch.h regex.h pwd.h grp.h stdio.h pthread.h ctype.h getopt.h limits.h stddef.h sys/utsname.h libintl.h sys/capability.h linux/capability.h utimes.h sys/time.h wchar.h wctype.h stddef.h])

AC_SYS_LARGEFILE

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_DECL_SYS_SIGLIST
AC_CHECK_TYPE(size_t, [AC_CHECK_SIZEOF(size_t)], [AC_MSG_ERROR([Cannot find size_t type])], [])
AC_CHECK_TYPE(time_t, [AC_CHECK_SIZEOF(time_t)], [AC_MSG_ERROR([Cannot find time_t type])], [])
AC_CHECK_TYPE(off_t, [AC_CHECK_SIZEOF(off_t)], [AC_MSG_ERROR([Cannot find off_t type])], [])


# Checks for library functions.
AC_FUNC_FNMATCH
AC_FUNC_FORK
AC_PROG_GCC_TRADITIONAL
AC_FUNC_LSTAT
AC_HEADER_MAJOR
AC_FUNC_MALLOC
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_FUNC_UTIME_NULL
AC_HEADER_TIME
AC_CHECK_FUNCS([lchown mkdir regcomp rmdir strerror_r utime fdopendir readdir_r ctime_r getgrnam_r getpwnam_r localtime_r])

AC_MSG_CHECKING([for c++14 support])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
				     #include <memory>
				   ],
                                   [ thread_local static int test = 0;
				     std::unique_ptr<int> x = std::make_unique<int>(0);
				   ])
                  ],
                  [ AC_MSG_RESULT(yes) ],
                  [
                     AC_MSG_RESULT([no])
                     AC_MSG_CHECKING([for c++14 support with -std=c++14 option set])
                     CXXSTDFLAGS="-std=c++14"
                     CXXFLAGS="$CXXFLAGS $CXXSTDFLAGS"
                     AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
							  #include <memory>
							],
                                                        [ thread_local static int test = 0;
							  std::unique_ptr<int> x = std::make_unique<int>(0);
							])
                                        ],
                                        [ AC_MSG_RESULT(yes) ],
                                        [
                                           AC_MSG_RESULT(no)
                                           AC_MSG_ERROR([C++ compiler lack support for c++14 standard])
                                        ]
                     )
                   ])

AC_MSG_CHECKING([for sed -r/-E option])
if sed -r -e 's/(c|o)+/\1/g' > /dev/null < /dev/null ; then
    local_sed="gnu"
    AC_MSG_RESULT([GNU sed, using -r option for regex])
else
   if sed -E -e 's/(c|o)+/\1/g' > /dev/null < /dev/null ; then
     local_sed="bsd"
     AC_MSG_RESULT([BSD sed, using -E option for regex])
   else
     local_sed=unknown
     AC_MSG_ERROR([unknown switch to use with sed to support regex])
   fi
fi

AC_MSG_CHECKING([for getopt() in <unistd.h>])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[extern "C" {
                                    #if HAVE_UNISTD_H
                                    #include <unistd.h>
                                    #endif
                                    }]],
                                    [ getopt(0, 0, 0); ])
                  ],
                  [
                    AC_DEFINE(HAVE_GETOPT_IN_UNISTD_H, 1, [a getopt() call is declared in <unistd.h>])
                    AC_MSG_RESULT(present)
                  ],
                   [AC_MSG_RESULT(absent)])

AC_MSG_CHECKING([for getopt_long() in <unistd.h>])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[extern "C" {
                                    #if HAVE_UNISTD_H
                                    #include <unistd.h>
                                    #endif
                                    }]],
                                    [ getopt_long(0, 0, 0, 0, 0); ])
                  ],
                  [
                    AC_DEFINE(HAVE_GETOPT_LONG_IN_UNISTD_H, 1, [a getopt_long() call is declared in <unistd.h>])
                    AC_MSG_RESULT(present)
                  ],
                   [AC_MSG_RESULT(absent)])


AC_MSG_CHECKING([for optreset presence])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[extern "C" {
                                   #if HAVE_STDIO_H
                                   #include <stdio.h>
                                   #endif
                                   #if HAVE_GETOPT_H
                                   #include <getopt.h>
                                   #else
                                   #if HAVE_UNISTD_H
                                   #include <unistd.h>
                                   #endif
                                   #endif
                                   }]], [ int x = optreset; return 0; ])
                  ],
                  [
                   AC_DEFINE(HAVE_OPTRESET, 1, [the optreset external variable exists to reset getopt standard call])
                   AC_MSG_RESULT(available)
                  ],
                  [AC_MSG_RESULT([not available])])

AC_MSG_CHECKING([for Door file support])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[extern "C" {
                                    #if HAVE_SYS_STAT_H
                                    #include <sys/stat.h>
                                    #endif
                                    #if HAVE_UNISTD_H
                                    #include <unistd.h>
                                    #endif
                                  }]],
                                  [ struct stat buf;
                                    if(S_ISDOOR(buf.st_mode))
                                      return 0;
                                    else
                                      return 1;
                                  ])
                  ],
                  [
                    AC_DEFINE(HAVE_DOOR, 1, [whether the system has the necessary routine to handle Door files])
                    AC_MSG_RESULT(available)
                  ],
                  [AC_MSG_RESULT([not available])])



AC_MSG_CHECKING([for POSIX.1e capabilities support])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[extern "C" {
                                  #if HAVE_SYS_CAPABILITY_H
                                  #include <sys/capability.h>
                                  #else
                                  #if HAVE_LINUX_CAPABILITY_H
                                  #include <linux/capability.h>
                                  #endif
                                  #endif
                                  #if HAVE_SYS_TYPES_H
                                  #include <sys/types.h>
                                  #endif
                                  }]],
                                  [ cap_t capaset = cap_get_proc();
                                    (void)cap_free((void *)capset);
                                  ])
                   ],
                   [
                      AC_DEFINE(HAVE_CAPABILITIES, 1, [whether the system has support for POSIX.1e capabilities])
                      AC_MSG_RESULT(available)
                   ],
                   [  AC_MSG_RESULT([not available])  ])


AC_MSG_CHECKING([for fdatasync() availability])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[extern "C" {
                                   #if HAVE_UNISTD_H
                                   #include <unistd.h>
                                   #endif
                                   }]],
                                   [
                                   (void)fdatasync(0);
                                   ])
                   ],
                   [ AC_DEFINE(HAVE_FDATASYNC, 1, [whether the system provides fdatasync() system call])
                     AC_MSG_RESULT(available)
                   ],
                   [ AC_MSG_RESULT([not available])
                   ])

AC_MSG_CHECKING([for syncfs() availability])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[extern "C" {
                                   #if HAVE_UNISTD_H
                                   #include <unistd.h>
                                   #endif
                                   }]],
                                   [
                                   (void)syncfs(0);
                                   ])
                   ],
                   [ AC_DEFINE(HAVE_SYNCFS, 1, [whether the system provides syncfs() system call])
                     AC_MSG_RESULT(available)
                   ],
                   [ AC_MSG_RESULT([not available])
                   ])


local_time_accuracy_second=0;
local_time_accuracy_microsecond=6
local_time_accuracy_nanosecond=9
AC_DEFINE_UNQUOTED(LIBDAR_TIME_ACCURACY_SECOND, $local_time_accuracy_second, [value for time accuracy representing an accuracy of 1 second])
AC_DEFINE_UNQUOTED(LIBDAR_TIME_ACCURACY_MICROSECOND, $local_time_accuracy_microsecond, [value for time accuracy representing an accuracy of 1 microsecond])
AC_DEFINE_UNQUOTED(LIBDAR_TIME_ACCURACY_NANOSECOND, $local_time_accuracy_nanosecond, [value for time accuracy representing an accuracy of 1 nanosecond])


AC_ARG_ENABLE( [limit-time-accuracy],
               AS_HELP_STRING(--enable-limit-time-accuracy, [limit time accuracy to nanosecond (ns) microsecond (us) or second (s)]),
               [
                AS_IF(  [ test "x$enable_limit_time_accuracy" != "xs" -a "x$enable_limit_time_accuracy" != "xus" -a "x$enable_limit_time_accuracy" != "xns" ],
                        [ AC_MSG_ERROR([invalid argument given to --enable-limit-time-accuracy, valide values are : s, us, ns])],
                        [ AC_MSG_WARN([limiting time accuracy to $enable_limit_time_accuracy])]
                        )
               ],
               [ enable_limit_time_accuracy=no ])


AC_MSG_CHECKING([for the timestamps write precision])

AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[extern "C" {
                                     #if HAVE_FCNTL_H
                                     #include <fcntl.h>
                                     #endif
                                     #if HAVE_SYS_STAT_H
                                     #include <sys/stat.h>
                                     #endif
                                   }]],
                                   [[
                                        struct timespec a[2];
                                        a[0].tv_sec = 0;
                                        a[1].tv_nsec = 1;
                                        (void)utimensat(0, "/tmp/testfile", a,  0);
                                        /* not that this test program is only compiled+lined not executed */
                                   ]])
                  ],
                  [
                        AS_IF([ test "x$enable_limit_time_accuracy" = "xs" -o "x$enable_limit_time_accuracy" = "xus" ],
                              [
                                local_time_write_accuracy=-1
                              ],
                              [
                                local_time_write_accuracy=$local_time_accuracy_nanosecond
                                AC_MSG_RESULT([1 nanosecond])
                              ])
                  ],
                  [
                        local_time_write_accuracy=-1
                  ])

AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[extern "C" {
                                     #if HAVE_SYS_TYPES_H
                                     #include <sys/types.h>
                                     #endif
                                     #if HAVE_SYS_STAT_H
                                     #include <sys/stat.h>
                                     #endif
                                     #if HAVE_UNISTD_H
                                     #include <unistd.h>
                                     #endif
                                     #if HAVE_UTIMES_H
                                     #include <utimes.h>
                                     #endif
                                     #if HAVE_SYS_TIME_H
                                     #include <sys/time.h>
                                     #endif
                                     }]],
                                     [[
                                       struct timeval tv[2];
                                       tv[0].tv_sec = 1000;
                                       tv[1].tv_usec = 2000;

                                       (void)utimes("/tmp/testfile.tmp", tv);
                                        /* note that this test program is only compiled+linked not run */
                                        return 0;
                                     ]])
                  ],
                  [
                    AS_IF([ test $local_time_write_accuracy -eq -1 ],
                          AS_IF([ test "x$enable_limit_time_accuracy" != "xs" ],
                                [
                                    local_time_write_accuracy=$local_time_accuracy_microsecond
                                    AC_MSG_RESULT([1 microsecond])
                                ],
                                [
                                    local_time_write_accuracy=$local_time_accuracy_second
                                    AC_MSG_RESULT([1 second])
                                ])
                        )
                  ],
                  [
                    AS_IF([ test $local_time_write_accuracy -eq -1 ],
                          [
                              local_time_write_accuracy=$local_time_accuracy_second
                              AC_MSG_RESULT([1 second])
                          ])
                  ])


AC_DEFINE_UNQUOTED(LIBDAR_TIME_WRITE_ACCURACY, $local_time_write_accuracy, [timestamps write accuracy])

AC_MSG_CHECKING([for the timestamps read precision])

AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[extern "C" {
                                     #if HAVE_SYS_TYPES_H
                                     #include <sys/types.h>
                                     #endif
                                     #if HAVE_SYS_STAT_H
                                     #include <sys/stat.h>
                                     #endif
                                     #if HAVE_UNISTD_H
                                     #include <unistd.h>
                                     #endif
                                     }]],
                                     [[
                                       struct stat st;
                                       if(st.st_atim.tv_nsec != 0)
                                         return 0;
                                       else
                                         return 1;
                                         /* whatever, this test program is only compiled no run */
                                     ]])
                 ],
                 [
                        AS_IF([ test "x$enable_limit_time_accuracy" = "xus" ],
                              [
                                  local_time_read_accuracy=$local_time_accuracy_microsecond
                                  AC_MSG_RESULT([1 microsecond])
                              ],
                              [
                                 AS_IF([ test "x$enable_limit_time_accuracy" = "xs" ],
                                       [
                                            local_time_read_accuracy=$local_time_accuracy_second
                                            AC_MSG_RESULT([1 second])
                                       ],
                                       [
                                            local_time_read_accuracy=$local_time_accuracy_nanosecond
                                            AC_MSG_RESULT([1 nanosecond])
                                       ])
                              ])
                  ],
                  [
                    local_time_read_accuracy=$local_time_accuracy_second
                    AC_MSG_RESULT([1 second])
                  ])

AC_DEFINE_UNQUOTED(LIBDAR_TIME_READ_ACCURACY, $local_time_read_accuracy, [timestamps read accuracy])


AC_MSG_CHECKING([for lutimes() availability])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[extern "C" {
                                   #if HAVE_SYS_TIME_H
                                   #include <sys/time.h>
                                   #endif
                                   } // extern "C"
                                   ]],
                                   [[
                                      struct timeval tv[2];
                                      int lu = lutimes("/tmp/noway", tv);
                                   ]])
                  ],
                  [ AC_DEFINE(HAVE_LUTIMES, 1, [if lutimes() system call is available])
                    local_lutimes=yes
                    AC_MSG_RESULT(available)
                  ],
                  [ AC_MSG_RESULT([not available])]
                )

AC_MSG_CHECKING([for strerror_r flavor])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[extern "C"
                                     {
                                     #if HAVE_STRING_H
                                     #include <string.h>
                                     #endif
                                     } // externe "C"
                                   ]],
                                   [[
                                        char *ptr = strerror_r(0, 0, 0);
                                   ]])
                  ],
                  [ AC_DEFINE(HAVE_STRERROR_R_CHAR_PTR, 1, [strerror_r() returns a char *])
                    AC_MSG_RESULT([GNU specific])
                  ],
                  [
                    AC_MSG_RESULT([XSI compliant])
                  ])


# DAR's features

AC_ARG_ENABLE( [linux-statx],
               AC_HELP_STRING(--disable-linux-statx, [ignore linux's statx() system call and do not save birthtime of files under Linux because it is only possible to set and thus restore it under BSD systems like MACOS X, no under Linux today]),
               [explicit_linux_statx=yes],
               [enable_linux_statx=yes])

AS_IF(         [test "x$enable_linux_statx" != "xyes"],
               [
                 AC_MSG_WARN([Linux statx() system call not used if present])
                 local_statx="no"
               ],
               [
                 AC_MSG_CHECKING([for linux's statx() availability])
                 AC_RUN_IFELSE([AC_LANG_PROGRAM(
                                 [[extern "C" {
                                     #if HAVE_SYS_TYPES_H
                                     #include <sys/types.h>
                                     #endif
                                     #if HAVE_SYS_STAT_H
                                     #include <sys/stat.h>
                                     #endif
                                     #if HAVE_UNISTD_H
                                     #include <unistd.h>
                                     #endif
                                     #if HAVE_FCNTL_H
                                     #include <fcntl.h>
                                     #endif
                                 }]],
                                 [
                                   struct statx val;

                                   int ret = statx(0, "/", 0, STATX_BTIME, &val);
                                   if(ret != 0)
                                      return 1; /* will not use statx */
                                   else
                                      return 0; /* possible to go further with statx */
                                 ])
                              ],
                              [
                                 AC_DEFINE(HAVE_STATX_SYSCALL, 1, [system provides statx() system call, will be used to get inode birthtime on Linux])
                                 local_statx=yes
                              ],
                              [  local_statx=no ],
                              [

                              # if cross compiling

                                AC_LINK_IFELSE(
                                [AC_LANG_PROGRAM(
                                 [[extern "C" {
                                     #if HAVE_SYS_TYPES_H
                                     #include <sys/types.h>
                                     #endif
                                     #if HAVE_SYS_STAT_H
                                     #include <sys/stat.h>
                                     #endif
                                     #if HAVE_UNISTD_H
                                     #include <unistd.h>
                                     #endif
                                     #if HAVE_FCNTL_H
                                     #include <fcntl.h>
                                     #endif
                                 }]],
                                 [
                                   struct statx val;

                                   (void) statx(0, "/", 0, STATX_BTIME, &val);
                                 ])
                              ],
                              [
                                 AC_DEFINE(HAVE_STATX_SYSCALL, 1, [system provides statx() system call, will be used to get inode birthtime on Linux])
                                 local_statx=yes
                              ],
                              [  local_statx=no ])
                             ]
                             )
               ])


AC_ARG_ENABLE( [libz-linking],
               AS_HELP_STRING(--disable-libz-linking, [disable linking with libz and disable libz compression support]),
               [explicit_libz_linking=yes],
               [enable_libz_linking=yes])

AS_IF(       [test "x$enable_libz_linking" != "xyes"],
             [
                AC_MSG_WARN([libz compression support has been disabled by user])
                local_libz="no"
             ],
             [
               AC_CHECK_LIB(z, [deflate], [], [AC_MSG_WARN([library zlib not found])])
               AC_CHECK_HEADER(zlib.h, [local_libz="yes"
                                        AC_DEFINE(HAVE_ZLIB_H, 1, [zlib.h header file is available])
                                       ],
                                       [AC_MSG_WARN([Cannot find zlib.h header file])
                                        local_libz="no"
                                       ])
               if test "$local_libz" = "yes" ; then
               AC_LINK_IFELSE([AC_LANG_PROGRAM([[ extern "C" {
                                                           #if HAVE_ZLIB_H
                                                           #include <zlib.h>
                                                           #endif
                                              }]],
                                              [[
                                                         z_stream *ptr = (z_stream *)0;
                                                         deflate(ptr, 0);
                                              ]])
                              ],
                              [ AC_DEFINE(LIBZ_AVAILABLE, 1, [header and linking is available to have libz functions])],
                              [ local_libz="no" ])
               else
                 AC_MSG_WARN([libz compression support not available])
               fi

               AS_IF( [ test "x$explicit_libz_linking" = "xyes" -a "$local_libz" != "yes" ],
                      [  AC_MSG_ERROR([libz linking failed]) ]
                    )
             ]
     )

AC_ARG_ENABLE([libbz2-linking],
              AS_HELP_STRING(--disable-libbz2-linking, [disable linking with libbz2 and disables libbz2 compression support]),
              [explicit_libbz2_linking=yes],
              [enable_libbz2_linking=yes])

AS_IF(      [test "x$enable_libbz2_linking" != "xyes"],
            [
               AC_MSG_WARN([libbz2 compression support has been disabled by user])
               local_libbz2="no"
            ],
            [
               AC_CHECK_LIB(bz2, [BZ2_bzCompress], [], [AC_MSG_WARN([library libbz2 not found])])
               AC_CHECK_HEADER(bzlib.h, [local_libbz2="yes"
                                       AC_DEFINE(HAVE_BZLIB_H, 1, [bzlib.h header file is available])
                                      ],
                                      [AC_MSG_WARN([Cannot find bzlib.h header file])
                                       local_libbz2="no"
                                      ])
               if test "$local_libbz2" = "yes" ; then
                 AC_LINK_IFELSE([AC_LANG_PROGRAM([[ extern "C" {
                                                       #if HAVE_BZLIB_H
                                                       #include <bzlib.h>
                                                       #endif
                                              }]],
                                              [[
                                                        bz_stream *ptr = (bz_stream *)0;
                                                        BZ2_bzCompress(ptr, 0);
                                              ]])
                                ],
                                [ AC_DEFINE(LIBBZ2_AVAILABLE, 1, [header and linking is available to have libbz2 functions])],
                                [ local_libbz2="no" ])
               else
                  AC_MSG_WARN([libbz2 compression support not available])
               fi

               AS_IF( [ test "x$explicit_libbz2_linking" = "xyes" -a "$local_libbz2" != "yes" ],
                      [ AC_MSG_ERROR([libbz2 linking failed]) ]
                    )
           ]
     )

AC_ARG_ENABLE( [liblzo2-linking],
               AS_HELP_STRING(--disable-liblzo2-linking, [disable linking with liblzo2 and disables lzo compression support]),
               [explicit_liblzo2_linking=yes],
               [enable_liblzo2_linking=yes]
             )

AS_IF(      [test "x$enable_liblzo2_linking" != "xyes"],
            [
               AC_MSG_WARN([lzo compression support has been disabled by user])
               local_liblzo2="no"
            ],
            [
             AC_CHECK_LIB(lzo2, [lzo1x_1_compress], [], [AC_MSG_WARN([library liblzo2 not found])])
             AC_CHECK_HEADER(lzo/lzo1x.h, [local_liblzo2="yes"
                                       AC_DEFINE(HAVE_LZO_LZO1X_H, 1, [lzo/lzo1x.h header file is available])
                                      ],
                                      [AC_MSG_WARN([Cannot find lzo/lzo1x.h header file])
                                       local_liblzo2="no"
                                      ])
              if test "$local_liblzo2" = "yes" ; then
                 AC_LINK_IFELSE([AC_LANG_PROGRAM([[ extern "C" {
                                                       #if HAVE_LZO_LZO1X_H
                                                       #include <lzo/lzo1x.h>
                                                       #endif
                                              }]],
                                              [[
                                                        (void)lzo1x_1_compress(0, 0, 0, 0, 0);
                                              ]])
                                ],
                                [ AC_DEFINE(LIBLZO2_AVAILABLE, 1, [header and linking is available to have lzo functions])],
                                [ local_liblzo2="no" ])
             else
                 AC_MSG_WARN([lzo compression support not available])
             fi

             AS_IF( [ test "x$explicit_liblzo2_linking" = "xyes" -a "$local_liblzo2" != "yes" ],
                    [ AC_MSG_ERROR([liblzo linking failed]) ]
                  )
           ]
     )

AC_ARG_ENABLE( [libxz-linking],
               AS_HELP_STRING(--disable-libxz-linking, [disable linking with libxz/liblzma and disable xz compression support]),
               [explicit_libxz_linking=yes],
               [enable_libxz_linking=yes]
             )

AS_IF(       [test "x$enable_libxz_linking" != "xyes"],
               AC_MSG_WARN([libxz compression support has been disabled by user])
               local_libxz="no"
             ],
             [
               AC_CHECK_LIB(lzma, [lzma_code], [], [AC_MSG_WARN([library liblzma not found])])
               AC_CHECK_HEADER(lzma.h, [local_libxz="yes"
                                        AC_DEFINE(HAVE_LZMA_H, 1, [lzma.h header file is available])
                                       ],
                                       [AC_MSG_WARN([Cannot find lzma.h header file])
                                        local_libxz="no"
                                       ])
               if test "$local_libxz" = "yes" ; then
               AC_LINK_IFELSE([AC_LANG_PROGRAM([[ extern "C" {
                                                           #if HAVE_LZMA_H
                                                           #include <lzma.h>
                                                           #endif
                                              }]],
                                              [[
                                                         lzma_stream ptr = LZMA_STREAM_INIT;
                                                         lzma_ret tmp = lzma_easy_encoder(&ptr,
                                                                                  2,
                                                                                  LZMA_CHECK_CRC32);
                                              ]])
                              ],
                              [ AC_DEFINE(LIBLZMA_AVAILABLE, 1, [header and linking is available to have liblzma functions])],
                              [ local_libxz="no" ])
               else
                 AC_MSG_WARN([libxz compression support not available])
               fi

               AS_IF( [ test "x$explicit_libxz_linking" = "xyes" -a "$local_libxz" != "yes" ],
                      [ AC_MSG_ERROR([libxz linking failed]) ]
                    )
             ]
     )

AC_ARG_ENABLE( [libzstd-linking],
               AS_HELP_STRING(--disable-libzstd-linking, [disable linking with libzstd and disable zstd compression support]),
               [explicit_libzstd_linking=yes],
               [enable_libzstd_linking=yes]
             )

AS_IF(       [test "x$enable_libzstd_linking" != "xyes"],
             [
               AC_MSG_WARN([libzstd compression support has been disabled by user])
               local_libzstd="no"
             ],
             [
               AC_CHECK_LIB(zstd, [ZSTD_createCStream], [], [AC_MSG_WARN([library libzstd not found])])
               AC_CHECK_HEADER(zstd.h, [local_libzstd="yes"
                                        AC_DEFINE(HAVE_ZSTD_H, 1, [zstd.h header file is available])
                                       ],
                                       [AC_MSG_WARN([Cannot find zstd.h header file])
                                        local_libzstd="no"
                                       ])
               if test "$local_libzstd" = "yes" ; then
               min_maj_version_zstd=1
               min_min_version_zstd=3
               AC_DEFINE_UNQUOTED(MIN_MAJ_VERSION_ZSTD, "$min_maj_version_zstd", [libzstd minimum major version])
               AC_DEFINE_UNQUOTED(MIN_MIN_VERSION_ZSTD, "$min_min_version_zstd", [libzstd minimum minor version])
               AC_RUN_IFELSE([AC_LANG_PROGRAM([[ extern "C" {
                                                           #if HAVE_ZSTD_H
                                                           #include <zstd.h>
                                                           #endif
                                                           #if HAVE_STDLIB_H
                                                           #include <stdlib.h>
                                                           #endif
                                              }]],
                                              [[
                                                        unsigned int min_version = atoi(MIN_MAJ_VERSION_ZSTD)*100*100 + atoi(MIN_MIN_VERSION_ZSTD)*100 + 0;
                                                        if(ZSTD_versionNumber() < min_version)
                                                           return 1;

                                                        ZSTD_CStream *zs = ZSTD_createCStream();
                                                        (void)ZSTD_freeCStream(zs);
                                                        return 0;
                                              ]])
                              ],
                              [ AC_DEFINE(LIBZSTD_AVAILABLE, 1, [header and linking is available to have libzstd functions])],
                              [ local_libzstd="no" ],
                              [
                                 # if cross compiling

                                 AC_LINK_IFELSE([AC_LANG_PROGRAM(
                                                        [[ extern "C" {
                                                           #if HAVE_ZSTD_H
                                                           #include <zstd.h>
                                                           #endif
                                                           #if HAVE_STDLIB_H
                                                           #include <stdlib.h>
                                                           #endif
                                                        }]],
                                                        [[
                                                        ZSTD_CStream *zs = ZSTD_createCStream();
                                                        (void)ZSTD_versionNumber();
                                                        (void)ZSTD_freeCStream(zs);
                                                        return 0;
                                                        ]])
                                                ],
                                                [ AC_DEFINE(LIBZSTD_AVAILABLE, 1, [header and linking is available to have libzstd functions])],
                                                [ local_libzstd="no" ]
                                        )
                              ]
                              )
               else
                 AC_MSG_WARN([libzstd compression support not available])
               fi

               AS_IF( [ test "x$explicit_libzstd_linking" = "xyes" -a "$local_libzstd" != "yes" ],
                      [ AC_MSG_ERROR([libzstd linking failed]) ]
                    )
             ]
     )

AC_ARG_ENABLE( [liblz4-linking],
               AC_HELP_STRING(--disable-liblz4-linking, [disable linking with liblz4 and disable lz4 compression support]),
               [explicit_liblz4_linking=yes],
               [enable_liblz4_linking=yes]
             )


AS_IF(       [ test "x$enable_liblz4_linking" != "xyes" ],
             [
               AC_MSG_WARN([liblz4 compression support has been disabled by user])
               local_liblz4="no"
             ],
             [
                AC_CHECK_LIB(lz4, [LZ4_decompress_safe], [], [AC_MSG_WARN([library liblz4 not found])])
                AC_CHECK_HEADER(lz4.h, [
                                        local_liblz4="yes"
                                        AC_DEFINE(HAVE_LZ4_H, 1, [lz4.h header file is available])
                                       ],
                                       [
                                        AC_MSG_WARN([Cannot find lz4.h header file])
                                        local_liblz4="no"
                                       ])

                if test "$local_liblz4" = "yes" ; then
                   AC_RUN_IFELSE([AC_LANG_PROGRAM([[ extern "C"
                                                     {
                                                        #if HAVE_LZ4_H
                                                        #include <lz4.h>
                                                        #endif
                                                        #if HAVE_STRING_H
                                                        #include <string.h>
                                                        #endif
                                                        #if HAVE_STDLIB_H
                                                        #include <stdlib.h>
                                                        #endif
                                                        #if HAVE_STDIO_H
                                                        #include <stdio.h>
                                                        #endif
                                                     }
                                                  ]],
                                                  [[
                                                        const char *src = "Quand on lui marche sur les pieds, le serpent hausse les epaules.";
                                                        unsigned int src_sz = strlen(src);
                                                        unsigned int zip_sz = LZ4_compressBound(src_sz);
                                                        char *zip = (char *)malloc(zip_sz);
                                                        char *unzip = (char *)malloc(src_sz + 1);

                                                        if(zip == 0 || unzip == 0)
                                                            return 1;  /* allocation problem */

                                                        zip_sz = LZ4_compress_default(src, zip, src_sz, zip_sz);

                                                        if(zip_sz <= 0)
                                                            return 1; /* compression problem */

                                                        if(LZ4_decompress_safe(zip, unzip, zip_sz, src_sz) != src_sz)
                                                            return 1; /* decompression problem */

                                                        unzip[src_sz] = '\0';
                                                        if(strncmp(src, unzip, src_sz) != 0)
                                                            return 1; /* decompression result does not match the original data */

                                                        free(zip);
                                                        free(unzip);

                                                        return 0;
                                                ]])
                                ],
                                [ AC_DEFINE(LIBLZ4_AVAILABLE, 1, [headar and linking is available to have liblz4 fonctions]) ],
                                [ local_liblz4="no" ],
                                [
                                    #if cross compiling

                                    AC_LINK_IFELSE([AC_LANG_PROGRAM(
                                                        [[ extern "C"
                                                        {
                                                        #if HAVE_LZ4_H
                                                        #include <lz4.h>
                                                        #endif
                                                        #if HAVE_STRING_H
                                                        #include <string.h>
                                                        #endif
                                                        #if HAVE_STDLIB_H
                                                        #include <stdlib.h>
                                                        #endif
                                                        #if HAVE_STDIO_H
                                                        #include <stdio.h>
                                                        #endif
                                                        }
                                                        ]],
                                                        [[
                                                        /* the following is not expected to be executed just compiled and linked */
                                                        const char *src = "Quand on lui marche sur les pieds, le serpent hausse les epaules.";
                                                        unsigned int src_sz = strlen(src);
                                                        unsigned int zip_sz = LZ4_compressBound(src_sz);
                                                        char *zip = (char *)malloc(zip_sz);
                                                        char *unzip = (char *)malloc(src_sz + 1);
                                                        zip_sz = LZ4_compress_default(src, zip, src_sz, zip_sz);
                                                        (void)LZ4_decompress_safe(zip, unzip, zip_sz, src_sz);
                                                        return 0;
                                                        ]])
                                                ],
                                                [ AC_DEFINE(LIBLZ4_AVAILABLE, 1, [headar and linking is available to have liblz4 fonctions]) ],
                                                [ local_liblz4="no" ])
                                ]
                        )

                else
                   AC_MSG_WARN([liblz4 compression support not available])
                fi

                AS_IF( [ test "x$explicit_liblz4_linking" = "xyes" -a "$local_liblz4" != "yes" ],
                       [ AC_MSG_ERROR([liblz4 linking failed]) ]
                     )
             ]
     )

AC_ARG_ENABLE( [libgcrypt-linking],
               AS_HELP_STRING(--disable-libgcrypt-linking, [disable linking with libgcrypt which disables strong encryption support]),
               [explicit_libgcrypt_linking=yes],
               [enable_libgcrypt_linking=yes])

AS_IF(      [test "x$enable_libgcrypt_linking" != "xyes"],
            [
               AC_MSG_WARN([strong encryption support has been disabled by user])
               local_crypto="no"
            ],
            [
              AC_CHECK_LIB(gpg-error, [gpg_err_init], [], [])
              AC_CHECK_LIB(gcrypt, [gcry_check_version], [], [])
              AC_CHECK_HEADER(gcrypt.h, [local_crypto="yes"
                                         AC_DEFINE(HAVE_GCRYPT_H, 1, [gcrypt.h header file is available])
                                        ],
                                        [AC_MSG_WARN([Cannt find gcrypt.h header file])
                                         local_crypto="no"
                                        ])
              if test "$local_crypto" = "yes" ; then
                 min_version_gcrypt="1.4.0"
                 AC_DEFINE_UNQUOTED(MIN_VERSION_GCRYPT, "$min_version_gcrypt", [libgcrypt minimum version])
                 min_version_gcrypt_hash_bug="1.6.0"
                 AC_DEFINE_UNQUOTED(MIN_VERSION_GCRYPT_HASH_BUG, "$min_version_gcrypt_hash_bug", [ligcrypt minimum version without hash bug])
                 AC_MSG_CHECKING([for libgcrypt usability])
                 AC_RUN_IFELSE([AC_LANG_PROGRAM([[ extern "C"
                                                  {
                                                       #if HAVE_GCRYPT_H
                                                       #include <gcrypt.h>
                                                       #endif
                                                  }
                                                  #include <iostream>
                                                  using namespace std;
                                              ]],
                                              [[
                                                 if(!gcry_check_version(MIN_VERSION_GCRYPT))
                                                 {
                                                   cout << "ligcrypt version too low, minimum version is " << MIN_VERSION_GCRYPT << endl;
                                                   exit(1);
                                                 }
                                                 else
                                                   exit(0);
                                              ]])
                                ],
                                [ AC_DEFINE(CRYPTO_AVAILABLE, 1, [header and linking is available to have strong encryption works])
                                  AC_MSG_RESULT([ok])
                                  AC_RUN_IFELSE([AC_LANG_PROGRAM([[ extern "C"
                                                                    {
                                                                     #if HAVE_GCRYPT_H
                                                                     #include <gcrypt.h>
                                                                     #endif
                                                                    }
                                                                 ]],
                                                                 [[
                                                                     if(!gcry_check_version(MIN_VERSION_GCRYPT_HASH_BUG))
                                                                       exit(1);
                                                                     else
                                                                       exit(0);
                                                                 ]])
                                                ],
                                                [],
                                                [ libgcrypt_hash_bug="yes" ])
                                ],
                                [
                                  if test "$?" = "1" ; then
                                    AC_MSG_RESULT([failed: need libgcypt >= $min_version_gcrypt, disabling strong encryption support])
                                  else
                                    AC_MSG_RESULT([failed: libgcrypt is unusable, cannot even call gcry_check_version(). Disabling strong encryption support])
                                  fi
                                  local_crypto="no"
                                ],
                                [
                                  # if cross compiling

                                   AC_LINK_IFELSE([AC_LANG_PROGRAM(
                                                   [[ extern "C"
                                                    {
                                                       #if HAVE_GCRYPT_H
                                                       #include <gcrypt.h>
                                                       #endif
                                                    }
                                                    #include <iostream>
                                                    using namespace std;
                                                   ]],
                                                   [[
                                                     (void)gcry_check_version(MIN_VERSION_GCRYPT);
                                                   ]])
                                                  ],
                                                  [ AC_DEFINE(CRYPTO_AVAILABLE, 1, [header and linking is available to have strong encryption works])
                                                    AC_MSG_RESULT([ok])
                                                    # libgcrypt_hash_bug="yes"
                                                    # we assume with time version with that bug will vanish
                                                  ],
                                                  [])
                                ])
             else
                AC_MSG_WARN([strong encryption support not available])
             fi

             AS_IF( [ test "x$explicit_libgcrypt_linking" = "xyes" -a "$local_crypto" != "yes" ],
                    [ AC_MSG_ERROR([ligcrypt linking failed]) ]
                  )
           ]
     )


AC_ARG_ENABLE(  [ea-support],
                AS_HELP_STRING(--disable-ea-support,[disable Extended Attributes support]),
                [explicit_ea_support=yes],
                [enable_ea_support=yes])

AS_IF(          [test "x$enable_ea_support" != "xyes"],
                [
                  AC_MSG_CHECKING([for Extended Attribute support])
                  AC_MSG_RESULT([disabled])
                  local_ea_support="no"
                ],
                [ AC_CHECK_HEADERS([attr/xattr.h sys/xattr.h])
                  AC_CHECK_LIB(attr, [lgetxattr], [], [])
                  AC_MSG_CHECKING([for Unix Extended Attribute support])
                  AC_LINK_IFELSE([AC_LANG_PROGRAM([[extern "C" {
                                                     #if HAVE_SYS_TYPES_H
                                                     #include <sys/types.h>
                                                     #endif
                                                     #if HAVE_ATTR_XATTR_H && ! HAVE_SYS_XATTR_H
                                                     #include <attr/xattr.h>
                                                     #endif
                                                     #if HAVE_SYS_XATTR_H
                                                     #include <sys/xattr.h>
                                                     #endif
                                                    }]],
                                                    [ lgetxattr((char *)0, (char *)0, (void *)0, 0); ])
                                 ],
                                 [
                                        AC_DEFINE(EA_SUPPORT, [], [if defined, activates support for Extended Attributes])
                                        local_ea_support="yes"
                                        AC_MSG_RESULT([yes])
                                 ],
                                 [
                                        AC_MSG_RESULT([no])
                                        AC_CHECK_HEADERS([sys/xattr.h])
                                        AC_CHECK_LIB(c, [fgetxattr])
                                        AC_MSG_CHECKING([for Mac OS X Extended Attribute support])
                                        AC_LINK_IFELSE([AC_LANG_PROGRAM([[extern "C" {
                                                                         #if HAVE_SYS_XATTR_H
                                                                         #include <sys/xattr.h>
                                                                         #endif
                                                                       }]],
                                                                       [ getxattr((char *)0, (char *)0, (void *)0, 0, 0, XATTR_NOFOLLOW); ])
                                                       ],
                                                       [
                                                          AC_DEFINE(EA_SUPPORT, [], [if defined, activates support for Extended Attributes])
                                                          AC_DEFINE(OSX_EA_SUPPORT, [], [if defined, activates support for Mac OS X Extended Attributes])
                                                          local_ea_support="yes"
                                                          AC_MSG_RESULT([yes])
                                                       ],
                                                       [
                                                          local_ea_support="no"
                                                          AC_MSG_RESULT([no])
                                                       ])
                                 ])
                   AS_IF( [ test "x$explicit_ea_support" = "xyes" -a "$local_ea_support" != "yes" ],
                          [ AC_MSG_ERROR([Failed finding Extended Attribute support]) ]
                        )
                 ]
     )

AC_MSG_CHECKING([ext2fs.h availability])
AC_ARG_ENABLE(  [nodump-flag],
                AS_HELP_STRING(--disable-nodump-flag, [disable the ext2/3/4 Filesystem Specific Attribute support, in particular the --nodump feature]),
                [explicit_nodump_flag=yes],
                [enable_nodump_flag=yes])

AS_IF(          [test "x$enable_nodump_flag" != "xyes"],
                [AC_MSG_RESULT([extX FSA disabled])],
                [AC_LINK_IFELSE([AC_LANG_PROGRAM([[extern "C" {
                                                    #include <ext2fs/ext2_fs.h>
                                                    #if HAVE_SYS_IOCTL_H
                                                    #include <sys/ioctl.h>
                                                    #endif
                                                    }]],[[int fd, f; ioctl(fd, EXT2_IOC_GETFLAGS, &f);]]
                                                )
                                ],
                                [
                                AC_DEFINE(LIBDAR_NODUMP_FEATURE, [NODUMP_EXT2FS], [if defined, activates the ext2/3 nodump flag feature])
                                local_nodump_feature="yes"
                                AC_MSG_RESULT([found <ext2fs/ext2_fs.h>])
                                ],
                                [
                                AC_LINK_IFELSE(  [AC_LANG_PROGRAM([[extern "C" {
                                                                    #include <linux/ext2_fs.h>
                                                                    #if HAVE_SYS_IOCTL_H
                                                                    #include <sys/ioctl.h>
                                                                    #endif
                                                                    }]],[[int fd, f; ioctl(fd, EXT2_IOC_GETFLAGS, &f);]])
                                                 ],
                                                 [
                                                   AC_DEFINE(LIBDAR_NODUMP_FEATURE, [NODUMP_LINUX], [if defined, activates the ext2/3 nodump flag feature])
                                                   local_nodump_feature="yes"
                                                   AC_MSG_RESULT([found <linux/ext2_fs.h>])
                                                 ],
                                                 [
                                                   AC_MSG_RESULT([NOT FOUND])
                                                   local_nodump_feature="no"
                                                   AC_MSG_WARN([cannot find ext2_fs.h header file, nodump-flag and extX FSA features will not be available])
                                                 ])
                                ])
                  AS_IF( [ test "x$explicit_nodump_flag" = "xyes" -a "$local_nodump_feature" != "yes" ],
                         [ AC_MSG_ERROR([Failed activating nodump feature]) ]
                       )
                ]
     )

AC_MSG_CHECKING([birth time availability])
AC_ARG_ENABLE( [birthtime],
               AS_HELP_STRING(--disable-birthtime, [disable the HFS+ Filesystem Specific Attribute support]),
               [explicit_birthtime=yes],
               [enable_birthtime=yes])

AS_IF(         [test "x$enable_birthtime" != "xyes"],
               [AC_MSG_RESULT([hfs+ FSA disabled])],
               [AC_LINK_IFELSE([AC_LANG_PROGRAM([[
                                                extern "C" {
                                                #if HAVE_SYS_TYPE_H
                                                #include <sys/types.h>
                                                #endif
                                                #if HAVE_SYS_STAT_H
                                                #include <sys/stat.h>
                                                #endif
                                                #if HAVE_UNISTD_H
                                                #include <unistd.h>
                                                #endif
                                                }]],
                                                [[
                                                struct stat tmp;
                                                int ret = stat("/", &tmp);
                                                time_t birth = tmp.st_birthtime;
                                                ]])
                                ],
                                [
                                  AC_DEFINE(LIBDAR_BIRTHTIME, 1, [if defined, activates the support for HFS+ create time FSA])
                                  local_birthtime="yes"
                                  AC_MSG_RESULT([found])
                                ],
                                [
                                  AC_MSG_RESULT([NOT FOUND])
                                  AC_MSG_WARN([Cannot find support for birthtime, HFS+ FSA support will not be available])
                                ])
                  AS_IF( [ test "x$explicit_birthtime" = "xyes" -a "$local_birthtime" != "yes" ],
                         [ AC_MSG_ERROR([birth time support not available]) ]
                       )
                ]
     )

AC_ARG_ENABLE(  [gnugetopt],
                AS_HELP_STRING(--disable-gnugetopt, [avoid linking with libgnugetopt]),
                [explicit_gnugetopt=yes],
                [enable_gnugetopt=yes])

AS_IF(          [test "x$enable_gnugetopt" != "xyes"],
                [],
                AC_CHECK_LIB(gnugetopt,
                             [getopt_long],
                             [],
                             [
                               AS_IF( [ test "x$explicit_gnugetopt" = "xyes"],
                                      [ AC_MSG_ERROR([gnugetopt linking failed]) ]
                                    )
                             ])
     )

AC_ARG_ENABLE( [librsync-linking],
               AS_HELP_STRING(--disable-librsync-linking, [disable linking with librsync and disable delta compression support]),
               [explicit_librsync_linking=yes],
               [enable_librsync_linking=yes])

AS_IF(        [test "x$enable_librsync_linking" != "xyes"],
              [
                 AC_MSG_WARN([librsync delta-compression support has been disabled by user])
                 local_librsync="no"
              ],
              [
               AC_CHECK_LIB(rsync, [rs_strerror], [], [AC_MSG_WARN([librsync library not found])])
               AC_CHECK_HEADER(librsync.h, [local_librsync="yes"
                                        AC_DEFINE(HAVE_LIBRSYNC_H, 1, [librsync.h header file is available])
                                       ],
                                       [AC_MSG_WARN([Cannot find librsync.h header file])
                                         local_librsync="no"
                                        ])
                if test "$local_librsync" = "yes" ; then
                AC_LINK_IFELSE([AC_LANG_PROGRAM([[ extern "C" {
                                                            #if HAVE_LIBRSYNC_H
                                                            #include <stdio.h>
                                                            #include <librsync.h>
                                                            #endif
                                               }]],
                                               [[
                                                           rs_result err = RS_DONE;
                                                           (void) rs_strerror(err);
                                               ]])
                              ],
                               [ AC_DEFINE(LIBRSYNC_AVAILABLE, 1, [librsync is usable])],
                               [ local_librsync="no" ])
                else
                  AC_MSG_WARN([librsync compression support not available])
                fi

                AS_IF( [ test "x$explicit_librsync_linking" = "xyes" -a "$local_librsync" != "yes" ],
                       [ AC_MSG_ERROR([librsync linking failed]) ]
                     )
              ]
     )

AC_ARG_ENABLE( [libcurl-linking],
               AS_HELP_STRING(--disable-libcurl-linking, [ignore libcurl and avoid linking against it]),
               [explicit_libcurl_linking=yes],
               [enable_libcurl_linking=yes])

AS_IF(         [test "x$enable_libcurl_linking" != "xyes"],
               [
                 AC_MSG_WARN([libcurl and thus remote repository support has been disabled by user])
                 local_libcurl="no"
               ],
               [
                  PKG_CHECK_EXISTS(libcurl,
                        [ PKG_CHECK_MODULES(LIBCURL, libcurl, [], [AC_MSG_ERROR([libcurl not found, but reported to exist !?!])])
                          AC_DEFINE(HAVE_LIBCURL, 1, [Libcurl library availability])
                        ],
                        [
                          AC_CHECK_LIB(curl, [curl_global_init], [], [AC_MSG_WARN([libcurl library not found])])
                          AC_DEFINE(HAVE_LIBCURL, 1, [Libcurl library availability])
                        ])

                  CPPFLAGS__cache="$CPPFLAGS"
                  CPPFLAGS="$LIBCURL_CFLAGS $CPPFLAGS"
                  LIBS___cache="$LIBS"
                  LIBS="$LIBCURL_LIBS $LIBS"
                  AC_CHECK_HEADER(curl/curl.h,
                        [
                                local_libcurl="yes"
                                AC_DEFINE(HAVE_CURL_CURL_H, 1, [libcurl.h header file is available])
                        ],
                        [
                                AC_MSG_WARN([Cannot find libcurl.h header file])
                                local_libcurl="no"
                        ])

                 if test "$local_libcurl" = "yes" ; then
                        AC_LINK_IFELSE([AC_LANG_PROGRAM([[ extern "C" {
                                                                  #if HAVE_CURL_CURL_H
                                                                  #include <curl/curl.h>
                                                                  #endif
                                                           }
                                                        ]],
                                                        [[
                                                            (void) curl_global_init(CURL_GLOBAL_ALL);
                                                        ]])
                                        ],
                                        [AC_DEFINE(LIBCURL_AVAILABLE, 1, [libcurl is usable])],
                                        [ local_libcurl="no" ])
                 else
                        AC_MSG_WARN([remote repository support not available])
                 fi
                 CPPFLAGS="$CPPFLAGS___cache"
                 LIBS="$LIBS___cache"
                 unset CPPFLAGS___cache
                 unset LIBS___cache
                 AS_IF( [ test "x$explicit_libcurl_linking" = "xyes" -a "$local_libcurl" != "yes" ],
                        [ AC_MSG_ERROR([libcurl linking failed]) ]
                      )
                ]
     )

AC_ARG_ENABLE(  [fadvise],
                AS_HELP_STRING(--disable-fadvise, [avoid using fadvise(2) system call]),
                [explicit_fadvise=yes],
                [enable_fadvise=yes])

AS_IF(          [test "x$enable_fadvise" != "xyes"],
                [ AC_MSG_WARN([avoiding the use of fadvise(2) system call, per user request])],
                [
                  AC_MSG_CHECKING([for posix_fadvise support])
                  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[extern "C" {
                                                       #if HAVE_FCNTL_H
                                                       #include <fcntl.h>
                                                       #endif
                                                       }]],
                                                       [ (void)posix_fadvise(0,0,1,POSIX_FADV_NORMAL); ])
                                    ],
                                    [
                                      AC_DEFINE(HAVE_POSIX_FADVISE, 1, [whether the system has support for posix_fadvise()])
                                      local_posix_fadvise="yes"
                                      AC_MSG_RESULT(available)
                                    ],
                                    [
                                      AC_MSG_RESULT([not available])
                                      AS_IF( [ test "x$explicit_fadvise" = "xyes" ],
                                             [ AC_MSG_ERROR([fadvise not available]) ]
                                           )
                                    ]
                                   )
                ]
     )

AC_MSG_CHECKING([for getopt() availability]);
AC_LINK_IFELSE([AC_LANG_PROGRAM([[extern "C" {
                                   #if HAVE_STDIO_H
                                   #include <stdio.h>
                                   #endif
                                   #if HAVE_GETOPT_H
                                   #include <getopt.h>
                                   #else
                                   #if HAVE_UNISTD_H
                                   #include <unistd.h>
                                   #endif
                                   #endif
                                   }]], [ getopt(0, 0, 0); ])
                  ],
                  [
                    AC_MSG_RESULT([ok])
                  ],
                  [AC_MSG_ERROR([absent but required])])

AC_MSG_CHECKING([for getopt_long() availability]);
AC_LINK_IFELSE([AC_LANG_PROGRAM([[extern "C" {
                                   #if HAVE_STDIO_H
                                   #include <stdio.h>
                                   #endif
                                   #if HAVE_GETOPT_H
                                   #include <getopt.h>
                                   #else
                                   #if HAVE_UNISTD_H
                                   #include <unistd.h>
                                   #endif
                                   #endif
                                   }]], [ getopt_long(0, 0, 0, 0, 0); ])
                  ],
                  [
                    local_have_getopt_long="yes"
                    AC_DEFINE(HAVE_GETOPT_LONG, 1, [whether getopt_long() is available])
                    AC_MSG_RESULT([available])
                  ],
                  [AC_MSG_RESULT([NOT AVAILABLE])])


AC_ARG_ENABLE(  [examples],
                AS_HELP_STRING(--enable-examples, [buld example and testing programs]),
                [
                   AS_IF([ test "x$enable_examples" != "xyes" ],
                         [ AC_MSG_ERROR([invalid argument given to --enable-examples]) ]
                        )
                   examples="yes"
                ],
                [examples="false"])

AC_ARG_ENABLE(  [os-bits],
                AS_HELP_STRING(--enable-os-bits=arg, [arg is 32 or 64. If for some reason, one wants to overcome detected system value]),
                [AC_DEFINE_UNQUOTED(OS_BITS, $enableval, [manually set CPU's registers' size])],
                [
                        AC_CHECK_HEADER(inttypes.h,
                                [
                                  AC_COMPILE_IFELSE(
                                        [AC_LANG_PROGRAM([extern "C" {
                                                          #include <inttypes.h>
                                                          }],
                                                         [ uint16_t a  = 0; uint32_t b = 0; uint64_t c = 0;
                                                           int16_t d = 0; int32_t e = 0; int64_t f = 0;
                                                           return a+b+c+d+e+f;
                                                         ])
                                        ],
                                        [ AC_DEFINE(HAVE_INTTYPES_H, 1, inttypes.h header availability) ],
                                        [ AC_MSG_ERROR([Cannot find *int*_t type declarations in headerfile, --enable-os-bits=... option must be used]) ])
                                ],
                                [ AC_MSG_ERROR([Cannot find inttypes.h headerfile, --enable-os-bits=... option must be used]) ]
                        )
                ]
             )

AC_ARG_ENABLE( [mode],
                AS_HELP_STRING(--enable-mode=arg, [where arg is either 32 or infinint. Makes dar internally use 32 bits or limitless integers in place of 64 bits integers (which is the case if this option is not given)]),
                [
                   build_mode=$enableval
                   if test "$build_mode" != "32" -a "$build_mode" != "64" -a "$build_mode" != "infinint" ; then
                      AC_MSG_ERROR([Invalid argument given to --enable-mode option])
                   fi
                ],
                [build_mode=64])

AC_ARG_ENABLE( [furtive-read],
               AS_HELP_STRING(--disable-furtive-read,
                              [Ignore furtive read mode availability on systems that support it]),
               [explicit_furtive_read=yes],
               [enable_furtive_read=yes])

AS_IF(         [test "x$enable_furtive_read" != "xyes"],
               [
                 local_furtive_read_mode="no"
                 AC_MSG_WARN([Furtive read mode disabled])
               ],
               [ AC_MSG_CHECKING([furtive read mode availability])
                 AC_LINK_IFELSE([AC_LANG_PROGRAM([[ extern "C" {
                                                 #if HAVE_SYS_TYPE_H
                                                 #include <sys/types.h>
                                                 #endif
                                                 #if HAVE_SYS_STAT_H
                                                 #include <sys/stat.h>
                                                 #endif
                                                 #if HAVE_FCNTL_H
                                                 #include <fcntl.h>
                                                 #endif
                                                 #if HAVE_DIRENT_H
                                                 #include <dirent.h>
                                                 #endif
                                                 }
                                                 ]],
                                                 [[
                                                 int x = O_NOATIME;
                                                 int fd = open("/",O_RDONLY|O_NOATIME);
                                                 #if HAVE_FDOPENDIR
                                                 (void)fdopendir(fd);
                                                 #else
                                                 syntaxically incorrect statement here to force compilation to fail!
                                                 #endif
                                                 ]])
                                ],
                                [ AC_DEFINE(FURTIVE_READ_MODE_AVAILABLE, 1, [furtive read mode is available])
                                  AC_MSG_RESULT(available)
                                  local_furtive_read_mode="yes"
                                ],
                                [ AC_MSG_RESULT(no available)
                                  local_furtive_read_mode="no"
                                ])
                 AS_IF(  [ test "x$explicit_furtive_read" = "xyes" -a "$local_furtive_read_mode" != "yes" ],
                         [ AC_MSG_ERROR([furtive read mode not available]) ]
                      )
               ]
     )


AC_ARG_ENABLE( [debug],
                AS_HELP_STRING(--enable-debug, [build targets with debugging option and no optimization]),
                [
                        AS_IF([ test "x$enable_debug" != "xyes" ],
                              [ AC_MSG_ERROR([invalid argument given to --enable-debug]) ]
                             )

                        CXXFLAGS="-g -Wall"
                        CFLAGS="-g -Wall"
                        LDFLAGS="-g -Wall"
                        debug_static="yes"
                        AC_DEFINE(LIBDAR_NO_OPTIMIZATION, 1, [if defined, informs the code that no optimization has been used for compilation])
                ],
                [
                        debug_static="no"
                ])


AC_ARG_ENABLE( [pedantic],
               AS_HELP_STRING(--enable-pedantic, [enable pedantic syntaxical check at compilation, use only for debugging purposes !]),
               [
                        AS_IF([ test "x$enable_pedantic" != "xyes" ],
                              [ AC_MSG_ERROR([invalid argument given to --enable-pedantic]) ]
                             )

                        CXXFLAGS="$CXXFLAGS -pedantic -Wno-long-long"
               ],
               [])


AC_ARG_ENABLE( [build-html],
               AS_HELP_STRING(--disable-build-html, [don't build programming documentation (in particular libdar API documentation) and html man page]),
               [explicit_build_html=yes],
               [enable_build_html=yes])

AS_IF(         [ test "x$enable_build_html" != "xyes"],
               [
                 AC_MSG_WARN([documentation no built per user request])
                 doxygen="no"
                 groff="no"
               ],
               [ AC_CHECK_PROG(doxygen, doxygen, [yes], [no], [$PATH])
                 AC_MSG_CHECKING([for doxygen version])
                   if test "$doxygen" = "yes" ; then
                       n1=`doxygen --version | cut -d '.' -f 1`
                       n2=`doxygen --version | cut -d '.' -f 2`
                       if test $n1 -gt 1 -o $n2 -ge 3 ; then
                           AC_MSG_RESULT([ >= 1.3])
                       else
                           AC_MSG_RESULT([ too old (< 1.3) ignoring doxygen])
                           doxygen="no"
                       fi
                   fi
                 AC_CHECK_PROG(dot, dot, [YES], [NO], [$PATH])
                 # upper case value for dot variable because it goes as is into doxyfile file

                 AC_CHECK_PROG(tmp, man, [yes], [no], [$PATH])
                 if test "$tmp" = "yes" ; then
                    AC_CHECK_PROG(groff, groff, [yes], [no], [$PATH])
                 else
                    groff = "no";
                 fi
                 AS_IF( [ test "x$explicit_build_html" = "xyes" -a \( "$doxygen" != "yes" -o "$groff" != "yes" \) ],
                        [ AC_MSG_ERROR([lacking prerequisit to build documentation]) ]
                      )
               ]
     )

AC_ARG_ENABLE( [upx],
                AS_HELP_STRING(--disable-upx, [by default configure looks for UPX and if available make executables compressed at installation time, you can disable this feature]),
                [explicit_upx=yes],
                [enable_upx=yes])

AS_IF(          [ test "x$enable_upx" != "xyes" ],
                [
                   AC_MSG_NOTICE([ignoring UPX])
                   upx="no"
                ],
                [
                   AC_CHECK_PROG(upx, upx, [yes], [no], [$PATH])
                   AS_IF( [ test "x$explicit_upx" = "xyes" -a "$upx" != "yes" ],
                          [ AC_MSG_ERROR([upx is missing]) ]
                        )
                ]
             )

AC_ARG_ENABLE( [fast-dir],
               AS_HELP_STRING(--disable-fast-dir, [disable optimization for large directories, doing so has a little positive impact on memory requirement but a huge drawback on execution time]),
               [
                    AS_IF([ test "x$enable_fast_dir" != "xno" ],
                          [ AC_MSG_ERROR([invalid argument given to --disable-fast_dir]) ]
                         )
               ],
               [AC_DEFINE(LIBDAR_FAST_DIR, 1, [activation of speed optimization for large directories])
                local_fast_dir="yes"
               ]
             )


AC_ARG_ENABLE(  [gpgme-linking],
                AS_HELP_STRING(--disable-gpgme-linking, [disable linking with gpgme which disables asymetric crypto algorithms]),
                [explicit_gpgme_linking=yes],
                [enable_gpgme_linking=yes])

AS_IF(          [ test "x$enable_gpgme_linking" != "xyes" ],
                [
                  AC_MSG_WARN([asymetrical encryption support has been disabled by user])
                  local_gpgme="no"
                ],
                [
                  if test $local_crypto != no ; then
                    gpgme_min_version="1.2.0"
                    AC_DEFINE_UNQUOTED(GPGME_MIN_VERSION, "$gpgme_min_version", [minimum version expected of GPGME])
                    m4_ifdef([AM_PATH_GPGME],
                        [ AM_PATH_GPGME($gpgme_min_version,
                                [
                                  CPPFLAGS___cache="$CPPFLAGS"
                                  CPPFLAGS="$GPGME_CFLAGS $CPPFLAGS"
                                  LIBS___cache="$LIBS"
                                  LIBS="$GPGME_LIBS $LIBS"
                                  AC_CHECK_HEADERS([gpgme.h])
                                  AC_CHECK_LIB(gpgme, [gpgme_signers_add], [], [])
                                  AC_MSG_CHECKING([for libgpgme usability])
                                  AC_LINK_IFELSE([AC_LANG_PROGRAM([[
                                                                    #if HAVE_GPGME_H
                                                                    #include <gpgme.h>
                                                                    #endif
                                                                 ]],
                                                                 [[
                                                                    gpgme_ctx_t context;
                                                                    gpgme_error_t err = gpgme_new(&context);
                                                                    gpgme_release(context);
                                                                    return err;
                                                                 ]])
                                                 ],
                                                 [
                                                   local_gpgme="yes"
                                                   AC_DEFINE(GPGME_SUPPORT, 1, [GPGME is available to support public key based ciphering])
                                                   AC_MSG_RESULT(ok)
                                                 ],
                                                 [
                                                   local_gpgme="no"
                                                   AC_MSG_RESULT([not usable! See config.log for details])
                                                 ])
                                  CPPFLAGS="$CPPFLAGS___cache"
                                  unset CPPFLAGS___cache
                                  LIBS="$LIBS___cache"
                                  unset LIBS___cache
                                ],
                                [ AC_MSG_WARN([Public key support (GPGME linking) requires version greater than $gpgme_min_version]) ]
                               )
                        ],
                        [AC_MSG_WARN([AM_PATH_GPGME macro not found!])])
                 else
                    AC_MSG_WARN([Public key support (GPGME linking) requires libgcrypt (strong encryption support)])
                 fi

                 AS_IF( [ test "x$explicit_gpgme_linking" = "xyes" -a "$local_gpgme" != "yes" ],
                        [ AC_MSG_ERROR([gpgme linking failed]) ]
                      )
                ]
     )

AC_ARG_ENABLE(  [thread-safe],
                AS_HELP_STRING(--disable-thread-safe, [libdar is thread safe if POSIX mutex are available, you can manually disable the use of POSIX mutex, the resulting libdar library will not be thread-safe anymore]),
                [explicit_thread_safe=yes],
                [enable_thread_safe=yes])

AS_IF(          [ test "x$enable_thread_safe" != "xyes" ],
                [ AC_MSG_NOTICE([thread-safe support disabled])],
                [ AC_CHECK_LIB(pthread, [pthread_mutex_init], [], [])
                  AC_MSG_CHECKING([for POSIX mutex])
                  AC_LINK_IFELSE([AC_LANG_PROGRAM([[extern "C" {
                                                        #if HAVE_PTHREAD_H
                                                        #include <pthread.h>
                                                        #endif
                                                     }]],
                                                    [[ pthread_mutex_t mutex;
                                                       pthread_mutex_init(&mutex, (const pthread_mutexattr_t*)0);
                                                       pthread_mutex_lock(&mutex);
                                                       pthread_mutex_unlock(&mutex);]])
                                 ],
                                 [ AC_DEFINE(MUTEX_WORKS, 1, [POSIX mutex (pthread_mutex_t) is available])
                                      local_mutex_works="yes"
                                      AC_MSG_RESULT(yes)
                                 ],
                                 [ AC_MSG_RESULT(no)])
                  AC_MSG_CHECKING([for reentrant stdlib calls])
                  AC_LINK_IFELSE([AC_LANG_PROGRAM([[extern "C" {
                                                         #if HAVE_TIME_H
                                                         #include <time.h>
                                                         #endif
                                                         #if HAVE_SYS_TYPE_H
                                                         #include <sys/type.h>
                                                         #endif
                                                         #if HAVE_GRP_H
                                                         #include <grp.h>
                                                         #endif
                                                         #if HAVE_PWD_H
                                                         #include <pwd.h>
                                                         #endif
                                                         #if HAVE_DIRENT_H
                                                         #include <dirent.h>
                                                         #endif
                                                }]],
                                                [[ #if HAVE_CTIME_R
                                                     char *val1 = ctime_r(0, 0);
                                                   #else
                                                     error(); // should not compile as expected
                                                   #endif

                                                   #if HAVE_GETGRNAM_R
                                                     int val2 = getgrnam_r(0, 0, 0, 0, 0);
                                                   #else
                                                     error(); // should not compile as expected
                                                   #endif

                                                   #if HAVE_GETPWNAM_R
                                                     int val3 = getpwnam_r(0, 0, 0, 0, 0);
                                                   #else
                                                     error(); // should not compile as expected
                                                   #endif

                                                   #if HAVE_LOCALTIME_R
                                                     struct tm *val4 = localtime_r(0, 0);
                                                   #else
                                                     error(); // should not compile as expected
                                                   #endif

                                                   #if HAVE_READDIR_R
                                                     int val5 = readdir_r(0, 0, 0);
                                                   #else
                                                     error(); // should not compile as expected
                                                   #endif
                                                 ]])
                                ],
                                [AC_MSG_RESULT([all could be found])],
                                [
                                  AC_DEFINE(MISSING_REENTRANT_LIBCALL, 1, [Some *_r() stdlib call are missing to permit complete thread-safe support by libdar])
                                  local_mutex="no"
                                  AC_MSG_RESULT([some are missing])
                                ]
                        )
                  AS_IF( [ test "x$explicit_thread_safe" = "xyes" -a "$local_mutex" != "yes" ],
                         [ AC_MSG_ERROR([thread safe support not available]) ]
                       )
                ]
     )


AC_ARG_ENABLE( [execinfo],
               AS_HELP_STRING(--disable-execinfo, [disable reporting stack information on self diagnostic bugs even]),
               [explicit_execinfo=yes],
               [enable_execinfo=yes])

AS_IF(       [ test "x$enable_execinfo" != "xyes" ],
             [ AC_MSG_WARN([ignoring execinfo even if available]) ],
             [
               AC_CHECK_HEADERS([execinfo.h])
               AC_CHECK_LIB(execinfo, [backtrace], [], [])
               AC_MSG_CHECKING([for backtrace() usability])
               AC_LINK_IFELSE([AC_LANG_PROGRAM([[extern "C" {
                                               #if HAVE_EXECINFO_H
                                               #include <execinfo.h>
                                               #endif
                                               }]],
                                               [[
                                               const int buf_size = 20;
                                               void *buffer[buf_size];
                                               int x = backtrace(buffer, buf_size);
                                               ]])
                               ],
                               [ AC_DEFINE(BACKTRACE_AVAILABLE, 1, [backtrace() call supported])
                                 AC_MSG_RESULT(yes)
                               ],
                               [
                                 AC_MSG_RESULT(no)
                                 AS_IF( [ test "x$explicit_execinfo" = "xyes" ],
                                        [ AC_MSG_ERROR([execinfo not found]) ]
                                      )
                               ])
             ]
     )


AC_ARG_ENABLE( [profiling],
               AS_HELP_STRING(--enable-profiling, [enable executable profiling]),
               [
                 AS_IF([ test "x$enable_profiling" != "xyes" ],
                       [ AC_MSG_ERROR([invalid argument given to --enable-profiling]) ]
                      )
                 profiling="yes"
               ])

AC_ARG_ENABLE( [debug-memory],
               AS_HELP_STRING(--enable-debug-memory, [log memory allocations and releases to /tmp/dar_debug_mem_allocation.txt this debugging option lead to a slow executable]),
               [
                 AS_IF([ test "x$enable_debug_memory" != "xyes" ],
                       [ AC_MSG_ERROR([invalid argument given to --enable-debug-memory]) ]
                      )
                 AC_DEFINE(LIBDAR_DEBUG_MEMORY, 1, [if defined, builds a very slow executable
               ])])

AC_ARG_ENABLE( [dar-static],
               AS_HELP_STRING(--disable-dar-static, [avoids building dar_static, a dar statically linked version]),
               [
                 explicit_dar_static=yes
                 AS_IF([ test "x$enable_dar_static" != "xyes" ],
                       [ build_static="no" ],
                       [ build_static="yes" ])
               ],
               [build_static="yes"])


AC_ARG_ENABLE( [threadar],
               AS_HELP_STRING(--disable-threadar, [avoid linking with libthreadar if available to prevent the use several threads inside libdar]),
               [explicit_threadar=yes],
               [enable_threadar=yes])

AS_IF(       [ test "x$enable_threadar" != "xyes" ],
             [ AC_MSG_WARN([libthreadar support has been disabled by user]) ],
             [
               PKG_CHECK_EXISTS(libthreadar,
                                [
                                  PKG_CHECK_MODULES(LIBTHREADAR, libthreadar, [], [AC_MSG_ERROR([libthreadar not found, but reported to exist !?!])])
                                ],
                                [
                                  AC_CHECK_LIB(threadar, [for_autoconf], [], [])
                                ])
               CPPFLAGS__cache="$CPPFLAGS"
               CPPFLAGS="$LIBTHREADAR_CFLAGS $CPPFLAGS"
               CXXFLAGS__cache="$CXXFLAGS"
               CXXFLAGS="$LIBTHREADAR_CFLAGS $CXXFLAGS"
               LIBS__cache="$LIBS"
               LIBS="$LIBTHREADAR_LIBS $LIBS"
               AC_CHECK_HEADER(libthreadar/libthreadar.hpp,
                               [ AC_DEFINE(HAVE_LIBTHREADAR_LIBTHREADAR_HPP, 1, [libthreadar.h header file availability]) ],
                               [ AC_MSG_WARN([Cannot find libthreadar.h header file]) ]
                              )
               AC_MSG_CHECKING([for libthreadar operationability])
               exp_maj_version_threadar=1
               min_med_version_threadar=3
               min_min_version_threadar=1
               AC_DEFINE_UNQUOTED(EXPECTED_MAJ_VERSION_THREADAR, "$exp_maj_version_threadar", [libthreadar expected major version])
               AC_DEFINE_UNQUOTED(MIN_MED_VERSION_THREADAR, "$min_med_version_threadar", [libthreadar minimal medium version])
               AC_DEFINE_UNQUOTED(MIN_MIN_VERSION_THREADAR, "$min_min_version_threadar", [libthreadar minimal minor version])
               AC_RUN_IFELSE([AC_LANG_PROGRAM(
                                   [[
                                    #if HAVE_LIBTHREADAR_LIBTHREADAR_HPP
                                    #include <libthreadar/libthreadar.hpp>
                                    #endif
                                    #include <iostream>
                                   ]],
                                   [[
                                        class mythread: public libthreadar::thread
                                        {
                                        public:
                                            mythread(int x): myx(x) {};
                                            int getx() const { return myx; };

                                        protected:
                                            virtual void inherited_run() { --myx; };

                                        private:
                                           int myx;
                                        };

                                        unsigned int maj, med, min;
                                        libthreadar::get_version(maj, med, min);
                                        if(maj != atoi(EXPECTED_MAJ_VERSION_THREADAR)
                                          || med < atoi(MIN_MED_VERSION_THREADAR)
                                          || (med == atoi(MIN_MED_VERSION_THREADAR)
                                             && min < atoi(MIN_MIN_VERSION_THREADAR)))
                                        {
                                             std::cout << "libthreadar version " << maj << "." << med << "." << min
                                                       << " is too old, use at least version "
                                                       << EXPECTED_MAJ_VERSION_THREADAR << "."
                                                       << MIN_MED_VERSION_THREADAR << "."
                                                       << MIN_MIN_VERSION_THREADAR << std::endl;
                                             return 1;
                                        }

                                        mythread toto(10);

                                        toto.run();
                                        toto.join();
                                        toto.getx();
                                        std::cout << "ok" << std::endl;

                                        return 0;
                                   ]])
                               ],
                               [
                                 local_threadar=yes
                                 AC_DEFINE(LIBTHREADAR_AVAILABLE, 1, [when libthreadar could be found and linked against])
                                 AC_MSG_RESULT(fine)
                               ],
                               [
                                 local_threadar=no
                                 AC_MSG_RESULT(wrong)
                               ],
                               [
                                 # if cross compiling

                                 AC_LINK_IFELSE(
                                  [AC_LANG_PROGRAM(
                                   [[
                                    #if HAVE_LIBTHREADAR_LIBTHREADAR_HPP
                                    #include <libthreadar/libthreadar.hpp>
                                    #endif
                                    #include <iostream>
                                   ]],
                                   [[
                                        class mythread: public libthreadar::thread
                                        {
                                        public:
                                            mythread(int x): myx(x) {};
                                            int getx() const { return myx; };

                                        protected:
                                            virtual void inherited_run() { --myx; };

                                        private:
                                           int myx;
                                        };

                                        mythread toto(10);

                                        toto.run();
                                        toto.join();
                                        toto.getx();
                                        std::cout << "ok" << std::endl;
                                   ]])
                                 ],
                                 [
                                   local_threadar=yes
                                   AC_DEFINE(LIBTHREADAR_AVAILABLE, 1, [when libthreadar could be found and linked against])
                                   AC_MSG_RESULT(fine)
                                 ],
                                 [
                                   local_threadar=no
                                   AC_MSG_RESULT(wrong)
                                 ])
                               ]
                             )
              CPPFLAGS="$CPPFLAGS__cache"
              unset CPPFLAGS__cache
              CXXFLAGS="$CXXFLAGS__cache"
              unset CXXFLAGS__cache
              LIBS="$LIBS__cache"
              unset LIBS__cache
              AS_IF( [ test "x$explicit_threadar" = "xyes" -a "$local_threadar" != "yes" ],
                     [ AC_MSG_ERROR([libthreadar linking failed]) ]
                   )
            ]
     )

AS_IF(
  [ test "$local_threadar" = "yes" ],
  [
    CPPFLAGS__cache="$CPPFLAGS"
    CPPFLAGS="$LIBTHREADAR_CFLAGS $CPPFLAGS"
    CXXFLAGS__cache="$CXXFLAGS"
    CXXFLAGS="$LIBTHREADAR_CFLAGS $CXXFLAGS"
    LIBS__cache="$LIBS"
    LIBS="$LIBTHREADAR_LIBS $LIBS"

    AC_RUN_IFELSE(
     [
       AC_LANG_PROGRAM( [[
                        #if HAVE_LIBTHREADAR_LIBTHREADAR_HPP
                        #include <libthreadar/libthreadar.hpp>
                        #endif
                       ]],
                       [[
		        unsigned int maj, med, min;
                        libthreadar::get_version(maj, med, min);
			if(libthreadar::barrier::used_implementation() != "")
			    return 0;
			else
			    return 1;
		       ]])
     ],
     [
	local_threadar_barrier_mac=yes
	AC_DEFINE(LIBTHREADAR_BARRIER_MAC, 1, [if libthreadar has different possible implementation])
     ],
     [
        local_threadar_barrier_mac=no
     ]
    )
    CPPFLAGS="$CPPFLAGS__cache"
    unset CPPFLAGS__cache
    CXXFLAGS="$CXXFLAGS__cache"
    unset CXXFLAGS__cache
    LIBS="$LIBS__cache"
    unset LIBS__cache
  ]
)

AC_ARG_ENABLE([libargon2-linking],
              AC_HELP_STRING(--disable-libargon2-linking, [avoid linking with libargon2 if available to prevent the argon2 hashing algorithm to be used]),
              [explicit_argon2=yes],
              [enable_argon2=yes])

AS_IF(
              [ test "x$enable_argon2" != "xyes" ],
              [
                AC_MSG_WARN([libargon2 support has been disabled by user])
                local_argon2=no
              ],
              [
                AC_CHECK_HEADER(argon2.h,
                                [ AC_DEFINE(HAVE_ARGON2_H, 1, [argon2.h header file availability]) ],
                                [ AC_MSG_WARN([Cannot find argon2.h header file]) ]
                               )
                AC_CHECK_LIB(argon2, [argon2id_hash_raw], [], [AC_MSG_WARN([library libargon2 not found])])
                AC_MSG_CHECKING([for libargon2 usability])
                AC_RUN_IFELSE([AC_LANG_PROGRAM([[ extern "C"
                                                  {
                                                       #if HAVE_ARGON2_H
                                                       #include <argon2.h>
                                                       #endif
                                                       #if HAVE_STRING_H
                                                       #include <string.h>
                                                       #endif
                                                   }
                                               ]],
                                               [[
                                                    char pass[] = "mot de passe";
                                                    char salt[] = "sel fin de cuisine";
                                                    constexpr unsigned int hash_size = 40;
                                                    char hash[hash_size];

                                                    if(argon2id_hash_raw(2000,
                                                                         100,
                                                                         1,
                                                                         pass, strlen(pass),
                                                                         salt, strlen(salt),
                                                                         hash, hash_size) != ARGON2_OK)
                                                        return 1;
                                                     else
                                                        return 0;
                                                ]])
                                ],
                                [
                                    local_argon2=yes
                                    AC_DEFINE(LIBARGON2_AVAILABLE, 1, [when libargon2 could be found and linked against])
                                    AC_MSG_RESULT(ok)
                                ],
                                [
                                    local_argon2=no
                                    AC_MSG_RESULT([not usable! See config.log for details])
                                ],
                                [
                                   # if cross compiling

                                     AC_LINK_IFELSE([AC_LANG_PROGRAM(
                                                        [[ extern "C"
                                                        {
                                                        #if HAVE_ARGON2_H
                                                        #include <argon2.h>
                                                        #endif
                                                        #if HAVE_STRING_H
                                                        #include <string.h>
                                                        #endif
                                                        }
                                                        ]],
                                                        [[
                                                        char pass[] = "mot de passe";
                                                        char salt[] = "sel fin de cuisine";
                                                        constexpr unsigned int hash_size = 40;
                                                        char hash[hash_size];

                                                        if(argon2id_hash_raw(2000,
                                                                         100,
                                                                         1,
                                                                         pass, strlen(pass),
                                                                         salt, strlen(salt),
                                                                         hash, hash_size) != ARGON2_OK)
                                                          return 1;
                                                        else
                                                          return 0;
                                                        ]])
                                                   ],
                                                   [
                                                     local_argon2=yes
                                                     AC_DEFINE(LIBARGON2_AVAILABLE, 1, [when libargon2 could be found and linked against])
                                                     AC_MSG_RESULT(ok)
                                                   ],
                                                   [
                                                     local_argon2=no
                                                     AC_MSG_RESULT([not usable! See config.log for details])
                                                   ])
                                ]
                        )
                ]
     )


CPPFLAGS__cache="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $GPGME_CFLAGS $LIBCURL_CFLAGS $LIBTHREADAR_CFLAGS"
CXXFLAGS__cache="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS"
LDFLAGS__cache="$LDFLAGS"
LDFLAGS="$LDFLAGS -static"
LIBS__cache="$LIBS"
LIBS="$LIBS $GPGME_LIBS $LIBCURL_LIBS $LIBTHREADAR_LIBS"
AC_MSG_CHECKING([static linking])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[ extern "C" {
                                                #if HAVE_STDIO_H
                                                #include <stdio.h>
                                                #endif
                                                #if HAVE_EXECINFO_H
                                                #include <execinfo.h>
                                                #endif
                                                #if HAVE_STDLIB_H
                                                #include <stdlib.h>
                                                #endif
                                                #if HAVE_PTHREAD_H
                                                #include <pthread.h>
                                                #endif
                                                #if HAVE_LIBRSYNC_H
                                                #include <librsync.h>
                                                #endif
                                                #if HAVE_PTHREAD_H
                                                #include <pthread.h>
                                                #endif
                                                #if HAVE_LIBRSYNC_H
                                                #include <stdio.h>
                                                #include <librsync.h>
                                                #endif
                                                #if HAVE_ZLIB_H
                                                #include <zlib.h>
                                                #endif
                                                #if HAVE_BZLIB_H
                                                #include <bzlib.h>
                                                #endif
                                                #if HAVE_LZO_LZO1X_H
                                                #include <lzo/lzo1x.h>
                                                #endif
                                                #if HAVE_LZMA_H
                                                #include <lzma.h>
                                                #endif
                                                #if HAVE_GCRYPT_H
                                                #include <gcrypt.h>
                                                #endif
                                                #if HAVE_CURL_CURL_H
                                                #include <curl/curl.h>
                                                #endif
                                                #if HAVE_GPGME_H
                                                #include <gpgme.h>
                                                #endif
                                             }
                               ]],
                               [[
                               #if BACKTRACE_AVAILABLE
                                  const int buf_size = 20;
                                  void *buffer[buf_size];
                                  int size = backtrace(buffer, buf_size);
                                  char **symbols = backtrace_symbols(buffer, size);

                                  if(symbols != 0)
                                    free(symbols);
                                  printf("testing execinfo info in static linked mode...");
                               #endif

                               #if MUTEX_WORKS
                               if(1)
                               {
                                 pthread_mutex_t test;
                                 if(pthread_mutex_init(&test, NULL) == 0)
                                 {
                                    if(pthread_mutex_lock(&test) == 0)
                                       pthread_mutex_unlock(&test);
                                 }
                                 pthread_mutex_destroy(&test);
                                 printf("testing mutex availability in static linked mode...");
                               }
                               #endif

                               #if LIBRSYNC_AVAILABLE
                               if(1)
                               {
                                 rs_result err = RS_DONE;
                                 (void) rs_strerror(err);
                                 printf("testing librsync availability in static linked mode...");
                               }
                               #endif

                               #if LIBZ_AVAILABLE
                               if(1)
                               {
                                  z_stream *ptr = (z_stream *)0;
                                  deflate(ptr, 0);
                                  printf("testing libz availability in static linked mode...");
                               }
                               #endif

                               #if LIBBZ2_AVAILABLE
                               if(1)
                               {
                                  bz_stream *ptr = (bz_stream *)0;
                                  BZ2_bzCompress(ptr, 0);
                                  printf("testing libbz2 availability in static linked mode...");
                               }
                               #endif

                               #if LIBLZO2_AVAILABLE
                               if(1)
                               {
                                  int x;
                                  printf("testing liblzo2 availability in static linked mode...");
                                  x = lzo1x_1_compress(0, 0, 0, 0, 0);
                               }
                               #endif

                               #if LIBLZMA_AVAILABLE
                               if(1)
                               {
                                  lzma_stream ptr = LZMA_STREAM_INIT;
                                  lzma_ret tmp = lzma_easy_encoder(&ptr,
                                                                   2,
                                                                   LZMA_CHECK_CRC32);
                                  printf("testing libxz/lzma availability in static linked mode...");
                               }
                               #endif

                               #if CRYPTO_AVAILABLE
                                  printf("testing gcrypt availability in static linked mode...");
                                  if(!gcry_check_version(MIN_VERSION_GCRYPT))
                                  {
                                    printf("ligcrypt version too low");
                                    exit(1);
                                  }
                                  else
                                    exit(0);
                               #endif

                               #if LIBCURL_AVAILABLE
                                  printf("testing librsync availability in static linked mode...");
                                  (void) curl_global_init(CURL_GLOBAL_ALL);
                               #endif

                               #if GPGME_SUPPORT
                               if(1)
                               {
                                 gpgme_ctx_t context;
                                 gpgme_error_t err = gpgme_new(&context);
                                 gpgme_release(context);
                               }
                               #endif

                                 return 0;
                               ]])
               ],
               [
                 AC_MSG_RESULT([yes, perfect!])
                 static_pb="no"
               ],
               [
                 AC_MSG_RESULT([failed])
                 static_pb="yes"
                 AS_IF( [ test "x$explicit_dar_static" = "xyes" -a "$build_static" = "yes" ],
                        [ AC_MSG_ERROR([Cannot build dar-static on this system, check config.log]) ]
                      )
               ])
CPPFLAGS="$CPPFLAGS__cache"
unset CPPFLAGS__cache
CXXFLAGS="$CXXFLAGS__cache"
unset CXXFLAGS__cache
LDFLAGS="$LDFLAGS__cache"
unset LDFLAGS__cache
LIBS="$LIBS__cache"
unset LIBS__cache

AC_ARG_ENABLE( [python-binding],
               AS_HELP_STRING(--disable-python-binding, [ignore python binding even if it is possible to build it]),
               [explicit_python_binding=yes],
               [enable_python_binding=yes])

AS_IF(         [ test "x$enable_python_binding" != "xyes" ],
               [
                 AC_MSG_WARN([python binding disabled per user request])
                 local_python="no"
               ],
               [
                 if test "x$enable_shared" != "xyes" -o "$debug_static" == "yes" ; then
                   AC_MSG_WARN([Cannot build python binding without shared library support])
                   local_python="no"
                 else
                   AC_MSG_CHECKING([for python binding])
                   pyext="python3-config --extension-suffix"
                   if  $pyext 1> /dev/null 2> /dev/null ; then
                      PYEXT="`$pyext`"
                   else
                       local_python="no"
                   fi

                   pyflags="python3 -m pybind11 --includes"
                   if test "$local_python" != "no" && $pyflags 1> /dev/null 2> /dev/null ; then
                      PYFLAGS="`$pyflags`"
                      local_python="yes"
                      AC_MSG_RESULT([ok])
                      AC_SUBST(PYEXT, [$PYEXT])
                      AC_SUBST(PYFLAGS, [$PYFLAGS])
                   else
                      local_python="no"
                      AC_MSG_RESULT([failed])
                   fi
                 fi
                 AS_IF( [ test "x$explicit_python_binding" = "xyes" -a "$local_python" != "yes" ],
                        [ AC_MSG_ERROR([prerequisit for python binding not met]) ]
                      )
               ]
     )

AM_CONDITIONAL([MAKE_ALL_DIR], [test $examples = "yes"])
AM_CONDITIONAL([BUILD_DAR_STATIC], [test $build_static = "yes" -a $static_pb = "no"])
AM_CONDITIONAL([DEBUG_STATIC], [test $debug_static = "yes" -a $static_pb = "no"])
AM_CONDITIONAL([BUILD_MODE32], [test "$build_mode" = "32"])
AM_CONDITIONAL([BUILD_MODE64], [test "$build_mode" = "64" -o -z "$build_mode"])
AM_CONDITIONAL([USE_UPX], [test "$upx" = "yes"])
AM_CONDITIONAL([USE_DOXYGEN], [test "$doxygen" = "yes"])
AM_CONDITIONAL([USE_GROFF], [test "$groff" = "yes"])
AM_CONDITIONAL([PROFILING], [test "$profiling" = "yes"])
AM_CONDITIONAL([BSD_SED], [test "$local_sed" = "bsd"])
AM_CONDITIONAL([WITH_LIBTHREADAR], [test "$local_threadar" = "yes"])
AM_CONDITIONAL([PYTHON_BINDING], [test "$local_python" = "yes"])
AC_SUBST([CXXSTDFLAGS], [$CXXSTDFLAGS])
AC_SUBST(UPX_PROG, [upx])
AC_SUBST(DOXYGEN_PROG, [doxygen])
AC_SUBST(HAS_DOT, [$dot])

# defaults
AC_PREFIX_DEFAULT(/usr/local)

# hack from libtool mailing-list to know from source point of view whether we are compiling for dynamic or static way

AC_CONFIG_COMMANDS([hack-libtool], [
                                    sed 's,^pic_flag=,pic_flag=" -D__DYNAMIC__ ",' libtool > libtoolT \
                                    && mv -f libtoolT libtool && chmod 755 libtool
])


AC_CONFIG_FILES([Makefile man/Makefile src/Makefile src/libdar/Makefile src/dar_suite/Makefile src/testing/Makefile src/examples/Makefile doc/Makefile doc/samples/Makefile misc/Makefile doc/mini-howto/Makefile src/libdar/libdar.pc.tmpl doc/man/Makefile src/check/Makefile src/python/Makefile])
AC_OUTPUT
[echo ""
echo "--"
echo "dar and libdar have been successfully configured with the following parameters:"
echo ""
echo "  LIBDAR parameters:"

printf "   Zlib compression (gzip)    : "
if [ "$local_libz" = "yes" ] ; then
  echo "YES"
else
  echo "NO"
fi

printf "   Libbz2 compression (bzip2) : "
if [ "$local_libbz2" = "yes" ] ; then
  echo "YES"
else
  echo "NO"
fi

printf "   Liblzo2 compression (lzo)  : "
if [ "$local_liblzo2" = "yes" ] ; then
  echo "YES"
else
  echo "NO"
fi

printf "   Liblxz compression (xz)    : "
if [ "$local_libxz" = "yes" ] ; then
  echo "YES"
else
  echo "NO"
fi

printf "   Liblzstd compression (zstd): "
if [ "$local_libzstd" = "yes" ] ; then
  echo "YES"
else
  echo "NO"
fi

printf "   Liblz4 compression (lz4)   : "
if [ "$local_liblz4" = "yes" ] ; then
  echo "YES"
else
  echo "NO"
fi

printf "   Strong encryption support  : "
if [ "$local_crypto" = "yes" ] ; then
  echo "YES"
else
  echo "NO"
fi

printf "   Public key cipher support  : "
if [ "$local_gpgme" = "yes" ] ; then
  echo "YES"
else
  echo "NO"
fi

printf "   Extended Attributes support: "
if [ "$local_ea_support" = "yes" ] ; then
  echo "YES"
else
  echo "NO"
fi

printf "   Large files support (> 2GB): "
if [ ! -z "$ac_cv_sys_file_offset_bits" -o ! -z "$ac_cv_sys_large_files" ] ; then
  echo "YES"
else
  echo "NO"
fi

printf "   extX FSA / nodump support  : "
if [ "$local_nodump_feature" = "yes" ] ; then
  echo "YES"
else
  echo "NO"
fi

printf "   HFS+ FSA support           : "
if [ "$local_birthtime" = "yes" ] ; then
  echo "YES"
else
  echo "NO"
fi

printf "   statx() support            : "
if [ "$local_statx" = "yes" ] ; then
  echo "YES"
else
  echo "NO"
fi

printf "   Integer size used          : "
if [ "$build_mode" = "infinint" ] ; then
  echo "infinint"
else
  if [ -z "$build_mode" ] ; then
     build_mode=64
  fi
  echo "$build_mode"
fi

printf "   Thread safe support        : "
if [ "$local_mutex_works" = "yes" -a -z "$local_test_memory" ] ; then
  echo "YES"
else
  echo "NO"
fi

printf "   Furtive read mode          : "
if [ "$local_furtive_read_mode" = "yes" ]; then
   echo "YES"
else
   echo "NO"
fi

printf "   Large directory optim.     : "
if [ "$local_fast_dir" = "yes" ] ; then
   echo "YES"
else
   echo "NO"
fi

printf "   posix fadvise support      : "
if  [ "$local_posix_fadvise" = "yes" ] ; then
    echo "YES"
else
    echo "NO"
fi

printf "   timepstamps write accuracy : "

if [ $local_time_write_accuracy -eq $local_time_accuracy_nanosecond ] ; then
   echo "1 nanosecond"
else
   if [ $local_time_write_accuracy -eq $local_time_accuracy_microsecond ] ; then
       echo "1 microsecond"
   else
       echo "1 second"
   fi
fi

printf "   timestamps read accuracy   : "

if [ $local_time_read_accuracy -eq $local_time_accuracy_nanosecond ] ; then
   echo "1 nanosecond"
else
   if [ $local_time_read_accuracy -eq $local_time_accuracy_microsecond ] ; then
       echo "1 microsecond"
   else
       echo "1 second"
   fi
fi

printf "   can restore symlink dates  : "
if [ "$local_lutimes" = "yes" ] ; then
    echo "YES"
else
    echo "NO"
fi

printf "   can uses multiple threads  : "
if [ "$local_threadar" = "yes" ] ; then
   echo "YES"
else
   echo "NO"
fi

printf "   Delta-compression support  : "
if [ "$local_librsync" = "yes" ] ; then
  echo "YES"
else
  echo "NO"
fi

printf "   Remote repository support  : "
if [ "$local_libcurl" = "yes" -a "$local_threadar" = "yes" ] ; then
  if [ "$local_threadar_barrier_mac" = "yes" ] ; then
    echo "YES (+ barrier implementations for MAC OS)"
  else
    echo "YES"
  fi
else
  echo "NO"
fi

printf "   Argon2 hashing algorithm   : "
if [ "$local_argon2" = "yes" ] ; then
  echo "YES"
else
  echo "NO"
fi

echo ""
echo "  DAR SUITE command line programs:"
printf "   Long options available : "
if [ "$local_have_getopt_long" = "yes" ] ; then
  echo "YES"
else
  echo "NO"
fi

printf "   Building examples      : "
if [ "$examples" = "yes" ] ; then
  echo "YES"
else
  echo "NO"
fi

printf "   Building dar_static    : "
if  [ "$build_static" = "yes" ]; then
  if [ "$static_pb" = "yes" ]; then
     echo "NO  (system does not support static linking, see note below)"
  else
     echo "YES"
  fi
else
  echo "NO"
fi

printf "   using upx at install   : "
if  [ "$upx" = "yes" ] ; then
  echo "YES"
else
  echo "NO"
fi

printf "   building documentation : "
if  [ "$doxygen" = "yes" ] ; then
  echo "YES"
else
  echo "NO"
fi

printf "   building python binding: "
if [ "$local_python" = "yes" ] ; then
   echo "YES"
else
   echo "NO"
fi

if [ "$static_pb" = "yes" -a "$build_static" = "yes" ] ; then
   echo ""
   echo "  Note:"
   echo ""
   echo "    If  you  want to know  which libraries  are not available  as  static"
   echo "    libraries check the logs in the config.log generated file. the command"
   echo ""
   echo "         'grep -e -static -A 2 config.log'"
   echo ""
   echo "    should bring you to the essentials."
   echo ""
   echo "You also might want to speed up the compilation process running ./configure"
   echo "with the --disable-static option"
   echo ""
fi

if [ -z "$build_mode" ] ; then
   echo ""
   echo "--------------------- N O T E -------------------------------------------"
   echo "Note: You are about to build a libdar/dar binary relying on \"infinint\""
   echo "integer type. You shall also consider using 64 bits integers (whatever"
   echo "your CPU has 32 or 64 bits registers) for better performances and reduced"
   echo "memory requirements, at the cost of the limitations explained here:"
   echo "   ./doc/Limitations.html (Paragraph about Integers)"
   echo "Document which is also available online at:"
   echo "   http://dar.linux.free.fr/doc/Limitations.html#Integers"
   echo "Unless you are impacted by these limitations, you can rerun ./configure"
   echo "adding the option --enable-mode=64 for dar/libdar better performances"
   echo "-------------------------------------------------------------------------"
fi

if [ "$libgcrypt_hash_bug" = "yes" ] ; then
   echo ""
   echo ""
   echo "#################### W A R N I N G ######################################"
   echo ""
   echo "libgcrypt version is lower than $min_version_gcrypt_hash_bug and has a bug"
   echo "concerning hash calculation for large files. Expect sha1 and md5 hash"
   echo "results for slices larger than 256 Gio (gibioctet) to be incorrect."
   echo ""
   echo "#################### W A R N I N G ######################################"
fi

echo ""
]
