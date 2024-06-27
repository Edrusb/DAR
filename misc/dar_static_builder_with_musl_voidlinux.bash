#!/bin/bash

if [ ! -z "$1" ] && [ "$1" -gt 1 ] ; then
    export MAKE_FLAGS="-j $1"
else
    echo "usage: $0 <num CPU cores to use>"
    exit 1
fi

# config/compilation/linking related variables

export LOCAL_PKG_CONFIG_DIR="$(pwd)/pkgconfig"

export MAKE_FLAGS="-j 8"
export PKG_CONFIG_PATH="${LOCAL_PKG_CONFIG_DIR}":/usr/local/lib/pkgconfig:/lib/pkgconfig:/usr/lib/pkgconfig
export LD_LIBRARY_PATH=/usr/local/lib:/usr/local/lib64:/usr/lib

#export LDFLAGS=-L/usr/local/lib
#export CFLAGS=-I/usr/local/include
#export CXXFLAGS=${CFLAGS}

# packages version

LIBTHREADAR_VERSION=1.4.0
LIBRSYNC_VERSION=2.3.2
LIBGPG_ERROR_VERSION=1.45
GNUTLS_VERSION=3.7.8
LIBCURL_VERSION=7.86.0
LIBP11KIT_VERSION=0.24.1
LIBSSH_VERSION=0.10.3
LIBSSH2_VERSION=1.10.0

# define whether to use libssh or libssh2
# note that both libraries do support ssh version 2 protocol,
# libssh even supports more recent ciphers than libssh2
LIBSSH=1
# LIBSSH=2


# wget options need for gnutls website that does not provide all chain of trust in its certificate
GNUTLS_WGET_OPT="--no-check-certificate"
REPO=$(pwd)/REPO

#
check()
{
    # checking we are under Void Linux
    if [ $(which lsb_release | wc -l) -eq 0 ] ; then
	echo "missing lsb_release command to check sanity of the environment"
	exit 1
    fi

    if [ "$(lsb_release -i | sed -rn -e 's/^Distributor ID:\s+//p')" != "VoidLinux" ] ; then
	echo "THis script is only expected to work on VoidLinux Distribution"
	exit 1
    fi

    # checking for xbps-install
    if [ "$(which xbps-install | wc -l)" -eq 0 ] ; then
	echo "Missing xbps-instal command"
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
	mkdir "${REPO}"
    fi

    if [ ! -d "${REPO}" ] ; then
       echo "${REPO} exists but is not a directory, aborting"
       exit 1
    fi

    if [ ! -e "${LOCAL_PKG_CONFIG_DIR}" ] ; then
	mkdir "${LOCAL_PKG_CONFIG_DIR}"
    fi

    if [ ! -d "${LOCAL_PKG_CONFIG_DIR}" ] ; then
	echo "${LOCAL_PKG_CONFIG_DIR} exists but is not a directory, aborting"
	exit 1
    fi
}

requirements()
{

    #updating xbps db
    xbps-install -SU -y

    # tools used to build the different packages involved here
    xbps-install -y gcc make wget pkg-config cmake xz || exit 1

    #direct dependencies of libdar
    xbps-install -y bzip2-devel e2fsprogs-devel libargon2-devel libgcc-devel libgcrypt-devel liblz4-devel \
		 liblzma-devel libstdc++-devel libzstd-devel lz4-devel \
		 lzo-devel musl-devel zlib-devel || exit 1

    # needed to build static flavor of librsync
    xbps-install -y libb2-devel || exit 1

    # needed to build static flavor of gnutls
    xbps-install -y  nettle-devel libtasn1-devel libunistring-devel unbound-devel unbound || exit 1

    #needed for static flavor of libcurl
    xbps-install -y libssp-devel || echo "ignoring error if libssp-devel fails to install due to musl-devel already installed"

    # need to tweak the hogweed.pc file provided by the system, we do not modify the system but shadow it by a local version located in higher priority dir
    HOGWEED_PC=/usr/lib/pkgconfig/hogweed.pc
    if [ -e "${HOGWEED_PC}" ] ; then
	sed -r -e 's/#\snettle/nettle/' < "${HOGWEED_PC}" > "${LOCAL_PKG_CONFIG_DIR}/$(basename ${HOGWEED_PC})"
    else
	echo "${HOGWEED_PC} not found"
	exit 1
    fi

    # optional but interesting to get a smaller dar_static binary
    xbps-install -y upx || echo "" && echo "WARNING!" && echo "Failed to install upx, will do without" && echo && sleep 3

}

libthreadar()
{
    local LIBTHREADAR_PKG=libthreadar-${LIBTHREADAR_VERSION}.tar.gz

    if [ ! -e "${REPO}/${LIBTHREADAR_PKG}" ] ; then wget "https://dar.edrusb.org/libthreadar/Releases/${LIBTHREADAR_PKG}" && mv "${LIBTHREADAR_PKG}" "${REPO}" || exit 1 ; fi
    tar -xf "${REPO}/${LIBTHREADAR_PKG}" || exit 1
    cd libthreadar-${LIBTHREADAR_VERSION} || exit 1
    ./configure && make ${MAKE_FLAGS} || exit 1
    make install
    cd ..
    ldconfig
    rm -rf libthreadar-${LIBTHREADAR_VERSION}
}

libgpg-error()
{
    local LIBGPG_ERROR_PKG=libgpg-error-${LIBGPG_ERROR_VERSION}.tar.bz2

    if [ ! -e "${REPO}/${LIBGPG_ERROR_PKG}" ] ; then wget https://www.gnupg.org/ftp/gcrypt/libgpg-error/${LIBGPG_ERROR_PKG} && mv "${LIBGPG_ERROR_PKG}" "${REPO}" || exit 1 ; fi
    tar -xf "${REPO}/${LIBGPG_ERROR_PKG}"
    cd libgpg-error-${LIBGPG_ERROR_VERSION}
    ./configure --enable-static
    make ${MAKE_FLAGS}
    make install
    cd ..
    ldconfig
    rm -rf libgpg-error-${LIBGPG_ERROR_VERSION}
}


librsync()
{
    local LIBRSYNC_PKG=v${LIBRSYNC_VERSION}.tar.gz

    if [ ! -e "${REPO}/${LIBRSYNC_PKG}" ] ; then wget "https://github.com/librsync/librsync/archive/refs/tags/${LIBRSYNC_PKG}" && mv "${LIBRSYNC_PKG}" "${REPO}" || exit 1 ; fi
    tar -xf "${REPO}/${LIBRSYNC_PKG}"
    cd librsync-${LIBRSYNC_VERSION}
    cmake .
    make ${MAKE_FLAGS}
    make install
    cmake -DBUILD_SHARED_LIBS=OFF .
    make ${MAKE_FLAGS}
    make install
    cd ..
    ldconfig
    rm -rf librsync-${LIBRSYNC_VERSION}
}

gnutls()
{
    local GNUTLS_PKG="gnutls-${GNUTLS_VERSION}.tar.xz"
    local REPODIR=$(echo ${GNUTLS_VERSION} | sed -rn -e 's/^([0-9]+\.[0-9]+).*/\1/p')

    if [ -z "$REPODIR" ] ; then echo "empty repo dir for guntls, check GNUTLS_VERSION is correct" ; exit 1 ; fi
    if [ ! -e "${REPO}/${GNUTLS_PKG}" ] ; then wget ${GNUTLS_WGET_OPT} "https://www.gnupg.org/ftp/gcrypt/gnutls/v${REPODIR}/${GNUTLS_PKG}" && mv "${GNUTLS_PKG}" "${REPO}" || exit 1 ; fi
    tar -xf "${REPO}/${GNUTLS_PKG}"
    cd "gnutls-${GNUTLS_VERSION}"
    ./configure --enable-static --without-p11-kit
    unbound-anchor -a "/etc/unbound/root.key"
    make ${MAKE_FLAGS}
    make install
    cd ..
    ldconfig
    rm -rf "gnutls-${GNUTLS_VERSION}"
}

libssh()
{
    local LIBSSH_PKG=libssh-${LIBSSH_VERSION}.tar.xz
    local REPODIR=$(echo ${LIBSSH_VERSION} | sed -rn -e 's/^([0-9]+\.[0-9]+).*/\1/p')
    local builddir="outofbuild-libssh"

    if [ ! -e "${REPO}/${LIBSSH_PKG}" ] ; then wget "https://www.libssh.org/files/${REPODIR}/${LIBSSH_PKG}" && mv "${LIBSSH_PKG}" "${REPO}" || exit 1 ; fi
    tar -xf "${REPO}/${LIBSSH_PKG}"
    cd libssh-${LIBSSH_VERSION}
    mkdir ${builddir} || (echo "Failed creating build dir for libssh" && return 1)
    cd ${builddir}
    cmake -DUNIT_TESTING=OFF -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=OFF -DWITH_GCRYPT=ON ..
    make ${MAKE_FLAGS}
    make install
    cd ../..
    ldconfig
    rm -rf libssh-${LIBSSH_VERSION}
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

    if [ ! -e "${REPO}/${LIBSSH2_PKG}" ] ; then wget "https://www.libssh2.org/download/${LIBSSH2_PKG}" && mv "${LIBSSH2_PKG}" "${REPO}" || exit 1 ; fi
    tar -xf "${REPO}/${LIBSSH2_PKG}"
    cd libssh2-${LIBSSH2_VERSION}
    ./configure --enable-shared --without-libssl --with-libz --with-crypto=libgcrypt
    make ${MAKE_FLAGS}
    make install
    cd ..
    ldconfig
    rm -rf libssh2-${LIBSSH2_VERSION}
}

libcurl()
{
    local LIBCURL_PKG=curl-${LIBCURL_VERSION}.tar.bz2

    if [ ! -e "${REPO}/${LIBCURL_PKG}" ] ; then wget "https://curl.se/download/${LIBCURL_PKG}" && mv "${LIBCURL_PKG}" "${REPO}" || exit 1 ; fi
    tar -xf "${REPO}/${LIBCURL_PKG}"
    cd curl-${LIBCURL_VERSION}
    ./configure --with-gnutls ${SSH_CURL_OPT} --disable-shared
    make ${MAKE_FLAGS}
    make install
    cd ..
    ldconfig
    rm -rf curl-${LIBCURL_VERSION}
}

dar_static()
{
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
		--prefix=/DAR || exit 1
    make ${MAKE_FLAGS} || exit 1
    make DESTDIR=${tmp_dir} install-strip || exit 1
    mv ${tmp_dir}/DAR/bin/dar_static . && echo "dar_static binary is available in the current directory"
    rm -rf ${tmp_dir}/DAR
}

check
requirements || (echo "Failed setting up requirements" && exit 1)
libthreadar || (echo "Failed building libthreadar" && exit 1)
libgpg-error || (echo "Failed building libgpg-error static version" && exit 1)
librsync || (echo "Failed building librsync" && exit 1)
gnutls || (echo "Failed building gnutls" && exit 1)
case "$LIBSSH" in
    "1")
	libssh || (echo "Failed building libssh" && exit 1)
	SSH_CURL_OPT="--with-libssh"
	;;
    "2")
	libssh2 || (echo "Failed building libssh2" && exit 1)
	SSH_CURL_OPT="--with-libssh2"
	;;
    *)
	echo "Unknow ssh library requested to build, aborting"
	exit 1
esac
libcurl || (echo "Failed building libcurl" && exit 1)
dar_static || (echo "Failed building dar_static" && exit 1)
