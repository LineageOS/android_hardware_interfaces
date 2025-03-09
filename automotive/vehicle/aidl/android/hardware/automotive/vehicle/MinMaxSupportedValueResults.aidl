/*
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

package android.hardware.automotive.vehicle;

import android.hardware.automotive.vehicle.MinMaxSupportedValueResult;
import android.os.ParcelFileDescriptor;

/**
 * The result structure for {@code getMinMaxSupportedValue}.
 *
 * Contains a list of results, one for each [propId, areaId] request. The
 * list must contain the same number of result as the {@code propIdAreaIds}.
 * The result must be in the same order, e.g. the first result is for the first
 * [propId, areaId].
 *
 * Java Client should use
 * {@link LargeParcelable.reconstructStableAIDLParcelable} to convert this back
 * to a regular parcelable and then use the converted parcelable's
 * {@code payloads} field.
 *
 * Native client should use
 * {@link LargeParcelable::stableLargeParcelableToParcelable}.
 *
 * VHAL implementation must store the results into {@link payloads} field and
 * use {@link LargeParcelable::parcelableToStableLargeParcelable} before
 * sending the converted large parcelable through binder.
 */
@VintfStability
@JavaDerive(equals=true, toString=true)
parcelable MinMaxSupportedValueResults {
    /**
     * The list of responses if they fit the binder memory limitation.
     */
    MinMaxSupportedValueResult[] payloads;
    /**
     * Shared memory file to store responses if they exceed binder memory
     * limitation. Created by VHAL, readable only for the client.
     * The client must close it after reading.
     */
    @nullable ParcelFileDescriptor sharedMemoryFd;
}
