#!/bin/sh
# $Id$

board="cvslog"
subject=$1

/usr/bin/mail -s "commit: $subject" pttbbs@oio.cx
/home/bbs/bin/post $board "commit: $subject" `id -un` - > /dev/null
