#!/usr/bin/env bash

cat version.h | egrep -q '\+git'
if [ $? -eq 1 ]; then # Version is a release.
	exit
fi

[ ! -z `which git` ] && git status &> /dev/null && {
	LASTCOMMIT="`git log --oneline -1 | cut -b 1-7`"
	CURRENTBRANCH="`git branch | grep '^\*' | cut -b 3-`"
    [ -z "`git status --porcelain -uno`" ] || LOCALCHANGES="+local"
	echo ':'${LASTCOMMIT}${LOCALCHANGES}'(b:'${CURRENTBRANCH}')'
	exit
}

[ ! -z `which basename` ] && {
	DIRNAME="`basename $PWD`"
	if echo $DIRNAME | egrep -q '^superkb-[0-9a-f]{7}$'; then
		echo ':'`echo $DIRNAME | sed -re 's/^superkb-//g'`'(snapshot)'
		exit
	fi
}

echo ':unidentified'

