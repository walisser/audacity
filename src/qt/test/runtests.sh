#!/bin/bash

TARGET=desktop
QMAKE=qmake
PCH_PREFIX=
PCH_SUFFIX=.gch

if [ "$1" == "-android" ]; then
   TARGET=android
   export JAVA_HOME=/usr/lib/jvm/java-8-oracle
   export ANDROID_HOME=/opt/android-sdk
   export ANDROID_SDK_ROOT=/opt/android-sdk
   export ANDROID_NDK_ROOT=/opt/android-ndk/r16b
   QT_SDK_ROOT=/opt/qt/Qt-5.10.0/5.10.0/android_x86
   QMAKE=${QT_SDK_ROOT}/bin/qmake
   ANDROID_DEPLOY_QT=${QT_SDK_ROOT}/bin/androiddeployqt
   ADB=${ANDROID_SDK_ROOT}/platform-tools/adb
   PCH_PREFIX=lib
   PCH_SUFFIX=.so.gch
fi

OBJ=build/${TARGET}/obj
MOC=build/${TARGET}/moc
mkdir -p ${OBJ}/common.gch
mkdir -p ${MOC}

for x in TestException.pro TestSampleFormat.pro TestXMLValueChecker.pro; do
#for x in Test*.pro; do

   test=`echo $x | sed 's/\.pro//'`
   echo Building $test...g

   (
      # symlink precompiled header so we
      # compile it once for all tests
      (cd ${OBJ} && ln -sf common.gch $PCH_PREFIX$test$PCH_SUFFIX) &&
      ${QMAKE} $test.pro MOC_DIR=${MOC} OBJECTS_DIR=${OBJ} DESTDIR=build/${TARGET} &&
      make -j12 &&
      make install INSTALL_ROOT=build/${TARGET}/$test/
      
   ) || exit -1
  
done

for x in TestException.pro TestSampleFormat.pro TestXMLValueChecker.pro; do
#for x in *.pro; do

   test=`echo $x | sed 's/\.pro//'`

   echo "Running $test"

   if [ "$TARGET" == "android" ]; then

      (
      # FIXME: don't redeploy if nothing changed
      ${ANDROID_DEPLOY_QT} --input *$test*.json --output build/${TARGET}/$test/ --gradle --install &&
      
      ${ADB} shell am start -a android.intent.action.MAIN \
         -n org.qtproject.example.$test/org.qtproject.qt5.android.bindings.QtActivity
         
      ) || exit -2
   else

      #./$test || exit -2
      ./build/${TARGET}/$test
   fi
   
done
