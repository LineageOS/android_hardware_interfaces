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

package android.hardware.audio.effect;

/**
 * Defines all commands supported by the effect instance.
 *
 * There are three groups of commands:
 * 1. Common part which MUST be supported by all effects.
 * 2. Commands MUST be supported by a specific type of effect.
 * 3. Extension commands for vendor.
 */
@VintfStability
@Backing(type="int")
enum CommandId {
    /**
     * Commands MUST be supported by all effects.
     */
    /**
     * Start effect engine processing.
     * An effect instance must start processing data and transfer to PROCESSING state if it is in
     * IDLE state and have all necessary information. Otherwise it must:
     * 1. Throw a EX_ILLEGAL_STATE exception if effect is not in IDLE state, or
     * 2. Throw a EX_TRANSACTION_FAILED for all other errors.
     *
     * Depending on parameters set to the effect instance, effect may do process or reverse
     * process after START command.
     */
    START = 0,
    /**
     * Stop effect engine processing with all resource kept.
     * The currently processed audio data will be discarded if the effect engine is in PROCESSING
     * state.
     * Effect instance must do nothing and return ok when it receive STOP command in IDLE state.
     */
    STOP = 1,
    /**
     * Keep all parameter settings but reset the buffer content, stop engine processing, and transit
     * instance state to IDLE if its in PROCESSING state.
     * Effect instance must be able to handle RESET command at IDLE and PROCESSING states.
     */
    RESET = 2,

    /**
     * Commands MUST be supported by a specific type of effect.
     */

    /**
     * Extension commands for vendor.
     */
    VENDOR_COMMAND_0 = 0x100,
    VENDOR_COMMAND_1,
    VENDOR_COMMAND_2,
    VENDOR_COMMAND_3,
    VENDOR_COMMAND_4,
    VENDOR_COMMAND_5,
    VENDOR_COMMAND_6,
    VENDOR_COMMAND_7,
    VENDOR_COMMAND_8,
    VENDOR_COMMAND_9,
}
