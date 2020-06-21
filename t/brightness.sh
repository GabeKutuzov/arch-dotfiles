#!/bin/bash
set -e
file="/sys/class/backlight/intel_backlight/brightness"
current=$(cat "$file")
new="$current"
if [ "$1" = "-init" ]
then
   sudo chmod 666 /sys/class/backlight/intel_backlight/brightness
fi
if [ "$1" = "-inc" ]
then
    new=$(( current + $2 ))
fi
if [ "$1" = "-dec" ]
then
    new=$(( current - $2 ))
fi
if [ "$1" = "-set" ]
then
    new=$(( $2 ))
fi
echo "$new" | tee "$file"
