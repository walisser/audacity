#!/bin/bash
trap "exit" INT TERM ERR
trap "kill 0" EXIT

export JAVA_HOME=/usr/lib/jvm/java-8-oracle
export ANDROID_HOME=/opt/android-sdk
export ANDROID_SDK_ROOT=/opt/android-sdk
export ANDROID_NDK_ROOT=/opt/android-ndk/r16b

ADB=${ANDROID_SDK_ROOT}/platform-tools/adb

if [ "$1" == "-clean" ]; then
   rm -rf android*.json
   rm -rf build
   exit 0
elif [ "$1" == "-logcat" ]; then

   multitail -l "${ADB} -s emulator-5554 logcat -v color 'QTestLib:*' '*:S'" -l "${ADB} -s 092a33f3 logcat -v color 'QTestLib:*' '*:S'"
   exit 0
elif [ "$1" == "-coverage" ]; then

   (
      source qmake.vars || exit -1
      mkdir -p "${DESTDIR}/lcov" &&
      lcov -d "${OBJECTS_DIR}" -c -o "${DESTDIR}/lcov/coverage.info"


      lcov --remove "${DESTDIR}/lcov/coverage.info" '/qt/test/build/*'     -o "${DESTDIR}/lcov/coverage.info"
      lcov --remove "${DESTDIR}/lcov/coverage.info" '/usr/include/*'       -o "${DESTDIR}/lcov/coverage.info"
      lcov --remove "${DESTDIR}/lcov/coverage.info" '/usr/local/include/*' -o "${DESTDIR}/lcov/coverage.info"

      #lcov --list-full-path -e ${1}/coverage.info ${2} -o ${1}/coverage-stripped.info

      genhtml --title "audacity/qt/test/${DESTDIR}" --demangle-cpp -o "${DESTDIR}/lcov" "${DESTDIR}/lcov/coverage.info"
   )

   exit 0
fi

TARGET=`uname -m`
BUILD=default
QMAKE=qmake
PCH_PREFIX=
PCH_SUFFIX=.gch

for arg in $@; do
   # look for arg that looks like Qt SDK target
   if [[ "$arg" == "android_"* ]]; then
      TARGET=$arg
      BUILD=android
      DEVICE="emulator-5554"
      #export QMAKESPEC=android-clang
      if [ "$1" == "android_armv7" ]; then
         DEVICE="092a33f3"
      fi
      QT_SDK_ROOT=/opt/qt/Qt-5.10.0/5.10.0/${TARGET}
      QMAKE=${QT_SDK_ROOT}/bin/qmake
      ANDROID_DEPLOY_QT=${QT_SDK_ROOT}/bin/androiddeployqt
      PCH_PREFIX=lib
      PCH_SUFFIX=.so.gch

   # do MXE build, part after "_" is the cpu part of MXE target
   elif [[ "$arg" == "mxe_"* ]]; then
      TARGET=$arg
      BUILD=windows
      DEVICE=""
      MXE_PREFIX=$(echo ${TARGET} | sed s/mxe_//)
      QMAKE="${MXE_PREFIX}-w64-mingw32.static-qmake-qt5 CONFIG+=mxe"

   # if arg is like TestFoo.pro then do manual test selection
   elif [[ "$arg" == "Test"*.pro ]]; then
      TESTS="${TESTS} $arg"
   else
      echo "I don't now what argument [$arg]" is for...
      exit -1
   fi

done

# if no tests specified, run them all
if [[ "x${TESTS}" == "x" ]]; then
   TESTS="Test*.pro"
fi

for x in ${TESTS}; do

   test=`echo $x | sed 's/\.pro//'`
   echo Building $test...g

   (
      # configure project
      ${QMAKE} $test.pro &&

      # import variables from qmake
      source qmake.vars &&

      # symlink precompiled header to share it and compile once
      (
         cd ${OBJECTS_DIR} &&
         rm -rfv $PCH_PREFIX$test$PCH_SUFFIX &&
         mkdir -pv shared.gch &&
         ln -sfv shared.gch $PCH_PREFIX$test$PCH_SUFFIX
      ) &&

      make -j12 &&

      # FIXME: install is forced even if there was no change
      make install INSTALL_ROOT="${DESTDIR}/$test/"

   ) || exit -1

done

for x in ${TESTS}; do

   test=`echo $x | sed 's/\.pro//'`

   if [ "${BUILD}" == "android" ]; then

      (
      source qmake.vars &&

      # FIXME: don't redeploy if nothing changed
      echo "Deploying $test..." &&

      ${ANDROID_DEPLOY_QT} --input *$test*.json --output "${DESTDIR}/$test/" --gradle --install --device "${DEVICE}" > android-deploy.log 2>&1 &&

      echo "Running $test..." &&

      # start test app and wait for exit
      ${ADB} -s "${DEVICE}" -d shell am start -W -a android.intent.action.MAIN \
            -n org.qtproject.example.$test/org.qtproject.qt5.android.bindings.QtActivity &&

      # dump the current log
      echo "Dumping log..." &&

      # TODO: only dump our own pid? .. this filter can't see anything outside testlib
      ${ADB} -s "${DEVICE}" logcat -v color -v brief -d  'QTestLib:*' '*:S' &&

      # clear log
      ${ADB} -s "${DEVICE}" logcat -c

      ) || (echo -e '\n**** Test deployment FAILED ****\n' && exit -2)

   elif [ "${BUILD}" == "windows" ]; then

      (
      source qmake.vars &&
      wine64 "${DESTDIR}/$test.exe"
      ) || (echo -e '\n**** Test execution FAILED ****\n' && exit -2)

   else

      (
      source qmake.vars &&
      "${DESTDIR}/$test"
      ) || (echo -e '\n**** Test execution FAILED ****\n' && exit -2)
   fi

done

echo -e '\n**** All tests completed ****\n'
