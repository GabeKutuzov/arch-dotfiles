#!/bin/bash

for f in *.mp3
do
   echo "Removing tags for mp3 file - $f" 
   `mp3info $f -d`
   echo "Removing non-digits from file - $f"
   NUMBER=$(echo $f | grep -o -E '[0-9].')
   echo $NUMBER
done
exit 0
