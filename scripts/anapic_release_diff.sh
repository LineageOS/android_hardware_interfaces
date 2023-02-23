#!/bin/bash
#
# Create a CL that contains the changes between this branch and a newer branch
# for a given AIDL interface.
# Be sure that BRANCH_BASE is the current upstream branch in order to get a CL.

if [[ $# -ne 3 ]]; then
    echo "Usage: $0 BRANCH_BASE BRANCH_NEW PACKAGE_NAME"
    echo "- BRANCH_BASE current branch, typically a previous release's dev branch"
    echo "- BRANCH_NEW end branch, typically goog/master as the latest branch"
    echo "- PACKAGE_NAME this is the AIDL package name"
    echo "example of creating the diffs for android.hardware.boot"
    echo "$ git checkout tm-dev ; repo start review"
    echo "$ ./anapic_release_diff.sh goog/tm-dev goog/master android.hardware.boot"
    exit 1
fi

# for pathmod
source ${ANDROID_BUILD_TOP}/build/make/envsetup.sh

set -ex

INTERFACE_NAME_NO_VER=${3%@*}
pushd $(pathmod $INTERFACE_NAME_NO_VER)
git diff "$1".."$2" android | git apply
git add -A
git commit -am "Android $1 to $2: $3" --no-edit
git diff HEAD~1 --stat
repo upload . --no-verify --wip --hashtag=anapic_release_review
popd
