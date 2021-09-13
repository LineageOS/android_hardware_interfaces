/*
 * Copyright (C) 2021 The Android Open Source Project
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

package android.hardware.automotive.vehicle;

/**
 * Property config defines the capabilities of it. User of the API
 * must first get the property config to understand the output from get()
 * commands and also to ensure that set() or events commands are in sync with
 * the expected output.
 */
@VintfStability
@Backing(type="int")
enum VehiclePropertyAccess {
    NONE = 0x00,

    READ = 0x01,
    WRITE = 0x02,
    READ_WRITE = 0x03,
}
