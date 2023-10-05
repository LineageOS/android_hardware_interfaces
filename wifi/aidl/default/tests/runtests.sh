#!/usr/bin/env bash

# Copyright(C) 2022 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0(the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http:// www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

if [ -z $ANDROID_BUILD_TOP ]; then
  echo "You need to source and lunch before you can use this script"
  exit 1
fi
set -e

adb root
adb wait-for-device
$ANDROID_BUILD_TOP/build/soong/soong_ui.bash --make-mode android.hardware.wifi-service-tests
adb sync data
adb shell /data/nativetest64/vendor/android.hardware.wifi-service-tests/android.hardware.wifi-service-tests
