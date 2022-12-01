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

package android.hardware.radio.network;

@VintfStability
@Backing(type="int")
@JavaDerive(toString=true)
enum LocationResponseType {
    /**
     * Network initiated Location request rejected by modem because the user has not given
     * permission for this use case
     */
    REJECTED = 0,
    /**
     * Network initiated Location request is accepted by modem however no location information has
     * been shared to network due to a failure
     */
    ACCEPTED_NO_LOCATION_PROVIDED = 1,
    /**
     * Network initiated Location request is accepted and location information is provided to the
     * network by modem
     */
    ACCEPTED_LOCATION_PROVIDED = 2,
}
