#/usr/bin/env bash

set -euo pipefail

if (echo "$@" |grep -qe -h); then
    echo "This script will generate a new HAL version identical to an other one."
    echo "This helps creating the boilerplate for a new version."
    echo
    echo "USAGE: $0 [BASE_VERSION] [NEW_VERSION]"
    echo "       BASE_VERSION default value is the highest version currently existing"
    echo "       NEW_VERSION default value is BASE_VERSION + 1"
    echo
    echo "Example: to generate a V6.0 by copying V5, do: $0 5.0 6.0"
    exit
fi
readonly HAL_DIRECTORY=hardware/interfaces/audio
readonly HAL_VTS_DIRECTORY=core/all-versions/vts/functional
readonly HAL_VTS_FILE=AudioPrimaryHidlHalTest.cpp
readonly HAL_SERVICE_DIRECTORY=common/all-versions/default/service/
readonly HAL_SERVICE_CPP=service.cpp

readonly FWK_DIRECTORY=frameworks/av/media/libaudiohal
readonly IMPL_DIRECTORY=impl
readonly IMPL_FACTORYHAL=FactoryHalHidl.cpp

readonly VTS_DIRECTORY=test/vts-testcase/hal/audio
readonly VTS_LIST=test/vts/tools/build/tasks/list/vts_test_lib_hidl_package_list.mk
readonly WATCHDOG=frameworks/base/services/core/java/com/android/server/Watchdog.cpp
readonly DUMP_UTILS=frameworks/native/libs/dumputils/dump_utils.cpp
readonly GSI_CURRENT=build/make/target/product/gsi/current.txt

readonly BASE_VERSION=${1:-$(ls $ANDROID_BUILD_TOP/$HAL_DIRECTORY | grep -E '[0-9]+\.[0-9]+' |
                                  sort -n |tail -n1)}
readonly BASE_MAJOR_VERSION=${BASE_VERSION%.*}
readonly BASE_MINOR_VERSION=${BASE_VERSION##*.}

readonly NEW_VERSION="${2:-$((${BASE_MAJOR_VERSION} + 1)).0}"
readonly NEW_MAJOR_VERSION=${NEW_VERSION%.*}
readonly NEW_MINOR_VERSION=${NEW_VERSION##*.}


readonly BASE_VERSION_REGEX="${BASE_MAJOR_VERSION}[._]${BASE_MINOR_VERSION}"
readonly NEW_VERSION_REGEX="${NEW_MAJOR_VERSION}[._]${NEW_MINOR_VERSION}"

readonly BASE_VERSION_ESCAPE="${BASE_MAJOR_VERSION}\.${BASE_MINOR_VERSION}"
readonly BASE_VERSION_UNDERSCORE="${BASE_MAJOR_VERSION}_${BASE_MINOR_VERSION}"
readonly NEW_VERSION_UNDERSCORE="${NEW_MAJOR_VERSION}_${NEW_MINOR_VERSION}"
updateVersion() {
    if [ $1 == "-e" ]; then
        local -r REGEX="$2"; shift 2
    else
        local -r REGEX="$BASE_VERSION_REGEX"
    fi
    awk -i inplace -e "{if (!/$REGEX/) print; else {
                            if (original_before) print
                            if (original_after) original_line=\$0;

                            gsub(/$BASE_VERSION_ESCAPE/,\"$NEW_VERSION\");
                            gsub(/$BASE_VERSION_UNDERSCORE/,\"$NEW_VERSION_UNDERSCORE\");
                            gsub(/MAJOR_VERSION=$BASE_MAJOR_VERSION/,
                                 \"MAJOR_VERSION=$NEW_MAJOR_VERSION\");
                            gsub(/MINOR_VERSION=$BASE_MINOR_VERSION/,
                                 \"MINOR_VERSION=$NEW_MINOR_VERSION\");

                            print
                            if (original_after) print original_line
                       }}" "$@"
}

updateAudioVersion() {
    updateVersion -e "audio.*$BASE_VERSION_REGEX" "$@"
}

updateLicenceDates() {
    # Update date on the 2 first lines
    sed -i "1,2 s/20[0-9][0-9]/$(date +"%Y")/g" "$@"
}

echo "Creating new audio HAL V$NEW_VERSION based on V$BASE_VERSION"
echo "Press Ctrl-C to cancel, Enter to continue"
read

MODIFIED=
runIfNeeded() {
    local -r TARGET=$1; shift
    cd $ANDROID_BUILD_TOP/$TARGET
    if grep -q -r "audio.*$NEW_VERSION_REGEX"; then
        echo "  Skipping $TARGET as already up to date"
    else
        echo "  Updating $PWD"
        MODIFIED+=$'\n - '$TARGET
        "$@"
    fi
}

createHALVersion() {
    local -r DIRS=". common effect"
    local COPY=
    echo "Copy $BASE_VERSION to $NEW_VERSION in $DIRS"
    for DIR in $DIRS; do
        cp -Tar $DIR/$BASE_VERSION $DIR/$NEW_VERSION
        COPY+=" $DIR/$NEW_VERSION"
    done

    echo "Replacing $BASE_VERSION by $NEW_VERSION in the copied files"
    updateVersion $(find $COPY -type f)
    updateLicenceDates $(find $COPY -type f)

    echo "Update implementation and VTS generic code"
    local -r FILES="*/all-versions/default/Android.bp */all-versions/vts/functional/Android.bp"
    updateVersion -v original_before=1 -v RS= -v ORS='\n\n' $FILES
    sed -i '${/^$/d}' $FILES # Remove \n at the end of the files

    updateVersion -v original_before=1 $HAL_SERVICE_DIRECTORY/Android.bp

    updateVersion -e "audio::.*$BASE_VERSION_REGEX" -v original_after=1 \
        $HAL_SERVICE_DIRECTORY/$HAL_SERVICE_CPP
    updateVersion -e "audio\/.*$BASE_VERSION_REGEX" -v original_before=1 \
        $HAL_SERVICE_DIRECTORY/$HAL_SERVICE_CPP

    local -r HAL_VTS_PATH=$HAL_VTS_DIRECTORY/$NEW_VERSION/$HAL_VTS_FILE
    mkdir -p $(dirname $HAL_VTS_PATH)
    cat > $HAL_VTS_PATH <<EOF
/*
 * Copyright (C) $(date +"%Y") The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// pull in all the <= $BASE_VERSION tests
#include "$BASE_VERSION/$(basename AudioPrimaryHidlHalTest.cpp)"
EOF

    echo "New HAL version $NEW_VERSION successfully created"
}

echo "Creating new audio HAL definition, default impl and VTS"
runIfNeeded $HAL_DIRECTORY createHALVersion


createFrameworkAdapter() {
    updateVersion -v original_before=1 Android.bp
    updateVersion -v original_before=1 -v RS= -v ORS='\n\n' $IMPL_DIRECTORY/Android.bp
    updateVersion -v original_after=1 $IMPL_FACTORYHAL
}
echo "Now creating the framework adapter version"
runIfNeeded $FWK_DIRECTORY createFrameworkAdapter

createVTSXML() {
    cp -Tar V$BASE_VERSION_UNDERSCORE V$NEW_VERSION_UNDERSCORE
    cp -Tar effect/{V$BASE_VERSION_UNDERSCORE,V$NEW_VERSION_UNDERSCORE}
    local -r FILES=$(find {.,effect}/V$NEW_VERSION_UNDERSCORE -type f)
    updateVersion $FILES
    updateLicenceDates $FILES
}
echo "Now update VTS XML"
runIfNeeded $VTS_DIRECTORY createVTSXML

echo "Now register new VTS"
runIfNeeded $(dirname $VTS_LIST) updateAudioVersion -v original_before=1 $(basename $VTS_LIST)

echo "Now update watchdog"
runIfNeeded $(dirname $WATCHDOG) updateAudioVersion -v original_before=1 $(basename $WATCHDOG)

echo "Now update dumputils"
runIfNeeded $(dirname $DUMP_UTILS) updateAudioVersion -v original_before=1 $(basename $DUMP_UTILS)

echo "Now update GSI current.txt"
runIfNeeded $(dirname $GSI_CURRENT) update-vndk-list.sh

if ! [ "$MODIFIED" ]; then
    echo
    echo "$NEW_VERSION already exist, nothing to do"
    exit
fi

cat << EOF

All File generated successfully. Please submit a patch in all those directories: $MODIFIED

-----------------------------------------------------------
WHAT WAS *NOT* DONE, and you need to do now:
 1) You need to choose if the new HAL is optional or not for new devices.
    Either add or replace $BASE_VERSION by $NEW_VERSION in
    compatibility_matrices/compatibility_matrix.current.xml
    Do not forget to update both the "audio" and "audio.effects" HAL'

 2) Then you need to choose a device to update its audio HAL implementation:
    a) Update the HAL manifest of your device: open your device manifest.xml
       and replace $BASE_VERSION by $NEW_VERSION for both
        - android.hardware.audio
        - android.hardware.audio.effect
    b) Go to your device device.mk (usually next to the manifest) and replace:
        - android.hardware.audio@$BASE_VERSION-impl by
          android.hardware.audio@$NEW_VERSION-impl
        - android.hardware.audio.effect@$BASE_VERSION-impl by
          android.hardware.audio.effect@$NEW_VERSION-impl
EOF
