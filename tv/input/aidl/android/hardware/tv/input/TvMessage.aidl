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

@VintfStability
parcelable TvMessage {
    /**
     * This value for TvMessage.groupId denotes that the message doesn't belong to any group.
     */
    const long NO_GROUP_ID = -1;
    /**
     * Extended data type, like “ATSC A/336 Watermark”, “ATSC_CC”, etc. This type is opaque to the
     * framework except when the subtype is "device_id". If the subtype is "device_id", the ID of
     * device that sent the message should be contained in {@link #groupId}.
     *
     * The first message in a list of messages contained in
     * {@link android.hardware.tv.input.TvMessageEvent} should always have the subtype "device_id",
     * otherwise the event is discarded.
     */
    String subType;
    /**
     * This group id is used to optionally identify messages that belong together, such as
     * headers and bodies of the same event. For messages that do not have a group, this value
     * should be -1.
     *
     * If {@link #subType} is "device_id", this value should contain the ID of the device that sent
     * this message.
     *
     * As -1 is a reserved value, -1 should not be used as a valid groupId.
     */
    long groupId;
    int dataLengthBytes;
}
