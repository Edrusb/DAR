# Process this file with autoconf to produce a configure script.
AC_PREREQ(2.59)
AC_INIT([DAR], [2.2.2], [http://sourceforge.net/tracker/?group_id=65612&atid=511612])
AC_CONFIG_HEADERS([config.h])
AC_LANG([C++])
AC_CONFIG_SRCDIR([src/libdar/catalogue.cpp])
AC_DEFINE_UNQUOTED(DAR_VERSION, "AC_PACKAGE_VERSION", [dar and dar_suite version, definition in configure.ac])

AM_INIT_AUTOMAKE
AM_GNU_GETTEXT([use-libtool])
AM_GNU_GETTEXT_VERSION
AM_ICONV

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_MAKE_SET
AC_PROG_RANLIB

# Checks for libraries.
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


AC_CHECK_HEADERS([fcntl.h netinet/in.h arpa/inet.h stdint.h stdlib.h string.h sys/ioctl.h sys/socket.h termios.h unistd.h utime.h sys/types.h signal.h errno.h sys/un.h sys/stat.h time.h fnmatch.h regex.h pwd.h grp.h stdio.h pthread.h ctype.h getopt.h openssl/blowfish.h])


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
AC_CHECK_FUNCS([lchown mkdir regcomp rmdir strerror utime])

AC_MSG_CHECKING([for getopt() in <unistd.h>])
AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[extern "C" {
                                    #if HAVE_UNISTD_H
                                    #include <unistd.h>
                                    #endif
                                    }]],
                                    [ getopt(0, 0, 0); ]),
                  [
                    AC_DEFINE(HAVE_GETOPT_IN_UNISTD_H, 1, [a getopt() call is declared in <unistd.h>])
                    AC_MSG_RESULT(present)
                  ],
                   [AC_MSG_RESULT(absent)])

AC_MSG_CHECKING([for getopt_long() in <unistd.h>])
AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[extern "C" {
                                    #if HAVE_UNISTD_H
                                    #include <unistd.h>
                                    #endif
                                    }]],
                                    [ getopt_long(0, 0, 0, 0, 0); ]),
                  [
                    AC_DEFINE(HAVE_GETOPT_LONG_IN_UNISTD_H, 1, [a getopt_long() call is declared in <unistd.h>])
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

AC_ARG_ENABLE( [libz-linking],
               AC_HELP_STRING(--disable-libz-linking, [disable linking with libz and disable libz compression support]),
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
               AC_LINK_IFELSE(AC_LANG_PROGRAM([[ extern "C" {
                                                           #if HAVE_ZLIB_H
                                                           #include <zlib.h>
                                                           #endif
                                              }]],
                                              [[
                                                         z_stream *ptr = NULL;
                                                         deflate(ptr, 0);
                                              ]]),
                                              [ AC_DEFINE(LIBZ_AVAILABLE, 1, [header and linking is available to have libz functions])],
                                              [ local_libz="no" ])
               else
                 AC_MSG_WARN([libz compression support not available])
               fi
             ]
             )

AC_ARG_ENABLE( [libbz2-linking],
               AC_HELP_STRING(--disable-libbz2-linking, [disable linking with libbz2 and disables libbz2 compression support]),
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
                 AC_LINK_IFELSE(AC_LANG_PROGRAM([[ extern "C" {
                                                       #if HAVE_BZLIB_H
                                                       #include <bzlib.h>
                                                       #endif
                                              }]],
                                              [[
                                                        bz_stream *ptr = NULL;
                                                        BZ2_bzCompress(ptr, 0);
                                              ]]),
                                              [ AC_DEFINE(LIBBZ2_AVAILABLE, 1, [header and linking is available to have libbz2 functions])],
                                              [ local_libbz2="no" ])
             else
                AC_MSG_WARN([libbz2 compression support not available])
             fi
           ])


AC_ARG_ENABLE( [libcrypto-linking],
               AC_HELP_STRING(--disable-libcrypto-linking, [disable linking with libcrypto (openssl) and disables strong encryption support]),
            [
             AC_MSG_WARN([strong encryption support has been disabled by user])
             local_crypto="no"
            ],
            [
              AC_CHECK_LIB(crypto, [BF_encrypt], [], [])
	      AC_CHECK_HEADER(openssl/crypto.h, [local_crypto="yes"
                                                AC_DEFINE(HAVE_OPENSSL_CRYPTO_H, 1, [openssl/crypto.h header file is available])
                                                ],
                                                [AC_MSG_WARN([Cannt find openssl/crypto.h header file])
                                                 local_crypto="no"
                                                ])
              if test "$local_crypto" = "yes" ; then
                 AC_LINK_IFELSE(AC_LANG_PROGRAM([[ extern "C" {
                                                       #if HAVE_OPENSSL_CRYPTO_H
                                                       #include <openssl/crypto.h>
                                                       #endif
						       #if HAVE_OPENSSL_BLOWFISH_H
						       #include <openssl/blowfish.h>
						       #endif
                                              }]],
                                              [[
							BF_encrypt(NULL, NULL);
                                              ]]),
                                              [ AC_DEFINE(CRYPTO_AVAILABLE, 1, [header and linking is available to have strong encryption functions])],
                                              [ local_crypto="no" ])
             else
                AC_MSG_WARN([strong encryption support not available])
             fi
           ])


AC_ARG_ENABLE(  [ea-support],
                AC_HELP_STRING(--disable-ea-support,[disable Extended Attributes support]),
                [ AC_MSG_CHECKING([for Extended Attribute support])
                  AC_MSG_RESULT([disabled])
                ],
                [ AC_CHECK_HEADERS([attr/xattr.h])
                  AC_CHECK_LIB(attr, [lgetxattr], [], [])
                  AC_MSG_CHECKING([for Extended Attribute support])
                  AC_LINK_IFELSE(AC_LANG_PROGRAM([[extern "C" {
                                                     #if HAVE_SYS_TYPES_H
                                                     #include <sys/types.h>
                                                     #endif
                                                     #if HAVE_ATTR_XATTR_H
                                                     #include <attr/xattr.h>
                                                     #endif
                                                    }]],
                                                    [ lgetxattr((char *)0, (char *)0, (void *)0, 0); ]),
                                 [
                                        AC_DEFINE(EA_SUPPORT, [], [if defined, activates support for Extended Attributes])
                                        local_ea_support="yes"
                                        AC_MSG_RESULT([yes])
                                 ],
                                 [ AC_MSG_RESULT([no]) ]
                                 )
                ]
             )

AC_MSG_CHECKING([ext2fs.h availability])
AC_ARG_ENABLE(  [nodump-flag],
                AC_HELP_STRING(--disable-nodump-flag, [deactivate the "ext2/3 nodump flag" feature check]),
                [AC_MSG_RESULT([nodump flag disabled])],
                [AC_LINK_IFELSE(AC_LANG_PROGRAM([[extern "C" {
                                                    #include <ext2fs/ext2_fs.h>
                                                    #if HAVE_SYS_IOCTL_H
                                                    #include <sys/ioctl.h>
                                                    #endif
                                                    }]],[[int fd, f; ioctl(fd, EXT2_IOC_GETFLAGS, &f);]]),
                                [
                                AC_DEFINE(LIBDAR_NODUMP_FEATURE, [ext2fs], [if defined, activates the ext2/3 nodump flag feature])
                                local_nodump_feature="yes"
                                AC_MSG_RESULT([found <ext2fs/ext2_fs.h>])
                                ],
                                [
                                AC_LINK_IFELSE(AC_LANG_PROGRAM([[extern "C" {
                                                                    #include <linux/ext2_fs.h>
                                                                    #if HAVE_SYS_IOCTL_H
                                                                    #include <sys/ioctl.h>
                                                                    #endif
                                                                    }]],[[int fd, f; ioctl(fd, EXT2_IOC_GETFLAGS, &f);]]),
                                                 [
                                                 AC_DEFINE(LIBDAR_NODUMP_FEATURE, [linux], [if defined, activates the ext2/3 nodump flag feature])
                                                 local_nodump_feature="yes"
                                                 AC_MSG_RESULT([found <linux/ext2_fs.h>])
                                                 ],
                                                 [
                                                     AC_MSG_RESULT([NOT FOUND])
                                                     AC_MSG_WARN([cannot find ext2_fs.h header file, nodump-flag feature will not be activated])
                                                 ])
                                ])

                ])

AC_ARG_ENABLE(  [gnugetopt],
                AC_HELP_STRING(--disable-gnugetopt, [avoid linking with libgnugetopt]),
                [],
                AC_CHECK_LIB(gnugetopt, [getopt_long], [], [])
             )


AC_MSG_CHECKING([for getopt() availability]);
AC_LINK_IFELSE(AC_LANG_PROGRAM([[extern "C" {
                                   #if HAVE_GETOPT_H
                                   #include <getopt.h>
                                   #else
                                   #if HAVE_UNISTD_H
                                   #include <unistd.h>
                                   #endif
                                   #endif
                                   }]], [ getopt(0, 0, 0); ]),
                  [
		    AC_MSG_RESULT([ok])
                  ],
                  [AC_MSG_ERROR([absent but required])])

AC_MSG_CHECKING([for getopt_long() availability]);
AC_LINK_IFELSE(AC_LANG_PROGRAM([[extern "C" {
                                   #if HAVE_GETOPT_H
                                   #include <getopt.h>
                                   #else
                                   #if HAVE_UNISTD_H
                                   #include <unistd.h>
                                   #endif
                                   #endif
                                   }]], [ getopt_long(0, 0, 0, 0, 0); ]),
                  [
		    local_have_getopt_long="yes"
                    AC_DEFINE(HAVE_GETOPT_LONG, 1, [whether getopt_long() is available])
		    AC_MSG_RESULT([available])
                  ],
                  [AC_MSG_RESULT([NOT AVAILABLE])])


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

AC_ARG_ENABLE( [libdl-test],
               AC_HELP_STRING(--disable-libdl-test, [avoids checking against a static version of libdl, but don't complain if static linking fails...]),
               [skip_libdl_test="yes"],
               [skip_libdl_test="no"])

AC_ARG_ENABLE( [dar-static],
                AC_HELP_STRING(--disable-dar-static, [avoids building dar_static, a dar statically linked version]),
                [build_static="no"],
                [
                if test $libdl_static_pb = "yes" -a $skip_libdl_test = "no" ; then
                   AC_MSG_NOTICE([static linking not completely supported on this system, CANNOT and WILL NOT build dar_static !])
                   build_static="no"
                else
                   build_static="yes"
                fi
                ])

AC_ARG_ENABLE( [mode],
                AC_HELP_STRING(--enable-mode=arg, [where arg is 32 or 64. Makes dar internally use 32 bits or 64 bits integers in place of infinint (which is the case if this option is not given)]),
                [build_mode=$enableval],
                [])

AC_ARG_ENABLE( [debug],
                AC_HELP_STRING(--enable-debug, [build targets with debugging option and no optimization]),
                [
                        CXXFLAGS="-ggdb -Wall"
                        CFLAGS="-ggdb -Wall"
                        debug_static="yes"
                ],
                [
                        debug_static="no"
                ])


AC_ARG_ENABLE( [pedantic],
               AC_HELP_STRING(--enable-pedantic, [enable pedantic syntaxical check at compilation, use only for debugging purposes !]),
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

AC_ARG_ENABLE( [build-html],
	       AC_HELP_STRING(--disable-build-html, [don't build programming documentation (in particular libdar API documentation)]),
	       [ doxygen="no" ],
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
	       ]
             )

AC_ARG_ENABLE(  [test-memory],
                AC_HELP_STRING(--enable-test-memory, [for debugging purpose only, activate a memory leakage detection mechanism, resulting executable is very slow]),
                [AC_DEFINE(TEST_MEMORY, [], [debuggging feature to track memory leakage, makes binary very slow])
                 local_test_memory="yes"
             ])

AC_ARG_ENABLE(  [special-alloc],
                AC_HELP_STRING(--disable-special-alloc, [special memory allcation scheme makes dar using less memory and run a bit faster, in particular in conjunction with --enable-mode=32 or 64, you can disable it if you like]),
                [],
                [AC_DEFINE(LIBDAR_SPECIAL_ALLOC, [], [if set activates specific new and delete operator which do better allocation for a lot of small items])
                local_special_alloc="yes"
             ])

AC_ARG_ENABLE( [upx],
                AC_HELP_STRING(--disable-upx, [by default configure looks for UPX and if available make executables compressed at installation time, you can disable this feature]),
                [  AC_MSG_NOTICE([ignoring UPX])
                   upx="no"
                ],
                [
                   AC_CHECK_PROG(upx, upx, [yes], [no], [$PATH])
                ]
             )

AC_ARG_ENABLE(  [thread-safe],
                AC_HELP_STRING(--disable-thread-safe, [libdar is thread safe if POSIX mutex are available, you can manually disable the use of POSIX mutex, the resulting libdar library will probably not be thread-safe]),
                [ AC_MSG_NOTICE([thread-safe support disabled])
                ],
                [ AC_CHECK_LIB(pthread, [pthread_mutex_init], [], [])
                  AC_MSG_CHECKING([for POSIX mutex])
                  AC_LINK_IFELSE(AC_LANG_PROGRAM([[extern "C" {
                                                        #if HAVE_PTHREAD_H
                                                        #include <pthread.h>
                                                        #endif
                                                     }]],
                                                    [[ pthread_mutex_t mutex;
                                                       pthread_mutex_init(&mutex, NULL);
                                                       pthread_mutex_lock(&mutex);
                                                       pthread_mutex_unlock(&mutex);]]),
                                    [ AC_DEFINE(MUTEX_WORKS, 1, [POSIX mutex (pthread_mutex_t) is available])
                                      local_mutex_works="yes"
                                      AC_MSG_RESULT(yes)
                                    ],
                                    [ AC_MSG_RESULT(no)])
                ]
             )

AC_ARG_ENABLE( [locale-dir],
               AC_HELP_STRING(--enable-locale-dir, [by default NLS files are installed in $prefix/share/locale. You override this with --enable-locale-dir=<path>]),
               [ AC_DEFINE_UNQUOTED(DAR_LOCALEDIR, ["$enableval"], [override the default location of *.mo files ($prefix/share/locale by default) ])],
               [ AC_DEFINE(DAR_LOCALEDIR, [""], [override the default location of *.mo files ($prefix/share/locale by default) ])])

AC_ARG_ENABLE( [profiling],
	       AC_HELP_STRING(--enable-profiling, [enable executable profiling]),
               [ profiling="yes" ])


AM_CONDITIONAL([MAKE_ALL_DIR], [test $examples = "yes"])
AM_CONDITIONAL([MAKE_USAGE], [test $build_usage = "yes"])
AM_CONDITIONAL([BUILD_DAR_STATIC], [test $build_static = "yes"])
AM_CONDITIONAL([DEBUG_STATIC], [test $debug_static = "yes" -a "$libdl_static_pb" != "yes"])
AM_CONDITIONAL([BUILD_MODE32], [test "$build_mode" = "32"])
AM_CONDITIONAL([BUILD_MODE64], [test "$build_mode" = "64"])
AM_CONDITIONAL([USE_UPX], [test "$upx" = "yes"])
AM_CONDITIONAL([USE_DOXYGEN], [test "$doxygen" = "yes"])
AM_CONDITIONAL([PROFILING], [test "$profiling" = "yes"])
AC_SUBST(UPX_PROG, [upx])
AC_SUBST(DOXYGEN_PROG, [doxygen])

# defaults
AC_PREFIX_DEFAULT(/usr/local)

AC_CONFIG_FILES([Makefile man/Makefile src/Makefile src/libdar/Makefile src/dar_suite/Makefile src/testing/Makefile src/examples/Makefile doc/Makefile doc/samples/Makefile misc/Makefile po/Makefile.in m4/Makefile po/Makefile intl/Makefile doc/mini-howto/Makefile ])
AC_OUTPUT
[echo ""
echo "--"
echo "dar and libdar have been successfully configured with the following parameters:"
echo ""
echo "  LIBDAR parameters:"

echo -n "   Zlib compression (gzip)    : "
if [ "$local_libz" = "yes" ] ; then
  echo "YES"
else
  echo "NO"
fi

echo -n "   Libbz2 compression (bzip2) : "
if [ "$local_libbz2" = "yes" ] ; then
  echo "YES"
else
  echo "NO"
fi

echo -n "   Strong encryption support  : "
if [ "$local_crypto" = "yes" ] ; then
  echo "YES"
else
  echo "NO"
fi

echo -n "   Extended Attributes support: "
if [ "$local_ea_support" = "yes" ] ; then
  echo "YES"
else
  echo "NO"
fi

echo -n "   Large files support (> 2GB): "
if [ ! -z "$ac_cv_sys_file_offset_bits" -o ! -z "$ac_cv_sys_large_files" ] ; then
  echo "YES"
else
  echo "NO"
fi

echo -n "   ext2fs NODUMP flag support : "
if [ "$local_nodump_feature" = "yes" ] ; then
  echo "YES"
else
  echo "NO"
fi

echo -n "   Special allocation scheme  : "
if [ "$local_special_alloc" = "yes" ] ; then
  echo "YES"
else
  echo "NO"
fi

echo -n "   Integer size used          : "
if [ -z "$build_mode" ] ; then
  echo "infinint"
else
  echo "$build_mode"
fi

echo -n "   Thread safe support        : "
if [ \( "$local_mutex_works" = "yes" -o ! "$local_special_alloc" = "yes" \) -a -z "$local_test_memory" ] ; then
  echo "YES"
else
  echo "NO"
fi


echo ""
echo "  DAR SUITE command line programs:"
echo -n "   Long options available : "
if [ "$local_have_getopt_long" = "yes" ] ; then
  echo "YES"
else
  echo "NO"
fi

echo -n "   Building examples      : "
if [ "$examples" = "yes" ] ; then
  echo "YES"
else
  echo "NO"
fi

echo -n "   Building dar_static    : "
if  [ "$build_static" = "yes" ]; then
  echo "YES"
else
  echo "NO"
fi

echo -n "   using upx at install   : "
if  [ "$upx" = "yes" ] ; then
  echo "YES"
else
  echo "NO"
fi

echo -n "   building documentation : "
if  [ "$doxygen" = "yes" ] ; then
  echo "YES"
else
  echo "NO"
fi


echo ""
]
