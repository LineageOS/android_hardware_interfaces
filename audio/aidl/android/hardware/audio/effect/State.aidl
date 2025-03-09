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
 * Possible states of an effect instance.
 * A typical effect instance will be in INIT state when it is created with IFactory.createEffect()
 * interface, transfer to IDLE after open(), and to PROCESSING after
 * IEffect.command(Command.Id.START) command. When an effect instance receive STOP or RESET command,
 * it should transfer to IDLE state after handle the command successfully. Effect instance should
 * consume minimal resource and transfer to INIT state after it was close().
 *
 * An effect instance can be destroyed from any state using `IFactory.destroyEffect()`.
 *
 * Refer to the state machine diagram `state.gv` for a detailed state diagram.
 */
@VintfStability
@Backing(type="byte")
enum State {
    /**
     * An effect instance is in INIT state by default after it was created with
     * IFactory.createEffect(). When an effect instance is in INIT state, it should have instance
     * context initialized, and ready to handle IEffect.setParameter(), IEffect.open() as well as
     * all getter interfaces.
     *
     * **Requirements in INIT state:**
     * In INIT state, effect instance must:
     * 1. Not handle any IEffect.command() and return EX_ILLEGAL_STATE with any Command.Id.
     * 2. Be able to handle all parameter setting with IEffect.setParameter().
     * 3. Be able to handle all getter interface calls like IEffect.getParameter() and
     * IEffect.getState().
     * 4. Be able to handle IEffect.open() successfully after configuration.
     *
     * **State Transitions:**
     * - Transitions to **IDLE** after successful `IEffect.open()`.
     * - Remains in **INIT** on `IEffect.getState()` and `IEffect.getDescriptor()`.
     * - Transitions to the final state on `IFactory.destroyEffect()`.
     */
    INIT,

    /**
     * An effect instance transfer to IDLE state after it was open successfully with IEffect.open()
     * in INIT state, or after it was stop/reset with Command.Id.STOP/RESET in PROCESSING state.
     *
     * **Requirements in IDLE state:**
     * 1. Be able to start effect processing engine with IEffect.command(Command.Id.START) call.
     * 2. Be able to handle all parameter setting with IEffect.setParameter().
     * 3. Be able to handle all getter interface calls like IEffect.getParameter() and
     * IEffect.getState().
     *
     * **State Transitions:**
     * - Transitions to **PROCESSING** on `IEffect.command(CommandId.START)` after starting
     *   processing data successfully.
     * - Transitions to **INIT** on `IEffect.close()`.
     * - Remains in **IDLE** on `IEffect.getParameter()`, `IEffect.setParameter()`,
     *   `IEffect.getDescriptor()`, `IEffect.command(CommandId.RESET)`, and `IEffect.reopen()`.
     * - Transitions to the final state on `IFactory.destroyEffect()`.
     */
    IDLE,

    /**
     * An effect instance is in PROCESSING state after it receive an START command and start
     * processing data successfully. Effect instance will transfer from PROCESSING to IDLE state if
     * it receive an STOP or RESET command and handle the command successfully.
     *
     * When an instance is in PROCESSING state, client should try not to close() it directly,
     * instead client should try to stop processing data first with STOP command before close(). In
     * the case of a close() call received when instance in PROCESSING state, it should try to stop
     * processing and transfer to IDLE first before close().
     *
     * **Requirements in PROCESSING state:**
     * 1. Return EX_ILLEGAL_STATE if it's not able to handle any parameter settings at runtime.
     * 2. Be able to handle STOP and RESET for IEffect.command() interface, and return
     * EX_ILLEGAL_STATE for all other commands.
     * 3. Must be able to handle all get* interface calls like IEffect.getParameter() and
     * IEffect.getState().
     *
     * **State Transitions:**
     * - Transitions to **IDLE** on `IEffect.command(CommandId.STOP)` ( if no draining is required
     *   or implemented) or `IEffect.command(CommandId.RESET)`.
     * - Transitions to **DRAINING** on `IEffect.command(CommandId.STOP)` if draining is required.
     * - Remains in **PROCESSING** on `IEffect.getParameter()`, `IEffect.setParameter()`,
     *   `IEffect.getDescriptor()`, and `IEffect.reopen()`.
     *
     * **Notes:**
     * - Clients should avoid calling `IEffect.close()` directly in this state; instead, they should
     *   stop processing with `CommandId.STOP` before closing.
     * - If `IEffect.close()` is called in this state, the effect instance should stop processing,
     *   transition to **IDLE**, and then close.
     * - Transitions to the final state on `IFactory.destroyEffect()`.
     */
    PROCESSING,

    /**
     * DRAINING is an optional transitional state where the effect instance completes processing
     * remaining input buffers or finalizes operations (e.g., fading) before stopping completely.
     * This state is typically entered after a `CommandId.STOP` command in the PROCESSING state when
     * draining is required.
     *
     * **Requirements in DRAINING state:**
     * 1. Must handle `CommandId.START` and transition back to **PROCESSING**.
     * 2. Must handle getter interface calls like `IEffect.getParameter()` and `IEffect.getState()`.
     * 3. Must automatically transition to **IDLE** after draining is complete.
     *
     * **State Transitions:**
     * - Transitions to **PROCESSING** on `IEffect.command(CommandId.START)`.
     * - Transitions to **IDLE** on `IEffect.command(CommandId.RESET)`.
     * - Transitions to **IDLE** automatically after draining is complete.
     * - Remains in **DRAINING** on `IEffect.getParameter()`, `IEffect.setParameter()`,
     *   `IEffect.getDescriptor()`, and `IEffect.reopen()`.
     *
     * **Notes:**
     * - If not implemented, the effect instance may transition directly from **PROCESSING** to
     *   **IDLE** without this intermediate state.
     * - Any `CommandId.STOP` commands received during **DRAINING** should be ignored.
     * - Transitions to the final state on `IFactory.destroyEffect()`.
     */
    DRAINING,
}
