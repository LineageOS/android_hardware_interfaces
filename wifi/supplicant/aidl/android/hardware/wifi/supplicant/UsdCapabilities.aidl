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

package android.hardware.wifi.supplicant;

/**
 * Capabilities supported by USD. Values are only valid if |isUsdPublisherSupported|
 * and/or |isUsdSubscriberSupported| are true.
 */
@VintfStability
parcelable UsdCapabilities {
    /**
     * Whether USD Publisher is supported on this device.
     */
    boolean isUsdPublisherSupported;

    /**
     * Whether USD Subscriber is supported on this device.
     */
    boolean isUsdSubscriberSupported;

    /**
     * Maximum allowed length (in bytes) for the Service Specific Info (SSI).
     */
    int maxLocalSsiLengthBytes;

    /**
     * Maximum allowed length (in bytes) for the service name.
     */
    int maxServiceNameLengthBytes;

    /**
     * Maximum allowed length (in bytes) for a match filter.
     */
    int maxMatchFilterLengthBytes;

    /**
     * Maximum number of allowed publish sessions.
     */
    int maxNumPublishSessions;

    /**
     * Maximum number of allowed subscribe sessions.
     */
    int maxNumSubscribeSessions;
}
