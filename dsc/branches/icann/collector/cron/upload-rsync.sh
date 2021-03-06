#!/bin/sh
set -e
#et -x

PATH=/usr/bin:/bin:/sbin:/usr/sbin:/usr/local/bin:/usr/local/sbin
export PATH
PROG=`basename $0`
PREFIX=/usr/local/dsc

NODE=$1; shift
DEST=$1; shift
RPATH=$1; shift

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

cd $PREFIX/var/run/$NODE/upload/$DEST

YYYYMMDD=`ls | grep '^[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]$' | head -1`
test -n "$YYYYMMDD" || exit 0
cd $YYYYMMDD

exec > $PROG.out
exec 2>&1

if test -f $HOME/.ssh/dsc_uploader_id ; then
	RSYNC_RSH="ssh -i $HOME/.ssh/dsc_uploader_id"
	export RSYNC_RSH
fi

k=`ls -r | grep xml$ | head -500` || true
test -n "$k" || exit 0
md5 $k > MD5s
rsync -av MD5s $k $RPATH | grep '\.xml$' | xargs rm -v

cd ..; rmdir $YYYYMMDD 2>/dev/null
