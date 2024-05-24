
/*
 * Copyright (C) 2023 The Android Open Source Project
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

package android.hardware.power;

import android.hardware.common.fmq.MQDescriptor;
import android.hardware.common.fmq.SynchronizedReadWrite;
import android.hardware.power.ChannelMessage;

@VintfStability
parcelable ChannelConfig {
    /**
     * The message queue descriptor that provides the information necessary for
     * a client to write to this channel.
     */
    MQDescriptor<ChannelMessage, SynchronizedReadWrite> channelDescriptor;

    /**
     * A message queue descriptor used to pass an optional event flag to clients,
     * used to synchronize multiple message queues using the same flag. If not
     * defined, the flag from the channelDescriptor should be used.
     */
    @nullable MQDescriptor<byte, SynchronizedReadWrite> eventFlagDescriptor;

    /**
     * The read flag bitmask to be used with the event flag, specifying the
     * bits used by this channel to mark that the buffer has been read from.
     * If set to 0, the default bitmask will be used.
     */
    int readFlagBitmask;

    /**
     * The write flag bitmask to be used with the event flag, specifying the
     * bits used by this channel to mark that the buffer has been written to.
     * If set to 0, the default bitmask will be used.
     */
    int writeFlagBitmask;
}
