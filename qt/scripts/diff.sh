#!/bin/bash
trap "exit" INT TERM ERR
trap "kill 0" EXIT

#
# 1. merge the upstream master
#
# 2. iterate over all ../src/ modified since qt-last-merge tag
#
# 3. if there is a corresponding qt/ port file, do three-way diff
#
# 4. after that tedious process, overwrite qt-last-merge tag
#

# sort to put .h first
#for x in $(ls ../src/$dir/*.{h,cpp} | sort -r); do
for x in $(git diff -r qt-last-merge | \
            grep '\-\-\- a/src' | sed 's#--- a/##' | \
            sort -r); do

   #y=`basename $x`;
   #y="core/$dir/$y";
   y=$(echo $x | sed s/src/core/)
   #echo $x
   #echo $y

   if [ -f $y ]; then
      echo diff ../$x $y
      diffuse -r qt-last-merge ../$x ../$x $y;
   fi;
done;
