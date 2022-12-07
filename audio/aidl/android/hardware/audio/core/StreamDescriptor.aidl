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
import android.media.audio.common.Void;

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
 *
 * There is a state machine defined for the stream, which executes on the
 * thread handling the commands from the queue. The states are defined based
 * on the model of idealized producer and consumer connected via a ring buffer.
 * For input streams, the "producer" is hardware, the "consumer" is software,
 * for outputs streams it's the opposite. When the producer is active, but
 * the buffer is full, the following actions are possible:
 *  - if the consumer is active, the producer blocks until there is space,
 *    this behavior is only possible for software producers;
 *  - if the consumer is passive:
 *    - the producer can preserve the buffer contents—a s/w producer can
 *      keep the data on their side, while a h/w producer can only drop captured
 *      data in this case;
 *    - or the producer overwrites old data in the buffer.
 * Similarly, when an active consumer faces an empty buffer, it can:
 *  - block until there is data (producer must be active), only possible
 *    for software consumers;
 *  - walk away with no data; when the consumer is hardware, it must emit
 *    silence in this case.
 *
 * The model is defined below, note the asymmetry regarding the 'IDLE' state
 * between input and output streams:
 *
 *  Producer | Buffer state | Consumer | Applies | State
 *  active?  |              | active?  | to      |
 * ==========|==============|==========|=========|==============================
 *  No       | Empty        | No       | Both    | STANDBY
 * ----------|--------------|----------|---------|-----------------------------
 *  Yes      | Filling up   | No       | Input   | IDLE, overwrite behavior
 * ----------|--------------|----------|---------|-----------------------------
 *  No       | Empty        | Yes†     | Output  | IDLE, h/w emits silence
 * ----------|--------------|----------|---------|-----------------------------
 *  Yes      | Not empty    | Yes      | Both    | ACTIVE, s/w x-runs counted
 * ----------|--------------|----------|---------|-----------------------------
 *  Yes      | Filling up   | No       | Input   | PAUSED, drop behavior
 * ----------|--------------|----------|---------|-----------------------------
 *  Yes      | Filling up   | No†      | Output  | PAUSED, s/w stops writing once
 *           |              |          |         | the buffer is filled up;
 *           |              |          |         | h/w emits silence.
 * ----------|--------------|----------|---------|-----------------------------
 *  No       | Not empty    | Yes      | Both    | DRAINING
 * ----------|--------------|----------|---------|-----------------------------
 *  No       | Not empty    | No†      | Output  | DRAIN_PAUSED,
 *           |              |          |         | h/w emits silence.
 *
 * † - note that for output, "buffer empty, h/w consuming" has the same outcome
 *     as "buffer not empty, h/w not consuming", but logically these conditions
 *     are different.
 *
 * State machines of both input and output streams start from the 'STANDBY'
 * state. Transitions between states happen naturally with changes in the
 * states of the model elements. For simplicity, we restrict the change to one
 * element only, for example, in the 'STANDBY' state, either the producer or the
 * consumer can become active, but not both at the same time. States 'STANDBY',
 * 'IDLE', 'READY', and '*PAUSED' are "stable"—they require an external event,
 * whereas a change from the 'DRAINING' state can happen with time as the buffer
 * gets empty, thus it's a "transient" state.
 *
 * The state machine for input streams is defined in the `stream-in-sm.gv` file,
 * for output streams—in the `stream-out-sm.gv` file. State machines define how
 * commands (from the enum 'CommandCode') trigger state changes. The full list
 * of states and commands is defined by constants of the 'State' enum. Note that
 * the 'CLOSED' state does not have a constant in the interface because the
 * client can never observe a stream with a functioning command queue in this
 * state. The 'ERROR' state is a special state which the state machine enters
 * when an unrecoverable hardware error is detected by the HAL module.
 *
 * Non-blocking (asynchronous) modes introduce a new 'TRANSFERRING' state, which
 * the state machine can enter after replying to the 'burst' command, instead of
 * staying in the 'ACTIVE' state. In this case the client gets unblocked
 * earlier, while the actual audio delivery to / from the observer is not
 * complete yet. Once the HAL module is ready for the next transfer, it notifies
 * the client via a oneway callback, and the machine switches to 'ACTIVE'
 * state. The 'TRANSFERRING' state is thus "transient", similar to the
 * 'DRAINING' state. For output streams, asynchronous transfer can be paused,
 * and it's another new state: 'TRANSFER_PAUSED'. It differs from 'PAUSED' by
 * the fact that no new writes are allowed. Please see 'stream-in-async-sm.gv'
 * and 'stream-out-async-sm.gv' files for details. Below is the table summary
 * for asynchronous only-states:
 *
 *  Producer | Buffer state | Consumer | Applies | State
 *  active?  |              | active?  | to      |
 * ==========|==============|==========|=========|==============================
 *  Yes      | Not empty    | Yes      | Both    | TRANSFERRING, s/w x-runs counted
 * ----------|--------------|----------|---------|-----------------------------
 *  Yes      | Not empty    | No       | Output  | TRANSFER_PAUSED,
 *           |              |          |         | h/w emits silence.
 *
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
        /**
         * The value used when the position can not be reported by the HAL
         * module.
         */
        const long UNKNOWN = -1;
        /** Frame count. */
        long frames = UNKNOWN;
        /** Timestamp in nanoseconds. */
        long timeNs = UNKNOWN;
    }

    @VintfStability
    @Backing(type="int")
    enum State {
        /**
         * 'STANDBY' is the initial state of the stream, entered after
         * opening. Since both the producer and the consumer are inactive in
         * this state, it allows the HAL module to put associated hardware into
         * "standby" mode to save power.
         */
        STANDBY = 1,
        /**
         * In the 'IDLE' state the audio hardware is active. For input streams,
         * the hardware is filling buffer with captured data, overwriting old
         * contents on buffer wraparounds. For output streams, the buffer is
         * still empty, and the hardware is outputting zeroes. The HAL module
         * must not account for any under- or overruns as the client is not
         * expected to perform audio I/O.
         */
        IDLE = 2,
        /**
         * The active state of the stream in which it handles audio I/O. The HAL
         * module can assume that the audio I/O will be periodic, thus inability
         * of the client to provide or consume audio data on time must be
         * considered as an under- or overrun and indicated via the 'xrunFrames'
         * field of the reply.
         */
        ACTIVE = 3,
        /**
         * In the 'PAUSED' state the consumer is inactive. For input streams,
         * the hardware stops updating the buffer as soon as it fills up (this
         * is the difference from the 'IDLE' state). For output streams,
         * "inactivity" of hardware means that it does not consume audio data,
         * but rather emits silence.
         */
        PAUSED = 4,
        /**
         * In the 'DRAINING' state the producer is inactive, the consumer is
         * finishing up on the buffer contents, emptying it up. As soon as it
         * gets empty, the stream transfers itself into the next state.
         */
        DRAINING = 5,
        /**
         * Used for output streams only, pauses draining. This state is similar
         * to the 'PAUSED' state, except that the client is not adding any
         * new data. If it emits a 'burst' command, this brings the stream
         * into the regular 'PAUSED' state.
         */
        DRAIN_PAUSED = 6,
        /**
         * Used only for streams in asynchronous mode. The stream enters this
         * state after receiving a 'burst' command and returning control back
         * to the client, thus unblocking it.
         */
        TRANSFERRING = 7,
        /**
         * Used only for output streams in asynchronous mode only. The stream
         * enters this state after receiving a 'pause' command while being in
         * the 'TRANSFERRING' state. Unlike 'PAUSED' state, this state does not
         * accept new writes.
         */
        TRANSFER_PAUSED = 8,
        /**
         * The ERROR state is entered when the stream has encountered an
         * irrecoverable error from the lower layer. After entering it, the
         * stream can only be closed.
         */
        ERROR = 100,
    }

    @VintfStability
    @Backing(type="byte")
    enum DrainMode {
        /**
         * Unspecified—used with input streams only, because the client controls
         * draining.
         */
        DRAIN_UNSPECIFIED = 0,
        /**
         * Used with output streams only, the HAL module indicates drain
         * completion when all remaining audio data has been consumed.
         */
        DRAIN_ALL = 1,
        /**
         * Used with output streams only, the HAL module indicates drain
         * completion shortly before all audio data has been consumed in order
         * to give the client an opportunity to provide data for the next track
         * for gapless playback. The exact amount of provided time is specific
         * to the HAL implementation.
         */
        DRAIN_EARLY_NOTIFY = 2,
    }

    /**
     * Used for sending commands to the HAL module. The client writes into
     * the queue, the HAL module reads. The queue can only contain a single
     * command.
     *
     * Variants of type 'Void' correspond to commands without
     * arguments. Variants of other types correspond to commands with an
     * argument. Would in future a need for a command with multiple argument
     * arise, a Parcelable type should be used for the corresponding variant.
     */
    @VintfStability
    @FixedSize
    union Command {
        /**
         * Reserved for the HAL implementation to allow unblocking the wait on a
         * command and exiting the I/O thread. A command of this variant must
         * never be sent from the client side. To prevent that, the
         * implementation must pass a random cookie as the command argument,
         * which is only known to the implementation.
         */
        int halReservedExit;
        /**
         * Retrieve the current state of the stream. This command must be
         * processed by the stream in any state. The stream must provide current
         * positions, counters, and its state in the reply. This command must be
         * handled by the HAL module without any observable side effects.
         */
        Void getStatus;
        /**
         * See the state machines on the applicability of this command to
         * different states.
         */
        Void start;
        /**
         * The 'burst' command used for audio I/O, see 'AudioBuffer'. The value
         * specifies:
         *
         *  - for output streams: the amount of bytes that the client requests the
         *    HAL module to use out of the data contained in the 'audio.fmq' queue.
         *
         *  - for input streams: the amount of bytes requested by the client to
         *    read from the hardware into the 'audio.fmq' queue.
         *
         * In both cases it is allowed for this field to contain any
         * non-negative number. Any sufficiently big value which exceeds the
         * size of the queue's area which is currently available for reading or
         * writing by the HAL module must be trimmed by the HAL module to the
         * available size. Note that the HAL module is allowed to consume or
         * provide less data than requested, and it must return the amount of
         * actually read or written data via the 'Reply.fmqByteCount'
         * field. Thus, only attempts to pass a negative number must be
         * constituted as a client's error.
         *
         * Differences for the MMap No IRQ mode:
         *
         *  - this command only provides updated positions and latency because
         *    actual audio I/O is done via the 'AudioBuffer.mmap' shared buffer.
         *    The client does not synchronize reads and writes into the buffer
         *    with sending of this command.
         *
         *  - the value must always be set to 0.
         *
         * See the state machines on the applicability of this command to
         * different states.
         */
        int burst;
        /**
         * See the state machines on the applicability of this command to
         * different states.
         */
        DrainMode drain;
        /**
         * See the state machines on the applicability of this command to
         * different states.
         *
         * Note that it's left on the discretion of the HAL implementation to
         * assess all the necessary conditions that could prevent hardware from
         * being suspended. Even if it can not be suspended, the state machine
         * must still enter the 'STANDBY' state for consistency. Since the
         * buffer must remain empty in this state, even if capturing hardware is
         * still active, captured data must be discarded.
         */
        Void standby;
        /**
         * See the state machines on the applicability of this command to
         * different states.
         */
        Void pause;
        /**
         * See the state machines on the applicability of this command to
         * different states.
         */
        Void flush;
    }
    MQDescriptor<Command, SynchronizedReadWrite> command;

    /**
     * The value used for the 'Reply.latencyMs' field when the effective
     * latency can not be reported by the HAL module.
     */
    const int LATENCY_UNKNOWN = -1;

    /**
     * Used for providing replies to commands. The HAL module writes into
     * the queue, the client reads. The queue can only contain a single reply,
     * corresponding to the last command sent by the client.
     */
    @VintfStability
    @FixedSize
    parcelable Reply {
        /**
         * One of Binder STATUS_* statuses:
         *  - STATUS_OK: the command has completed successfully;
         *  - STATUS_BAD_VALUE: invalid value in the 'Command' structure;
         *  - STATUS_INVALID_OPERATION: the command is not applicable in the
         *                              current state of the stream, or to this
         *                              type of the stream;
         *  - STATUS_NOT_ENOUGH_DATA: a read or write error has
         *                            occurred for the 'audio.fmq' queue;
         */
        int status;
        /**
         * Used with the 'burst' command only.
         *
         * For output streams: the amount of bytes of data actually consumed
         *   by the HAL module.
         * For input streams: the amount of bytes actually provided by the HAL
         *   in the 'audio.fmq' queue.
         *
         * The returned value must not exceed the value passed as the
         * argument of the corresponding command, or be negative.
         */
        int fmqByteCount;
        /**
         * It is recommended to report the current position for any command. If
         * the position can not be reported, for example because the mix port is
         * not connected to any producer or consumer, or because the HAL module
         * does not support positions reporting for this AudioSource (on input
         * streams), the 'Position::UNKNOWN' value must be used.
         *
         * For output streams: the moment when the specified stream position
         *   was presented to an external observer (i.e. presentation position).
         * For input streams: the moment when data at the specified stream position
         *   was acquired (i.e. capture position).
         *
         * The observable position must never be reset by the HAL module.
         * The data type of the frame counter is large enough to support
         * continuous counting for years of operation.
         */
        Position observable;
        /**
         * Used only for MMap streams to provide the hardware read / write
         * position for audio data in the shared memory buffer 'audio.mmap'.
         */
        Position hardware;
        /**
         * Current latency reported by the hardware. It is recommended to
         * report the current latency for any command. If the value of latency
         * can not be determined, this field must be set to 'LATENCY_UNKNOWN'.
         */
        int latencyMs;
        /**
         * Number of frames lost due to an underrun (for input streams),
         * or not provided on time (for output streams) for the **previous**
         * transfer operation.
         */
        int xrunFrames;
        /**
         * The state that the stream was in while the HAL module was sending the
         * reply.
         */
        State state = State.STANDBY;
    }
    MQDescriptor<Reply, SynchronizedReadWrite> reply;

    /**
     * The size of one frame of audio data in bytes. For PCM formats this is
     * usually equal to the size of a sample multiplied by the number of
     * channels used. For encoded bitstreams encapsulated into PCM the sample
     * size of the underlying PCM stream is used. For encoded bitstreams that
     * are passed without encapsulation, the frame size is usually 1 byte.
     */
    int frameSizeBytes;
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
         * The fast message queue used for BURST commands in all modes except
         * MMap No IRQ. Both reads and writes into this queue are non-blocking
         * because access to this queue is synchronized via the 'command' and
         * 'reply' queues as described below. The queue nevertheless uses
         * 'SynchronizedReadWrite' because there is only one reader, and the
         * reading position must be shared.
         *
         * Note that the fast message queue is a transient buffer, only used for
         * data transfer. Neither of the sides can use it to store any data
         * outside of the 'BURST' operation. The consumer must always retrieve
         * all data available in the fast message queue, even if it can not use
         * it. The producer must re-send any unconsumed data on the next
         * transfer operation. This restriction is posed in order to make the
         * fast message queue fully transparent from the latency perspective.
         *
         * For output streams the following sequence of operations is used:
         *  1. The client writes audio data into the 'audio.fmq' queue.
         *  2. The client writes the BURST command into the 'command' queue,
         *     and hangs on waiting on a read from the 'reply' queue.
         *  3. The high priority thread in the HAL module wakes up due to 2.
         *  4. The HAL module reads the command and audio data. According
         *     to the statement above, the HAL module must always read
         *     from the FMQ all the data it contains. The amount of data that
         *     the HAL module has actually consumed is indicated to the client
         *     via the 'reply.fmqByteCount' field.
         *  5. The HAL module writes the command status and current positions
         *     into 'reply' queue, and hangs on waiting on a read from
         *     the 'command' queue.
         *  6. The client wakes up due to 5. and reads the reply.
         *     Note: in non-blocking mode, when the HAL module goes to
         *           the 'TRANSFERRING' state (as indicated by the 'reply.state'
         *           field), the client must wait for the 'IStreamCallback.onTransferReady'
         *           notification to arrive before starting the next burst.
         *
         * For input streams the following sequence of operations is used:
         *  1. The client writes the BURST command into the 'command' queue,
         *     and hangs on waiting on a read from the 'reply' queue.
         *  2. The high priority thread in the HAL module wakes up due to 1.
         *  3. The HAL module writes audio data into the 'audio.fmq' queue.
         *     The value of 'reply.fmqByteCount' must be the equal to the amount
         *     of data in the queue.
         *  4. The HAL module writes the command status and current positions
         *     into 'reply' queue, and hangs on waiting on a read from
         *     the 'command' queue.
         *  5. The client wakes up due to 4.
         *  6. The client reads the reply and audio data. The client must
         *     always read from the FMQ all the data it contains.
         *     Note: in non-blocking mode, when the HAL module goes to
         *           the 'TRANSFERRING' state (as indicated by the 'reply.state'
         *           field) the client must wait for the 'IStreamCallback.onTransferReady'
         *           notification to arrive before starting the next burst.
         *
         */
        MQDescriptor<byte, SynchronizedReadWrite> fmq;
        /**
         * MMap buffers are shared directly with the DSP, which operates
         * independently from the CPU. Writes and reads into these buffers are
         * not synchronized with 'command' and 'reply' queues. However, the
         * client still uses the same commands for controlling the audio data
         * exchange and for obtaining current positions and latency from the HAL
         * module.
         */
        MmapBufferDescriptor mmap;
    }
    AudioBuffer audio;
}
