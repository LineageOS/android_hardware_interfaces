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

package android.hardware.broadcastradio;

/**
 * The emergency alert message type
 *
 * <p>The message type indicates the emergency alert message nature.
 * (see ITU-T X.1303 bis for more info).
 */
@VintfStability
@Backing(type="int")
@JavaDerive(equals=true, toString=true)
enum AlertMessageType {
    /**
     * Initial information requiring attention by targeted recipients.
     */
    ALERT,

    /**
     * Updates and supersedes the earlier message(s).
     */
    UPDATE,

    /**
     * Cancels the earlier message(s).
     */
    CANCEL,
}
