#!/bin/sh
set -e
#et -x

PATH=/usr/bin:/bin:/sbin:/usr/sbin:/usr/local/bin:/usr/local/sbin
export PATH
PROG=`basename $0`
PREFIX=/udir/wessels/dsc

NODE=$1; shift
DEST=$1; shift
LOGIN=$1; shift

SSH="/usr/local/bin/ssh"
ID="$PREFIX/etc/to-$DEST-certs/$NODE.id"

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

cd $PREFIX/run-$NODE/to-$DEST

#xec > $PROG.out
#xec 2>&1

k=`ls -r | grep \.xml$ | head -500` || true
test -n "$k" || exit 0
tar czf - $k \
	| $SSH -i $ID $LOGIN "dsc $NODE" \
	| while read marker md5 file x; do
		# ignore lines not beginning with the word "MD5".
		case "$marker" in
		MD5) ;;
		*) echo >&2 "not MD5 in '$marker'"; continue ;;
		esac

		# disallow / in returned filename.
		case "$file" in
		*/*) echo >&2 "slash in '$file'"; continue ;;
		esac

		# if the MD5 matches, remove the local file.
		if [ "$md5" = "`md5 < $file`" ]; then
			echo -n "$file "
		fi
	  done \
	| xargs rm

exit 0
