#!/bin/bash

mkdir -p build/obj/common.gch
mkdir -p build/moc

for x in *.pro; do

   base=`echo $x | sed 's/\.pro//'`
   echo Building $base...

   (
      # symlink precompiled header so we
      # compile it once for all tests
      (cd build/obj && ln -sf common.gch $base.gch) &&
      qmake $base.pro &&
      make -j12
      
   ) || exit -1
  
done

for x in *.pro; do

   base=`echo $x | sed 's/\.pro//'`
   #./$base || exit -2
   ./$base
   
done
