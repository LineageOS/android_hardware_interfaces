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

package android.hardware.audio.core;

import android.hardware.audio.core.MmapBufferDescriptor;
import android.hardware.common.fmq.MQDescriptor;
import android.hardware.common.fmq.SynchronizedReadWrite;
import android.hardware.common.fmq.UnsynchronizedWrite;

/**
 * Stream descriptor contains fast message queues and buffers used for sending
 * and receiving audio data. The descriptor complements IStream* interfaces by
 * providing communication channels that serve as an alternative to Binder
 * transactions.
 *
 * Handling of audio data and commands must be done by the HAL module on a
 * dedicated thread with high priority, for all modes, including MMap No
 * IRQ. The HAL module is responsible for creating this thread and setting its
 * priority. The HAL module is also responsible for serializing access to the
 * internal components of the stream while serving commands invoked via the
 * stream's AIDL interface and commands invoked via the command queue of the
 * descriptor.
 */
@JavaDerive(equals=true, toString=true)
@VintfStability
parcelable StreamDescriptor {
    /**
     * Position binds together a position within the stream and time.
     *
     * The timestamp must use "monotonic" clock.
     *
     * The frame count must advance between consecutive I/O operations, and stop
     * advancing when the stream was put into the 'standby' mode. On exiting the
     * 'standby' mode, the frame count must not reset, but continue counting.
     */
    @VintfStability
    @FixedSize
    parcelable Position {
        /** Frame count. */
        long frames;
        /** Timestamp in nanoseconds. */
        long timeNs;
    }

    /**
     * The exit command is used to unblock the HAL thread and ask it to exit.
     * This is the last command that the client sends via the StreamDescriptor.
     * The HAL module must reply to this command in order to unblock the client,
     * and cease waiting on the command queue.
     */
    const int COMMAND_EXIT = 0;
    /**
     * The command used for audio I/O, see 'AudioBuffer'. For MMap No IRQ mode
     * this command only provides updated positions and latency because actual
     * audio I/O is done via the 'AudioBuffer.mmap' shared buffer.
     */
    const int COMMAND_BURST = 1;

    /**
     * Used for sending commands to the HAL module. The client writes into
     * the queue, the HAL module reads. The queue can only contain a single
     * command.
     */
    @VintfStability
    @FixedSize
    parcelable Command {
        /**
         * One of COMMAND_* codes.
         */
        int code;
        /**
         * For output streams: the amount of bytes provided by the client in the
         *   'audio.fmq' queue.
         * For input streams: the amount of bytes requested by the client to read
         *   from the hardware into the 'audio.fmq' queue.
         */
        int fmqByteCount;
    }
    MQDescriptor<Command, SynchronizedReadWrite> command;

    /**
     * No error, the command completed successfully.
     */
    const int STATUS_OK = 0;
    /**
     * Invalid data provided in the command, e.g. unknown command code or
     * negative 'fmqByteCount' value.
     */
    const int STATUS_ILLEGAL_ARGUMENT = 1;
    /**
     * The HAL module is not in the state when it can complete the command.
     */
    const int STATUS_ILLEGAL_STATE = 2;

    /**
     * Used for providing replies to commands. The HAL module writes into
     * the queue, the client reads. The queue can only contain a single reply,
     * corresponding to the last command sent by the client.
     */
    @VintfStability
    @FixedSize
    parcelable Reply {
        /**
         * One of STATUS_* statuses.
         */
        int status;
        /**
         * For output streams: the amount of bytes actually consumed by the HAL
         *   module from the 'audio.fmq' queue.
         * For input streams: the amount of bytes actually provided by the HAL
         *   in the 'audio.fmq' queue.
         */
        int fmqByteCount;
        /**
         * For output streams: the moment when the specified stream position
         *   was presented to an external observer (i.e. presentation position).
         * For input streams: the moment when data at the specified stream position
         *   was acquired (i.e. capture position).
         */
        Position observable;
        /**
         * Used only for MMap streams to provide the hardware read / write
         * position for audio data in the shared memory buffer 'audio.mmap'.
         */
        Position hardware;
        /**
         * Current latency reported by the hardware.
         */
        int latencyMs;
    }
    MQDescriptor<Reply, SynchronizedReadWrite> reply;

    /**
     * Total buffer size in frames. This applies both to the size of the 'audio.fmq'
     * queue and to the size of the shared memory buffer for MMap No IRQ streams.
     * Note that this size may end up being slightly larger than the size requested
     * in a call to 'IModule.openInputStream' or 'openOutputStream' due to memory
     * alignment requirements.
     */
    long bufferSizeFrames;

    /**
     * Used for sending or receiving audio data to/from the stream. In the case
     * of MMap No IRQ streams this structure only contains the information about
     * the shared memory buffer. Audio data is sent via the shared buffer
     * directly.
     */
    @VintfStability
    union AudioBuffer {
        /**
         * The fast message queue used for all modes except MMap No IRQ. Access
         * to this queue is synchronized via the 'command' and 'reply' queues
         * as described below.
         *
         * For output streams the following sequence of operations is used:
         *  1. The client puts audio data into the 'audio.fmq' queue.
         *  2. The client writes the 'BURST' command into the 'command' queue,
         *     and hangs on waiting on a read from the 'reply' queue.
         *  3. The high priority thread in the HAL module wakes up due to 2.
         *  4. The HAL module reads the command and audio data.
         *  5. The HAL module writes the command status and current positions
         *     into 'reply' queue, and hangs on waiting on a read from
         *     the 'command' queue.
         *
         * For input streams the following sequence of operations is used:
         *  1. The client writes the 'BURST' command into the 'command' queue,
         *     and hangs on waiting on a read from the 'reply' queue.
         *  2. The high priority thread in the HAL module wakes up due to 1.
         *  3. The HAL module puts audio data into the 'audio.fmq' queue.
         *  4. The HAL module writes the command status and current positions
         *     into 'reply' queue, and hangs on waiting on a read from
         *     the 'command' queue.
         *  5. The client wakes up due to 4.
         *  6. The client reads the reply and audio data.
         */
        MQDescriptor<byte, UnsynchronizedWrite> fmq;
        /**
         * MMap buffers are shared directly with the DSP, which operates
         * independently from the CPU. Writes and reads into these buffers
         * are not synchronized with 'command' and 'reply' queues. However,
         * the client still uses the 'BURST' command for obtaining current
         * positions from the HAL module.
         */
        MmapBufferDescriptor mmap;
    }
    AudioBuffer audio;
}
