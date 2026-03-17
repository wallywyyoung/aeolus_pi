#!/bin/sh
case "$1" in
    start)
        # Shairport-sync is starting playback
        killall -STOP organ_software || killall organ_software
        ;;
    stop)
        /etc/init.d/S99organ start
        ;;
esac