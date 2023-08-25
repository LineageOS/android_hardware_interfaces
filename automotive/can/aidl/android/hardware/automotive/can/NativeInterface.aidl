/*
 * Copyright (C) 2022 The Android Open Source Project
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
package android.hardware.automotive.can;

@VintfStability
parcelable NativeInterface {
    union InterfaceId {
        /** Interface name, such as can0. */
        String ifname;

        /**
         * Alternatively to providing {@see ifname}, one may provide a list of
         * interface serial number suffixes. If there happens to be a device
         * (like USB2CAN) with a matching serial number suffix, the HAL service
         * will locate it.
         *
         * Client may utilize this in two ways: by matching against the
         * entire serial number, or the last few characters (usually
         * one). The former is better for small-scale test deployments
         * (with just a handful of vehicles), the latter is good for
         * larger scale (where a small suffix list may support large
         * test fleet).
         */
        String[] serialno;
    }

    InterfaceId interfaceId;
}
