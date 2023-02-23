#!/bin/bash
#
# Create two CLs for the given HIDL interface to see the diff between the
# hidl2aidl output and the source at the tip-of-tree.
# The first CL contains the hidl2aidl output after removing all existing AIDL
# files.
# The second CL contains all of the changes on top of the raw hidl2aidl output
# that can be used for review.

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
git diff HEAD~1 --stat
repo upload . --no-verify --wip --hashtag=anapic_release_review
popd
