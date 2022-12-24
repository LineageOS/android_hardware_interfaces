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

import android.hardware.audio.effect.CommandId;
import android.hardware.audio.effect.Descriptor;
import android.hardware.audio.effect.Parameter;
import android.hardware.audio.effect.State;
import android.hardware.common.fmq.MQDescriptor;
import android.hardware.common.fmq.SynchronizedReadWrite;

/**
 * Effect interfaces definitions to configure and control the effect instance.
 */
@VintfStability
interface IEffect {
    @VintfStability
    @FixedSize
    parcelable Status {
        /**
         * One of Binder STATUS_* statuses:
         *  - STATUS_OK: the command has completed successfully;
         *  - STATUS_BAD_VALUE: invalid parameters or state detected in effects;
         *  - STATUS_INVALID_OPERATION: an internal error happens in effect audio buffer processing;
         *  - STATUS_NOT_ENOUGH_DATA: a read or write error has occurred for the 'inputDataMQ' or
         * 'outputDataMQ';
         *
         */
        int status;
        /**
         * The amount of audio data samples in the floating point format consumed by the effect
         * instance.
         */
        int fmqConsumed;
        /**
         * The amount of audio data samples in the floating point format produced by the effect
         * instance.
         */
        int fmqProduced;
    }

    /**
     * Return data structure of IEffect.open() interface.
     */
    @VintfStability
    parcelable OpenEffectReturn {
        /**
         * Message queue for effect processing status.
         */
        MQDescriptor<Status, SynchronizedReadWrite> statusMQ;
        /**
         * Message queue for input data buffer.
         */
        MQDescriptor<float, SynchronizedReadWrite> inputDataMQ;
        /**
         * Message queue for output data buffer.
         */
        MQDescriptor<float, SynchronizedReadWrite> outputDataMQ;
    }

    /**
     * Open an effect instance, effect must not start processing data before receive
     * CommandId::START command. All necessary information should be allocated and instance must
     * transfer to State::IDLE state after open() call has been handled successfully. After open,
     * the effect instance must be able to handle all IEffect interface calls.
     *
     * @param common Parameters which MUST pass from client at open time.
     * @param specific Effect specific parameters which can optional pass from client at open time.
     *
     * @throws EX_ILLEGAL_ARGUMENT if the effect instance receive unsupported command.
     * @throws a EX_UNSUPPORTED_OPERATION if device capability/resource is not enough or system
     *         failure happens.
     * @note Open an already-opened effect instance should do nothing and should not throw an error.
     */
    OpenEffectReturn open(
            in Parameter.Common common, in @nullable Parameter.Specific specific);

    /**
     * Called by the client to close the effect instance, processing thread should be destroyed and
     * consume no CPU after close.
     *
     * It is recommended to close the effect on the client side as soon as it becomes unused, it's
     * client responsibility to make sure all parameter/buffer is correct if client wants to reopen
     * a closed instance.
     *
     * Effect instance close interface should always succeed unless:
     * 1. The effect instance is not in a proper state to be closed, for example it's still in
     * State::PROCESSING state.
     * 2. There is system/hardware related failure when close.
     *
     * @throws EX_ILLEGAL_STATE if the effect instance is not in a proper state to be closed.
     * @throws EX_UNSUPPORTED_OPERATION if the effect instance failed to close for any other reason.
     * @note Close an already-closed effect should do nothing and should not throw an error.
     */
    void close();

    /**
     * Return the @c Descriptor of this effect instance.
     *
     * Must be available for the effect instance at anytime and should always succeed.
     *
     * @return Descriptor The @c Descriptor of this effect instance.
     */
    Descriptor getDescriptor();

    /**
     * Send a command (defined in enum CommandId) to the effect instance, instance state can be
     * changed as result of command handling.
     *
     * Must be available for the effect instance after it has been open().
     *
     * @param commandId ID of the command send to the effect instance.
     *
     * @throws EX_ILLEGAL_STATE if the effect instance is not in a proper state to handle the
     * command.
     * @throws EX_ILLEGAL_ARGUMENT if the effect instance receive unsupported command.
     */
    void command(in CommandId commandId);

    /**
     * Get current state of the effect instance.
     *
     * Must be available for the effect instance at anytime and should always succeed.
     *
     * @return Current effect instance state.
     */
    State getState();

    /**
     * Set a parameter to the effect instance.
     *
     * Must be available for the effect instance after open().
     *
     * @param param Parameter data to set to the effect instance.
     *
     * @throws EX_ILLEGAL_ARGUMENT if the effect instance receive unsupported parameter.
     */
    void setParameter(in Parameter param);

    /**
     * Get a parameter from the effect instance with parameter ID.
     *
     * This interface must return the current parameter of the effect instance, if no parameter
     * has been set by client yet, the default value must be returned.
     *
     * Must be available for the effect instance after open().
     *
     * @param paramId The tag enum of parameter to get.
     * @return Parameter The parameter to get from the effect instance.
     *
     * @throws EX_ILLEGAL_ARGUMENT if the effect instance receive unsupported parameter tag.
     */
    Parameter getParameter(in Parameter.Id paramId);
}
