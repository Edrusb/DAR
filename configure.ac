# Process this file with autoconf to produce a configure script.
AC_PREREQ(2.59)
AC_INIT([DAR], [2.1.2], [http://sourceforge.net/tracker/?group_id=65612&atid=511612])
AC_CONFIG_HEADERS([config.h])
AC_LANG([C++])
AC_CONFIG_SRCDIR([src/libdar/catalogue.cpp])
AC_DEFINE_UNQUOTED(DAR_VERSION, "AC_PACKAGE_VERSION", [dar and dar_suite version, definition in configure.ac])

AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_MAKE_SET

# Checks for libraries.
AC_CHECK_LIB(z, [deflate], [], [AC_MSG_ERROR([library zlib not found])])
AC_CHECK_LIB(bz2, [BZ2_bzCompress], [], [AC_MSG_ERROR([library libbz2 not found])])
AC_CHECK_LIB(socket, [socket], [], [])
AC_CHECK_LIB(nsl, [endnetconfig], [], [])


AC_MSG_CHECKING([for static linking with libdl])
MEM_LDFLAGS=$LDFLAGS
LDFLAGS="-static -ldl"
AC_RUN_IFELSE(
        [
        AC_LANG_PROGRAM([extern void *dlopen(const char *p, int mode);],
                        [if(0) dlopen("coucou", 0);])
        ],
        [libdl_static_pb="no"
         AC_MSG_RESULT([available])
        ],
        [libdl_static_pb="yes"
         AC_MSG_RESULT([failed])
        ],
        [])
LDFLAGS=$MEM_LDFLAGS

AC_CHECK_LIB(dl, [dlsym], [], [])


# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADER(zlib.h, [], [AC_MSG_ERROR([Cannot find zlib.h header file])])
AC_CHECK_HEADER(bzlib.h, [], [AC_MSG_ERROR([Cannot find bzlib.h header file])])
AC_CHECK_HEADERS([fcntl.h netinet/in.h stdint.h stdlib.h string.h sys/ioctl.h sys/socket.h termios.h unistd.h utime.h sys/types.h signal.h errno.h sys/un.h sys/stat.h time.h fnmatch.h regex.h pwd.h grp.h getopt.h stdio.h])

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
AC_REPLACE_FNMATCH
AC_FUNC_FORK
AC_PROG_GCC_TRADITIONAL
AC_FUNC_LSTAT
AC_HEADER_MAJOR
AC_FUNC_MALLOC
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_FUNC_UTIME_NULL
AC_HEADER_TIME
AC_CHECK_FUNCS([lchown mkdir regcomp rmdir strerror utime])

AC_MSG_CHECKING([for getopt() in <unistd.h>])
AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[extern "C" {
                                    #if HAVE_UNISTD_H
                                    #include <unistd.h>
                                    #endif
                                    }]],
                                    [ getopt(0, NULL, NULL); ]),
                  [
                    AC_DEFINE(HAVE_GETOPT_IN_UNISTD_H, 1, [the getopt() call is declared in <unistd.h>])
                    AC_MSG_RESULT(present)
                  ],
                   [AC_MSG_RESULT(absent)])

AC_MSG_CHECKING([for getopt_long() in <unistd.h>])
AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[extern "C" {
                                    #if HAVE_UNISTD_H
                                    #include <unistd.h>
                                    #endif
                                    }]],
                                    [ getopt_long(0, NULL, NULL, NULL, NULL); ]),
                  [
                    AC_DEFINE(HAVE_GETOPT_LONG_IN_UNISTD_H, 1, [the getopt_long() call is declared in <unistd.h>]),
                    AC_MSG_RESULT(present)
                  ],
                   [AC_MSG_RESULT(absent)])


AC_MSG_CHECKING([for optreset presence])
AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[extern "C" {
                                   #if HAVE_GETOPT_H
				   #include <getopt.h>
                                   #else
                                   #if HAVE_UNISTD_H
                                   #include <unistd.h>
                                   #endif
                                   #endif
                                   }]], [ int x = optreset; return 0; ]),
                  [
                   AC_DEFINE(HAVE_OPTRESET, 1, [the optreset external variable exists to reset getopt standard call])
		   AC_MSG_RESULT(available)
                  ],
                  [AC_MSG_RESULT([not available])])


# DAR's features
AC_ARG_ENABLE(  [ea-support],
                AC_HELP_STRING(--enable-ea-support,[extended attributes support]),
                [ AC_CHECK_HEADER(attr/xattr.h, [], [AC_MSG_ERROR([need xattr.h you cannot activate ea-support])] )
                  AC_CHECK_LIB(attr, [lgetxattr], [], [AC_MSG_ERROR([libxattr not found, you cannot activate ea-support])])
                  AC_DEFINE(EA_SUPPORT, [], [if defined, activates support for Extended Attributes])
                ])

AC_MSG_CHECKING([ext2fs.h availability])
AC_ARG_ENABLE(  [nodump-flag],
                AC_HELP_STRING(--disable-nodump-flag, [deactivate the ext2/3 nodump flag check feature, if you don't have such filesystem]),
                [AC_MSG_RESULT([nodump flag disabled])],
                [AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[extern "C" {
                                                    #include <ext2fs/ext2_fs.h>
					            #if HAVE_SYS_IOCTL_H
                                      		    #include <sys/ioctl.h>
         					    #endif
                                                    }]],[[int fd, f; ioctl(fd, EXT2_IOC_GETFLAGS, &f);]]),
                                [
                                AC_DEFINE(LIBDAR_NODUMP_FEATURE, [ext2fs], [if defined, activates the ext2/3 nodump flag feature])
                                AC_MSG_RESULT([found <ext2fs/ext2_fs.h>])
                                ],
                                [
				AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[extern "C" {
		                                                    #include <linux/ext2_fs.h>
							            #if HAVE_SYS_IOCTL_H
                                      				    #include <sys/ioctl.h>
         							    #endif
                                                                    }]],[[int fd, f; ioctl(fd, EXT2_IOC_GETFLAGS, &f);]]),
                                                 [
 				                 AC_DEFINE(LIBDAR_NODUMP_FEATURE, [linux], [if defined, activates the ext2/3 nodump flag feature])
  						 AC_MSG_RESULT([found <linux/ext2_fs.h>])
                                                 ],
						 [
						     AC_MSG_RESULT([NOT FOUND])
                                                     AC_MSG_WARN([cannot find ext2_fs.h header file, nodump-flag feature will not be activated])
	                                         ])
                                ])

                ])

AC_ARG_ENABLE(  [gnugetopt],
	        AC_HELP_STRING(--disable-gnugetopt, [avoid linking with libgnugetopt if available]),
		[AC_DEFINE(NO_GNUGETOPT, 1, [do not use getopt.h header])],
    	        AC_CHECK_LIB(gnugetopt, [getopt_long], [], [])
             )

AC_ARG_ENABLE(  [examples],
                AC_HELP_STRING(--enable-examples, [buld example and testing programs]),
                [examples="yes"],
                [examples="false"])

AC_ARG_ENABLE(  [os-bits],
                AC_HELP_STRING(--enable-os-bits=arg, [arg is 32 or 64. If for some reason, one wants to overcome detected system value]),
                [AC_DEFINE_UNQUOTED(OS_BITS, $enableval, [manually set CPU's registers' size])],
                [
                        AC_CHECK_HEADER(inttypes.h,
                                [
                                  AC_RUN_IFELSE(
                                        [AC_LANG_PROGRAM([extern "C" {
						          #include <inttypes.h>
                                                          }],
                                                         [ uint16_t a  = 0; uint32_t b = 0; uint64_t c = 0;
                                                           int16_t d = 0; int32_t e = 0; int64_t f = 0;
                                                           return a+b+c+d+e+f;
                                                         ])
                                        ],
                                        [],
                                        [AC_MSG_ERROR([Cannot find *int*_t type declarations in headerfile, --enable-os-bits=... option must be used])] )
                                ],
                                [AC_MSG_ERROR([Cannot find inttypes.h headerfile, --enable-os-bits=... option must be used])]
                        )
                ]
             )

AC_ARG_ENABLE( [dar-static],
                AC_HELP_STRING(--disable-dar-static, [avoids building dar_static, a dar statically linked version]),
                [build_static="no"],
                [
                if test $libdl_static_pb = "yes" ; then
                   AC_MSG_NOTICE([static linking not completely supported on this system, CANNOT and WILL NOT build dar_static !])
                   build_static="no"
                else
                   build_static="yes"
                fi
                ])

AC_ARG_ENABLE( [mode],
                AC_HELP_STRING(--enable-mode=arg, [where arg is 32 or 64. Makes dar internally use 32 bits or 64 bits integers in place of infinint (which is the case if this option is not given]),
                [build_mode=$enableval],
                [])

AC_ARG_ENABLE( [debug],
                AC_HELP_STRING(--enable-debug, [build targets with debugging option and no optimization]),
                [
	                CXXFLAGS="-g -Wall"
                        CFLAGS="-g -Wall"
                        debug_static="yes"
                ],
                [
                        debug_static="no"
                ])


AC_ARG_ENABLE( [pedantic],
               AC_HELP_STRING(--enable-pedantic, [enable pedantic syntaxic check at compilation, use only for debugging purposes !]),
               [
                        CXXFLAGS="$CXXFLAGS -pedantic -Wno-long-long"
               ],
               [])

AC_ARG_ENABLE( [build-usage],
                AC_HELP_STRING(--enable-build-usage, [re-build C++ generated usage files with Chris Martin's dar-help program, non-developper should not activate that option]),
               [
                AC_CHECK_HEADER([libxml/tree.h], [AC_DEFINE(HAVE_LIBXML_TREE_H, [1], [defined if libxml/tree.h is present])], [AC_MSG_ERROR([Cannot find libxml/tree.h header file])])
                AC_CHECK_HEADER([libxml/parser.h], [AC_DEFINE(HAVE_LIBXML_PARSER_H, [1], [defined if libxml/parser.h is present])], [AC_MSG_ERROR([Cannot find libxml/parser.h header file])])
                AC_CHECK_LIB(xml2, [reference], [], [AC_MSG_ERROR([library xml2 not found])])
                build_usage="yes"
               ],
               [build_usage="no"])


AC_ARG_ENABLE(  [test-memory],
                AC_HELP_STRING(--enable-test-memory, [for debugging purpose only, activate a memory leakage detection mechanism, executable gets very slow]),
                [AC_DEFINE(TEST_MEMORY, [], [debuggging feature to track memory leakage, makes binary very slow])
             ])

AC_ARG_ENABLE(  [special-alloc],
                AC_HELP_STRING(--disable-special-alloc, [makes dar using less memory, in particular in conjunction with --enable-mode=32 or 64]),
                [],
                [AC_DEFINE(LIBDAR_SPECIAL_ALLOC, [], [if set activates specific new and delete operator which do better allocation for a lot of small items])
             ])

AC_ARG_ENABLE( [upx],
                AC_HELP_STRING(--disable-upx, [if not given, configure will look for UPX and if available make executables compressed]),
                [  AC_MSG_NOTICE([ignoring UPX])
                   upx="no"
                ],
                [
                   AC_CHECK_PROG(upx, upx, [yes], [no], [$PATH])
                ]
             )

AM_CONDITIONAL([MAKE_ALL_DIR], [test $examples = "yes"])
AM_CONDITIONAL([MAKE_USAGE], [test $build_usage = "yes"])
AM_CONDITIONAL([BUILD_DAR_STATIC], [test $build_static = "yes"])
AM_CONDITIONAL([DEBUG_STATIC], [test $debug_static = "yes" -a "$libdl_static_pb" != "yes"])
AM_CONDITIONAL([BUILD_MODE32], [test "$build_mode" = "32"])
AM_CONDITIONAL([BUILD_MODE64], [test "$build_mode" = "64"])
AM_CONDITIONAL([USE_UPX], [test "$upx" = "yes"])
AC_SUBST(UPX_PROG,[upx])

# defaults
AC_PREFIX_DEFAULT(/usr/local)

AC_CONFIG_FILES([Makefile man/Makefile src/Makefile src/libdar/Makefile src/dar_suite/Makefile src/testing/Makefile src/examples/Makefile doc/Makefile doc/samples/Makefile misc/Makefile])
AC_OUTPUT
