#!/bin/bash

source system/tools/hidl/update-makefiles-helper.sh

do_makefiles_update \
  "android.hardware:hardware/interfaces" \
  "android.hidl:system/libhidl/transport"

