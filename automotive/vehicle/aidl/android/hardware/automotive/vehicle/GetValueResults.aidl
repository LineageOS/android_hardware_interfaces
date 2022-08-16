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

import android.hardware.automotive.vehicle.GetValueResult;
import android.os.ParcelFileDescriptor;

@VintfStability
@JavaDerive(equals=true, toString=true)
parcelable GetValueResults {
    // The list of responses if they fit the binder memory limitation.
    GetValueResult[] payloads;
    // Shared memory file to store responses if they exceed binder memory
    // limitation. Created by VHAL, readable only for the client.
    // The client must close it after reading.
    @nullable ParcelFileDescriptor sharedMemoryFd;
}
