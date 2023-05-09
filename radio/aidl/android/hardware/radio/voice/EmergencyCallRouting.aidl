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

package android.hardware.radio.voice;

/**
 * Indicates how the implementation should handle the emergency call if it is required by Android.
 * @hide
 */
@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum EmergencyCallRouting {
    /**
     * Indicates Android does not require how to handle the corresponding emergency call; it is
     * decided by implementation.
     */
    UNKNOWN,
    /**
     * Indicates the implementation must handle the call through emergency routing.
     */
    EMERGENCY,
    /**
     * Indicates the implementation must handle the call through normal call routing.
     */
    NORMAL,
}
