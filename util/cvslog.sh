#!/bin/sh
board="cvslog"
subject=$1

/home/bbs/bin/post $board "commit: $subject" `id -un` - > /dev/null
