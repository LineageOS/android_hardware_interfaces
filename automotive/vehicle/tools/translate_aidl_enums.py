#!/usr/bin/python3

# Copyright (C) 2023 The Android Open Source Project
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
#
"""A script to generate ENUM_NAME.java file and test files using ENUM_NAME.aidl file.

   Need ANDROID_BUILD_TOP environmental variable to be set. This script will update ENUM_NAME.java
   under packages/services/Car/car-lib/src/android/car/hardware/property, as well as the
   ENUM_NAMETest.java files in cts/tests/tests/car/src/android/car/cts and
   packages/services/Car/tests/android_car_api_test/src/android/car/apitest

   Also needs a flag name e.g. FLAG_ANDROID_VIC_VEHICLE_PROPERTIES

   Usage:
   $ python translate_aidl_enums.py ENUM_NAME.aidl FLAG_NAME
"""
import os
import sys

LICENSE = """/*
 * Copyright (C) 2024 The Android Open Source Project
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
"""

class EnumParser:
    def __init__(self, file_path, file_name, flag_name):
        self.filePath = file_path
        self.fileName = file_name
        self.flagName = flag_name
        self.lowerFileName = self.fileName[0].lower() + self.fileName[1:]
        self.enumNames = []
        self.enums = []
        self.outputMsg = []
        self.outputMsg.append(LICENSE)
        self.outputMsg.append("\npackage android.car.hardware.property;\n")
        self.outputMsg.append("""
import static android.car.feature.Flags.{};

import android.annotation.FlaggedApi;
import android.annotation.IntDef;
import android.annotation.NonNull;

import com.android.car.internal.util.ConstantDebugUtils;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
""".format(self.flagName))

        comment_block = []
        in_comment = False

        with open(self.filePath, 'r') as f:
            lines = f.readlines()
            for line in lines:
                line = line.rstrip('\n')
                if line.strip() in ["package android.hardware.automotive.vehicle;",
                                    "@VintfStability",
                                    '@Backing(type="int")']:
                    continue

                if line.strip().startswith('/**') or line.strip().startswith('/*'):
                    in_comment = True
                    comment_block.append(line + '\n')
                    continue
                elif in_comment:
                    comment_block.append(line + '\n')
                    if line.strip().endswith('*/'):
                        in_comment = False
                    continue
                elif line.strip().startswith('*'):
                    comment_block.append(line + '\n')
                    continue

                msg = line + '\n'
                msgSplit = msg.strip().split()
                if len(msgSplit) > 0 and msgSplit[0] == "enum":
                    if comment_block:
                        self.outputMsg.extend(comment_block)
                        comment_block = []
                    self.outputMsg.append("@FlaggedApi({})\n".format(self.flagName))
                    msgSplit[0] = "public final class"
                    msg = " ".join(msgSplit) + "\n"
                    self.outputMsg.append(msg)
                elif len(msgSplit) > 1 and msgSplit[1] == '=':
                    if comment_block:
                        indented_comment_block = [line for line in comment_block]
                        self.outputMsg.extend(indented_comment_block)
                        comment_block = []
                    msgSplit.insert(0, "    public static final int")
                    enum_name = msgSplit[1].strip()
                    self.enumNames.append(enum_name)
                    enum = msgSplit[3].strip(",")
                    self.enums.append(enum)
                    if msgSplit[-1].endswith(','):
                        msgSplit[-1] = msgSplit[-1][:-1] + ";"
                    msg = " ".join(msgSplit) + "\n"
                    self.outputMsg.append(msg)
                elif line.strip() == '}':
                    if comment_block:
                        self.outputMsg.extend(comment_block)
                        comment_block = []
                    self.outputMsg.append("""
    private {2}() {{}}

    /**
     * Returns a user-friendly representation of {{@code {2}}}.
     */
    @NonNull
    public static String toString(@{2}Int int {0}) {{
        String {0}String = ConstantDebugUtils.toName(
                {2}.class, {0});
        return ({0}String != null)
                ? {0}String
                : "0x" + Integer.toHexString({0});
    }}

    /** @hide */
    @IntDef({1})
    @Retention(RetentionPolicy.SOURCE)
    public @interface {2}Int {{}}\n""".format(self.lowerFileName, "{" + ", ".join(self.enums) + "}",
                                              self.fileName))
        self.outputMsg.append("}")

        self.outputMsgApiTest = []
        self.outputMsgApiTest.append(LICENSE)
        self.outputMsgApiTest.append("""package android.car.apitest;

import static android.car.feature.Flags.{1};

import static com.google.common.truth.Truth.assertWithMessage;

import android.platform.test.annotations.RequiresFlagsEnabled;
import android.platform.test.flag.junit.CheckFlagsRule;
import android.platform.test.flag.junit.DeviceFlagsValueProvider;

import androidx.test.filters.SmallTest;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;

import java.util.Arrays;
import java.util.Collection;

@SmallTest
@RunWith(Parameterized.class)
public class {0}Test {{
    @Rule
    public final CheckFlagsRule mCheckFlagsRule = DeviceFlagsValueProvider.createCheckFlagsRule();
    private final int mJavaConstantValue;
    private final int mHalConstantValue;

    public {0}Test(int javaConstantValue, int halConstantValue) {{
        mJavaConstantValue = javaConstantValue;
        mHalConstantValue = halConstantValue;
    }}

    @Parameterized.Parameters
    public static Collection constantValues() {{
        return Arrays.asList(
                new Object[][] {{""".format(self.fileName, self.flagName))
        for enum in self.enumNames:
            self.outputMsgApiTest.append("""
                        {{
                                android.car.hardware.property.{0}.{1},
                                android.hardware.automotive.vehicle.{0}.{1}
                        }},""".format(self.fileName, enum))
        self.outputMsgApiTest.append("""
                }});
    }}

    @Test
    @RequiresFlagsEnabled({})
    public void testMatchWithVehicleHal() {{
        assertWithMessage("Java constant")
                .that(mJavaConstantValue)
                .isEqualTo(mHalConstantValue);
    }}
}}
""".format(self.flagName))

        self.outputMsgCtsTest = []
        self.outputMsgCtsTest.append(LICENSE)
        self.outputMsgCtsTest.append("""
package android.car.cts;

import static android.car.feature.Flags.{1};

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import android.car.cts.utils.VehiclePropertyUtils;
import android.car.hardware.property.{0};
import android.platform.test.annotations.RequiresFlagsEnabled;
import android.platform.test.flag.junit.CheckFlagsRule;
import android.platform.test.flag.junit.DeviceFlagsValueProvider;

import org.junit.Rule;
import org.junit.Test;

import java.util.List;

public class {0}Test {{
    @Rule
    public final CheckFlagsRule mCheckFlagsRule = DeviceFlagsValueProvider.createCheckFlagsRule();

    @Test
    @RequiresFlagsEnabled({1})
    public void testToString() {{""".format(self.fileName, self.flagName))
        for enum in self.enumNames:
            self.outputMsgCtsTest.append("""
        assertThat({0}.toString(
                {0}.{1}))
                .isEqualTo("{1}");""".format(self.fileName, enum))
        max_enum_value = len(self.enums)
        self.outputMsgCtsTest.append("""
        assertThat({0}.toString({1})).isEqualTo("{2}");
        assertThat({0}.toString(12)).isEqualTo("0xc");
    }}

    @Test
    @RequiresFlagsEnabled({4})
    public void testAll{0}sAreMappedInToString() {{
        List<Integer> {3}s =
                VehiclePropertyUtils.getIntegersFromDataEnums({0}.class);
        for (Integer {3} : {3}s) {{
            String {3}String = {0}.toString(
                    {3});
            assertWithMessage("%s starts with 0x", {3}String).that(
                    {3}String.startsWith("0x")).isFalse();
        }}
    }}
}}
""".format(self.fileName, len(self.enums), hex(len(self.enums)), self.lowerFileName, self.flagName))

def main():
    if len(sys.argv) != 3:
        print("Usage: {} enum_aidl_file ALL_CAPS_FLAG_NAME".format(sys.argv[0]))
        sys.exit(1)
    print("WARNING: This file only generates the base enum values in the framework layer. The "
          + "generated files must be reviewed by you and edited if any additional changes are "
          + "required. The java enum file should be updated with app-developer facing "
          + "documentation, the @FlaggedApi tag for the new API, and with the @SystemApi tag if "
          + "the new property is system API")
    file_path = sys.argv[1]
    file_name = file_path.split('/')[-1][:-5]
    flag_name = sys.argv[2]
    parser = EnumParser(file_path, file_name, flag_name)

    android_top = os.environ['ANDROID_BUILD_TOP']
    if not android_top:
        print('ANDROID_BUILD_TOP is not in environmental variable, please run source and lunch '
              + 'at the android root')
        sys.exit(1)

    with open(android_top + "/packages/services/Car/car-lib/src/android/car/hardware/property/"
              + file_name + ".java", 'w') as f:
        f.write("".join(parser.outputMsg))

    with open(android_top
              + "/packages/services/Car/tests/android_car_api_test/src/android/car/apitest/"
              + file_name + "Test.java", 'w') as f:
        f.write("".join(parser.outputMsgApiTest))

    with open(android_top + "/cts/tests/tests/car/src/android/car/cts/" + file_name + "Test.java",
              'w') as f:
        f.write("".join(parser.outputMsgCtsTest))

if __name__ == "__main__":
    main()
