#!/bin/bash
# Script to update Android make-files for HAL and VTS modules.

set -e

if [ -z "$ANDROID_BUILD_TOP" ]; then
    echo "Missing ANDROID_BUILD_TOP env variable. Run 'lunch' first."
    exit 1
fi

source $ANDROID_BUILD_TOP/system/tools/hidl/update-makefiles-helper.sh

do_makefiles_update \
  "android.hardware:hardware/interfaces" \
  "android.hidl:system/libhidl/transport"

echo "Updating files at $ANDROID_BUILD_TOP/test/vts-testcase/hal"
pushd $ANDROID_BUILD_TOP/test/vts-testcase/hal
./script/update_makefiles.py
popd

