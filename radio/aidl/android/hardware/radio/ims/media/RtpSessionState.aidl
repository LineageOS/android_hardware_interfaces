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

package android.hardware.radio.ims.media;

@VintfStability
@Backing(type="int")
enum RtpSessionState {
    /** The RTP session is opened and media flow is not started */
    OPEN = 0,
    /** The RTP session has active media flow */
    ACTIVE = 1,
    /** The RTP session is suspended */
    SUSPENDED = 2,
    /** The RTP session is closed */
    CLOSED = 3,
}
