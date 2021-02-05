#!/bin/bash

# Copyright (C) 2020 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# This script is used to update audio policy configuration files
# to comply with the updated audio_policy_configuration.xsd from V7.0.
#
# The main difference is the separator used in lists for attributes.
# Since the XML Schema Definition standard only allows space to be
# used as a separator (see https://www.w3.org/TR/xmlschema11-2/#list-datatypes)
# the previous versions used a regular expression to validate lists
# in attribute values. E.g. the channel masks were validated using
# the following regexp: [_A-Z][_A-Z0-9]*(,[_A-Z][_A-Z0-9]*)*
# This has an obvious drawback of missing typos in the config file.
#
# The V7.0 has shifted to defining most of the frequently changed
# types in the XSD schema only. This allows for verifying all the values
# in lists, but in order to comply with XML Schema requirements
# list elements must be separated by space.
#
# Since the APM config files typically use include directives,
# the script must be pointed to the main APM config file and will
# take care all the included files automatically.
# If the included file is a shared version from 'frameworks/av',
# instead of updating it the script checks if there is a newer
# version with the corresponding name suffix (e.g.
# 'a2dp_audio_policy_configuration_7_0.xml') and updates the include
# path instead.

set -euo pipefail

if (echo "$@" | grep -qe -h); then
    echo "This script will update Audio Policy Manager config file"
    echo "to the format required by V7.0 XSD schema from a previous"
    echo "version."
    echo
    echo "USAGE: $0 [APM_XML_FILE] [OLD_VERSION]"
    echo "       APM_XML_FILE specifies the path to audio_policy_configuration.xml"
    echo "                    relative to Android repository root"
    echo "       OLD_VERSION specifies the version of schema currently used"
    echo
    echo "Example: $0 device/generic/goldfish/audio/policy/audio_policy_configuration.xml 6.0"
    exit
fi
readonly HAL_DIRECTORY=hardware/interfaces/audio
readonly SHARED_CONFIGS_DIRECTORY=frameworks/av/services/audiopolicy/config
readonly OLD_VERSION=${2:-$(ls ${ANDROID_BUILD_TOP}/${HAL_DIRECTORY} | grep -E '[0-9]+\.[0-9]+' |
                                sort -n | tail -n1)}
readonly NEW_VERSION=7.0
readonly NEW_VERSION_UNDERSCORE=7_0

readonly SOURCE_CONFIG=${ANDROID_BUILD_TOP}/$1

# First, validate the input using the schema of the current version

echo Validating the source against the $OLD_VERSION schema
xmllint --noout --xinclude \
        --nofixup-base-uris --path "$ANDROID_BUILD_TOP/$SHARED_CONFIGS_DIRECTORY" \
        --schema ${ANDROID_BUILD_TOP}/${HAL_DIRECTORY}/${OLD_VERSION}/config/audio_policy_configuration.xsd \
        ${SOURCE_CONFIG}
if [ $? -ne 0 ]; then
    echo
    echo "Config file fails validation for the specified version $OLD_VERSION--unsafe to update"
    exit 1
fi

# Find all the source files recursively

SOURCE_FILES=${SOURCE_CONFIG}
SHARED_FILES=
findIncludes() {
    local FILES_TO_CHECK=
    for F in $1; do
        local FOUND_INCLUDES=$(grep -Po '<xi:include href="\K[^"]+(?="\/>)' ${F})
        for I in ${FOUND_INCLUDES}; do
            SOURCE_FULL_PATH=$(dirname ${F})/${I}
            SHARED_FULL_PATH=${ANDROID_BUILD_TOP}/${SHARED_CONFIGS_DIRECTORY}/${I}
            if [ -f "$SOURCE_FULL_PATH" ]; then
                # Device-specific file.
                SOURCE_FILES+=$'\n'${SOURCE_FULL_PATH}
                FILES_TO_CHECK+=$'\n'${SOURCE_FULL_PATH}
            elif [ -f "$SHARED_FULL_PATH" ]; then
                # Shared file from the frameworks repo.
                SHARED_FILES+=$'\n'${I}
                FILES_TO_CHECK+=$'\n'${SHARED_FULL_PATH}
            else
                echo
                echo "Include file not found: $I"
                exit 1
            fi
        done
    done
    if [ "$FILES_TO_CHECK" ]; then
        findIncludes "$FILES_TO_CHECK"
    fi
}
findIncludes ${SOURCE_FILES}

echo "Will update $1 and included device-specific files in place."
echo "Will update paths to shared included files."
echo "Press Ctrl-C to cancel, Enter to continue"
read

# Update 'audioPolicyConfiguration version="1.0"' -> 7.0 in the main file
sed -i -r -e 's/(audioPolicyConfiguration version=")1.0/\17.0/' ${SOURCE_CONFIG}

updateFile() {
    FILE=$1
    ATTR=$2
    SEPARATOR=$3
    SRC_LINES=$(grep -nPo "$ATTR=\"[^\"]+\"" ${FILE} || true)
    for S in $SRC_LINES; do
        # Prepare instruction for 'sed' for in-place editing of specified line
        R=$(echo ${S} | sed -e 's/^[0-9]\+:/\//' | sed -e "s/$SEPARATOR/ /g")
        S=$(echo ${S} | sed -e 's/:/s\//')${R}/
        echo ${S} | sed -i -f - ${FILE}
    done
}
for F in $SOURCE_FILES; do
    updateFile ${F} "channelMasks" ","
    updateFile ${F} "samplingRates" ","
    updateFile ${F} "flags" "|"
done;

updateIncludes() {
    FILE=$1
    for I in $SHARED_FILES; do
        NEW_VERSION_I=${I%.*}_${NEW_VERSION_UNDERSCORE}.${I##*.}
        if [ -e "$ANDROID_BUILD_TOP/$SHARED_CONFIGS_DIRECTORY/$NEW_VERSION_I" ]; then
            echo "s/$I/$NEW_VERSION_I/g" | sed -i -f - ${FILE}
        fi
    done
}
for F in $SOURCE_FILES; do
    updateIncludes ${F}
done

# Validate the results against the new schema

echo Validating the result against the $NEW_VERSION schema
xmllint --noout --xinclude \
        --nofixup-base-uris --path "$ANDROID_BUILD_TOP/$SHARED_CONFIGS_DIRECTORY" \
        --schema ${ANDROID_BUILD_TOP}/${HAL_DIRECTORY}/${NEW_VERSION}/config/audio_policy_configuration.xsd \
        ${SOURCE_CONFIG}
if [ $? -ne 0 ]; then
    echo
    echo "Config file fails validation for the specified version $NEW_VERSION--please check the changes"
    exit 1
fi
echo
echo "Please check the diff and update path to APM shared files in the device makefile!"
