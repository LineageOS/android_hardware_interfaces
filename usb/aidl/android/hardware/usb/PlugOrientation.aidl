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

package android.hardware.usb;

@VintfStability
@Backing(type="int")
/**
 * Used to indicate plug orientation as defined by USB Type-C
 * Cable and Connector Specification.
 */
enum PlugOrientation {
    UNKNOWN = 0,
    UNPLUGGED = 1,
    /**
     * The device can detect that a plug is inserted, but
     * the plug may not present a normal or flipped orientation.
     */
    PLUGGED_UNKNOWN = 2,
    PLUGGED_NORMAL = 3,
    PLUGGED_FLIPPED = 4,
}
