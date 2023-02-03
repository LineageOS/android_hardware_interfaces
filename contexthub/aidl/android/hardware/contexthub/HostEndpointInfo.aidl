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

package android.hardware.contexthub;

/**
 * Stores metadata regarding a host endpoint that may communicate with the Context Hub.
 */
@VintfStability
parcelable HostEndpointInfo {
    /** The ID of the host endpoint asscociated with this host. */
    char hostEndpointId;

    /** The type of endpoint. */
    Type type;

    /** The (optional) package name of the host. */
    @nullable String packageName;

    /** The (optional) attribution tag associated with this host. */
    @nullable String attributionTag;

    @VintfStability
    @Backing(type="int")
    enum Type {
        /**
         * This endpoint is from the Android framework, where packageName and attributionTag may be
         * empty.
         */
        FRAMEWORK = 1,

        /** This endpoint is an Android app. */
        APP = 2,

        /** This endpoint is from an Android native program. */
        NATIVE = 3,
    }
}
