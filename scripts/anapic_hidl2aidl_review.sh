#!/bin/bash

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 INTERFACE_NAME"
    echo "- INTERFACE_NAME fully qualified HIDL interface name with version"
    echo "example of creating the diffs for android.hardware.boot@1.2"
    echo "$ ./anapic_hidl2aidl_review.sh android.hardware.boot@1.2"
    exit 1
fi

# for pathmod
source ${ANDROID_BUILD_TOP}/build/make/envsetup.sh

set -ex
type hidl2aidl 2>/dev/null || m hidl2aidl

INTERFACE_NAME_NO_VER=${1%@*}
pushd $(pathmod $INTERFACE_NAME_NO_VER)
rm -rf android
hidl2aidl -o . "$1"
rm -rf conversion.log translate include
git add -A
git commit -am "convert $1" --no-edit
git revert HEAD --no-edit
git commit --amend --no-edit
repo upload . --no-verify
popd
