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
 * Refer to State.gv for detailed state diagram.
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
     * In INIT state, effect instance must:
     * 1. Not handle any IEffect.command() and return EX_ILLEGAL_STATE with any Command.Id.
     * 2. Be able to handle all parameter setting with IEffect.setParameter().
     * 3. Be able to handle all getter interface calls like IEffect.getParameter() and
     * IEffect.getState().
     * 4. Be able to handle IEffect.open() successfully after configuration.
     *
     * Client is expected to do necessary configuration with IEffect.setParameter(), get all
     * resource ready with IEffect.open(), and make sure effect instance transfer to IDLE state
     * before sending commands with IEffect.command() interface. Effect instance must transfer
     * from INIT to IDLE state after handle IEffect.open() call successfully.
     */
    INIT,
    /**
     * An effect instance transfer to IDLE state after it was open successfully with IEffect.open()
     * in INIT state, or after it was stop/reset with Command.Id.STOP/RESET in PROCESSING state.
     *
     * In IDLE state, effect instance must:
     * 1. Be able to start effect processing engine with IEffect.command(Command.Id.START) call.
     * 2. Be able to handle all parameter setting with IEffect.setParameter().
     * 3. Be able to handle all getter interface calls like IEffect.getParameter() and
     * IEffect.getState().
     *
     * The following state transfer can happen in IDLE state:
     * 1. Transfer to PROCESSING if instance receive an START command and start processing data
     * successfully.
     * 2. Transfer to INIT if instance receive a close() call.
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
     * In PROCESSING state, effect instance must:
     * 1. Return EX_ILLEGAL_STATE if it's not able to handle any parameter settings at runtime.
     * 2. Be able to handle STOP and RESET for IEffect.command() interface, and return
     * EX_ILLEGAL_STATE for all other commands.
     * 3. Must be able to handle all get* interface calls like IEffect.getParameter() and
     * IEffect.getState().
     */
    PROCESSING,
}
