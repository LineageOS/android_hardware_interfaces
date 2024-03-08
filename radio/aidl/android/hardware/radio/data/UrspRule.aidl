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

package android.hardware.radio.data;

import android.hardware.radio.data.RouteSelectionDescriptor;
import android.hardware.radio.data.TrafficDescriptor;

/**
 * This struct represents a single URSP rule as defined in 3GPP TS 24.526.
 * @hide
 */
@VintfStability
@JavaDerive(toString=true)
parcelable UrspRule {
    /**
     * Precedence value in the range of 0 to 255. Higher value has lower precedence.
     */
    int precedence;
    /**
     * Used as a matcher for network requests.
     */
    TrafficDescriptor[] trafficDescriptors;
    /**
     * List of routes (connection parameters) that must be used for requests matching a
     * trafficDescriptor.
     */
    RouteSelectionDescriptor[] routeSelectionDescriptor;
}
