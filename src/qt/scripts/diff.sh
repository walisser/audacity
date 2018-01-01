#!/bin/bash

# empty string puts extra / in the path which is OK on unixen
for dir in "" blockfile xml; do

   # sort to put .h first
   for x in $(ls ../../$dir/*.{h,cpp} | sort -r); do
      
      y=`basename $x`;
      if [ -f ../../qt/core/$dir/$y ]; then
         echo diff $y
         diffuse $x ../../qt/core/$dir/$y;
      fi;
   done;

done;



