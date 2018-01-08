#!/bin/bash

#if [ ! $# -eq 3 ]; then
#  echo "usage: ${0} <gcov-files-dir> \"<file-pattern>\" <target-dir>"
#  exit 0
#fi

mkdir -p build/lcov
lcov -d build/obj -c -o build/lcov/coverage.info


lcov --remove build/lcov/coverage.info '/qt/test/build/*' -o build/lcov/coverage.info
lcov --remove build/lcov/coverage.info '/usr/include/*' -o build/lcov/coverage.info
lcov --remove build/lcov/coverage.info '/usr/local/include/*' -o build/lcov/coverage.info

#lcov --list-full-path -e ${1}/coverage.info ${2} -o ${1}/coverage-stripped.info

genhtml --demangle-cpp -o build/lcov build/lcov/coverage.info

#lcov -d ${1} -z

exit 0
