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
     * IDLE or DRAINING state and has all necessary information. Otherwise, it must:
     * 1. Throw an EX_ILLEGAL_STATE exception if the effect is not in IDLE or DRAINING state, or
     * 2. Throw an EX_TRANSACTION_FAILED for all other errors.
     *
     * If an effect instance in DRAINING state receives a START command, it must transit back to
     * PROCESSING state.
     */
    START = 0,
    /**
     * Stop effect engine processing with all resources kept.
     * If the effect is in **PROCESSING** state:
     *   - It must transition to **IDLE** state if no intermediate operations are required.
     *   - It must transition to **DRAINING** state if draining (e.g., fading) is required.
     *     - The instance must automatically transition to **IDLE** after draining.
     *     - It must ignore any new `STOP` commands during **DRAINING**.
     *     - `START` commands during **DRAINING** must transition the instance back to
     *       **PROCESSING**.
     * If the effect instance is already in **IDLE** state, it must do nothing and return success.
     *
     * If the effect instance transitions to DRAINING state:
     * 1. It must automatically transition to IDLE after completing draining tasks.
     * 2. It must ignore any new STOP commands received during the DRAINING state.
     * 3. START commands during DRAINING must immediately transfer the instance back to PROCESSING.
     *
     */
    STOP = 1,
    /**
     * Keep all parameter settings but reset the buffer content, stop engine processing, and transit
     * the instance state to IDLE if it is in PROCESSING state.
     * Effect instance must be able to handle RESET command at IDLE and PROCESSING states.
     *
     * If the implementation includes intermediate operations such as draining, the RESET command
     * must bypass DRAINING and immediately transition the state to IDLE.
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
