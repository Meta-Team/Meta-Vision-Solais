#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"  # current script file dir
A="$(whoami)"
B="$(arch)"

if [ "$A" != 'root' ]; then
   echo "You have to be the root to run this script"
   exit 1
fi

echo "Copy 88-mvusb.rules to /etc/udev/rules.d/"
cp "${DIR}/88-mvusb.rules" "/etc/udev/rules.d/"

#echo "Copy header files to /usr/include/MindVision"
#mkdir -p "/usr/include/MindVision"
#cp -rf "${DIR}/include/*" "/usr/include/MindVision"
#
#if [[ "$B" == 'x86_64' ]]; then
#	LIB="lib/x64/libMVSDK.so"
#elif [[ "$B" == 'aarch64' ]]; then
#	LIB="lib/arm64/libMVSDK.so"
#elif [[ ${B:2} == '86' ]]; then
#	LIB="lib/x86/libMVSDK.so"
#else
#	LIB="lib/arm/libMVSDK.so"
#fi
#
#echo "Copy ${LIB} to /lib"
#cp "${DIR}/${LIB}" "/lib"

echo "Succeeded! Please restart the system!"
