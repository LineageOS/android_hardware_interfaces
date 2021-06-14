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

package android.hardware.radio;

/**
 * Defines range of ports. start and end are the first and last port numbers (inclusive) in the
 * range. Both start and end are in QosPortRange.MIN to QosPortRange.MAX range. A single port shall
 * be represented by the same start and end value.
 */
@VintfStability
parcelable PortRange {
    int start;
    int end;
}
