#!/bin/sh

if [ "$1" = "" ] ; then
  echo "usage $0: <filename>"
  exit 1
fi

CWD=`pwd`

if [ ! -x ../dar_suite/dar ] ; then
  echo "********************************************"
  echo ""
  echo "Dar is not built, aborting"
  echo ""
  echo "********************************************"
  exit 2
else
  export DAR=$CWD/../dar_suite/dar
fi

if [ ! -x ../dar_suite/dar_slave ] ; then
  echo "********************************************"
  echo ""
  echo "Dar_slave is not built, aborting"
  echo ""
  echo "********************************************"
  exit 2
else
  export DAR_SLAVE=$CWD/../dar_suite/dar_slave
fi

if [ ! -x ../dar_suite/dar_xform ] ; then
  echo "********************************************"
  echo ""
  echo "Dar_xform is not built, aborting"
  echo ""
  echo "********************************************"
  exit 2
else
  export DAR_XFORM=$CWD/../dar_suite/dar_xform
fi

if [ ! -x ./all_features ] ; then
  echo "********************************************"
  echo ""
  echo "all_features program not built, aborting"
  echo ""
  echo "********************************************"
  exit 2
fi

if [ `id -u` -ne 0 ]; then
  echo "********************************************"
  echo ""
  echo "need to be run as root"
  echo ""
  echo "********************************************"
  exit 3
fi

if [ -z "$DAR_KEY" ] ; then
    echo "You need to set the environmental variable"
    echo "DAR_KEY to an email for which you have public"
    echo "and private key available for encryption and"
    echo "signature."
    echo "You can use the GNUPGHOME variable to"
    echo "point to another keyring than ~root/.gnupg"
    echo ""
    echo "Example of use with bash:"
    echo "export DAR_KEY=your.email@your.domain"
    echo "export GNUPGHOME=~me/.gnupg"
    echo ""
    echo "Example of use with tcsh:"
    echo "setenv DAR_KEY your.email@your.domain"
    echo "setenv GNUPGHOME ~me/.gnupg"
  exit 3
fi

if [ -z "$DAR_SFTP_REPO" -o -z "$DAR_FTP_REPO" ] ; then
    echo "You need to set the environment variables"
    echo "DAR_SFTP_REPO and DAR_FTP_REPO with an URL where"
    echo "to upload and download file using SFTP and FTP"
    echo "respectively, for example sftp://login:pass@host/some/path"
    echo "and ftp://login:pass@host/some/path respectively"
    exit 3
fi

if ./all_features ; then
  echo "OK, all required features are available for testing"
else
  exit 3
fi

echo "Generating file $1"

cat > "$1" <<EOF
export DAR="$DAR"
export DAR_SLAVE="$DAR_SLAVE"
export DAR_XFORM="$DAR_XFORM"
export DAR_SFTP_REPO="$DAR_SFTP_REPO"
export DAR_FTP_REPO="$DAR_FTP_REPO"
EOF
