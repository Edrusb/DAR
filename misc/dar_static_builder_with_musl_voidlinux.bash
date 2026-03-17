#!/bin/bash

if [ ! -z "$1" ] ; then
   if [ "$1" -ge 1 ] ; then
       export MAKE_FLAGS="-j $1"
   else
       export MAKE_FLAGS=""
   fi
else
    echo "usage: $0 <num CPU cores to use>"
    exit 1
fi

# config/compilation/linking related variables

export LOCAL_PREFIX="/usr/local"

export LOCAL_PKG_CONFIG_DIR1="$LOCAL_PREFIX/lib/pkgconfig"
export LOCAL_PKG_CONFIG_DIR2="$LOCAL_PREFIX/lib64/pkgconfig"

export PKG_CONFIG_PATH="$LOCAL_PKG_CONFIG_DIR1:$LOCAL_PKG_CONFIG_DIR2:/usr/lib/pkgconfig:/usr/lib64/pkgconfig:/usr/local/lib/pkgconfig:/usr/local/lib64/pkgconfig"
export CFLAGS="-I$LOCAL_PREFIX/include -I/usr/local/include -I/usr/include -O2"
export CXXFLAGS="$CFLAGS"
export LDFLAGS="-L$LOCAL_PREFIX/lib -L$LOCAL_PREFIX/lib64 -L/usr/local/lib -L/usr/local/lib64 -L/usr/lib -L/usr/lib64 -L/lib -L/lib64"
export LD_LIBRARY_PATH="$LOCAL_PREFIX/lib:$LOCAL_PREFIX/lib64:/usr/local/lib:/usr/local/lib64:/usr/lib:/usr/lib64:/lib:/lib64"
export PATH="$LOCAL_PREFIX/bin:/usr/local/bin:$PATH"


# packages version

LIBTHREADAR_VERSION=1.6.1
LIBGCRYPT_VERSION=1.11.2
LIBRSYNC_VERSION=2.3.4
LIBGPG_ERROR_VERSION=1.59
GNUTLS_VERSION=3.8.12
LIBCURL_VERSION=8.19.0
LIBSSH_VERSION=0.11.4
LIBRHASH_VERSION=1.4.6
LIBZ_VERSION=1.3.2

# wget options need for gnutls website that does not provide all chain of trust in its certificate
GNUTLS_WGET_OPT="--no-check-certificate"
REPO="$LOCAL_PREFIX/src"

nok()
{
    echo "$*"
    exit 1
}

#
check()
{
    # checking we are under Void Linux
    if [ $(which lsb_release | wc -l) -eq 0 ] ; then
	echo "missing lsb_release command to check sanity of the environment"
	exit 1
    fi

    if [ "$(lsb_release -i | sed -rn -e 's/^Distributor ID:\s+//p')" != "VoidLinux" ] ; then
	echo "This script is only expected to work on VoidLinux Distribution"
	exit 1
    fi

    # checking for xbps-install
    if [ "$(which xbps-install | wc -l)" -eq 0 ] ; then
	echo "Missing xbps-install command"
	exit 1
    fi

    # checking musl libc is the system standard C library
    if [ $(xbps-query -l | grep musl | wc -l) -le 0 ] ; then
	echo "Cannot find musl standard C library"
	echo "static linking with glibc is broken, musl is needed"
	exit 1
    fi

    if [ ! -f configure -o ! -f configure.ac ] || [ $(grep DAR_VERSION configure.ac | wc -l) -ne 1 ] ; then
	echo "This script must be run from the root directory of the dar/libdar source package"
	exit 1
    fi

    if [ ! -e "${REPO}" ] ; then
	mkdir "${REPO}" || nok "Failed creating ${REPO}"
    fi

    if [ ! -d "${REPO}" ] ; then
       echo "${REPO} exists but is not a directory, aborting"
       exit 1
    fi

    if [ ! -e "${LOCAL_PKG_CONFIG_DIR1}" ] ; then
	mkdir -p "${LOCAL_PKG_CONFIG_DIR1}"  || nok "Failed creating ${LOCAL_PKG_CONFIG_DIR1}"
    fi

    if [ ! -e "${LOCAL_PKG_CONFIG_DIR2}" ] ; then
	mkdir -p "${LOCAL_PKG_CONFIG_DIR2}" ||  nok "Failed creating ${LOCAL_PKG_CONFIG_DI2}"
    fi

    if [ ! -d "${LOCAL_PKG_CONFIG_DIR1}" ] ; then
	echo "${LOCAL_PKG_CONFIG_DIR1} exists but is not a directory, aborting"
	exit 1
    fi

    if [ ! -d "${LOCAL_PKG_CONFIG_DIR2}" ] ; then
	echo "${LOCAL_PKG_CONFIG_DIR2} exists but is not a directory, aborting"
	exit 1
    fi
}

requirements()
{
    #updating xbps db
    xbps-install -Suy || (xbps-install -uy xbps && xbps-install -Suy) || return 1

    # tools used to build the different packages involved here
    xbps-install -yf gcc make wget pkg-config cmake xz || return 1

    #direct dependencies of libdar
    xbps-install -yf bzip2-devel e2fsprogs-devel libargon2-devel libgcc-devel liblz4-devel\
		 liblzma-devel libstdc++-devel libzstd-devel \
		 lzo-devel musl-devel || return 1

    # needed to build static flavor of librsync
    xbps-install -yf libb2-devel || return 1

    # needed to build static flavor of gnutls
    xbps-install -yf  nettle-devel libtasn1-devel libunistring-devel unbound-devel unbound || return 1

    # optional but interesting to get a smaller dar_static binary
    xbps-install -yf upx || return 1

    # need to tweak the hogweed.pc file provided by the system, we do not modify the system but shadow it by a local version located in higher priority dir
    HOGWEED_PC=/usr/lib/pkgconfig/hogweed.pc
    if [ -e "${HOGWEED_PC}" ] ; then
	sed -r -e 's/#\snettle/nettle/' < "${HOGWEED_PC}" > "${LOCAL_PKG_CONFIG_DIR1}/$(basename ${HOGWEED_PC})"
    else
	echo "${HOGWEED_PC} not found"
	return 1
    fi

    uname > /dev/null || nok "missing uname command, aborting"
}

libthreadar()
{
    local LIBTHREADAR_PKG=libthreadar-${LIBTHREADAR_VERSION}.tar.gz

    if [ ! -e "${REPO}/${LIBTHREADAR_PKG}" ] ; then wget "https://dar.edrusb.org/libthreadar/Releases/${LIBTHREADAR_PKG}" && mv "${LIBTHREADAR_PKG}" "${REPO}" || return 1 ; fi
    tar -xf "${REPO}/${LIBTHREADAR_PKG}" || return 1
    cd libthreadar-${LIBTHREADAR_VERSION} || return 1
    ./configure --prefix="$LOCAL_PREFIX" && make ${MAKE_FLAGS} || return 1
    make install || (cd .. ; return 1)
    cd ..
    ldconfig
    rm -rf libthreadar-${LIBTHREADAR_VERSION}
}

libgcrypt()
{
    local LIBGCRYPT_PKG=libgcrypt-${LIBGCRYPT_VERSION}.tar.bz2

    if [ ! -e "${REPO}/${LIBGCRYPT_PKG}" ] ; then wget https://www.gnupg.org/ftp/gcrypt/libgcrypt/${LIBGCRYPT_PKG} && mv "${LIBGCRYPT_PKG}" "${REPO}" || return 1 ; fi

    tar -xf "${REPO}/${LIBGCRYPT_PKG}" || return 1
    cd libgcrypt-${LIBGCRYPT_VERSION}
    ./configure --enable-static --prefix="$LOCAL_PREFIX" || return 1
    make ${MAKE_FLAGS} || return 1
    make ${MAKE_FLAGS} check || return 1
    make install || return 1
    cd ..
    ldconfig
    rm -rf libgcrypt-${LIBGCRYPT_VERSION}
}

libgpg-error()
{
    local LIBGPG_ERROR_PKG=libgpg-error-${LIBGPG_ERROR_VERSION}.tar.bz2

    if [ ! -e "${REPO}/${LIBGPG_ERROR_PKG}" ] ; then wget https://www.gnupg.org/ftp/gcrypt/libgpg-error/${LIBGPG_ERROR_PKG} && mv "${LIBGPG_ERROR_PKG}" "${REPO}" || return 1 ; fi
    tar -xf "${REPO}/${LIBGPG_ERROR_PKG}"  || return 1
    cd libgpg-error-${LIBGPG_ERROR_VERSION}  || return 1
    ./configure --enable-static --prefix="$LOCAL_PREFIX" || return 1
    make ${MAKE_FLAGS} || return 1
    make install || return 1
    cd ..
    ldconfig
    rm -rf libgpg-error-${LIBGPG_ERROR_VERSION}
}


librsync()
{
    local LIBRSYNC_PKG=v${LIBRSYNC_VERSION}.tar.gz

    if [ ! -e "${REPO}/${LIBRSYNC_PKG}" ] ; then wget "https://github.com/librsync/librsync/archive/refs/tags/${LIBRSYNC_PKG}" && mv "${LIBRSYNC_PKG}" "${REPO}" || return 1 ; fi
    tar -xf "${REPO}/${LIBRSYNC_PKG}" || return 1
    cd librsync-${LIBRSYNC_VERSION} || return 1
    cmake --install-prefix "$LOCAL_PREFIX" .  || return 1
    make ${MAKE_FLAGS} || return 1
    make install || return 1
    cmake -DBUILD_SHARED_LIBS=OFF . || return 1
    make ${MAKE_FLAGS} || return 1
    make install || return 1
    cd ..
    ldconfig
    rm -rf librsync-${LIBRSYNC_VERSION}
}

gnutls()
{
    local GNUTLS_PKG="gnutls-${GNUTLS_VERSION}.tar.xz"
    local REPODIR=$(echo ${GNUTLS_VERSION} | sed -rn -e 's/^([0-9]+\.[0-9]+).*/\1/p')

    if [ -z "$REPODIR" ] ; then echo "empty repo dir for guntls, check GNUTLS_VERSION is correct" ; return 1 ; fi
    if [ ! -e "${REPO}/${GNUTLS_PKG}" ] ; then wget ${GNUTLS_WGET_OPT} "https://www.gnupg.org/ftp/gcrypt/gnutls/v${REPODIR}/${GNUTLS_PKG}" && mv "${GNUTLS_PKG}" "${REPO}" || return 1 ; fi
    echo "extracting gnutls archive"
    tar -xf "${REPO}/${GNUTLS_PKG}" || return 1
    echo "chdir to gnutls dir"
    cd "gnutls-${GNUTLS_VERSION}" || return 1
    echo "unbound-anchor"

        # calling twice unbound anchor as the first time it may fails but not the second time
    unbound-anchor -a "/etc/unbound/root.key" || unbound-anchor -v -a "/etc/unbound/root.key" || return 1
    echo "configuring gnutls"
    ./configure --enable-static --without-p11-kit --disable-doc --prefix="$LOCAL_PREFIX" || return 1
    echo "running gnutls make"
    make ${MAKE_FLAGS} || return 1
    echo "installing gnutls"
    make install || return 1
    cd ..
    ldconfig
    rm -rf "gnutls-${GNUTLS_VERSION}"
}

libssh()
{
    local LIBSSH_PKG=libssh-${LIBSSH_VERSION}.tar.xz
    local REPODIR=$(echo ${LIBSSH_VERSION} | sed -rn -e 's/^([0-9]+\.[0-9]+).*/\1/p')
    local builddir="outofbuild-libssh"

    if [ ! -e "${REPO}/${LIBSSH_PKG}" ] ; then wget "https://www.libssh.org/files/${REPODIR}/${LIBSSH_PKG}" && mv "${LIBSSH_PKG}" "${REPO}" || return 1 ; fi
    tar -xf "${REPO}/${LIBSSH_PKG}" || return 1
    cd libssh-${LIBSSH_VERSION} || return 1
    mkdir ${builddir} || nok "Failed creating build dir for libssh"
    cd ${builddir} || return 1
    cmake -DUNIT_TESTING=OFF -DCMAKE_INSTALL_PREFIX="$LOCAL_PREFIX" -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=OFF -DWITH_GCRYPT=OFF ..  || return 1
    make ${MAKE_FLAGS} || return 1
    make install || return 1

    # fixing the libssh.pc to include -lcrypto as dependency
    local PKGCONFIGFILE="$LOCAL_PREFIX/lib64/pkgconfig/libssh.pc"
    if [ ! -f "$PKGCONFIGFILE" ] ; then
        echo "could not find file \"$PKGCONFIGFILE\" to patch it"
        exit 1
    else
        echo "patching \"$PKGCONFIGFILE\""
    fi
    cp "$PKGCONFIGFILE" "${PKGCONFIGFILE}.old"
    sed -e "s/-lssh/-lssh -lcrypto/" < "${PKGCONFIGFILE}.old" > "$PKGCONFIGFILE" && rm "${PKGCONFIGFILE}.old"

    cd ../.. || return 1
    ldconfig || return 1
    rm -rf libssh-${LIBSSH_VERSION} || return 1
    local needed_libs="-lgcrypt -lgpg-error"
    if [ -z "$LDFLAGS" ] ; then
	export LDFLAGS="${needed_libs}"
    else
	export LDFLAGS="${LDFLAGS} ${needed_libs}"
    fi
}

libcurl()
{
    local LIBCURL_PKG=curl-${LIBCURL_VERSION}.tar.bz2

    if [ ! -e "${REPO}/${LIBCURL_PKG}" ] ; then wget "https://curl.se/download/${LIBCURL_PKG}" && mv "${LIBCURL_PKG}" "${REPO}" || return 1 ; fi
    tar -xf "${REPO}/${LIBCURL_PKG}" || return 1
    cd curl-${LIBCURL_VERSION} || return 1
    ./configure --without-libidn2 --without-ssl --without-zlib --without-zstd --without-ca-bundle --without-gnutls --without-brotli --without-libpsl --without-libgsasl  --enable-shared --enable-static --without-libssh --without-libssh2 --without-librtmp --prefix="$LOCAL_PREFIX" || return 1
    make ${MAKE_FLAGS} || return 1
    make install || return 1
    cd ..
    ldconfig
    rm -rf curl-${LIBCURL_VERSION}
    misc/fix_libcurl_pc

    # fixing lame pkgconfig from libpsl used by libcurl
    sed -ri -e "s/-lpsl/-lpsl -lunistring/" /usr/lib/pkgconfig/libpsl.pc
}

librhash()
{
    local LIBRHASH_PKG=librhash-${LIBRHASH_VERSION}.tar.gz

    if [ ! -e "${REPO}/${LIBRHASH_PKG}" ] ; then wget "https://github.com/rhash/RHash/archive/refs/tags/v${LIBRHASH_VERSION}.tar.gz" && mv "v${LIBRHASH_VERSION}.tar.gz" "${REPO}"/"${LIBRHASH_PKG}" || return 1 ; fi
    tar -xf "${REPO}/${LIBRHASH_PKG}" || return 1
    cd "RHash-${LIBRHASH_VERSION}" || return 1
    ./configure --enable-static=librhash --prefix="$LOCAL_PREFIX" || return 1
    make ${MAKE_FLAGS} lib-static lib-shared || return 1
    make install-lib-static install-lib-shared || return 1
    cd ..
    ldconfig
    rm -rf "RHash-${LIBRHASH_VERSION}"
}

libz()
{
    local LIBZ_PKG=zlib-${LIBZ_VERSION}.tar.gz

    if [ ! -e "${REPO}/${LIBZ_PKG}" ] ; then wget "https://zlib.net/${LIBZ_PKG}" && mv "${LIBZ_PKG}" "${REPO}" || return 1 ; fi
    tar -xf "${REPO}/${LIBZ_PKG}" || return 1
    cd "zlib-${LIBZ_VERSION}" || return 1
    ./configure --prefix=/usr/local || return 1
    make ${MAKE_FLAGS} || return 1
    make ${MAKE_FLAGS} test || return 1
    make install
    cd ..
    ldconfig
    rm -rf "zlib-${LIBZ_VERSION}"
}

dar_static()
{
    #libpsl does not mention -lunistring which it relies on: updating pkgconfig file
#    local pkg_file=/usr/lib/pkgconfig/libpsl.pc
#    if [ -e "$pkg_file" ] ; then
#	cp "$pkg_file" "${pkg_file}.bak"
#	sed -r -e 's/-lpsl$/-lpsl -lunistring/' < "${pkg_file}.bak" > "$pkg_file"
#    else
#	echo "could not find pkg-config file for libpsl, and thus cannot patch it"
#    fi

    make clean || /bin/true
    make distclean || /bin/true
    ./configure --enable-static --disable-shared\
		--enable-libz-linking\
		--enable-libbz2-linking\
		--enable-liblzo2-linking\
		--enable-libxz-linking\
		--enable-libzstd-linking\
		--enable-liblz4-linking\
		--enable-libgcrypt-linking\
		--enable-librsync-linking\
		--enable-libcurl-linking\
		--enable-libssh-linking\
		--disable-build-html\
		--disable-gpgme-linking\
		--enable-threadar\
		--enable-libargon2-linking\
		--disable-python-binding\
		--enable-librhash-linking\
                --enable-dar-static\
		--enable-upx\
		--prefix="$LOCAL_PREFIX" || return 1
    make ${MAKE_FLAGS} || return 1
    make install-strip || return 1
    VERSION=$(${LOCAL_PREFIX}/bin/dar_static -V | sed -rn -e 's/.*dar_static version\s+([\.0-9]+),\s+Copyright.*/\1/p')
    OS=$(uname -o | sed -e 's#/#_#g')
    TARGET="dar_static_${VERSION}_$(uname -m)_${OS}"
    mkdir "$TARGET"
    cp ${LOCAL_PREFIX}/bin/*_static "$TARGET" && echo "static binaries are available in the directory $TARGET"
}

check

requirements || nok "Failed setting up requirements"
libz || nok "Failed building zlib"
libthreadar || nok "Failed building libthreadar"
libgpg-error || nok "Failed building libgpg-error static version"
libgcrypt || nok "Failed building libgcrypt"
librsync || nok "Failed building librsync"
gnutls || nok "Failed building gnutls"
libssh || nok "Failed building libssh"
libcurl || nok "Failed building libcurl"
librhash || nok "Failed building librhash"
dar_static || nok "Failed building dar_static"
