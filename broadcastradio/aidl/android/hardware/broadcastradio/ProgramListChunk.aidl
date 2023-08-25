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

package android.hardware.broadcastradio;

import android.hardware.broadcastradio.ProgramIdentifier;
import android.hardware.broadcastradio.ProgramInfo;

/**
 * An update packet of the program list.
 *
 * The order of entries in the arrays is unspecified.
 */
@VintfStability
@JavaDerive(equals=true, toString=true)
parcelable ProgramListChunk {
    /**
     * Treats all previously added entries as removed.
     *
     * This is meant to save binder transaction bandwidth on 'removed' array
     * and provide a clear empty state.
     *
     * If set, 'removed' array must be null.
     *
     * The client may wait with taking action on this until it received the
     * chunk with complete flag set (to avoid part of stations temporarily
     * disappearing from the list).
     */
    boolean purge;

    /**
     * If false, it means there are still programs not transmitted,
     * due for transmission in following updates.
     *
     * Used by UIs that wait for complete list instead of displaying
     * programs while scanning.
     *
     * After the whole channel range was scanned and all discovered programs
     * were transmitted, the last chunk must have set this flag to {@code true}.
     * This must happen within {@link IBroadcastRadio#LIST_COMPLETE_TIMEOUT_MS}
     * from the startProgramListUpdates call. If it doesn't, client may assume
     * the tuner came into a bad state and display error message.
     */
    boolean complete;

    /**
     * Added or modified program list entries.
     *
     * Two entries with the same primaryId (ProgramSelector member)
     * are considered the same.
     */
    ProgramInfo[] modified;

    /**
     * Removed program list entries.
     *
     * Contains primaryId (ProgramSelector member) of a program to remove.
     */
    @nullable ProgramIdentifier[] removed;
}
