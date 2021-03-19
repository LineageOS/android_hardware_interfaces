/*
 * Copyright (C) 2020 The Android Open Source Project
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

package android.hardware.biometrics.common;

@VintfStability
parcelable ComponentInfo {
    /**
     * An identifier uniquely identifying a subsystem.
     * It must not be an empty string.
     */
    String componentId;

    /**
     * The hardware version. For example, <vendor>/<model>/<revision>.
     * If there's no hardware version for this component, it must be empty.
     */
    String hardwareVersion;

    /**
     * The firmware version.
     * If there's no firmware version for this component, it must be empty.
     */
    String firmwareVersion;

    /**
     * The sensor's serial number.
     * If there's no serial number for this component, it must be empty.
     */
    String serialNumber;

    /**
     * The software version. For example, <vendor>/<version>/<revision>.
     * If there's no software version for this component, it must be empty.
     */
    String softwareVersion;
}
