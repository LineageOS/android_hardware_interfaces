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

import android.hardware.automotive.vehicle.VehiclePropValue;
import android.os.ParcelFileDescriptor;

@VintfStability
parcelable GetValueRequests {
    // The list of properties to get if they fit the binder memory limitation.
    VehiclePropValue[] payloads;
    // A list of request IDs, one for each VehiclePropValue. For every client,
    // the request ID must start with 1 and monotonically increase for every
    // VehiclePropValue in GetValueRequests. If it hits LONG_MAX (very
    // unlikely), it must loop back to 0.
    long[] requestId;
    // Shared memory file to store requests if they exceed binder memory
    // limitation. Created by client, readable only at VHAL during the call.
    // Caller could close it after the call.
    @nullable ParcelFileDescriptor sharedMemoryFd;
}
