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

package android.hardware.automotive.evs;

import android.hardware.automotive.evs.DeviceStatusType;

/**
 * The status of the devices, as sent by EVS HAL through the
 * IEvsEnumeratorCallback::deviceStatusChanged() call.
 */
@VintfStability
parcelable DeviceStatus {
    /**
     * The identifier of a device that has transitioned to a new status.
     */
    @utf8InCpp
    String id;
    /**
     * A new status of this device
     */
    DeviceStatusType status;
}
