#!/bin/bash

if [ ! -z "$1" ] ; then
   if [ "$1" -gt 1 ] ; then
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

LIBTHREADAR_VERSION=1.6.0
LIBGCRYPT_VERSION=1.11.2
LIBRSYNC_VERSION=2.3.4
LIBGPG_ERROR_VERSION=1.55
GNUTLS_VERSION=3.8.9
LIBCURL_VERSION=8.14.1
LIBSSH_VERSION=0.11.2
LIBSSH2_VERSION=1.11.1
LIBRHASH_VERSION=1.4.6

# define whether to use libssh or libssh2
# note that both libraries do support ssh version 2 protocol,
# libssh even supports more recent ciphers than libssh2
LIBSSH=1
# LIBSSH=2


# wget options need for gnutls website that does not provide all chain of trust in its certificate
GNUTLS_WGET_OPT="--no-check-certificate"
REPO="$LOCAL_PREFIX/src"

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
	mkdir "${REPO}" || (echo "Failed creating ${REPO}" ; return 1)
    fi

    if [ ! -d "${REPO}" ] ; then
       echo "${REPO} exists but is not a directory, aborting"
       exit 1
    fi

    if [ ! -e "${LOCAL_PKG_CONFIG_DIR1}" ] ; then
	mkdir -p "${LOCAL_PKG_CONFIG_DIR1}"  || (echo "Failed creating ${LOCAL_PKG_CONFIG_DIR1}" ; exit 1)
    fi

    if [ ! -e "${LOCAL_PKG_CONFIG_DIR2}" ] ; then
	mkdir -p "${LOCAL_PKG_CONFIG_DIR2}" ||  (echo "Failed creating ${LOCAL_PKG_CONFIG_DI2}" ; exit 1)
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
    xbps-install -SUy || (xbps-install -uy xbps && xbps-install -SUy) || return 1

    # tools used to build the different packages involved here
    xbps-install -y gcc make wget pkg-config cmake xz || return 1

    #direct dependencies of libdar
    xbps-install -y bzip2-devel e2fsprogs-devel libargon2-devel libgcc-devel liblz4-devel \
		 liblzma-devel libstdc++-devel libzstd-devel lz4-devel \
		 lzo-devel musl-devel zlib-devel || return 1

    # needed to build static flavor of librsync
    xbps-install -y libb2-devel || return 1

    # needed to build static flavor of gnutls
    xbps-install -y  nettle-devel libtasn1-devel libunistring-devel unbound-devel unbound || return 1

    #needed for static flavor of libcurl
    xbps-install -y libpsl-devel libidn2-devel || return 1

    # optional but interesting to get a smaller dar_static binary
    xbps-install -y upx || return 1

    # need to tweak the hogweed.pc file provided by the system, we do not modify the system but shadow it by a local version located in higher priority dir
    HOGWEED_PC=/usr/lib/pkgconfig/hogweed.pc
    if [ -e "${HOGWEED_PC}" ] ; then
	sed -r -e 's/#\snettle/nettle/' < "${HOGWEED_PC}" > "${LOCAL_PKG_CONFIG_DIR1}/$(basename ${HOGWEED_PC})"
    else
	echo "${HOGWEED_PC} not found"
	return 1
    fi
}

libthreadar()
{
    local LIBTHREADAR_PKG=libthreadar-${LIBTHREADAR_VERSION}.tar.gz

    if [ ! -e "${REPO}/${LIBTHREADAR_PKG}" ] ; then wget "https://dar.edrusb.org/libthreadar/Releases/${LIBTHREADAR_PKG}" && mv "${LIBTHREADAR_PKG}" "${REPO}" || return 1 ; fi
    tar -xf "${REPO}/${LIBTHREADAR_PKG}" || return 1
    cd libthreadar-${LIBTHREADAR_VERSION} || return 1
    ./configure --prefix="$LOCAL_PREFIX" && make ${MAKE_FLAGS} || (cd .. ; return 1)
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
    ./configure --enable-static --prefix="$LOCAL_PREFIX" || (cd .. : return 1)
    make ${MAKE_FLAGS} || (cd .. : return 1)
    make check || (cd .. : return 1)
    make install || (cd .. : return 1)
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
    ./configure --enable-static --prefix="$LOCAL_PREFIX" || (cd .. ; return 1)
    make ${MAKE_FLAGS} || (cd .. ; return 1)
    make install || (cd .. ; return 1)
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
    tar -xf "${REPO}/${GNUTLS_PKG}" || return 1
    cd "gnutls-${GNUTLS_VERSION}" || return 1
    unbound-anchor -a "/etc/unbound/root.key" || (cd .. ; return 1)
    ./configure --enable-static --without-p11-kit --prefix="$LOCAL_PREFIX" || (cd .. ; return 1)
    make ${MAKE_FLAGS} || (cd .. ; return 1)
    make install || (cd .. ; return 1)
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
    mkdir ${builddir} || (echo "Failed creating build dir for libssh" ; cd .. ; return 1)
    cd ${builddir} || (cd .. ; return 1)
    cmake -DUNIT_TESTING=OFF -DCMAKE_INSTALL_PREFIX="$LOCAL_PREFIX" -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=OFF -DWITH_GCRYPT=ON  ..  || (cd ../.. ; return 1)
    make ${MAKE_FLAGS} || (cd ../.. ; return 1)
    make install || (cd ../.. ; return 1)
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

libssh2()
{
    local LIBSSH2_PKG=libssh2-${LIBSSH2_VERSION}.tar.gz

    if [ ! -e "${REPO}/${LIBSSH2_PKG}" ] ; then wget "https://www.libssh2.org/download/${LIBSSH2_PKG}" && mv "${LIBSSH2_PKG}" "${REPO}" || return 1 ; fi
    tar -xf "${REPO}/${LIBSSH2_PKG}" || return 1
    cd libssh2-${LIBSSH2_VERSION} || return 1
    ./configure --enable-shared --without-libssl --with-libz --with-crypto=libgcrypt --prefix="$LOCAL_PREFIX" || (cd .. ; return 1)
    make ${MAKE_FLAGS} || (cd .. ; return 1)
    make install || (cd .. ; return 1)
    cd ..
    ldconfig
    rm -rf libssh2-${LIBSSH2_VERSION}
}

libcurl()
{
    local LIBCURL_PKG=curl-${LIBCURL_VERSION}.tar.bz2

    if [ ! -e "${REPO}/${LIBCURL_PKG}" ] ; then wget "https://curl.se/download/${LIBCURL_PKG}" && mv "${LIBCURL_PKG}" "${REPO}" || return 1 ; fi
    tar -xf "${REPO}/${LIBCURL_PKG}" || return 1
    cd curl-${LIBCURL_VERSION} || return 1
    ./configure --with-gnutls ${SSH_CURL_OPT} --enable-shared --enable-static --prefix="$LOCAL_PREFIX" || (cd .. : return 1)
    make ${MAKE_FLAGS} || (cd .. ; return 1)
    make install || (cd . ; return 1)
    cd ..
    ldconfig
    rm -rf curl-${LIBCURL_VERSION}
    misc/fix_libcurl_pc
}

librhash()
{
    local LIBRHASH_PKG=librhash-${LIBRHASH_VERSION}.tar.gz

    if [ ! -e "${REPO}/${LIBRHASH_PKG}" ] ; then wget "https://github.com/rhash/RHash/archive/refs/tags/v${LIBRHASH_VERSION}.tar.gz" && mv "v${LIBRHASH_VERSION}.tar.gz" "${REPO}"/"${LIBRHASH_PKG}" || return 1 ; fi
    tar -xf "${REPO}/${LIBRHASH_PKG}" || return 1
    cd "RHash-${LIBRHASH_VERSION}" || return 1
    ./configure --enable-static=librhash --prefix="$LOCAL_PREFIX" || (cd .. ; return 1)
    make ${MAKE_FLAGS} lib-static lib-shared || (cd .. ; return 1)
    make install-lib-static install-lib-shared || (cd .. ; return 1)
    cd ..
    ldconfig
    rm -rf "RHash-${LIBRHASH_VERSION}"
}

dar_static()
{
    #libpsl does not mention -lunistring which it relies on: updating pkgconfig file
    local pkg_file=/usr/lib/pkgconfig/libpsl.pc
    if [ -e "$pkg_file" ] ; then
	cp "$pkg_file" "${pkg_file}.bak"
	sed -r -e 's/-lpsl$/-lpsl -lunistring/' < "${pkg_file}.bak" > "$pkg_file"
    else
	echo "could not find pkg-config file for libpsl, and thus cannot patch it"
    fi

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
		--disable-build-html\
		--disable-gpgme-linking\
		--enable-threadar\
		--enable-libargon2-linking\
		--disable-python-binding\
		--enable-librhash-linking\
                --enable-dar-static\
		--prefix="$LOCAL_PREFIX" || return 1
    make ${MAKE_FLAGS} || return 1
    make install-strip || return 1
    cp ${LOCAL_PREFIX}/bin/dar_static . && echo "dar_static binary is available in the current directory"
}

check

requirements || (echo "Failed setting up requirements" ; exit 1)
libthreadar || (echo "Failed building libthreadar" ; exit 1)
libgcrypt || (echo "Failed building libgcrypt" ; exit 1)
libgpg-error || (echo "Failed building libgpg-error static version" ; exit 1)
librsync || (echo "Failed building librsync" ; exit 1)
gnutls || (echo "Failed building gnutls" ; exit 1)
case "$LIBSSH" in
    "1")
	libssh || (echo "Failed building libssh" ; exit 1)
	export SSH_CURL_OPT="--with-libssh"
	;;
    "2")
	libssh2 || (echo "Failed building libssh2" ; exit 1)
	export SSH_CURL_OPT="--with-libssh2"
	;;
    *)
	echo "Unknow ssh library requested to build, aborting"
	exit 1
esac
if [ -z "$SSH_CURL_OPT" ] ; then
    echo "SSH_CURL_OPT is not defined, but is needed to compile libcurl"
    exit 1;
fi
libcurl || (echo "Failed building libcurl" ; exit 1)
librhash || (echo "Failed building librhash" ; exit 1)
dar_static || (echo "Failed building dar_static" ; exit 1)
