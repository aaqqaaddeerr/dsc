#!/bin/sh
set -e
#et -x

PATH=/usr/bin:/bin:/sbin:/usr/sbin:/usr/local/bin:/usr/local/sbin
export PATH
PROG=`basename $0`
PREFIX=/usr/local/dsc

NODE=$1; shift
DEST=$1; shift
URI=$1; shift

CURL="curl --silent"
CLTAUTH="--cert $PREFIX/certs/$DEST/$NODE.pem"
SRVAUTH="--cacert $PREFIX/certs/$DEST/cacert.pem"

PIDF="/tmp/$PROG-$NODE-$DEST.pid"
if test -f $PIDF; then
	PID=`cat $PIDF`
	if kill -0 $PID ; then
		exit 0
	else
		true
	fi
fi
echo $$ >$PIDF
trap "rm -f $PIDF" EXIT

perl -e 'sleep((rand 10) + 5)'

cd $PREFIX/run/$NODE/upload/$DEST

exec > $PROG.out
exec 2>&1

k=`ls -r | grep xml$ | head -500` || true
test -n "$k" || exit 0
md5 -r $k > MD5s
TF=`mktemp /tmp/put.XXXXXXXXXXXXX`
tar czf $TF MD5s $k
mv $TF $TF.tar
TF="$TF.tar"
UPLOAD="--upload $TF"

set +e
$CURL $SRVAUTH $CLTAUTH $UPLOAD $URI | awk '$1 == "Stored" {print $2}' | xargs rm -v
rm -f $TF
