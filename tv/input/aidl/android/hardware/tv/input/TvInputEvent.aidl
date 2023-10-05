/*
 * Copyright 2022 The Android Open Source Project
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

package android.hardware.tv.input;

import android.hardware.tv.input.TvInputDeviceInfo;
import android.hardware.tv.input.TvInputEventType;

@VintfStability
parcelable TvInputEvent {
    TvInputEventType type;

    /**
     * TvInputEventType::DEVICE_AVAILABLE: all fields are relevant.
     * TvInputEventType::DEVICE_UNAVAILABLE: only deviceId is relevant.
     * TvInputEventType::STREAM_CONFIGURATIONS_CHANGED: only deviceId is relevant.
     */
    TvInputDeviceInfo deviceInfo;
}
